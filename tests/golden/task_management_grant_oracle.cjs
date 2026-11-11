'use strict';

// Independent Node crypto/CBOR oracle, not a deployed Service signer.
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');

const properties = new Map();
for (const line of fs.readFileSync(process.argv[2], 'utf8').split(/\r?\n/)) {
    if (!line || line.startsWith('#')) continue;
    const split = line.indexOf('=');
    assert(split > 0 && !properties.has(line.slice(0, split)));
    properties.set(line.slice(0, split), line.slice(split + 1));
}
function bytes(key) {
    const value = properties.get(key);
    assert.match(value, /^(?:[0-9a-f]{2})+$/);
    return Buffer.from(value, 'hex');
}
function head(major, n) {
    assert(Number.isInteger(n) && n >= 0 && n <= 0xffffffff);
    if (n < 24) return Buffer.from([(major << 5) | n]);
    if (n <= 255) return Buffer.from([(major << 5) | 24, n]);
    const size = n <= 65535 ? 2 : 4;
    const out = Buffer.alloc(1 + size);
    out[0] = (major << 5) | (size === 2 ? 25 : 26);
    out.writeUIntBE(n, 1, size);
    return out;
}
const uint = n => head(0, n);
const bstr = b => Buffer.concat([head(2, b.length), b]);
const array = values => Buffer.concat([head(4, values.length), ...values]);
const text = value => {
    const b = Buffer.from(value, 'utf8');
    return Buffer.concat([head(3, b.length), b]);
};
const sha256 = b => crypto.createHash('sha256').update(b).digest();

for (const [name, algorithm] of [['ed25519', 1], ['p256', 2]]) {
    const unsigned = [uint(1), ...['device_instance_id', 'task_scope', 'nonce', 'request_id',
        'operation_digest'].map(key => bstr(bytes(key))), uint(0xffffffff), uint(algorithm)];
    const transcript = array([text('blinker.task-grant.v1'), ...unsigned]);
    const digest = sha256(transcript);
    const signature = bytes(`${name}.signature`);
    const encoded = array([...unsigned, bstr(signature)]);
    assert.equal(encoded.length, 176);
    assert.deepEqual(encoded, bytes(`${name}.encoded`));
    assert.deepEqual(digest, bytes(`${name}.digest`));
    const key = crypto.createPublicKey({
        key: bytes(`${name}.spki`), format: 'der', type: 'spki',
    });
    // MCU verifies an already hashed digest. Node ECDSA hashes its input:
    // pass the transcript, NOT digest, to avoid an accidental second SHA-256.
    const verify = (input, sig) => name === 'ed25519'
        ? crypto.verify(null, sha256(input), key, sig)
        : crypto.verify('sha256', input, { key, dsaEncoding: 'ieee-p1363' }, sig);
    assert(verify(transcript, signature));
    for (let i = 0; i < transcript.length; ++i) {
        const altered = Buffer.from(transcript);
        altered[i] ^= 1;
        assert(!verify(altered, signature));
    }
    for (let i = 0; i < signature.length; ++i) {
        const altered = Buffer.from(signature);
        altered[i] ^= 1;
        assert(!verify(transcript, altered));
    }
}
// Actual-root and Initialize/Rebind operations reuse the same Grant envelope.
// Deterministic values are dummy test secrets, NOT production credentials.
const sequence = (start, size) => Buffer.from(Array.from({ length: size }, (_, i) => start + i));
const nil = Buffer.from([0xf6]);
const adminFingerprint = sha256(Buffer.concat([Buffer.from('blinker/direct-admin/fingerprint/v1\0'),
    sequence(17, 16), Buffer.from([0, 0, 0, 1]), sequence(97, 32)]));
assert.deepEqual(adminFingerprint, bytes('authority.admin_fingerprint'));
const admin = array([uint(1), uint(2), uint(0xffffffff), uint(1), uint(15),
    bstr(sequence(17, 16)), bstr(adminFingerprint)]);
