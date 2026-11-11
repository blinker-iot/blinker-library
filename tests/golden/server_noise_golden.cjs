'use strict';

const crypto = require('crypto');
const fs = require('fs');

const PROTOCOLS = {
    NN: Buffer.from('Noise_NN_25519_AESGCM_SHA256', 'ascii'),
    NNpsk0: Buffer.from('Noise_NNpsk0_25519_AESGCM_SHA256', 'ascii')
};
const TAG_SIZE = 16;
const PKCS8_X25519_PREFIX = Buffer.from(
    '302e020100300506032b656e04220420',
    'hex');
let activeTrace = null;

function hex(value) {
    return Buffer.from(value).toString('hex');
}

function fromHex(value) {
    if (typeof value !== 'string' || (value.length & 1) !== 0 ||
        !/^[0-9a-f]*$/.test(value)) {
        throw new Error(`invalid lowercase hex: ${value}`);
    }
    return Buffer.from(value, 'hex');
}

function sha256(...parts) {
    const hash = crypto.createHash('sha256');
    for (const part of parts) hash.update(part);
    return hash.digest();
}

function hmac(key, ...parts) {
    const mac = crypto.createHmac('sha256', key);
    for (const part of parts) mac.update(part);
    return mac.digest();
}

function hkdf(chainingKey, input, count) {
    const temporary = hmac(chainingKey, input);
    const outputs = [];
    let previous = Buffer.alloc(0);
    for (let index = 1; index <= count; ++index) {
        previous = hmac(temporary, previous, Buffer.from([index]));
        outputs.push(previous);
    }
    return outputs;
}

function privateKey(raw) {
    return crypto.createPrivateKey({
        key: Buffer.concat([PKCS8_X25519_PREFIX, raw]),
        format: 'der',
        type: 'pkcs8'
    });
}

function publicKey(rawPrivate) {
    const der = crypto.createPublicKey(privateKey(rawPrivate)).export({
        format: 'der',
        type: 'spki'
    });
    const result = der.subarray(der.length - 32);
    if (activeTrace !== null) {
        const normalized = Buffer.from(rawPrivate);
        normalized[0] &= 248;
        normalized[31] &= 127;
        normalized[31] |= 64;
        activeTrace.push({
            kind: 'public',
            privateKey: normalized,
            output: Buffer.from(result)
        });
    }
    return result;
}

function sharedSecret(rawPrivate, rawRemotePublic) {
    const spkiPrefix = Buffer.from('302a300506032b656e032100', 'hex');
    const remote = crypto.createPublicKey({
        key: Buffer.concat([spkiPrefix, rawRemotePublic]),
        format: 'der',
        type: 'spki'
    });
    const result = crypto.diffieHellman({
        privateKey: privateKey(rawPrivate),
        publicKey: remote
    });
    if (activeTrace !== null) {
        const normalized = Buffer.from(rawPrivate);
        normalized[0] &= 248;
        normalized[31] &= 127;
        normalized[31] |= 64;
        activeTrace.push({
            kind: 'dh',
            privateKey: normalized,
            remotePublic: Buffer.from(rawRemotePublic),
            output: Buffer.from(result)
        });
    }
    return result;
}

function nonceBytes(value) {
    const result = Buffer.alloc(12);
    result.writeBigUInt64BE(value, 4);
    return result;
}

function aesGcmEncrypt(key, nonce, aad, plaintext) {
    const cipher = crypto.createCipheriv(
        'aes-256-gcm',
        key,
        nonceBytes(nonce),
        {authTagLength: TAG_SIZE});
    cipher.setAAD(aad);
    const ciphertext = Buffer.concat([
        cipher.update(plaintext),
        cipher.final()
    ]);
    const result = Buffer.concat([ciphertext, cipher.getAuthTag()]);
    if (activeTrace !== null) {
        activeTrace.push({
            kind: 'aes',
            key: Buffer.from(key),
            nonce: nonceBytes(nonce),
            aad: Buffer.from(aad),
            plaintext: Buffer.from(plaintext),
            output: Buffer.from(result)
        });
    }
    return result;
}

class CipherState {
    constructor(key) {
        this.key = Buffer.from(key);
        this.nonce = 0n;
    }

    encrypt(plaintext, aad = Buffer.alloc(0)) {
        if (this.nonce === 0xffffffffffffffffn) {
            throw new Error('Noise nonce exhausted');
        }
        const result = aesGcmEncrypt(this.key, this.nonce, aad, plaintext);
        this.nonce += 1n;
        return result;
    }
}

