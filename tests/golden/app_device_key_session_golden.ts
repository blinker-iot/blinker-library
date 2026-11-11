/* Browser-safe App reference for DeviceKey session contract v1. */

type DeviceKeyProperties = { [key: string]: string };

function dkeyFail(message: string): never {
    throw new Error(`DeviceKey App golden: ${message}`);
}

function dkeyParse(text: string): DeviceKeyProperties {
    const output: DeviceKeyProperties = Object.create(null) as DeviceKeyProperties;
    text.split(/\r?\n/).forEach((line, index) => {
        if (line.length === 0 || line.charAt(0) === "#") return;
        const split = line.indexOf("=");
        const key = line.slice(0, split);
        if (split <= 0 || Object.prototype.hasOwnProperty.call(output, key)) {
            dkeyFail(`invalid fixture line ${index + 1}`);
        }
        output[key] = line.slice(split + 1);
    });
    return output;
}

function dkeyRequired(properties: DeviceKeyProperties, key: string): string {
    if (!Object.prototype.hasOwnProperty.call(properties, key)) {
        dkeyFail(`missing property: ${key}`);
    }
    return properties[key];
}

function dkeyNumber(properties: DeviceKeyProperties, key: string): number {
    const value = dkeyRequired(properties, key);
    if (!/^(?:0|[1-9][0-9]*)$/.test(value)) dkeyFail(`invalid number: ${key}`);
    const output = Number(value);
    if (!Number.isSafeInteger(output) || output > 0xffffffff) {
        dkeyFail(`number out of range: ${key}`);
    }
    return output;
}

function dkeyHexToBytes(value: string): number[] {
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) dkeyFail("invalid lowercase hex");
    const output: number[] = [];
    for (let index = 0; index < value.length; index += 2) {
        output.push(parseInt(value.slice(index, index + 2), 16));
    }
    return output;
}

function dkeyBytes(properties: DeviceKeyProperties, key: string): number[] {
    return dkeyHexToBytes(dkeyRequired(properties, key));
}

function dkeyHex(value: number[]): string {
    return value.map(byte => byte.toString(16).padStart(2, "0")).join("");
}

function dkeyAscii(value: string): number[] {
    const output: number[] = [];
    for (let index = 0; index < value.length; ++index) {
        const code = value.charCodeAt(index);
        if (code > 0x7f) dkeyFail("non-ASCII protocol constant");
        output.push(code);
    }
    return output;
}

function dkeyUtf8(value: string): number[] {
    return Array.from(new TextEncoder().encode(value));
}

function dkeyConcat(values: number[][]): number[] {
    const output: number[] = [];
    values.forEach(value => value.forEach(byte => output.push(byte)));
    return output;
}

