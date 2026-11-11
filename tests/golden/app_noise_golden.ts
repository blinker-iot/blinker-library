/* Browser-safe App reference for portable Noise state/wire. Cryptographic
 * primitives are replayed from the Node/OpenSSL trace so TypeScript 4.3 can
 * validate transcript/HKDF/nonce logic without pretending to provide a
 * production X25519 or AES-GCM implementation. */

type Properties = { [key: string]: string };

function fail(message: string): never {
    throw new Error(`portable Noise App golden: ${message}`);
}

function parseProperties(text: string): Properties {
    const result: Properties = Object.create(null) as Properties;
    text.split(/\r?\n/).forEach((line, index) => {
        if (line.length === 0 || line.charAt(0) === "#") return;
        const separator = line.indexOf("=");
        if (separator <= 0) fail(`invalid fixture line ${index + 1}`);
        const key = line.slice(0, separator);
        if (Object.prototype.hasOwnProperty.call(result, key)) {
            fail(`duplicate fixture key: ${key}`);
        }
        result[key] = line.slice(separator + 1);
    });
    return result;
}

function required(properties: Properties, key: string): string {
    if (!Object.prototype.hasOwnProperty.call(properties, key)) {
        fail(`missing fixture key: ${key}`);
    }
    return properties[key];
}

function count(properties: Properties, key: string): number {
    const text = required(properties, key);
    if (!/^[1-9][0-9]*$/.test(text)) fail(`invalid count: ${key}`);
    return Number(text);
}

function fromHex(value: string): number[] {
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) fail("invalid lowercase hex");
    const output: number[] = [];
    for (let index = 0; index < value.length; index += 2) {
        output.push(parseInt(value.slice(index, index + 2), 16));
    }
    return output;
}

function toHex(value: number[]): string {
    return value.map(byte => byte.toString(16).padStart(2, "0")).join("");
}