class SymmetricState {
    constructor(pattern, prologue, psk = Buffer.alloc(0)) {
        const protocol = PROTOCOLS[pattern];
        if (!protocol) throw new Error(`unsupported Noise pattern: ${pattern}`);
        if (protocol.length <= 32) {
            this.h = Buffer.alloc(32);
            protocol.copy(this.h);
        } else {
            this.h = sha256(protocol);
        }
        this.ck = Buffer.from(this.h);
        this.key = null;
        this.nonce = 0n;
        this.pattern = pattern;
        this.mixHash(prologue);
        if (pattern === 'NNpsk0') this.mixKeyAndHash(psk);
    }

    mixHash(data) {
        this.h = sha256(this.h, data);
    }

    mixKey(data) {
        const outputs = hkdf(this.ck, data, 2);
        this.ck = outputs[0];
        this.key = outputs[1];
        this.nonce = 0n;
    }

    mixKeyAndHash(data) {
        const outputs = hkdf(this.ck, data, 3);
        this.ck = outputs[0];
        this.mixHash(outputs[1]);
        this.key = outputs[2];
        this.nonce = 0n;
    }

    mixEphemeral(publicBytes) {
        this.mixHash(publicBytes);
        if (this.pattern === 'NNpsk0') this.mixKey(publicBytes);
    }

    encryptAndHash(plaintext) {
        const ciphertext = this.key === null
            ? Buffer.from(plaintext)
            : aesGcmEncrypt(this.key, this.nonce++, this.h, plaintext);
        this.mixHash(ciphertext);
        return ciphertext;
    }

    split() {
        const outputs = hkdf(this.ck, Buffer.alloc(0), 2);
        return [new CipherState(outputs[0]), new CipherState(outputs[1])];
    }
}

function runVector(input) {
    const state = new SymmetricState(input.pattern, input.prologue, input.psk);
    const initiatorPublic = publicKey(input.initiatorPrivate);
    state.mixEphemeral(initiatorPublic);
    const message1 = Buffer.concat([
        initiatorPublic,
        state.encryptAndHash(input.handshakePayloads[0])
    ]);

    const responderPublic = publicKey(input.responderPrivate);
    state.mixEphemeral(responderPublic);
    state.mixKey(sharedSecret(input.initiatorPrivate, responderPublic));
    const message2 = Buffer.concat([
        responderPublic,
        state.encryptAndHash(input.handshakePayloads[1])
    ]);
    const handshakeHash = Buffer.from(state.h);
    const [initiatorToResponder, responderToInitiator] = state.split();
    const transport = [];
    for (let index = 0; index < input.transportPayloads.length; ++index) {
        const cipher = (index & 1) === 0
            ? initiatorToResponder
            : responderToInitiator;
        transport.push(cipher.encrypt(input.transportPayloads[index]));
    }
    return {message1, message2, handshakeHash, transport};
}

function encodeRecord(type, body) {
    if (type < 1 || type > 3 || body.length === 0 || body.length > 1024) {
        throw new Error('invalid local secure record');
    }
    const header = Buffer.alloc(4);
    header[0] = 0xb3;
    header[1] = 0x10 | type;
    header.writeUInt16BE(body.length, 2);
    return Buffer.concat([header, body]);
}

function assertHex(label, actual, expected) {
    const actualHex = hex(actual);
    if (actualHex !== expected) {
        throw new Error(`${label}\nexpected=${expected}\nactual  =${actualHex}`);
    }
}

function verifyCacophony() {
    const result = runVector({
        pattern: 'NNpsk0',
        prologue: fromHex('4a6f686e2047616c74'),
        psk: fromHex(
            '54686973206973206d7920417573747269616e20706572737065637469766521'),
        initiatorPrivate: fromHex(
            '893e28b9dc6ca8d611ab664754b8ceb7bac5117349a4439a6b0569da977c464a'),
        responderPrivate: fromHex(
            'bbdb4cdbd309f1a1f2e1456967fe288cadd6f712d65dc7b7793d5e63da6b375b'),
        handshakePayloads: [
            fromHex('4c756477696720766f6e204d69736573'),
            fromHex('4d757272617920526f746862617264')
        ],
        transportPayloads: [
            fromHex('462e20412e20486179656b'),
            fromHex('4361726c204d656e676572'),
            fromHex('4a65616e2d426170746973746520536179'),
            fromHex('457567656e2042f6686d20766f6e2042617765726b')
        ]
    });
    assertHex(
        'cacophony.message1',
        result.message1,
        'ca35def5ae56cec33dc2036731ab14896bc4c75dbb07a61f879f8e3afa4c7944' +
        '7c37a89fc17813788d30df2d59501d6066f5f8aecc3406bbd9829f2d24a531b1');
    assertHex(
        'cacophony.message2',
        result.message2,
        '95ebc60d2b1fa672c1f46a8aa265ef51bfe38e7ccb39ec5be34069f144808843' +
        '2f29d79368ef9de4e1ae7a362fb05bf0ad668e026bf5714554497b3a720461');
    assertHex(
        'cacophony.handshakeHash',
        result.handshakeHash,
        '9f3b1f9afd7767767b5b9d1069844a6154fdbdcc36dfb3e31c0fffc5c973d530');
    const expectedTransport = [
        '7c01e898a6201f36aecee532e29cd0b7f8ffa29d3366ad0dce39a6',
        'a9ff535aace3db6a1da1a7fb00fe2de2622bb373abad9fb42c00e0',
        '6cfc49a3e65472edc2d0152abe5ce950170a8a47c959f30bd48b5f0aacf29adbfd',
        '9bc86e6d974ed21ba04604a94aa68de21677eb409ead070c0cd569b069ee19ba142aa4c1a9'
    ];
    result.transport.forEach((value, index) =>
        assertHex(`cacophony.transport${index}`, value, expectedTransport[index]));
}

