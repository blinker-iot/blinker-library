'use strict';

const crypto = require('crypto');
const fs = require('fs');

const AUTH_DOMAIN = Buffer.from('BLINKER-DIRECT-AUTH/1', 'ascii');
const KDF_INFO = Buffer.from('BLINKER-DIRECT-SESSION/1', 'ascii');
const RECORD_DOMAIN = Buffer.from('BLINKER-DIRECT-RECORD/1', 'ascii');
const TAG_SIZE = 16;

function inputVector() {
    return new Map([
        ['fixture.version', '1'],
        ['method', '2'],
        ['suite', '1'],
        ['secret', 'a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf'],
        ['controller_id', '101112131415161718191a1b1c1d1e1f'],
        ['domain', '1'],
        ['ownership_generation', '0'],
        ['credential_version', '7'],
        ['permissions', '3'],
        ['client_nonce', '202122232425262728292a2b2c2d2e2f'],
        ['device_nonce', '404142434445464748494a4b4c4d4e4f'],
        ['plaintext.command', '424b0212010a12340007a1646c616d70f4'],
        ['plaintext.ack', '424b0214020a20010005a100191234'],
        ['plaintext.patch', '424b0211010a2002000da30000010102a1646c616d70f5'],
        ['att.payload', '20'],
        ['fragment.header', '4'],
        ['fragment.frame_id', '7']
    ]);
}

function fromHex(value) {
    if (typeof value !== 'string' || !/^(?:[0-9a-f]{2})*$/.test(value)) {
        throw new Error(`invalid lowercase hex: ${value}`);
    }
    return Buffer.from(value, 'hex');
}

function hex(value) {
    return Buffer.from(value).toString('hex');
}

function u16(value) {
    const output = Buffer.alloc(2);
    output.writeUInt16BE(value);
    return output;
}

function u32(value) {
    const output = Buffer.alloc(4);
    output.writeUInt32BE(value);
    return output;
}

function hmac(key, ...parts) {
    const mac = crypto.createHmac('sha256', key);
    for (const part of parts) mac.update(part);
    return mac.digest();
}

function hkdf(salt, input, info, size) {
    const pseudoRandomKey = hmac(salt, input);
    const output = [];
    let previous = Buffer.alloc(0);
    for (let counter = 1; Buffer.concat(output).length < size; ++counter) {
        previous = hmac(
            pseudoRandomKey,
            previous,
            info,
            Buffer.from([counter]));
        output.push(previous);
    }
    return Buffer.concat(output).subarray(0, size);
}

function transcript(input) {
    return Buffer.concat([
        Buffer.from([1]),
        u16(Number(input.get('method'))),
        Buffer.from([Number(input.get('suite'))]),
        fromHex(input.get('controller_id')),
        Buffer.from([Number(input.get('domain'))]),
        u32(Number(input.get('ownership_generation'))),
        u32(Number(input.get('credential_version'))),
        u32(Number(input.get('permissions'))),
        fromHex(input.get('client_nonce')),
        fromHex(input.get('device_nonce'))
    ]);
}

function keyMaterial(input) {
    const transcriptHash = crypto.createHash('sha256')
        .update(AUTH_DOMAIN)
        .update(transcript(input))
        .digest();
    const material = hkdf(
        transcriptHash,
        fromHex(input.get('secret')),
        KDF_INFO,
        96);
    return {
        transcriptHash,
        initiatorKey: material.subarray(0, 32),
        responderKey: material.subarray(32, 64),
        initiatorPrefix: material.subarray(64, 72),
        responderPrefix: material.subarray(72, 80),
        binding: material.subarray(80, 96)
    };
}

function seal(key, prefix, binding, direction, sequence, plaintext) {
    const header = Buffer.concat([
        Buffer.from([0xd3, 0x10]),
        u16(plaintext.length),
        u32(sequence)
    ]);
    const nonce = Buffer.concat([prefix, u32(sequence)]);
    const aad = Buffer.concat([
        RECORD_DOMAIN,
        Buffer.from([direction]),
        binding,
        header
    ]);
    const cipher = crypto.createCipheriv(
        'aes-256-gcm', key, nonce, {authTagLength: TAG_SIZE});
    cipher.setAAD(aad);
    const ciphertext = Buffer.concat([cipher.update(plaintext), cipher.final()]);
    const output = Buffer.concat([ciphertext, cipher.getAuthTag()]);
    return {header, nonce, aad, output, record: Buffer.concat([header, output])};
}

