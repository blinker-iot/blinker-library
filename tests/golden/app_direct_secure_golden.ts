/* Browser-safe App reference for DirectSecureRecord v1. AES-GCM outputs are
 * replayed from the Node/OpenSSL trace so this gate independently validates
 * transcript, HKDF, nonce, AAD, direction, sequence and outer wire bytes. */

type DirectProperties = { [key: string]: string };

function directFail(message: string): never {
    throw new Error(`DirectSecureRecord App golden: ${message}`);
}

function parseProperties(text: string): DirectProperties {
    const output: DirectProperties = Object.create(null) as DirectProperties;
    text.split(/\r?\n/).forEach((line, index) => {
        if (line.length === 0 || line.charAt(0) === "#") return;
        const split = line.indexOf("=");
        const key = line.slice(0, split);
        if (split <= 0 || Object.prototype.hasOwnProperty.call(output, key)) {
            directFail(`invalid fixture line ${index + 1}`);
        }
        output[key] = line.slice(split + 1);
    });
    return output;
}

function required(properties: DirectProperties, key: string): string {
    if (!Object.prototype.hasOwnProperty.call(properties, key)) {
        directFail(`missing property: ${key}`);
    }
    return properties[key];
}

function numberValue(properties: DirectProperties, key: string): number {
    const value = required(properties, key);
    if (!/^(?:0|[1-9][0-9]*)$/.test(value)) directFail(`invalid number: ${key}`);
    const output = Number(value);
    if (!Number.isSafeInteger(output) || output > 0xffffffff) {
        directFail(`number out of range: ${key}`);
    }
    return output;
}

function fromHex(value: string): number[] {
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) directFail("invalid lowercase hex");
    const output: number[] = [];
    for (let index = 0; index < value.length; index += 2) {
        output.push(parseInt(value.slice(index, index + 2), 16));
    }
    return output;
}

function hex(value: number[]): string {
    return value.map(byte => byte.toString(16).padStart(2, "0")).join("");
}

function ascii(value: string): number[] {
    const output: number[] = [];
    for (let index = 0; index < value.length; ++index) {
        const code = value.charCodeAt(index);
        if (code > 0x7f) directFail("non-ASCII protocol constant");
        output.push(code);
    }
    return output;
}

function u16(value: number): number[] {
    return [(value >>> 8) & 0xff, value & 0xff];
}

function u32(value: number): number[] {
    return [
        (value >>> 24) & 0xff,
        (value >>> 16) & 0xff,
        (value >>> 8) & 0xff,
        value & 0xff
    ];
}

function equal(first: number[], second: number[]): boolean {
    if (first.length !== second.length) return false;
    let difference = 0;
    for (let index = 0; index < first.length; ++index) {
        difference |= first[index] ^ second[index];
    }
    return difference === 0;
}