function blinkerInputs() {
    return {
        pattern: 'NN',
        prologue: Buffer.concat([
            Buffer.from('BLINKER-LOCAL-SETUP-V1', 'ascii'),
            Buffer.from([0xb3, 0x01, 0x01])
        ]),
        initiatorPrivate: fromHex(
            '202122232425262728292a2b2c2d2e2f' +
            '303132333435363738393a3b3c3d3e3f'),
        responderPrivate: fromHex(
            '404142434445464748494a4b4c4d4e4f' +
            '505152535455565758595a5b5c5d5e5f'),
        handshakePayloads: [
            fromHex('a200010101'),
            fromHex('a10001')
        ],
        transportPayloads: [
            fromHex('01040007a10001'),
            fromHex('01050007')
        ]
    };
}

function parseProperties(path) {
    const result = new Map();
    const source = fs.readFileSync(path, 'utf8');
    for (const rawLine of source.split(/\r?\n/)) {
        const line = rawLine.trim();
        if (line.length === 0 || line.startsWith('#')) continue;
        const split = line.indexOf('=');
        if (split <= 0) throw new Error(`invalid property: ${rawLine}`);
        result.set(line.slice(0, split), line.slice(split + 1));
    }
    return result;
}

function blinkerOutputs() {
    activeTrace = [];
    const result = runVector(blinkerInputs());
    const trace = activeTrace;
    activeTrace = null;
    const output = new Map([
        ['expected.prologue', hex(blinkerInputs().prologue)],
        ['expected.message1', hex(result.message1)],
        ['expected.message2', hex(result.message2)],
        ['expected.handshake_hash', hex(result.handshakeHash)],
        ['expected.transport1', hex(result.transport[0])],
        ['expected.transport2', hex(result.transport[1])],
        ['expected.record1', hex(encodeRecord(1, result.message1))],
        ['expected.record2', hex(encodeRecord(2, result.message2))],
        ['expected.record3', hex(encodeRecord(3, result.transport[0]))]
    ]);
    const publicOps = trace.filter((item) => item.kind === 'public');
    const dhOps = trace.filter((item) => item.kind === 'dh');
    const aesOps = trace.filter((item) => item.kind === 'aes');
    output.set('primitive.public.count', String(publicOps.length));
    publicOps.forEach((item, index) => {
        const prefix = `primitive.public.${index + 1}`;
        output.set(`${prefix}.private`, hex(item.privateKey));
        output.set(`${prefix}.output`, hex(item.output));
    });
    output.set('primitive.dh.count', String(dhOps.length));
    dhOps.forEach((item, index) => {
        const prefix = `primitive.dh.${index + 1}`;
        output.set(`${prefix}.private`, hex(item.privateKey));
        output.set(`${prefix}.remote`, hex(item.remotePublic));
        output.set(`${prefix}.output`, hex(item.output));
    });
    output.set('primitive.aes.count', String(aesOps.length));
    aesOps.forEach((item, index) => {
        const prefix = `primitive.aes.${index + 1}`;
        output.set(`${prefix}.key`, hex(item.key));
        output.set(`${prefix}.nonce`, hex(item.nonce));
        output.set(`${prefix}.aad`, hex(item.aad));
        output.set(`${prefix}.plaintext`, hex(item.plaintext));
        output.set(`${prefix}.output`, hex(item.output));
    });
    return output;
}

function main() {
    verifyCacophony();
    const output = blinkerOutputs();
    if (process.argv.includes('--emit')) {
        for (const [key, value] of output) console.log(`${key}=${value}`);
        return;
    }
    const fixturePath = process.argv[2];
    if (!fixturePath) {
        throw new Error('usage: node server_noise_golden.cjs <fixture>');
    }
    const fixture = parseProperties(fixturePath);
    for (const [key, expected] of output) {
        if (fixture.get(key) !== expected) {
            throw new Error(`${key} does not match fixture`);
        }
    }
    console.log(`Node Noise golden passed: ${output.size} exact values + Cacophony`);
}

main();