function fragments(record, payloadSize, headerSize, frameId) {
    const capacity = payloadSize - headerSize;
    const output = [];
    for (let offset = 0, index = 0; offset < record.length; ++index) {
        const end = Math.min(record.length, offset + capacity);
        let flags = offset === 0 ? 1 : 0;
        if (end === record.length) flags |= 2;
        output.push(Buffer.concat([
            Buffer.from([0xb2, 0x10 | flags, frameId, index]),
            record.subarray(offset, end)
        ]));
        offset = end;
    }
    return output;
}

function outputs() {
    const input = inputVector();
    const material = keyMaterial(input);
    const command = seal(
        material.initiatorKey,
        material.initiatorPrefix,
        material.binding,
        1,
        0,
        fromHex(input.get('plaintext.command')));
    const ack = seal(
        material.responderKey,
        material.responderPrefix,
        material.binding,
        2,
        0,
        fromHex(input.get('plaintext.ack')));
    const patch = seal(
        material.responderKey,
        material.responderPrefix,
        material.binding,
        2,
        1,
        fromHex(input.get('plaintext.patch')));
    const records = {command, ack, patch};
    const result = new Map(input);
    result.set('expected.transcript_hash', hex(material.transcriptHash));
    result.set('expected.initiator_key', hex(material.initiatorKey));
    result.set('expected.responder_key', hex(material.responderKey));
    result.set('expected.initiator_nonce_prefix', hex(material.initiatorPrefix));
    result.set('expected.responder_nonce_prefix', hex(material.responderPrefix));
    result.set('expected.session_binding', hex(material.binding));
    let primitive = 0;
    for (const [name, record] of Object.entries(records)) {
        ++primitive;
        result.set(`primitive.aes.${primitive}.key`, hex(
            name === 'command' ? material.initiatorKey : material.responderKey));
        result.set(`primitive.aes.${primitive}.nonce`, hex(record.nonce));
        result.set(`primitive.aes.${primitive}.aad`, hex(record.aad));
        result.set(`primitive.aes.${primitive}.plaintext`, input.get(`plaintext.${name}`));
        result.set(`primitive.aes.${primitive}.output`, hex(record.output));
        result.set(`expected.record.${name}`, hex(record.record));
        const rawSize = fromHex(input.get(`plaintext.${name}`)).length;
        const capacity = Number(input.get('att.payload')) -
            Number(input.get('fragment.header'));
        result.set(`expected.att20.raw.${name}`, String(Math.ceil(rawSize / capacity)));
        result.set(`expected.att20.secure.${name}`, String(Math.ceil(record.record.length / capacity)));
    }
    result.set('primitive.aes.count', String(primitive));
    const packets = fragments(
        command.record,
        Number(input.get('att.payload')),
        Number(input.get('fragment.header')),
        Number(input.get('fragment.frame_id')));
    result.set('expected.fragment.command.count', String(packets.length));
    packets.forEach((packet, index) => {
        result.set(`expected.fragment.command.${index}`, hex(packet));
    });
    return result;
}

function parseProperties(path) {
    const result = new Map();
    const lines = fs.readFileSync(path, 'utf8').split(/\r?\n/);
    lines.forEach((line, index) => {
        if (line.length === 0 || line.startsWith('#')) return;
        const separator = line.indexOf('=');
        if (separator <= 0 || result.has(line.slice(0, separator))) {
            throw new Error(`invalid property at line ${index + 1}`);
        }
        result.set(line.slice(0, separator), line.slice(separator + 1));
    });
    return result;
}

function main() {
    const expected = outputs();
    if (process.argv.includes('--emit')) {
        for (const [key, value] of expected) console.log(`${key}=${value}`);
        return;
    }
    if (process.argv.length !== 3) {
        throw new Error('usage: server_direct_secure_golden.cjs <fixture>');
    }
    const fixture = parseProperties(process.argv[2]);
    for (const [key, value] of expected) {
        if (fixture.get(key) !== value) throw new Error(`mismatch: ${key}`);
    }
    if (fixture.size !== expected.size) throw new Error('unexpected fixture property');
    console.log(`Node DirectSecureRecord golden passed: ${expected.size} exact values`);
}

main();
