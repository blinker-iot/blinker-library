/* Browser-safe App reference for Controller Control Contract v2. */

type ControlProperties = { [key: string]: string };

function controlFail(message: string): never {
    throw new Error(`controller control App golden: ${message}`);
}

function controlProperties(textValue: string): ControlProperties {
    const output: ControlProperties = Object.create(null) as ControlProperties;
    textValue.split(/\r?\n/).forEach((line, index) => {
        if (line.length === 0 || line.charAt(0) === "#") return;
        const split = line.indexOf("=");
        const key = line.slice(0, split);
        if (split <= 0 || Object.prototype.hasOwnProperty.call(output, key)) {
            controlFail(`invalid fixture line ${index + 1}`);
        }
        output[key] = line.slice(split + 1);
    });
    return output;
}

function controlRequired(properties: ControlProperties, key: string): string {
    if (!Object.prototype.hasOwnProperty.call(properties, key)) {
        controlFail(`missing property: ${key}`);
    }
    return properties[key];
}

function controlNumber(properties: ControlProperties, key: string): number {
    const textValue = controlRequired(properties, key);
    if (!/^(?:0|[1-9][0-9]*)$/.test(textValue)) {
        controlFail(`invalid number: ${key}`);
    }
    const output = Number(textValue);
    if (!Number.isSafeInteger(output)) controlFail(`number out of range: ${key}`);
    return output;
}

function controlHexBytes(value: string): number[] {
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) {
        controlFail("invalid lowercase hex");
    }
    const output: number[] = [];
    for (let index = 0; index < value.length; index += 2) {
        output.push(parseInt(value.slice(index, index + 2), 16));
    }
    return output;
}

function controlHex(value: number[]): string {
    return value.map(byte => byte.toString(16).padStart(2, "0")).join("");
}

function controlUtf8(value: string): number[] {
    const output: number[] = [];
    for (let index = 0; index < value.length; ++index) {
        let code = value.charCodeAt(index);
        if (code >= 0xd800 && code <= 0xdbff) {
            if (++index >= value.length) controlFail("unpaired surrogate");
            const low = value.charCodeAt(index);
            if (low < 0xdc00 || low > 0xdfff) controlFail("unpaired surrogate");
            code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
        } else if (code >= 0xdc00 && code <= 0xdfff) {
            controlFail("unpaired surrogate");
        }
        if (code <= 0x7f) output.push(code);
        else if (code <= 0x7ff) {
            output.push(0xc0 | (code >>> 6), 0x80 | (code & 0x3f));
        } else if (code <= 0xffff) {
            output.push(
                0xe0 | (code >>> 12),
                0x80 | ((code >>> 6) & 0x3f),
                0x80 | (code & 0x3f));
        } else {
            output.push(
                0xf0 | (code >>> 18),
                0x80 | ((code >>> 12) & 0x3f),
                0x80 | ((code >>> 6) & 0x3f),
                0x80 | (code & 0x3f));
        }
    }
    return output;
}

function controlHead(major: number, value: number): number[] {
    if (!Number.isSafeInteger(value) || value < 0) {
        controlFail("CBOR integer out of range");
    }
    const prefix = major << 5;
    if (value < 24) return [prefix | value];
    if (value <= 0xff) return [prefix | 24, value];
    if (value <= 0xffff) return [prefix | 25, value >>> 8, value & 0xff];
    if (value <= 0xffffffff) return [
        prefix | 26,
        (value >>> 24) & 0xff,
        (value >>> 16) & 0xff,
        (value >>> 8) & 0xff,
        value & 0xff
    ];
    const high = Math.floor(value / 0x100000000);
    const low = value - high * 0x100000000;
    return [
        prefix | 27,
        (high >>> 24) & 0xff,
        (high >>> 16) & 0xff,
        (high >>> 8) & 0xff,
        high & 0xff,
        (low >>> 24) & 0xff,
        (low >>> 16) & 0xff,
        (low >>> 8) & 0xff,
        low & 0xff
    ];
}

const controlUint = (value: number): number[] => controlHead(0, value);
const controlBytes = (value: number[]): number[] =>
    controlHead(2, value.length).concat(value);
const controlText = (value: string): number[] => {
    const encoded = controlUtf8(value);
    return controlHead(3, encoded.length).concat(encoded);
};
const controlArray = (values: number[][]): number[] =>
    controlHead(4, values.length).concat(...values);
const controlMap = (values: number[][]): number[] => {
    const pairs: number[][] = [];
    values.forEach((value, key) => {
        pairs.push(controlUint(key), value);
    });
    return controlHead(5, values.length).concat(...pairs);
};