const root = (key, hasAdmin) => sha256(array([
    text('blinker.task-root.v1'), uint(1), bstr(bytes('device_instance_id')),
    key ? bstr(key) : nil, hasAdmin ? admin : nil,
]));
const deviceKey = sequence(65, 32);
assert.deepEqual(root(deviceKey, false), bytes('authority.wifi'));
assert.deepEqual(root(null, true), bytes('authority.ble'));
assert.deepEqual(root(deviceKey, true), bytes('authority.mixed'));
deviceKey[0] ^= 128;
assert.deepEqual(root(deviceKey, true), bytes('authority.rotated'));
const authorityKey = crypto.createPublicKey({ key: bytes('authority.spki'), format: 'der', type: 'spki' });
for (const [name, previous, current] of [
    ['initialize', Buffer.alloc(0), bytes('authority.mixed')],
    ['rebind', bytes('authority.mixed'), bytes('authority.rotated')],
]) {
    const prefix = `authority.${name}`;
    const fields = [uint(1), bstr(bytes('request_id')), bstr(bytes('device_instance_id')),
        bstr(bytes('task_scope')), bstr(previous), bstr(current)];
    const operationTranscript = array([text('blinker.task-authority-change.v1'), ...fields]);
    const operationDigest = sha256(operationTranscript);
    assert.deepEqual(array(fields), bytes(`${prefix}.encoded`));
    assert.equal(array(fields).length, name === 'initialize' ? 88 : 121);
    assert.deepEqual(operationDigest, bytes(`${prefix}.digest`));
    const unsigned = [uint(1), ...['device_instance_id', 'task_scope', 'nonce', 'request_id'].map(k => bstr(bytes(k))),
        bstr(operationDigest), uint(0xffffffff), uint(1)];
    const transcript = array([text('blinker.task-grant.v1'), ...unsigned]);
    const digest = sha256(transcript);
    const signature = bytes(`${prefix}.signature`);
    assert.deepEqual(digest, bytes(`${prefix}.grant_digest`));
    assert.deepEqual(array([...unsigned, bstr(signature)]), bytes(`${prefix}.grant`));
    assert(crypto.verify(null, digest, authorityKey, signature));
    for (let i = 0; i < operationTranscript.length; ++i) {
        const changed = Buffer.from(operationTranscript); changed[i] ^= 1;
        const changedGrant = [...unsigned]; changedGrant[5] = bstr(sha256(changed));
        assert(!crypto.verify(null, sha256(array([text('blinker.task-grant.v1'), ...changedGrant])), authorityKey, signature));
    }
}
const bool = value => Buffer.from([value ? 0xf5 : 0xf4]);
const definition = array([uint(0), uint(0), uint(1000), uint(0), uint(0), uint(0),
    bool(true), text('power'), uint(0), bool(true)]);
const prepare = array([uint(1), bstr(bytes('request_id')), uint(0xffffffff), uint(1),
    uint(0xffffffff), bool(false), definition]);
const read = array([uint(1), bstr(bytes('request_id')), uint(0), uint(0xffffffff)]);
for (const [name, encoded] of [['mutation', prepare], ['read', read]]) {
    assert.deepEqual(encoded, bytes(`prepare.${name}.encoded`));
    assert.deepEqual(sha256(array([text(`blinker.task-${name}.v1`), bstr(bytes('authority.mixed')), encoded])),
        bytes(`prepare.${name}.digest`));
}
const challenge = array([uint(1), bstr(bytes('device_instance_id')), bstr(bytes('task_scope')),
    bstr(bytes('authority.mixed')), bstr(bytes('nonce')), bstr(bytes('request_id')), bstr(bytes('operation_digest'))]);
