'use strict';

const crypto = require('crypto');
const fs = require('fs');

const DOMAIN_LOCATOR = Buffer.from('blinker/device-key/locator/v1', 'ascii');
const INFO_AUTH = Buffer.from('blinker/device-key/auth/v1', 'ascii');
const DOMAIN_PROOF = 'blinker/device-key/session-proof/v1';
const BASE64URL = /^[A-Za-z0-9_-]{43}$/;

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
    const value = required(properties, key);
    if (!/^(?:0|[1-9][0-9]*)$/.test(value)) {
        throw new Error(`invalid number: ${key}`);
    }
    const output = Number(value);
    if (!Number.isSafeInteger(output) || output > 0xffffffff) {
        throw new Error(`number out of range: ${key}`);
    }
    return output;
}

function head(major, value) {
    if (!Number.isSafeInteger(value) || value < 0) {
        throw new Error('invalid CBOR head');
    }
    if (value < 24) return Buffer.from([(major << 5) | value]);
    if (value <= 0xff) return Buffer.from([(major << 5) | 24, value]);
    if (value <= 0xffff) {
        const output = Buffer.alloc(3);
        output[0] = (major << 5) | 25;
        output.writeUInt16BE(value, 1);
        return output;
    }
    const output = Buffer.alloc(5);
    output[0] = (major << 5) | 26;
    output.writeUInt32BE(value, 1);
    return output;
}

function uint(value) { return head(0, value); }
function byteString(value) { return Buffer.concat([head(2, value.length), value]); }
function text(value) {
    const encoded = Buffer.from(value, 'utf8');
    return Buffer.concat([head(3, encoded.length), encoded]);
}
function array(values) {
    return Buffer.concat([head(4, values.length), ...values]);
}
function map(entries) {
    const values = [head(5, entries.length)];
    for (const [key, value] of entries) values.push(uint(key), value);
    return Buffer.concat(values);
}

function encodeDeviceKey(key) {
    if (key.length !== 32 || key.every(value => value === 0)) {
        throw new Error('invalid raw DeviceKey');
    }
    return key.toString('base64url');
}

function decodeDeviceKey(value) {
    if (!BASE64URL.test(value)) throw new Error('invalid DeviceKey text');
    const output = Buffer.from(value, 'base64url');
    if (output.length !== 32 || encodeDeviceKey(output) !== value) {
        throw new Error('non-canonical DeviceKey text');
    }
    return output;
}

function outputsFrom(properties) {
    const key = bytes(properties, 'raw_key');
    const requestId = bytes(properties, 'request_id');
    const locator = crypto.createHash('sha256')
        .update(DOMAIN_LOCATOR).update(Buffer.from([0])).update(key)
        .digest().subarray(0, 16);
    const authKey = Buffer.from(crypto.hkdfSync(
        'sha256', key, Buffer.alloc(0), INFO_AUTH, 32));
    const instance = bytes(properties, 'device_instance_id');
    const clientNonce = bytes(properties, 'client_nonce');
    const challengeId = bytes(properties, 'challenge_id');
    const serverNonce = bytes(properties, 'server_nonce');
    const credentialVersion = number(properties, 'credential_version');
    const protocolVersion = number(properties, 'protocol_version');
    const firmwareVersion = required(properties, 'firmware_version');
    const security = number(properties, 'requested_security');
    const algorithm = number(properties, 'proof_algorithm');
    const challengeExpires = number(properties, 'challenge_expires');
    const challengeRequest = map([
        [0, uint(1)], [1, byteString(requestId)], [2, byteString(locator)],
        [3, byteString(instance)], [4, byteString(clientNonce)]
    ]);
    const challengeResponse = map([
        [0, uint(1)], [1, byteString(requestId)],
        [2, byteString(challengeId)], [3, byteString(serverNonce)],
        [4, uint(credentialVersion)], [5, uint(challengeExpires)]
    ]);
    const transcript = array([
        text(DOMAIN_PROOF), uint(1), byteString(requestId),
        byteString(locator), byteString(instance), byteString(clientNonce),
        byteString(challengeId), byteString(serverNonce),
        uint(credentialVersion), uint(protocolVersion), text(firmwareVersion),
        uint(security), uint(algorithm)
    ]);
    const proof = crypto.createHmac('sha256', authKey)
        .update(transcript).digest();
    const sessionRequest = map([
        [0, uint(1)], [1, byteString(requestId)],
        [2, byteString(challengeId)], [3, uint(protocolVersion)],
        [4, text(firmwareVersion)], [5, uint(security)],
        [6, uint(algorithm)], [7, byteString(proof)]
    ]);
    const sessionResponse = map([
        [0, uint(1)], [1, byteString(requestId)],
        [2, uint(credentialVersion)], [3, uint(number(properties, 'broker'))],
        [4, text(required(properties, 'host'))],
        [5, uint(number(properties, 'port'))], [6, uint(security)],
        [7, text(required(properties, 'client_id'))],
        [8, text(required(properties, 'username'))],
        [9, text(required(properties, 'password'))],
        [10, text(required(properties, 'publish_topic'))],
        [11, text(required(properties, 'subscribe_topic'))],
        [12, byteString(bytes(properties, 'session_id'))],
        [13, uint(number(properties, 'session_expires'))],
        [14, uint(number(properties, 'session_refresh_after'))]
    ]);
    return new Map([
        ['encoded_key', Buffer.from(encodeDeviceKey(key), 'ascii')],
        ['expected.locator', locator],
        ['expected.auth_key', authKey],
        ['expected.challenge_request', challengeRequest],
        ['expected.challenge_response', challengeResponse],
        ['expected.proof_transcript', transcript],
        ['expected.proof', proof],
        ['expected.session_request', sessionRequest],
        ['expected.session_response', sessionResponse]
    ]);
}

