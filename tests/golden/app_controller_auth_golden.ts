/* Browser-safe App reference for Controller HMAC authorization method 2. */

type ControllerProperties = { [key: string]: string };

function controllerFail(message: string): never {
    throw new Error(`controller authorization App golden: ${message}`);
}

function parseControllerProperties(text: string): ControllerProperties {
    const output: ControllerProperties = Object.create(null) as ControllerProperties;
    text.split(/\r?\n/).forEach((line, index) => {
        if (line.length === 0 || line.charAt(0) === "#") return;
        const split = line.indexOf("=");
        const key = line.slice(0, split);
        if (split <= 0 || Object.prototype.hasOwnProperty.call(output, key)) {
            controllerFail(`invalid fixture line ${index + 1}`);
        }
        output[key] = line.slice(split + 1);
    });
    return output;
}

function controllerRequired(
    properties: ControllerProperties,
    key: string): string {
    if (!Object.prototype.hasOwnProperty.call(properties, key)) {
        controllerFail(`missing property: ${key}`);
    }
    return properties[key];
}

function controllerNumber(
    properties: ControllerProperties,
    key: string): number {
    const value = controllerRequired(properties, key);
    if (!/^(?:0|[1-9][0-9]*)$/.test(value)) {
        controllerFail(`invalid number: ${key}`);
    }
    const output = Number(value);
    if (!Number.isSafeInteger(output) || output > 0xffffffff) {
        controllerFail(`number out of range: ${key}`);
    }
    return output;
}

function controllerFromHex(value: string): number[] {
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) {
        controllerFail("invalid lowercase hex");
    }
    const output: number[] = [];
    for (let index = 0; index < value.length; index += 2) {
        output.push(parseInt(value.slice(index, index + 2), 16));
    }
    return output;
}

function controllerHex(value: number[]): string {
    return value.map(byte => byte.toString(16).padStart(2, "0")).join("");
}

function controllerAscii(value: string): number[] {
    const output: number[] = [];
    for (let index = 0; index < value.length; ++index) {
        const code = value.charCodeAt(index);
        if (code > 0x7f) controllerFail("non-ASCII protocol constant");
        output.push(code);
    }
    return output;
}

function controllerU32(value: number): number[] {
    return [
        (value >>> 24) & 0xff,
        (value >>> 16) & 0xff,
        (value >>> 8) & 0xff,
        value & 0xff
    ];
}

const CONTROLLER_SHA256_K = [
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

function controllerRotateRight(value: number, count: number): number {
    return (value >>> count) | (value << (32 - count));
}

function controllerSha256(input: number[]): number[] {
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
            const small0 = controllerRotateRight(x, 7) ^
                controllerRotateRight(x, 18) ^ (x >>> 3);
            const small1 = controllerRotateRight(y, 17) ^
                controllerRotateRight(y, 19) ^ (y >>> 10);
            words[index] = (words[index - 16] + small0 +
                words[index - 7] + small1) | 0;
        }
        let a = state[0], b = state[1], c = state[2], d = state[3];
        let e = state[4], f = state[5], g = state[6], h = state[7];
        for (let index = 0; index < 64; ++index) {
            const big1 = controllerRotateRight(e, 6) ^
                controllerRotateRight(e, 11) ^ controllerRotateRight(e, 25);
            const choice = (e & f) ^ (~e & g);
            const temporary1 = (h + big1 + choice +
                CONTROLLER_SHA256_K[index] + words[index]) | 0;
            const big0 = controllerRotateRight(a, 2) ^
                controllerRotateRight(a, 13) ^ controllerRotateRight(a, 22);
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

function controllerHmac(key: number[], message: number[]): number[] {
    let normalized = key.length > 64 ? controllerSha256(key) : key.slice();
    while (normalized.length < 64) normalized.push(0);
    const inner = normalized.map(byte => byte ^ 0x36);
    const outer = normalized.map(byte => byte ^ 0x5c);
    return controllerSha256(
        outer.concat(controllerSha256(inner.concat(message))));
}

function runControllerGolden(properties: ControllerProperties): void {
    const method = controllerNumber(properties, "method");
    const suite = controllerNumber(properties, "suite");
    const domain = controllerNumber(properties, "domain");
    const generation = controllerNumber(properties, "ownership_generation");
    const version = controllerNumber(properties, "credential_version");
    const permissions = controllerNumber(properties, "permissions");
    const secret = controllerFromHex(controllerRequired(properties, "secret"));
    const controllerId = controllerFromHex(
        controllerRequired(properties, "controller_id"));
    const clientNonce = controllerFromHex(
        controllerRequired(properties, "client_nonce"));
    const deviceNonce = controllerFromHex(
        controllerRequired(properties, "device_nonce"));
    const prefix = [1, method >>> 8, method & 0xff, suite];
    const metadata = [domain].concat(
        controllerU32(generation), controllerU32(version),
        controllerU32(permissions));
    const outputs: ControllerProperties = Object.create(null) as ControllerProperties;
    outputs["expected.init"] = controllerHex([1].concat(
        controllerId, [domain], controllerU32(generation),
        controllerU32(version), clientNonce));
    let deviceProof: number[] = [];
    ["device", "app"].forEach(role => {
        const roleDomain = controllerAscii(
            role === "device"
                ? "BLINKER-CONTROLLER-DEVICE-PROOF/1"
                : "BLINKER-CONTROLLER-APP-PROOF/1");
        const transcript = roleDomain.concat(
            prefix, controllerId, metadata, clientNonce, deviceNonce);
        const proof = controllerHmac(secret, transcript);
        outputs[`expected.${role}_transcript`] = controllerHex(transcript);
        outputs[`expected.${role}_proof`] = controllerHex(proof);
        if (role === "device") deviceProof = proof;
        else outputs["expected.proof_payload"] = controllerHex([2].concat(proof));
    });
    outputs["expected.challenge"] = controllerHex([0x81].concat(
        deviceNonce, controllerU32(permissions), deviceProof));
    const keys = Object.keys(outputs);
    keys.forEach(key => {
        if (outputs[key] !== controllerRequired(properties, key)) {
            controllerFail(`mismatch: ${key}`);
        }
    });
    console.log(`App controller authorization golden passed: ${keys.length} exact values`);
}

const controllerFs = require("fs") as {
    readFileSync(path: string, encoding: string): string
};
if (process.argv.length !== 3) {
    controllerFail("usage: app_controller_auth_golden <fixture>");
}
runControllerGolden(parseControllerProperties(
    controllerFs.readFileSync(process.argv[2], "utf8")));
