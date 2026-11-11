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

function u32(value) {
    const output = Buffer.alloc(4);
    output.writeUInt32BE(value);
    return output;
}

function main() {
    if (process.argv.length !== 3) {
        throw new Error('usage: server_controller_auth_golden <fixture>');
    }
    const properties = parseProperties(process.argv[2]);
    const method = Number(required(properties, 'method'));
    const suite = Number(required(properties, 'suite'));
    const domain = Number(required(properties, 'domain'));
    const generation = Number(required(properties, 'ownership_generation'));
    const version = Number(required(properties, 'credential_version'));
    const permissions = Number(required(properties, 'permissions'));
    const secret = bytes(properties, 'secret');
    const controllerId = bytes(properties, 'controller_id');
    const clientNonce = bytes(properties, 'client_nonce');
    const deviceNonce = bytes(properties, 'device_nonce');
    const prefix = Buffer.from([1, method >>> 8, method & 0xff, suite]);
    const metadata = Buffer.concat([
        Buffer.from([domain]), u32(generation), u32(version), u32(permissions)
    ]);
    const outputs = new Map();
    outputs.set('expected.init', Buffer.concat([
        Buffer.from([1]), controllerId, Buffer.from([domain]),
        u32(generation), u32(version), clientNonce
    ]));
    let deviceProof;
    for (const role of ['device', 'app']) {
        const roleDomain = Buffer.from(
            role === 'device'
                ? 'BLINKER-CONTROLLER-DEVICE-PROOF/1'
                : 'BLINKER-CONTROLLER-APP-PROOF/1',
            'ascii');
        const transcript = Buffer.concat([
            roleDomain, prefix, controllerId, metadata,
            clientNonce, deviceNonce
        ]);
        const proof = crypto.createHmac('sha256', secret)
            .update(transcript).digest();
        outputs.set(`expected.${role}_transcript`, transcript);
        outputs.set(`expected.${role}_proof`, proof);
        if (role === 'device') deviceProof = proof;
        else outputs.set('expected.proof_payload',
            Buffer.concat([Buffer.from([2]), proof]));
    }
    outputs.set('expected.challenge', Buffer.concat([
        Buffer.from([0x81]), deviceNonce, u32(permissions), deviceProof
    ]));
    for (const [key, value] of outputs) {
        if (value.toString('hex') !== required(properties, key)) {
            throw new Error(`${key} does not match fixture`);
        }
    }
    console.log(`Node controller authorization golden passed: ${outputs.size} exact values`);
}

main();
