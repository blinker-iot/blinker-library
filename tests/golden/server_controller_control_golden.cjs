'use strict';

const crypto = require('crypto');
const fs = require('fs');

function parseProperties(path) {
    const output = new Map();
    for (const [index, line] of fs.readFileSync(path, 'utf8')
        .split(/\r?\n/).entries()) {
        if (line.length === 0 || line.startsWith('#')) continue;
        const split = line.indexOf('=');
        if (split <= 0 || output.has(line.slice(0, split))) {
            throw new Error(`invalid fixture line ${index + 1}`);
        }
        output.set(line.slice(0, split), line.slice(split + 1));
    }
    return output;
}

function required(properties, key) {
    if (!properties.has(key)) throw new Error(`missing property: ${key}`);
    return properties.get(key);
}

function bytes(properties, key) {
    const value = required(properties, key);
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) {
        throw new Error(`invalid lowercase hex: ${key}`);
    }
    return Buffer.from(value, 'hex');
}

function number(properties, key) {
    const value = Number(required(properties, key));
    if (!Number.isSafeInteger(value) || value < 0) {
        throw new Error(`invalid number: ${key}`);
    }
    return value;
}

function head(major, value) {
    if (!Number.isSafeInteger(value) || value < 0) {
        throw new Error('CBOR integer out of range');
    }
    const prefix = major << 5;
    if (value < 24) return Buffer.from([prefix | value]);
    if (value <= 0xff) return Buffer.from([prefix | 24, value]);
    if (value <= 0xffff) {
        const output = Buffer.alloc(3);
        output[0] = prefix | 25;
        output.writeUInt16BE(value, 1);
        return output;
    }
    if (value <= 0xffffffff) {
        const output = Buffer.alloc(5);
        output[0] = prefix | 26;
        output.writeUInt32BE(value, 1);
        return output;
    }
    const output = Buffer.alloc(9);
    output[0] = prefix | 27;
    const high = Math.floor(value / 0x100000000);
    output.writeUInt32BE(high, 1);
    output.writeUInt32BE(value - high * 0x100000000, 5);
    return output;
}

const uint = value => head(0, value);
const bstr = value => Buffer.concat([head(2, value.length), value]);
const text = value => {
    const encoded = Buffer.from(value, 'utf8');
    return Buffer.concat([head(3, encoded.length), encoded]);
};
const array = values => Buffer.concat([head(4, values.length), ...values]);
const map = entries => Buffer.concat([
    head(5, entries.length),
    ...entries.flatMap(([key, value]) => [uint(key), value])
]);

function grantTranscript(input) {
    return array([
        text('blinker.controller-grant.v2'), uint(2), uint(input.operation),
        bstr(input.grantId), bstr(input.deviceId), uint(input.generation),
        bstr(input.controllerId),
        uint(input.expectedVersion), uint(input.version), uint(input.permissions),
        bstr(input.secretDigest), bstr(input.nonce), uint(input.serverKeyId),
        uint(2)
    ]);
}

function grantBody(input) {
    return array([
        uint(2), uint(input.operation), bstr(input.grantId),
        bstr(input.deviceId), uint(input.generation), bstr(input.controllerId),
        uint(input.expectedVersion), uint(input.version), uint(input.permissions),
        bstr(input.secretDigest), bstr(input.nonce), uint(input.serverKeyId), uint(2),
        bstr(input.signature)
    ]);
}

function receiptTranscript(input) {
    return array([
        text('blinker.controller-receipt.v2'), uint(2), uint(input.operation),
        bstr(input.grantId), bstr(input.deviceId), uint(input.generation),
        bstr(input.controllerId), uint(input.version), uint(input.permissions),
        bstr(input.secretDigest), uint(input.proofKind)
    ]);
}