const DKEY_SHA256_K = [
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

function dkeyRotateRight(value: number, count: number): number {
    return (value >>> count) | (value << (32 - count));
}

function dkeySha256(input: number[]): number[] {
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
            const small0 = dkeyRotateRight(x, 7) ^
                dkeyRotateRight(x, 18) ^ (x >>> 3);
            const small1 = dkeyRotateRight(y, 17) ^
                dkeyRotateRight(y, 19) ^ (y >>> 10);
            words[index] = (words[index - 16] + small0 +
                words[index - 7] + small1) | 0;
        }
        let a = state[0], b = state[1], c = state[2], d = state[3];
        let e = state[4], f = state[5], g = state[6], h = state[7];
        for (let index = 0; index < 64; ++index) {
            const big1 = dkeyRotateRight(e, 6) ^
                dkeyRotateRight(e, 11) ^ dkeyRotateRight(e, 25);
            const choice = (e & f) ^ (~e & g);
            const temporary1 = (h + big1 + choice +
                DKEY_SHA256_K[index] + words[index]) | 0;
            const big0 = dkeyRotateRight(a, 2) ^
                dkeyRotateRight(a, 13) ^ dkeyRotateRight(a, 22);
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

function dkeyHmac(key: number[], message: number[]): number[] {
    const normalized = key.length > 64 ? dkeySha256(key) : key.slice();
    while (normalized.length < 64) normalized.push(0);
    const inner = normalized.map(byte => byte ^ 0x36);
    const outer = normalized.map(byte => byte ^ 0x5c);
    return dkeySha256(outer.concat(dkeySha256(inner.concat(message))));
}

function dkeyHkdf(key: number[]): number[] {
    const pseudoRandomKey = dkeyHmac(new Array<number>(32).fill(0), key);
    return dkeyHmac(
        pseudoRandomKey,
        dkeyAscii("blinker/device-key/auth/v1").concat([1]));
}

const DKEY_BASE64URL =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

function dkeyEncode(key: number[]): string {
    if (key.length !== 32 || key.every(value => value === 0)) {
        dkeyFail("invalid raw DeviceKey");
    }
    let output = "";
    let index = 0;
    while (index < 30) {
        const a = key[index++], b = key[index++], c = key[index++];
        output += DKEY_BASE64URL.charAt(a >>> 2);
        output += DKEY_BASE64URL.charAt(((a & 3) << 4) | (b >>> 4));
        output += DKEY_BASE64URL.charAt(((b & 15) << 2) | (c >>> 6));
        output += DKEY_BASE64URL.charAt(c & 63);
    }
    const a = key[index++], b = key[index];
    output += DKEY_BASE64URL.charAt(a >>> 2);
    output += DKEY_BASE64URL.charAt(((a & 3) << 4) | (b >>> 4));
    output += DKEY_BASE64URL.charAt((b & 15) << 2);
    return output;
}

function dkeyDecode(text: string): number[] {
    if (!/^[A-Za-z0-9_-]{43}$/.test(text)) dkeyFail("invalid DeviceKey text");
    const values = text.split("").map(value => DKEY_BASE64URL.indexOf(value));
    if ((values[42] & 3) !== 0) dkeyFail("non-canonical DeviceKey text");
    const output: number[] = [];
    for (let index = 0; index < 40; index += 4) {
        output.push((values[index] << 2) | (values[index + 1] >>> 4));
        output.push(((values[index + 1] & 15) << 4) |
            (values[index + 2] >>> 2));
        output.push(((values[index + 2] & 3) << 6) | values[index + 3]);
    }
    output.push((values[40] << 2) | (values[41] >>> 4));
    output.push(((values[41] & 15) << 4) | (values[42] >>> 2));
    if (dkeyEncode(output) !== text) dkeyFail("non-canonical DeviceKey");
    return output;
}

function dkeyHead(major: number, value: number): number[] {
    if (value < 24) return [(major << 5) | value];
    if (value <= 0xff) return [(major << 5) | 24, value];
    if (value <= 0xffff) {
        return [(major << 5) | 25, (value >>> 8) & 0xff, value & 0xff];
    }
    return [(major << 5) | 26, (value >>> 24) & 0xff,
        (value >>> 16) & 0xff, (value >>> 8) & 0xff, value & 0xff];
}

function dkeyUint(value: number): number[] { return dkeyHead(0, value); }
function dkeyByteString(value: number[]): number[] {
    return dkeyHead(2, value.length).concat(value);
}
function dkeyText(value: string): number[] {
    const encoded = dkeyUtf8(value);
    return dkeyHead(3, encoded.length).concat(encoded);
}
function dkeyArray(values: number[][]): number[] {
    return dkeyConcat([dkeyHead(4, values.length)].concat(values));
}
function dkeyMap(entries: Array<[number, number[]]>): number[] {
    const output: number[][] = [dkeyHead(5, entries.length)];
    entries.forEach(entry => output.push(dkeyUint(entry[0]), entry[1]));
    return dkeyConcat(output);
}

function runDeviceKeyGolden(properties: DeviceKeyProperties): void {
    const key = dkeyBytes(properties, "raw_key");
    const encodedKey = dkeyEncode(key);
    if (dkeyHex(dkeyDecode(encodedKey)) !== dkeyHex(key)) {
        dkeyFail("DeviceKey roundtrip mismatch");
    }
    [
        "invalid.too_short", "invalid.prefixed", "invalid.padding",
        "invalid.standard_alphabet", "invalid.noncanonical_tail",
        "invalid.all_zero", "invalid.trailing_space"
    ].forEach(invalidKey => {
        let rejected = false;
        try { dkeyDecode(dkeyRequired(properties, invalidKey)); }
        catch (_) { rejected = true; }
        if (!rejected) dkeyFail(`invalid DeviceKey accepted: ${invalidKey}`);
    });
    const requestId = dkeyBytes(properties, "request_id");
    const locator = dkeySha256(dkeyConcat([
        dkeyAscii("blinker/device-key/locator/v1"), [0], key
    ])).slice(0, 16);
    const authKey = dkeyHkdf(key);
    const instance = dkeyBytes(properties, "device_instance_id");
    const clientNonce = dkeyBytes(properties, "client_nonce");
    const challengeId = dkeyBytes(properties, "challenge_id");
    const serverNonce = dkeyBytes(properties, "server_nonce");
    const credentialVersion = dkeyNumber(properties, "credential_version");
    const protocolVersion = dkeyNumber(properties, "protocol_version");
    const firmwareVersion = dkeyRequired(properties, "firmware_version");
    const security = dkeyNumber(properties, "requested_security");
    const algorithm = dkeyNumber(properties, "proof_algorithm");
    const transcript = dkeyArray([
        dkeyText("blinker/device-key/session-proof/v1"), dkeyUint(1),
        dkeyByteString(requestId), dkeyByteString(locator),
        dkeyByteString(instance), dkeyByteString(clientNonce),
        dkeyByteString(challengeId), dkeyByteString(serverNonce),
        dkeyUint(credentialVersion), dkeyUint(protocolVersion),
        dkeyText(firmwareVersion), dkeyUint(security), dkeyUint(algorithm)
    ]);
    const proof = dkeyHmac(authKey, transcript);
    const outputs: DeviceKeyProperties = Object.create(null) as DeviceKeyProperties;
    outputs["encoded_key"] = encodedKey;
    outputs["expected.locator"] = dkeyHex(locator);
    outputs["expected.auth_key"] = dkeyHex(authKey);
    outputs["expected.challenge_request"] = dkeyHex(dkeyMap([
        [0, dkeyUint(1)], [1, dkeyByteString(requestId)],
        [2, dkeyByteString(locator)], [3, dkeyByteString(instance)],
        [4, dkeyByteString(clientNonce)]
    ]));
    outputs["expected.challenge_response"] = dkeyHex(dkeyMap([
        [0, dkeyUint(1)], [1, dkeyByteString(requestId)],
        [2, dkeyByteString(challengeId)], [3, dkeyByteString(serverNonce)],
        [4, dkeyUint(credentialVersion)],
        [5, dkeyUint(dkeyNumber(properties, "challenge_expires"))]
    ]));
    outputs["expected.proof_transcript"] = dkeyHex(transcript);
    outputs["expected.proof"] = dkeyHex(proof);
    outputs["expected.session_request"] = dkeyHex(dkeyMap([
        [0, dkeyUint(1)], [1, dkeyByteString(requestId)],
        [2, dkeyByteString(challengeId)], [3, dkeyUint(protocolVersion)],
        [4, dkeyText(firmwareVersion)], [5, dkeyUint(security)],
        [6, dkeyUint(algorithm)], [7, dkeyByteString(proof)]
    ]));
    outputs["expected.session_response"] = dkeyHex(dkeyMap([
        [0, dkeyUint(1)], [1, dkeyByteString(requestId)],
        [2, dkeyUint(credentialVersion)],
        [3, dkeyUint(dkeyNumber(properties, "broker"))],
        [4, dkeyText(dkeyRequired(properties, "host"))],
        [5, dkeyUint(dkeyNumber(properties, "port"))],
        [6, dkeyUint(security)],
        [7, dkeyText(dkeyRequired(properties, "client_id"))],
        [8, dkeyText(dkeyRequired(properties, "username"))],
        [9, dkeyText(dkeyRequired(properties, "password"))],
        [10, dkeyText(dkeyRequired(properties, "publish_topic"))],
        [11, dkeyText(dkeyRequired(properties, "subscribe_topic"))],
        [12, dkeyByteString(dkeyBytes(properties, "session_id"))],
        [13, dkeyUint(dkeyNumber(properties, "session_expires"))],
        [14, dkeyUint(dkeyNumber(properties, "session_refresh_after"))]
    ]));
    const keys = Object.keys(outputs);
    keys.forEach(keyName => {
        if (outputs[keyName] !== dkeyRequired(properties, keyName)) {
            dkeyFail(`mismatch: ${keyName}`);
        }
    });
    console.log(`App DeviceKey session golden passed: ${keys.length} exact values`);
}

const dkeyFs = require("fs") as {
    readFileSync(path: string, encoding: string): string
};
if (process.argv.length !== 3) {
    dkeyFail("usage: app_device_key_session_golden <fixture>");
}
runDeviceKeyGolden(dkeyParse(dkeyFs.readFileSync(process.argv[2], "utf8")));