const CONTROL_SHA256_K = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
];

function controlRotateRight(value: number, count: number): number {
    return (value >>> count) | (value << (32 - count));
}

function controlSha256(input: number[]): number[] {
    const padded = input.slice();
    const bitLength = input.length * 8;
    padded.push(0x80);
    while ((padded.length % 64) !== 56) padded.push(0);
    padded.push(0, 0, 0, 0,
        (bitLength >>> 24) & 0xff, (bitLength >>> 16) & 0xff,
        (bitLength >>> 8) & 0xff, bitLength & 0xff);
    const state = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    ];
    const words = new Array<number>(64);
    for (let block = 0; block < padded.length; block += 64) {
        for (let index = 0; index < 16; ++index) {
            const offset = block + index * 4;
            words[index] = (padded[offset] << 24) |
                (padded[offset + 1] << 16) |
                (padded[offset + 2] << 8) | padded[offset + 3];
        }
        for (let index = 16; index < 64; ++index) {
            const x = words[index - 15];
            const y = words[index - 2];
            const small0 = controlRotateRight(x, 7) ^
                controlRotateRight(x, 18) ^ (x >>> 3);
            const small1 = controlRotateRight(y, 17) ^
                controlRotateRight(y, 19) ^ (y >>> 10);
            words[index] = (words[index - 16] + small0 +
                words[index - 7] + small1) | 0;
        }
        let a = state[0], b = state[1], c = state[2], d = state[3];
        let e = state[4], f = state[5], g = state[6], h = state[7];
        for (let index = 0; index < 64; ++index) {
            const big1 = controlRotateRight(e, 6) ^
                controlRotateRight(e, 11) ^ controlRotateRight(e, 25);
            const choice = (e & f) ^ (~e & g);
            const temporary1 = (h + big1 + choice +
                CONTROL_SHA256_K[index] + words[index]) | 0;
            const big0 = controlRotateRight(a, 2) ^
                controlRotateRight(a, 13) ^ controlRotateRight(a, 22);
            const majority = (a & b) ^ (a & c) ^ (b & c);
            const temporary2 = (big0 + majority) | 0;
            h = g; g = f; f = e; e = (d + temporary1) | 0;
            d = c; c = b; b = a; a = (temporary1 + temporary2) | 0;
        }
        state[0] = (state[0] + a) | 0; state[1] = (state[1] + b) | 0;
        state[2] = (state[2] + c) | 0; state[3] = (state[3] + d) | 0;
        state[4] = (state[4] + e) | 0; state[5] = (state[5] + f) | 0;
        state[6] = (state[6] + g) | 0; state[7] = (state[7] + h) | 0;
    }
    const output: number[] = [];
    state.forEach(value => output.push(
        (value >>> 24) & 0xff, (value >>> 16) & 0xff,
        (value >>> 8) & 0xff, value & 0xff));
    return output;
}

function controlHmac(key: number[], message: number[]): number[] {
    let normalized = key.length > 64 ? controlSha256(key) : key.slice();
    while (normalized.length < 64) normalized.push(0);
    const inner = normalized.map(byte => byte ^ 0x36);
    const outer = normalized.map(byte => byte ^ 0x5c);
    return controlSha256(outer.concat(controlSha256(inner.concat(message))));
}

interface ControlInput {
    operation: number;
    grantId: number[];
    deviceId: number[];
    generation: number;
    controllerId: number[];
    expectedVersion: number;
    version: number;
    permissions: number;
    secretDigest: number[];
    nonce: number[];
    serverKeyId: number;
    signature: number[];
}

function controlGrantTranscript(input: ControlInput): number[] {
    return controlArray([
        controlText("blinker.controller-grant.v2"), controlUint(2),
        controlUint(input.operation), controlBytes(input.grantId),
        controlBytes(input.deviceId), controlUint(input.generation),
        controlBytes(input.controllerId),
        controlUint(input.expectedVersion), controlUint(input.version),
        controlUint(input.permissions), controlBytes(input.secretDigest),
        controlBytes(input.nonce), controlUint(input.serverKeyId), controlUint(2)
    ]);
}

function controlGrant(input: ControlInput): number[] {
    return controlArray([
        controlUint(2), controlUint(input.operation), controlBytes(input.grantId),
        controlBytes(input.deviceId), controlUint(input.generation),
        controlBytes(input.controllerId), controlUint(input.expectedVersion),
        controlUint(input.version), controlUint(input.permissions),
        controlBytes(input.secretDigest), controlBytes(input.nonce),
        controlUint(input.serverKeyId), controlUint(2), controlBytes(input.signature)
    ]);
}