function receiptBody(input) {
    const values = [
        uint(2), uint(input.operation), bstr(input.grantId), bstr(input.deviceId),
        uint(input.generation), bstr(input.controllerId), uint(input.version),
        uint(input.permissions), bstr(input.secretDigest), uint(input.proofKind),
        bstr(input.proof)
    ];
    return map(values.map((value, key) => [key, value]));
}

function commitBody(input) {
    const values = [
        uint(2), bstr(input.grantId), uint(input.operation), bstr(input.deviceId),
        uint(input.generation), bstr(input.controllerId), uint(input.version),
        bstr(input.receiptDigest), uint(input.verification),
        bstr(input.idempotencyKey)
    ];
    return map(values.map((value, key) => [key, value]));
}

function main() {
    if (process.argv.length < 3 || process.argv.length > 4) {
        throw new Error('usage: server_controller_control_golden <fixture> [--emit]');
    }
    const properties = parseProperties(process.argv[2]);
    const common = {
        deviceId: bytes(properties, 'device_instance_id'),
        generation: number(properties, 'ownership_generation'),
        controllerId: bytes(properties, 'controller_id'),
        version: 1,
        serverKeyId: number(properties, 'server_key_id')
    };
    const secret = bytes(properties, 'controller_secret');
    const secretDigest = crypto.createHash('sha256').update(secret).digest();
    const install = {
        ...common,
        operation: 1,
        grantId: bytes(properties, 'install_grant_id'),
        expectedVersion: 0,
        permissions: number(properties, 'permissions'),
        secretDigest,
        nonce: bytes(properties, 'install_control_nonce'),
        signature: bytes(properties, 'install_signature')
    };
    const revoke = {
        ...common,
        operation: 3,
        grantId: bytes(properties, 'revoke_grant_id'),
        expectedVersion: 1,
        permissions: 0,
        secretDigest: Buffer.alloc(32),
        nonce: bytes(properties, 'revoke_control_nonce'),
        signature: bytes(properties, 'revoke_signature')
    };

    const outputs = new Map();
    outputs.set('expected.install_secret_digest', secretDigest);
    const installTranscript = grantTranscript(install);
    outputs.set('expected.install_grant_transcript', installTranscript);
    outputs.set('expected.install_grant_digest',
        crypto.createHash('sha256').update(installTranscript).digest());
    outputs.set('expected.install_grant', grantBody(install));
    const installReceipt = {
        ...install,
        proofKind: 1,
        proof: Buffer.alloc(0)
    };
    const receiptTranscriptBytes = receiptTranscript(installReceipt);
    outputs.set('expected.install_receipt_transcript', receiptTranscriptBytes);
    installReceipt.proof = crypto.createHmac('sha256', secret)
        .update(receiptTranscriptBytes).digest();
    outputs.set('expected.install_receipt_proof', installReceipt.proof);
    const receiptBytes = receiptBody(installReceipt);
    outputs.set('expected.install_receipt', receiptBytes);
    const receiptDigest = crypto.createHash('sha256').update(receiptBytes).digest();
    outputs.set('expected.install_receipt_digest', receiptDigest);
    outputs.set('expected.install_commit', commitBody({
        ...install,
        receiptDigest,
        verification: 1,
        idempotencyKey: bytes(properties, 'commit_idempotency_key')
    }));
    outputs.set('expected.revoke_grant_transcript', grantTranscript(revoke));
    outputs.set('expected.revoke_grant', grantBody(revoke));
    outputs.set('expected.revoke_receipt', receiptBody({
        ...revoke,
        proofKind: 0,
        proof: Buffer.alloc(0)
    }));

    if (process.argv[3] === '--emit') {
        for (const [key, value] of outputs) {
            console.log(`${key}=${value.toString('hex')}`);
        }
        return;
    }
    for (const [key, value] of outputs) {
        if (value.toString('hex') !== required(properties, key)) {
            throw new Error(`${key} does not match fixture`);
        }
    }
    console.log(`Node controller control golden passed: ${outputs.size} exact values`);
}

main();