const SHA256_K = [
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

function rotateRight(value: number, count: number): number {
    return (value >>> count) | (value << (32 - count));
}

function sha256(input: number[]): number[] {
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
            const small0 = rotateRight(x, 7) ^ rotateRight(x, 18) ^ (x >>> 3);
            const small1 = rotateRight(y, 17) ^ rotateRight(y, 19) ^ (y >>> 10);
            words[index] = (words[index - 16] + small0 +
                words[index - 7] + small1) | 0;
        }
        let a = state[0], b = state[1], c = state[2], d = state[3];
        let e = state[4], f = state[5], g = state[6], h = state[7];
        for (let index = 0; index < 64; ++index) {
            const big1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const choice = (e & f) ^ (~e & g);
            const temporary1 = (h + big1 + choice + SHA256_K[index] + words[index]) | 0;
            const big0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
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

function hmac(key: number[], ...parts: number[][]): number[] {
    let normalized = key.length > 64 ? sha256(key) : key.slice();
    while (normalized.length < 64) normalized.push(0);
    const inner = normalized.map(byte => byte ^ 0x36);
    const outer = normalized.map(byte => byte ^ 0x5c);
    let message: number[] = [];
    parts.forEach(part => { message = message.concat(part); });
    return sha256(outer.concat(sha256(inner.concat(message))));
}

function hkdf(salt: number[], input: number[], info: number[], size: number): number[] {
    const pseudoRandomKey = hmac(salt, input);
    let output: number[] = [];
    let previous: number[] = [];
    for (let counter = 1; output.length < size; ++counter) {
        previous = hmac(pseudoRandomKey, previous, info, [counter]);
        output = output.concat(previous);
    }
    return output.slice(0, size);
}

function replayAes(
    properties: DirectProperties,
    key: number[],
    nonce: number[],
    aad: number[],
    plaintext: number[]): number[] {
    const count = numberValue(properties, "primitive.aes.count");
    for (let index = 1; index <= count; ++index) {
        const prefix = `primitive.aes.${index}.`;
        if (equal(key, fromHex(required(properties, `${prefix}key`))) &&
            equal(nonce, fromHex(required(properties, `${prefix}nonce`))) &&
            equal(aad, fromHex(required(properties, `${prefix}aad`))) &&
            equal(plaintext, fromHex(required(properties, `${prefix}plaintext`)))) {
            return fromHex(required(properties, `${prefix}output`));
        }
    }
    directFail("unexpected AES-GCM operation");
}

function replayAesOpen(
    properties: DirectProperties,
    key: number[],
    nonce: number[],
    aad: number[],
    ciphertextAndTag: number[]): number[] {
    const count = numberValue(properties, "primitive.aes.count");
    for (let index = 1; index <= count; ++index) {
        const prefix = `primitive.aes.${index}.`;
        if (equal(key, fromHex(required(properties, `${prefix}key`))) &&
            equal(nonce, fromHex(required(properties, `${prefix}nonce`))) &&
            equal(aad, fromHex(required(properties, `${prefix}aad`))) &&
            equal(ciphertextAndTag, fromHex(required(properties, `${prefix}output`)))) {
            return fromHex(required(properties, `${prefix}plaintext`));
        }
    }
    directFail("unexpected AES-GCM open operation");
}

function seal(
    properties: DirectProperties,
    key: number[],
    prefix: number[],
    binding: number[],
    direction: number,
    sequence: number,
    plaintext: number[]): number[] {
    const header = [0xd3, 0x10].concat(u16(plaintext.length), u32(sequence));
    const nonce = prefix.concat(u32(sequence));
    const aad = ascii("BLINKER-DIRECT-RECORD/1").concat(
        [direction], binding, header);
    return header.concat(replayAes(properties, key, nonce, aad, plaintext));
}

function open(
    properties: DirectProperties,
    key: number[],
    prefix: number[],
    binding: number[],
    direction: number,
    expectedSequence: number,
    record: number[]): number[] {
    if (record.length < 24 || record[0] !== 0xd3 || record[1] !== 0x10) {
        directFail("invalid secure record header");
    }
    const plaintextSize = (record[2] << 8) | record[3];
    const sequence = ((record[4] << 24) | (record[5] << 16) |
        (record[6] << 8) | record[7]) >>> 0;
    if (record.length !== plaintextSize + 24 || sequence !== expectedSequence) {
        directFail("secure record size or sequence mismatch");
    }
    const header = record.slice(0, 8);
    const nonce = prefix.concat(u32(sequence));
    const aad = ascii("BLINKER-DIRECT-RECORD/1").concat(
        [direction], binding, header);
    const plaintext = replayAesOpen(
        properties, key, nonce, aad, record.slice(8));
    if (plaintext.length !== plaintextSize) directFail("plaintext size mismatch");
    return plaintext;
}

function fragment(record: number[], frameId: number): number[][] {
    const output: number[][] = [];
    const capacity = 16;
    for (let offset = 0, index = 0; offset < record.length; ++index) {
        const end = Math.min(record.length, offset + capacity);
        let flags = offset === 0 ? 1 : 0;
        if (end === record.length) flags |= 2;
        output.push([0xb2, 0x10 | flags, frameId, index].concat(
            record.slice(offset, end)));
        offset = end;
    }
    return output;
}

function run(properties: DirectProperties): void {
    const method = numberValue(properties, "method");
    const transcript = [1, method >>> 8, method & 0xff,
        numberValue(properties, "suite")].concat(
        fromHex(required(properties, "controller_id")),
        [numberValue(properties, "domain")],
        u32(numberValue(properties, "ownership_generation")),
        u32(numberValue(properties, "credential_version")),
        u32(numberValue(properties, "permissions")),
        fromHex(required(properties, "client_nonce")),
        fromHex(required(properties, "device_nonce")));
    const transcriptHash = sha256(
        ascii("BLINKER-DIRECT-AUTH/1").concat(transcript));
    const material = hkdf(
        transcriptHash,
        fromHex(required(properties, "secret")),
        ascii("BLINKER-DIRECT-SESSION/1"),
        96);
    const initiatorKey = material.slice(0, 32);
    const responderKey = material.slice(32, 64);
    const initiatorPrefix = material.slice(64, 72);
    const responderPrefix = material.slice(72, 80);
    const binding = material.slice(80, 96);
    const plaintexts: { [key: string]: number[] } = {
        command: fromHex(required(properties, "plaintext.command")),
        ack: fromHex(required(properties, "plaintext.ack")),
        patch: fromHex(required(properties, "plaintext.patch"))
    };
    const records: { [key: string]: number[] } = {
        command: seal(properties, initiatorKey, initiatorPrefix,
            binding, 1, 0, plaintexts.command),
        ack: seal(properties, responderKey, responderPrefix,
            binding, 2, 0, plaintexts.ack),
        patch: seal(properties, responderKey, responderPrefix,
            binding, 2, 1, plaintexts.patch)
    };
    if (!equal(open(properties, responderKey, responderPrefix,
        binding, 2, 0, records.ack), plaintexts.ack) ||
        !equal(open(properties, responderKey, responderPrefix,
        binding, 2, 1, records.patch), plaintexts.patch)) {
        directFail("Device-to-App open mismatch");
    }
    const outputs: DirectProperties = Object.create(null) as DirectProperties;
    outputs["expected.transcript_hash"] = hex(transcriptHash);
    outputs["expected.initiator_key"] = hex(initiatorKey);
    outputs["expected.responder_key"] = hex(responderKey);
    outputs["expected.initiator_nonce_prefix"] = hex(initiatorPrefix);
    outputs["expected.responder_nonce_prefix"] = hex(responderPrefix);
    outputs["expected.session_binding"] = hex(binding);
    const capacity = numberValue(properties, "att.payload") -
        numberValue(properties, "fragment.header");
    ["command", "ack", "patch"].forEach(name => {
        outputs[`expected.record.${name}`] = hex(records[name]);
        outputs[`expected.att20.raw.${name}`] =
            String(Math.ceil(plaintexts[name].length / capacity));
        outputs[`expected.att20.secure.${name}`] =
            String(Math.ceil(records[name].length / capacity));
    });
    const packets = fragment(
        records.command,
        numberValue(properties, "fragment.frame_id"));
    outputs["expected.fragment.command.count"] = String(packets.length);
    packets.forEach((packet, index) => {
        outputs[`expected.fragment.command.${index}`] = hex(packet);
    });
    Object.keys(outputs).forEach(key => {
        if (outputs[key] !== required(properties, key)) directFail(`mismatch: ${key}`);
    });
    console.log(`App DirectSecureRecord golden passed: ${Object.keys(outputs).length} exact values`);
}

const directFs = require("fs") as {
    readFileSync(path: string, encoding: string): string
};
if (process.argv.length !== 3) {
    directFail("usage: app_direct_secure_golden <fixture>");
}
run(parseProperties(directFs.readFileSync(process.argv[2], "utf8")));