function controlReceiptTranscript(input: ControlInput, proofKind: number): number[] {
    return controlArray([
        controlText("blinker.controller-receipt.v2"), controlUint(2),
        controlUint(input.operation), controlBytes(input.grantId),
        controlBytes(input.deviceId), controlUint(input.generation),
        controlBytes(input.controllerId), controlUint(input.version),
        controlUint(input.permissions), controlBytes(input.secretDigest),
        controlUint(proofKind)
    ]);
}

function controlReceipt(
    input: ControlInput,
    proofKind: number,
    proof: number[]): number[] {
    return controlMap([
        controlUint(2), controlUint(input.operation), controlBytes(input.grantId),
        controlBytes(input.deviceId), controlUint(input.generation),
        controlBytes(input.controllerId), controlUint(input.version),
        controlUint(input.permissions), controlBytes(input.secretDigest),
        controlUint(proofKind), controlBytes(proof)
    ]);
}

function runControlGolden(properties: ControlProperties): void {
    const deviceId = controlHexBytes(controlRequired(properties, "device_instance_id"));
    const controllerId = controlHexBytes(controlRequired(properties, "controller_id"));
    const secret = controlHexBytes(controlRequired(properties, "controller_secret"));
    const secretDigest = controlSha256(secret);
    const common = {
        deviceId,
        generation: controlNumber(properties, "ownership_generation"),
        controllerId,
        version: 1,
        serverKeyId: controlNumber(properties, "server_key_id")
    };
    const install: ControlInput = Object.assign({}, common, {
        operation: 1,
        grantId: controlHexBytes(controlRequired(properties, "install_grant_id")),
        expectedVersion: 0,
        permissions: controlNumber(properties, "permissions"),
        secretDigest,
        nonce: controlHexBytes(controlRequired(properties, "install_control_nonce")),
        signature: controlHexBytes(controlRequired(properties, "install_signature"))
    });
    const revoke: ControlInput = Object.assign({}, common, {
        operation: 3,
        grantId: controlHexBytes(controlRequired(properties, "revoke_grant_id")),
        expectedVersion: 1,
        permissions: 0,
        secretDigest: new Array<number>(32).fill(0),
        nonce: controlHexBytes(controlRequired(properties, "revoke_control_nonce")),
        signature: controlHexBytes(controlRequired(properties, "revoke_signature"))
    });
    const outputs: ControlProperties = Object.create(null) as ControlProperties;
    outputs["expected.install_secret_digest"] = controlHex(secretDigest);
    const grantTranscript = controlGrantTranscript(install);
    outputs["expected.install_grant_transcript"] = controlHex(grantTranscript);
    outputs["expected.install_grant_digest"] = controlHex(controlSha256(grantTranscript));
    outputs["expected.install_grant"] = controlHex(controlGrant(install));
    const receiptTranscript = controlReceiptTranscript(install, 1);
    outputs["expected.install_receipt_transcript"] = controlHex(receiptTranscript);
    const proof = controlHmac(secret, receiptTranscript);
    outputs["expected.install_receipt_proof"] = controlHex(proof);
    const receipt = controlReceipt(install, 1, proof);
    outputs["expected.install_receipt"] = controlHex(receipt);
    const receiptDigest = controlSha256(receipt);
    outputs["expected.install_receipt_digest"] = controlHex(receiptDigest);
    outputs["expected.install_commit"] = controlHex(controlMap([
        controlUint(2), controlBytes(install.grantId), controlUint(1),
        controlBytes(deviceId), controlUint(common.generation),
        controlBytes(controllerId), controlUint(1), controlBytes(receiptDigest),
        controlUint(1), controlBytes(controlHexBytes(
            controlRequired(properties, "commit_idempotency_key")))
    ]));
    outputs["expected.revoke_grant_transcript"] = controlHex(
        controlGrantTranscript(revoke));
    outputs["expected.revoke_grant"] = controlHex(controlGrant(revoke));
    outputs["expected.revoke_receipt"] = controlHex(
        controlReceipt(revoke, 0, []));

    Object.keys(outputs).forEach(key => {
        if (outputs[key] !== controlRequired(properties, key)) {
            controlFail(`mismatch: ${key}`);
        }
    });
    console.log(`App controller control golden passed: ${Object.keys(outputs).length} exact values`);
}

const controlFs = require("fs") as {
    readFileSync(path: string, encoding: string): string
};
if (process.argv.length !== 3) {
    controlFail("usage: app_controller_control_golden <fixture>");
}
runControlGolden(controlProperties(
    controlFs.readFileSync(process.argv[2], "utf8")));