function emitFixture() {
    const seed = new Map([
        ['raw_key', Buffer.from(Array.from({ length: 32 }, (_, i) => i)).toString('hex')],
        ['request_id', Buffer.from(Array.from({ length: 16 }, (_, i) => 0x10 + i)).toString('hex')],
        ['device_instance_id', Buffer.from(Array.from({ length: 16 }, (_, i) => 0x30 + i)).toString('hex')],
        ['client_nonce', Buffer.from(Array.from({ length: 16 }, (_, i) => 0x40 + i)).toString('hex')],
        ['challenge_id', Buffer.from(Array.from({ length: 16 }, (_, i) => 0x50 + i)).toString('hex')],
        ['server_nonce', Buffer.from(Array.from({ length: 16 }, (_, i) => 0x60 + i)).toString('hex')],
        ['credential_version', '7'], ['protocol_version', '2'],
        ['firmware_version', '2.0.0-test'], ['requested_security', '0'],
        ['proof_algorithm', '1'], ['challenge_expires', '120'],
        ['broker', '1'], ['host', 'mqtt.example.test'], ['port', '1883'],
        ['client_id', 'dvc-123'], ['username', 'user'], ['password', 'secret'],
        ['publish_topic', 'd/dvc-123/up'],
        ['subscribe_topic', 'd/dvc-123/down'],
        ['session_id', Buffer.from(Array.from({ length: 16 }, (_, i) => 0xb0 + i)).toString('hex')],
        ['session_expires', '3600'], ['session_refresh_after', '1800']
    ]);
    const outputs = outputsFrom(seed);
    console.log('# DeviceKey session contract v1 exact vector');
    for (const [key, value] of seed) console.log(`${key}=${value}`);
    for (const [key, value] of outputs) {
        console.log(`${key}=${key === 'encoded_key' ? value.toString('ascii') : value.toString('hex')}`);
    }
}

function main() {
    if (process.argv.length === 3 && process.argv[2] === '--emit') {
        emitFixture();
        return;
    }
    if (process.argv.length !== 3) {
        throw new Error('usage: server_device_key_session_golden <fixture>');
    }
    const properties = parseProperties(process.argv[2]);
    const decoded = decodeDeviceKey(required(properties, 'encoded_key'));
    if (!decoded.equals(bytes(properties, 'raw_key'))) {
        throw new Error('DeviceKey decode mismatch');
    }
    for (const key of [
        'invalid.too_short', 'invalid.prefixed', 'invalid.padding',
        'invalid.standard_alphabet', 'invalid.noncanonical_tail',
        'invalid.all_zero', 'invalid.trailing_space'
    ]) {
        let rejected = false;
        try { decodeDeviceKey(required(properties, key)); }
        catch (_) { rejected = true; }
        if (!rejected) throw new Error(`invalid DeviceKey accepted: ${key}`);
    }
    const outputs = outputsFrom(properties);
    for (const [key, value] of outputs) {
        const expected = required(properties, key);
        const actual = key === 'encoded_key'
            ? value.toString('ascii') : value.toString('hex');
        if (actual !== expected) throw new Error(`${key} mismatch`);
    }
    console.log(`Node DeviceKey session golden passed: ${outputs.size} exact values`);
}

main();