assert.deepEqual(challenge, bytes('challenge.encoded'));
assert.equal(challenge.length, 138);
const resultMac = transcript => crypto.createHmac('sha256', sequence(0xa0, 32)).update(transcript).digest();
const retirement = array([uint(1), bstr(bytes('request_id')), bstr(bytes('device_instance_id')),
    bstr(bytes('task_scope')), bstr(bytes('authority.mixed'))]);
assert.equal(retirement.length, 87);
assert.deepEqual(retirement, bytes('prepare.retirement.encoded'));
for (const name of ['retire', 'retirement-read']) {
    assert.deepEqual(sha256(array([text(`blinker.task-${name}.v1`), retirement])), bytes(`prepare.${name}.digest`));
}
for (const [label, role] of [['device', 1], ['admin', 2]]) {
    for (let kind = 0; kind < 5; ++kind) {
        const detail = kind === 1 ? array([uint(3), bool(true)]) : kind === 3
            ? array([uint(1), array([uint(3), uint(8), uint(0), uint(7), uint(3), definition])]) : kind === 4 ? bstr(bytes('retirement_id')) : nil;
        const fields = [uint(1), uint(role), uint(kind), bstr(bytes('request_id')), uint(9), uint(kind === 4 ? 0 : 2), detail];
        const transcript = array([text('blinker.task-result.v1'), challenge, array(fields)]);
        const tag = resultMac(transcript);
        assert.deepEqual(array([...fields, bstr(tag)]), bytes(`result.${label}.${kind}`));
        for (let i = 0; i < transcript.length; ++i) {
            const altered = Buffer.from(transcript); altered[i] ^= 1;
            assert.notDeepEqual(resultMac(altered), tag);
        }
    }
}
// The catalog extends the existing Read shape, not the transport or signer.
const catalogFingerprint = sha256(array([text('blinker.task-targets.v1'), uint(2),
    array([array([text('power'), uint(0)]), array([text('level'), uint(2)])])]));
const catalogVectors = { 'catalog.fingerprint': catalogFingerprint };
for (const [name, index] of [['metadata', 0xffffffff], ['target', 1]]) {
    const prepare = array([uint(1), bstr(bytes('request_id')), uint(0), uint(index),
        bstr(index === 0xffffffff ? Buffer.alloc(0) : catalogFingerprint)]);
    catalogVectors[`catalog.${name}.prepare`] = prepare;
    catalogVectors[`catalog.${name}.digest`] = sha256(array([text('blinker.task-read.v1'), bstr(bytes('authority.mixed')), prepare]));
    for (const [label, role] of [['device', 1], ['admin', 2]]) {
        const kind = index === 0xffffffff ? 5 : 6;
        const detail = array([uint(2), uint(2), bstr(catalogFingerprint),
            ...(kind === 6 ? [uint(1), text('level'), uint(2)] : [])]);
        const fields = [uint(1), uint(role), uint(kind), bstr(bytes('request_id')), uint(9), uint(2), detail];
        catalogVectors[`catalog.${name}.${label}.result`] = array([...fields,
            bstr(resultMac(array([text('blinker.task-result.v1'), challenge, array(fields)])))]);
    }
}
for (const [key, value] of Object.entries(catalogVectors)) {
    if (process.argv.includes('--emit-catalog')) console.log(`${key}=${value.toString('hex')}`);
    else assert.deepEqual(value, bytes(key));
}
for (const [label, role] of [['device', 1], ['admin', 2]]) {
    for (const reason of [1, 2]) {
        const fields = [uint(1), uint(role), uint(7), bstr(bytes('request_id')), uint(9), uint(2), uint(reason)];
        const encoded = array([...fields, bstr(resultMac(array([text('blinker.task-result.v1'), challenge, array(fields)])))]);
        const key = `rejection.${label}.${reason}`;
        if (process.argv.includes('--emit-rejection')) console.log(`${key}=${encoded.toString('hex')}`);
        else assert.deepEqual(encoded, bytes(key));
    }
}
console.log('Task Grant + catalog + rejection canonical vectors and HMAC verified; tampering rejected');
