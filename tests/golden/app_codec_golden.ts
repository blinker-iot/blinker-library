/*
 * Browser-safe BBP/2 freeze-candidate codec exercised by a tiny Node harness.
 * Keep this file compatible with the blinker-app TypeScript 4.3 compiler: the
 * encoding and SHA-256 code uses only ES2017/DOM primitives and no Node Buffer.
 */

type Properties = { [key: string]: string };

interface Field {
    key: string;
    kind: number;
    type: number;
    access: number;
    minimum?: number;
    maximum?: number;
    step?: number;
    maxLength?: number;
    unit?: string;
    enumValues?: string[];
}

function fail(message: string): never {
    throw new Error(`golden App TypeScript: ${message}`);
}

function parseProperties(text: string): Properties {
    const result: Properties = Object.create(null) as Properties;
    text.split(/\r?\n/).forEach((line, zeroBasedLine) => {
        if (line.length === 0 || line.charAt(0) === "#") return;
        const separator = line.indexOf("=");
        if (separator <= 0) fail(`invalid fixture line ${zeroBasedLine + 1}`);
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

function unsigned(properties: Properties, key: string): number {
    const text = required(properties, key);
    if (!/^(0|[1-9][0-9]*)$/.test(text)) fail(`invalid unsigned: ${key}`);
    const value = Number(text);
    if (!Number.isSafeInteger(value)) fail(`unsafe unsigned: ${key}`);
    return value;
}

function finite(properties: Properties, key: string): number {
    const value = Number(required(properties, key));
    if (!Number.isFinite(value)) fail(`invalid number: ${key}`);
    return value;
}

function utf8(value: string): number[] {
    const output: number[] = [];
    for (let index = 0; index < value.length; ++index) {
        let code = value.charCodeAt(index);
        if (code >= 0xd800 && code <= 0xdbff) {
            if (index + 1 >= value.length) fail("unpaired UTF-16 surrogate");
            const low = value.charCodeAt(++index);
            if (low < 0xdc00 || low > 0xdfff) fail("unpaired UTF-16 surrogate");
            code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
        } else if (code >= 0xdc00 && code <= 0xdfff) {
            fail("unpaired UTF-16 surrogate");
        }
        if (code <= 0x7f) {
            output.push(code);
        } else if (code <= 0x7ff) {
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

class CborWriter {
    readonly bytes: number[] = [];

    private head(major: number, value: number): void {
        if (!Number.isSafeInteger(value) || value < 0 || value > 0xffffffff) {
            fail("CBOR integer out of fixture range");
        }
        const prefix = major << 5;
        if (value < 24) {
            this.bytes.push(prefix | value);
        } else if (value <= 0xff) {
            this.bytes.push(prefix | 24, value);
        } else if (value <= 0xffff) {
            this.bytes.push(prefix | 25, value >>> 8, value & 0xff);
        } else {
            this.bytes.push(
                prefix | 26,
                (value >>> 24) & 0xff,
                (value >>> 16) & 0xff,
                (value >>> 8) & 0xff,
                value & 0xff);
        }
    }

    uint(value: number): this {
        this.head(0, value);
        return this;
    }

    int(value: number): this {
        if (!Number.isSafeInteger(value)) fail("CBOR signed integer is unsafe");
        if (value >= 0) return this.uint(value);
        this.head(1, -1 - value);
        return this;
    }

    bool(value: boolean): this {
        this.bytes.push(value ? 0xf5 : 0xf4);
        return this;
    }

    float32(value: number): this {
        if (!Number.isFinite(value)) fail("non-finite float32");
        const data = new DataView(new ArrayBuffer(4));
        data.setFloat32(0, value, false);
        this.bytes.push(0xfa);
        for (let index = 0; index < 4; ++index) this.bytes.push(data.getUint8(index));
        return this;
    }

    float64(value: number): this {
        if (!Number.isFinite(value)) fail("non-finite float64");
        const data = new DataView(new ArrayBuffer(8));
        data.setFloat64(0, value, false);
        this.bytes.push(0xfb);
        for (let index = 0; index < 8; ++index) this.bytes.push(data.getUint8(index));
        return this;
    }

    byteString(value: number[]): this {
        this.head(2, value.length);
        this.bytes.push(...value);
        return this;
    }

    text(value: string): this {
        const encoded = utf8(value);
        this.head(3, encoded.length);
        this.bytes.push(...encoded);
        return this;
    }

    array(size: number): this {
        this.head(4, size);
        return this;
    }

    map(size: number): this {
        this.head(5, size);
        return this;
    }

    encoded(value: number[]): this {
        this.bytes.push(...value);
        return this;
    }
}

function fields(properties: Properties): Field[] {
    const count = unsigned(properties, "manifest.count");
    const output: Field[] = [];
    for (let index = 0; index < count; ++index) {
        const prefix = `manifest.${index}.`;
        const field: Field = {
            key: required(properties, `${prefix}key`),
            kind: unsigned(properties, `${prefix}kind`),
            type: unsigned(properties, `${prefix}type`),
            access: unsigned(properties, `${prefix}access`)
        };
        if (Object.prototype.hasOwnProperty.call(properties, `${prefix}minimum`)) {
            field.minimum = finite(properties, `${prefix}minimum`);
        }
        if (Object.prototype.hasOwnProperty.call(properties, `${prefix}maximum`)) {
            field.maximum = finite(properties, `${prefix}maximum`);
        }
        if (Object.prototype.hasOwnProperty.call(properties, `${prefix}step`)) {
            field.step = finite(properties, `${prefix}step`);
        }
        if (Object.prototype.hasOwnProperty.call(properties, `${prefix}max_length`)) {
            field.maxLength = unsigned(properties, `${prefix}max_length`);
        }
        if (Object.prototype.hasOwnProperty.call(properties, `${prefix}unit`)) {
            field.unit = required(properties, `${prefix}unit`);
        }
        if (Object.prototype.hasOwnProperty.call(properties, `${prefix}enum`)) {
            field.enumValues = required(properties, `${prefix}enum`).split("|");
        }
        output.push(field);
    }
    return output;
}

function encodeField(writer: CborWriter, field: Field, index: number): void {
    let pairCount = 5;
    if (field.minimum !== undefined) ++pairCount;
    if (field.maximum !== undefined) ++pairCount;
    if (field.step !== undefined) ++pairCount;
    if (field.maxLength !== undefined) ++pairCount;
    if (field.unit !== undefined) ++pairCount;
    if (field.enumValues !== undefined) ++pairCount;
    writer.map(pairCount)
        .uint(0).text(field.key)
        .uint(1).uint(field.kind)
        .uint(2).uint(field.type)
        .uint(3).uint(field.access)
        .uint(4).uint(index + 1);
    // BBP/2 deterministic profile: Manifest numeric constraints are always
    // binary64. This is deterministic but intentionally not RFC 8949's
    // shortest-float preferred serialization.
    if (field.minimum !== undefined) writer.uint(5).float64(field.minimum);
    if (field.maximum !== undefined) writer.uint(6).float64(field.maximum);
    if (field.step !== undefined) writer.uint(7).float64(field.step);
    if (field.maxLength !== undefined) writer.uint(8).uint(field.maxLength);
    if (field.unit !== undefined) writer.uint(9).text(field.unit);
    if (field.enumValues !== undefined) {
        writer.uint(10).array(field.enumValues.length);
        field.enumValues.forEach(value => writer.text(value));
    }
}

function encodeManifest(revision: number, manifestFields: Field[]): number[] {
    const writer = new CborWriter();
    writer.map(2).uint(0).uint(revision).uint(1).array(manifestFields.length);
    manifestFields.forEach((field, index) => encodeField(writer, field, index));
    return writer.bytes;
}

function encodeManifestPage(
    revision: number,
    manifestFields: Field[],
    fingerprint: number[],
    cursor: number,
    count: number): number[] {
    const writer = new CborWriter();
    writer.map(6)
        .uint(0).uint(revision)
        .uint(1).byteString(fingerprint)
        .uint(2).uint(cursor)
        .uint(3).uint(cursor + count)
        .uint(4).uint(manifestFields.length)
        .uint(5).array(count);
    for (let offset = 0; offset < count; ++offset) {
        encodeField(writer, manifestFields[cursor + offset], cursor + offset);
    }
    return writer.bytes;
}

function encodeDeviceHello(
    properties: Properties,
    manifestRevision: number,
    manifestFingerprint: number[]): number[] {
    return new CborWriter()
        .map(7)
        .uint(0).uint(0)
        .uint(1).array(1).uint(2)
        .uint(2).uint(0x03)
        .uint(3).uint(unsigned(properties, "hello.max_frame"))
        .uint(4).uint(unsigned(properties, "hello.max_reassembly"))
        .uint(6).uint(manifestRevision)
        .uint(7).byteString(manifestFingerprint)
        .bytes;
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
    // Fixture inputs are far below 2^29 bytes, so the high 32 bits are zero.
    padded.push(0, 0, 0, 0,
        (bitLength >>> 24) & 0xff,
        (bitLength >>> 16) & 0xff,
        (bitLength >>> 8) & 0xff,
        bitLength & 0xff);

    const state = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    ];
    const words = new Array<number>(64);
    for (let block = 0; block < padded.length; block += 64) {
        for (let index = 0; index < 16; ++index) {
            const offset = block + index * 4;
            words[index] = (
                (padded[offset] << 24) |
                (padded[offset + 1] << 16) |
                (padded[offset + 2] << 8) |
                padded[offset + 3]);
        }
        for (let index = 16; index < 64; ++index) {
            const x = words[index - 15];
            const y = words[index - 2];
            const small0 = rotateRight(x, 7) ^ rotateRight(x, 18) ^ (x >>> 3);
            const small1 = rotateRight(y, 17) ^ rotateRight(y, 19) ^ (y >>> 10);
            words[index] = (words[index - 16] + small0 + words[index - 7] + small1) | 0;
        }
        let a = state[0];
        let b = state[1];
        let c = state[2];
        let d = state[3];
        let e = state[4];
        let f = state[5];
        let g = state[6];
        let h = state[7];
        for (let index = 0; index < 64; ++index) {
            const big1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const choice = (e & f) ^ (~e & g);
            const temporary1 = (h + big1 + choice + SHA256_K[index] + words[index]) | 0;
            const big0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            const majority = (a & b) ^ (a & c) ^ (b & c);
            const temporary2 = (big0 + majority) | 0;
            h = g;
            g = f;
            f = e;
            e = (d + temporary1) | 0;
            d = c;
            c = b;
            b = a;
            a = (temporary1 + temporary2) | 0;
        }
        state[0] = (state[0] + a) | 0;
        state[1] = (state[1] + b) | 0;
        state[2] = (state[2] + c) | 0;
        state[3] = (state[3] + d) | 0;
        state[4] = (state[4] + e) | 0;
        state[5] = (state[5] + f) | 0;
        state[6] = (state[6] + g) | 0;
        state[7] = (state[7] + h) | 0;
    }
    const output: number[] = [];
    state.forEach(value => output.push(
        (value >>> 24) & 0xff,
        (value >>> 16) & 0xff,
        (value >>> 8) & 0xff,
        value & 0xff));
    return output;
}

function encodeStateValue(
    writer: CborWriter,
    type: string,
    value: string): void {
    switch (type) {
        case "bool":
            if (value !== "true" && value !== "false") fail("invalid bool");
            writer.bool(value === "true");
            break;
        case "int":
            writer.int(Number(value));
            break;
        case "uint":
            writer.uint(Number(value));
            break;
        case "f32":
            writer.float32(Number(value));
            break;
        case "f64":
            writer.float64(Number(value));
            break;
        case "text":
            writer.text(value);
            break;
        case "bytes":
            writer.byteString(fromHex(value));
            break;
        case "int_array": {
            const items = value.split("|").map(item => Number(item));
            writer.array(items.length);
            items.forEach(item => writer.int(item));
            break;
        }
        default:
            fail(`unknown state type: ${type}`);
    }
}

function encodeStateValues(
    properties: Properties,
    manifestFields: Field[],
    idMode: boolean): number[] {
    const count = unsigned(properties, "state.count");
    if (count !== manifestFields.length) fail("state/Manifest count mismatch");
    const writer = new CborWriter();
    writer.map(count);
    for (let index = 0; index < count; ++index) {
        if (idMode) writer.uint(index + 1);
        else writer.text(manifestFields[index].key);
        encodeStateValue(
            writer,
            required(properties, `state.${index}.type`),
            required(properties, `state.${index}.value`));
    }
    return writer.bytes;
}

function encodeStatePage(properties: Properties, values: number[]): number[] {
    const count = unsigned(properties, "state.count");
    return new CborWriter()
        .map(5)
        .uint(0).uint(unsigned(properties, "state.revision"))
        .uint(1).uint(0)
        .uint(2).uint(count)
        .uint(3).uint(count)
        .uint(4).encoded(values)
        .bytes;
}

function encodeStateRequest(cursor: number, revision?: number): number[] {
    const writer = new CborWriter();
    if (cursor === 0) return writer.map(1).uint(0).uint(0).bytes;
    if (revision === undefined) fail("continuation StateRequest needs revision");
    return writer.map(2).uint(0).uint(cursor).uint(1).uint(revision).bytes;
}

function encodeFrame(body: number[], sequence: number, idMode: boolean): number[] {
    if (body.length > 0xffff) fail("frame body too large");
    const flags = 0x02 | (idMode ? 0x04 : 0);
    return [
        0x42, 0x4b, 0x02, 0x16, flags, 0x0a,
        (sequence >>> 8) & 0xff, sequence & 0xff,
        (body.length >>> 8) & 0xff, body.length & 0xff,
        ...body
    ];
}

function fragment(frame: number[], frameId: number, packetSize: number): number[][] {
    const capacity = packetSize - 4;
    if (capacity <= 0) fail("BLE packet size too small");
    const count = Math.ceil(frame.length / capacity);
    if (count < 1 || count > 256) fail("BLE fragment count out of range");
    const packets: number[][] = [];
    for (let index = 0; index < count; ++index) {
        const flags = (index === 0 ? 1 : 0) | (index + 1 === count ? 2 : 0);
        packets.push([
            0xb2, 0x10 | flags, frameId, index,
            ...frame.slice(index * capacity, (index + 1) * capacity)
        ]);
    }
    return packets;
}

interface DecodedHead {
    major: number;
    value: number;
    next: number;
}

function decodeHead(bytes: number[], offset: number): DecodedHead | string {
    if (offset >= bytes.length) return "truncated_input";
    const first = bytes[offset];
    const major = first >>> 5;
    const additional = first & 0x1f;
    if (additional < 24) return { major, value: additional, next: offset + 1 };
    let size = 0;
    if (additional === 24) size = 1;
    else if (additional === 25) size = 2;
    else if (additional === 26) size = 4;
    else return "invalid_encoding";
    if (offset + 1 + size > bytes.length) return "truncated_input";
    let value = 0;
    for (let index = 0; index < size; ++index) {
        value = value * 256 + bytes[offset + 1 + index];
    }
    if ((additional === 24 && value < 24) ||
        (additional === 25 && value <= 0xff) ||
        (additional === 26 && value <= 0xffff)) {
        return "non_canonical_encoding";
    }
    return { major, value, next: offset + 1 + size };
}

function validateStateRequest(bytes: number[]): string {
    const mapHead = decodeHead(bytes, 0);
    if (typeof mapHead === "string") return mapHead;
    if (mapHead.major !== 5) return "invalid_encoding";
    let offset = mapHead.next;
    let seen = 0;
    let cursor = 0;
    let hasCursor = false;
    let hasRevision = false;
    for (let pair = 0; pair < mapHead.value; ++pair) {
        const keyHead = decodeHead(bytes, offset);
        if (typeof keyHead === "string") return keyHead;
        if (keyHead.major !== 0) return "invalid_encoding";
        offset = keyHead.next;
        const valueHead = decodeHead(bytes, offset);
        if (typeof valueHead === "string") return valueHead;
        if (valueHead.major !== 0) return "invalid_encoding";
        offset = valueHead.next;
        if (keyHead.value > 1) continue;
        const bit = 1 << keyHead.value;
        if ((seen & bit) !== 0) return "duplicate_field";
        seen |= bit;
        if (keyHead.value === 0) {
            cursor = valueHead.value;
            hasCursor = true;
        } else {
            hasRevision = true;
        }
    }
    if (offset !== bytes.length) return "trailing_data";
    if (!hasCursor || (cursor === 0) === hasRevision) return "invalid_encoding";
    return "ok";
}

function validateFrame(bytes: number[]): string {
    if (bytes.length < 10) return "truncated_input";
    if (bytes[0] !== 0x42 || bytes[1] !== 0x4b) return "invalid_magic";
    if (bytes[2] !== 2) return "unsupported_version";
    if ((bytes[4] & ~0x0f) !== 0 || (bytes[4] & 0x08) !== 0) {
        return "unsupported_feature";
    }
    if (bytes[5] < 10) return "invalid_header";
    if (bytes.length < bytes[5]) return "truncated_input";
    const bodyLength = bytes[8] * 256 + bytes[9];
    const expected = bytes[5] + bodyLength;
    if (bytes.length < expected) return "truncated_input";
    if (bytes.length > expected) return "trailing_data";
    return "ok";
}

function validateBle(bytes: number[]): string {
    if (bytes.length <= 4) return "truncated_input";
    if (bytes[0] !== 0xb2) return "invalid_magic";
    if ((bytes[1] >>> 4) !== 1) return "unsupported_version";
    const flags = bytes[1] & 0x0f;
    if ((flags & ~3) !== 0) return "unsupported_feature";
    if (bytes[2] === 0 || (((flags & 1) !== 0) !== (bytes[3] === 0))) {
        return "invalid_encoding";
    }
    return "ok";
}

function verifyInvalid(
    properties: Properties,
    prefix: string,
    verifier: (bytes: number[]) => string): void {
    const actual = verifier(fromHex(required(properties, `${prefix}.hex`)));
    const expected = required(properties, `${prefix}.error`);
    if (actual !== expected) fail(`${prefix} expected ${expected}, got ${actual}`);
}

function run(properties: Properties): void {
    if (required(properties, "fixture.version") !== "1") {
        fail("unsupported fixture version");
    }
    const manifestFields = fields(properties);
    const revision = unsigned(properties, "manifest.revision");
    const manifest = encodeManifest(revision, manifestFields);
    const fingerprint = sha256(manifest);
    const outputs: Properties = Object.create(null) as Properties;
    outputs["expected.manifest"] = toHex(manifest);
    outputs["expected.manifest_sha256"] = toHex(fingerprint);

    const pageCapacity = unsigned(properties, "manifest.page_body_capacity");
    let cursor = 0;
    let pageIndex = 0;
    while (cursor < manifestFields.length) {
        let selected = 0;
        for (let candidate = 0; candidate <= manifestFields.length - cursor; ++candidate) {
            if (encodeManifestPage(
                revision, manifestFields, fingerprint, cursor, candidate).length > pageCapacity) {
                break;
            }
            selected = candidate;
        }
        if (selected === 0) fail("Manifest page cannot advance");
        const page = encodeManifestPage(
            revision, manifestFields, fingerprint, cursor, selected);
        outputs[`expected.manifest_page.${pageIndex}`] = toHex(page);
        cursor += selected;
        ++pageIndex;
    }
    outputs["expected.manifest_page.count"] = String(pageIndex);

    outputs["expected.hello.device"] = toHex(encodeDeviceHello(
        properties,
        revision,
        fingerprint));
    outputs["expected.state_request.initial"] = toHex(encodeStateRequest(0));
    outputs["expected.state_request.continuation"] = toHex(encodeStateRequest(
        unsigned(properties, "state.request.cursor"),
        unsigned(properties, "state.request.observed_revision")));
    const keyedValues = encodeStateValues(properties, manifestFields, false);
    const idValues = encodeStateValues(properties, manifestFields, true);
    outputs["expected.state_values.keyed"] = toHex(keyedValues);
    outputs["expected.state_values.id"] = toHex(idValues);
    const keyedPage = encodeStatePage(properties, keyedValues);
    const idPage = encodeStatePage(properties, idValues);
    outputs["expected.state_page.keyed"] = toHex(keyedPage);
    outputs["expected.state_page.id"] = toHex(idPage);
    const sequence = unsigned(properties, "frame.sequence");
    const keyedFrame = encodeFrame(keyedPage, sequence, false);
    const idFrame = encodeFrame(idPage, sequence + 1, true);
    outputs["expected.frame.state_page_keyed"] = toHex(keyedFrame);
    outputs["expected.frame.state_page_id"] = toHex(idFrame);

    [20, 244].forEach(packetSize => {
        const packets = fragment(
            keyedFrame, unsigned(properties, "ble.frame_id"), packetSize);
        outputs[`expected.ble.att${packetSize}.count`] = String(packets.length);
        packets.forEach((packet, index) => {
            outputs[`expected.ble.att${packetSize}.${index}`] = toHex(packet);
        });
    });

    Object.keys(outputs).forEach(key => {
        if (outputs[key] !== required(properties, key)) fail(`${key} byte mismatch`);
    });

    verifyInvalid(properties, "invalid.frame.unknown_flags", validateFrame);
    verifyInvalid(properties, "invalid.frame.truncated", validateFrame);
    verifyInvalid(properties, "invalid.state_request.duplicate", validateStateRequest);
    verifyInvalid(properties, "invalid.state_request.noncanonical", validateStateRequest);
    verifyInvalid(properties, "invalid.ble.unknown_flags", validateBle);
    verifyInvalid(properties, "invalid.ble.start_index_mismatch", validateBle);
    console.log(
        `golden App TypeScript: ${Object.keys(outputs).length} exact outputs ` +
        "and 6 invalid vectors passed");
}

if (process.argv.length !== 3) {
    fail("usage: node app_codec_golden.js <fixture>");
}
const fs = require("fs");
run(parseProperties(fs.readFileSync(process.argv[2], "utf8")));