function ascii(value: string): number[] {
    const output: number[] = [];
    for (let index = 0; index < value.length; ++index) {
        const code = value.charCodeAt(index);
        if (code > 0x7f) fail("non-ASCII protocol constant");
        output.push(code);
    }
    return output;
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

function rotateRight(value: number, countValue: number): number {
    return (value >>> countValue) | (value << (32 - countValue));
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

function hmacSha256(key: number[], message: number[]): number[] {
    let normalized = key.length > 64 ? sha256(key) : key.slice();
    while (normalized.length < 64) normalized.push(0);
    const inner = normalized.map(byte => byte ^ 0x36);
    const outer = normalized.map(byte => byte ^ 0x5c);
    return sha256(outer.concat(sha256(inner.concat(message))));
}

function hkdf(chainingKey: number[], input: number[], outputCount: number): number[][] {
    const temporary = hmacSha256(chainingKey, input);
    const outputs: number[][] = [];
    let previous: number[] = [];
    for (let index = 1; index <= outputCount; ++index) {
        previous = hmacSha256(temporary, previous.concat([index]));
        outputs.push(previous);
    }
    return outputs;
}

function nonceBytes(value: number): number[] {
    if (!Number.isSafeInteger(value) || value < 0) fail("invalid nonce");
    const high = Math.floor(value / 0x100000000);
    const low = value - high * 0x100000000;
    return [0, 0, 0, 0,
        (high >>> 24) & 0xff, (high >>> 16) & 0xff,
        (high >>> 8) & 0xff, high & 0xff,
        (low >>> 24) & 0xff, (low >>> 16) & 0xff,
        (low >>> 8) & 0xff, low & 0xff];
}

class ReplayPrimitives {
    constructor(private readonly properties: Properties) {}

    publicKey(privateKey: number[]): number[] {
        for (let index = 1; index <= count(this.properties, "primitive.public.count"); ++index) {
            const prefix = `primitive.public.${index}`;
            if (equal(privateKey, fromHex(required(this.properties, `${prefix}.private`)))) {
                return fromHex(required(this.properties, `${prefix}.output`));
            }
        }
        fail("unexpected X25519 public operation");
    }

    dh(privateKey: number[], remotePublic: number[]): number[] {
        const firstPrivate = fromHex(required(this.properties, "primitive.dh.1.private"));
        const firstRemote = fromHex(required(this.properties, "primitive.dh.1.remote"));
        const reversePrivate = fromHex(required(this.properties, "primitive.public.2.private"));
        const reverseRemote = fromHex(required(this.properties, "primitive.public.1.output"));
        if ((!equal(privateKey, firstPrivate) || !equal(remotePublic, firstRemote)) &&
            (!equal(privateKey, reversePrivate) || !equal(remotePublic, reverseRemote))) {
            fail("unexpected X25519 DH operation");
        }
        return fromHex(required(this.properties, "primitive.dh.1.output"));
    }

    aes(key: number[], nonce: number[], aad: number[], plaintext: number[]): number[] {
        for (let index = 1; index <= count(this.properties, "primitive.aes.count"); ++index) {
            const prefix = `primitive.aes.${index}`;
            if (equal(key, fromHex(required(this.properties, `${prefix}.key`))) &&
                equal(nonce, fromHex(required(this.properties, `${prefix}.nonce`))) &&
                equal(aad, fromHex(required(this.properties, `${prefix}.aad`))) &&
                equal(plaintext, fromHex(required(this.properties, `${prefix}.plaintext`)))) {
                return fromHex(required(this.properties, `${prefix}.output`));
            }
        }
        fail("unexpected AES-GCM operation");
    }
}

class SymmetricState {
    h: number[];
    ck: number[];
    key: number[] = [];
    nonce = 0;

    constructor(
        private readonly primitives: ReplayPrimitives,
        prologue: number[]) {
        const protocol = ascii("Noise_NN_25519_AESGCM_SHA256");
        this.h = protocol.length <= 32
            ? protocol.concat(new Array<number>(32 - protocol.length).fill(0))
            : sha256(protocol);
        this.ck = this.h.slice();
        this.mixHash(prologue);
    }

    mixHash(input: number[]): void { this.h = sha256(this.h.concat(input)); }

    mixKey(input: number[]): void {
        const outputs = hkdf(this.ck, input, 2);
        this.ck = outputs[0];
        this.key = outputs[1];
        this.nonce = 0;
    }

    mixEphemeral(publicKey: number[]): void {
        this.mixHash(publicKey);
    }

    encryptAndHash(plaintext: number[]): number[] {
        const output = this.key.length === 0
            ? plaintext.slice()
            : this.primitives.aes(
                this.key,
                nonceBytes(this.nonce++),
                this.h,
                plaintext);
        this.mixHash(output);
        return output;
    }

    split(): CipherState[] {
        const outputs = hkdf(this.ck, [], 2);
        return [
            new CipherState(this.primitives, outputs[0]),
            new CipherState(this.primitives, outputs[1])
        ];
    }
}

class CipherState {
    private nonce = 0;
    constructor(
        private readonly primitives: ReplayPrimitives,
        private readonly key: number[]) {}

    encrypt(plaintext: number[]): number[] {
        const output = this.primitives.aes(
            this.key,
            nonceBytes(this.nonce),
            [],
            plaintext);
        ++this.nonce;
        return output;
    }
}

function encodeRecord(type: number, body: number[]): number[] {
    if (type < 1 || type > 3 || body.length === 0 || body.length > 1024) {
        fail("invalid local secure record");
    }
    return [0xb3, 0x10 | type, body.length >>> 8, body.length & 0xff].concat(body);
}

function run(properties: Properties): void {
    const primitives = new ReplayPrimitives(properties);
    const prologue = ascii("BLINKER-LOCAL-SETUP-V1").concat([0xb3, 0x01, 0x01]);
    const initiatorPrivate = fromHex(required(
        properties, "primitive.public.1.private"));
    const responderPrivate = fromHex(required(
        properties, "primitive.public.2.private"));
    const state = new SymmetricState(primitives, prologue);
    const initiatorPublic = primitives.publicKey(initiatorPrivate);
    state.mixEphemeral(initiatorPublic);
    const message1 = initiatorPublic.concat(
        state.encryptAndHash(fromHex("a200010101")));
    const responderPublic = primitives.publicKey(responderPrivate);
    state.mixEphemeral(responderPublic);
    state.mixKey(primitives.dh(initiatorPrivate, responderPublic));
    const message2 = responderPublic.concat(
        state.encryptAndHash(fromHex("a10001")));
    const handshakeHash = state.h.slice();
    const ciphers = state.split();
    const transport1 = ciphers[0].encrypt(fromHex("01040007a10001"));
    const transport2 = ciphers[1].encrypt(fromHex("01050007"));

    const outputs: Properties = Object.create(null) as Properties;
    outputs["expected.prologue"] = toHex(prologue);
    outputs["expected.message1"] = toHex(message1);
    outputs["expected.message2"] = toHex(message2);
    outputs["expected.handshake_hash"] = toHex(handshakeHash);
    outputs["expected.transport1"] = toHex(transport1);
    outputs["expected.transport2"] = toHex(transport2);
    outputs["expected.record1"] = toHex(encodeRecord(1, message1));
    outputs["expected.record2"] = toHex(encodeRecord(2, message2));
    outputs["expected.record3"] = toHex(encodeRecord(3, transport1));
    const keys = Object.keys(outputs);
    keys.forEach(key => {
        if (outputs[key] !== required(properties, key)) fail(`mismatch: ${key}`);
    });
    console.log(`App Noise golden passed: ${keys.length} exact values`);
}

const fs = require("fs") as { readFileSync(path: string, encoding: string): string };
if (process.argv.length !== 3) fail("usage: app_noise_golden <fixture>");
run(parseProperties(fs.readFileSync(process.argv[2], "utf8")));
