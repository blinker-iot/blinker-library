"use strict";

// Independent server-side verifier. It intentionally uses Node Buffer and the
// built-in crypto implementation instead of importing the App TypeScript or
// invoking the C++ executable.

const fs = require("fs");
const crypto = require("crypto");

function fail(message) {
    throw new Error(`golden server Node: ${message}`);
}

function parseProperties(text) {
    const properties = Object.create(null);
    text.split(/\r?\n/).forEach((line, zeroBasedLine) => {
        if (line.length === 0 || line[0] === "#") return;
        const separator = line.indexOf("=");
        if (separator <= 0) fail(`invalid fixture line ${zeroBasedLine + 1}`);
        const key = line.slice(0, separator);
        if (Object.prototype.hasOwnProperty.call(properties, key)) {
            fail(`duplicate fixture key: ${key}`);
        }
        properties[key] = line.slice(separator + 1);
    });
    return properties;
}

function required(properties, key) {
    if (!Object.prototype.hasOwnProperty.call(properties, key)) {
        fail(`missing fixture key: ${key}`);
    }
    return properties[key];
}

function unsigned(properties, key) {
    const text = required(properties, key);
    if (!/^(0|[1-9][0-9]*)$/.test(text)) fail(`invalid unsigned: ${key}`);
    const value = Number(text);
    if (!Number.isSafeInteger(value)) fail(`unsafe unsigned: ${key}`);
    return value;
}

function finite(properties, key) {
    const value = Number(required(properties, key));
    if (!Number.isFinite(value)) fail(`invalid number: ${key}`);
    return value;
}

function decodeHex(value) {
    if (!/^(?:[0-9a-f]{2})*$/.test(value)) fail("invalid lowercase hex");
    return Buffer.from(value, "hex");
}

class CborEncoder {
    constructor() {
        this.parts = [];
    }

    head(major, value) {
        if (!Number.isSafeInteger(value) || value < 0 || value > 0xffffffff) {
            fail("CBOR integer out of fixture range");
        }
        if (value < 24) {
            this.parts.push(Buffer.from([(major << 5) | value]));
        } else if (value <= 0xff) {
            this.parts.push(Buffer.from([(major << 5) | 24, value]));
        } else if (value <= 0xffff) {
            const head = Buffer.allocUnsafe(3);
            head[0] = (major << 5) | 25;
            head.writeUInt16BE(value, 1);
            this.parts.push(head);
        } else {
            const head = Buffer.allocUnsafe(5);
            head[0] = (major << 5) | 26;
            head.writeUInt32BE(value, 1);
            this.parts.push(head);
        }
        return this;
    }

    uint(value) {
        return this.head(0, value);
    }

    int(value) {
        if (!Number.isSafeInteger(value)) fail("unsafe signed integer");
        return value >= 0 ? this.uint(value) : this.head(1, -1 - value);
    }

    bool(value) {
        this.parts.push(Buffer.from([value ? 0xf5 : 0xf4]));
        return this;
    }

    float32(value) {
        if (!Number.isFinite(value)) fail("non-finite float32");
        const encoded = Buffer.allocUnsafe(5);
        encoded[0] = 0xfa;
        encoded.writeFloatBE(value, 1);
        this.parts.push(encoded);
        return this;
    }

    float64(value) {
        if (!Number.isFinite(value)) fail("non-finite float64");
        const encoded = Buffer.allocUnsafe(9);
        encoded[0] = 0xfb;
        encoded.writeDoubleBE(value, 1);
        this.parts.push(encoded);
        return this;
    }

    bytes(value) {
        this.head(2, value.length);
        this.parts.push(value);
        return this;
    }

    text(value) {
        const encoded = Buffer.from(value, "utf8");
        // Node replaces malformed UTF-16; fixture generation forbids it by
        // checking round-trip equality before accepting a string.
        if (encoded.toString("utf8") !== value) fail("invalid UTF-16 input");
        this.head(3, encoded.length);
        this.parts.push(encoded);
        return this;
    }

    array(size) {
        return this.head(4, size);
    }

    map(size) {
        return this.head(5, size);
    }

    encoded(value) {
        this.parts.push(value);
        return this;
    }

    finish() {
        return Buffer.concat(this.parts);
    }
}

function readFields(properties) {
    const count = unsigned(properties, "manifest.count");
    const fields = [];
    for (let index = 0; index < count; ++index) {
        const prefix = `manifest.${index}.`;
        const field = {
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
        fields.push(field);
    }
    return fields;
}

function encodeField(writer, field, index) {
    let count = 5;
    if (field.minimum !== undefined) ++count;
    if (field.maximum !== undefined) ++count;
    if (field.step !== undefined) ++count;
    if (field.maxLength !== undefined) ++count;
    if (field.unit !== undefined) ++count;
    if (field.enumValues !== undefined) ++count;
    writer.map(count)
        .uint(0).text(field.key)
        .uint(1).uint(field.kind)
        .uint(2).uint(field.type)
        .uint(3).uint(field.access)
        .uint(4).uint(index + 1);
    // BBP/2 deterministic Manifest profile fixes constraints at binary64.
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

function encodeManifest(revision, fields) {
    const writer = new CborEncoder();
    writer.map(2).uint(0).uint(revision).uint(1).array(fields.length);
    fields.forEach((field, index) => encodeField(writer, field, index));
    return writer.finish();
}

function encodeManifestPage(revision, fields, fingerprint, cursor, count) {
    const writer = new CborEncoder();
    writer.map(6)
        .uint(0).uint(revision)
        .uint(1).bytes(fingerprint)
        .uint(2).uint(cursor)
        .uint(3).uint(cursor + count)
        .uint(4).uint(fields.length)
        .uint(5).array(count);
    for (let offset = 0; offset < count; ++offset) {
        encodeField(writer, fields[cursor + offset], cursor + offset);
    }
    return writer.finish();
}

function encodeDeviceHello(
    properties,
    manifestRevision,
    manifestFingerprint) {
    return new CborEncoder()
        .map(7)
        .uint(0).uint(0)
        .uint(1).array(1).uint(2)
        .uint(2).uint(0x03)
        .uint(3).uint(unsigned(properties, "hello.max_frame"))
        .uint(4).uint(unsigned(properties, "hello.max_reassembly"))
        .uint(6).uint(manifestRevision)
        .uint(7).bytes(manifestFingerprint)
        .finish();
}
function encodeStateValue(writer, type, value) {
    switch (type) {
        case "bool":
            if (value !== "true" && value !== "false") fail("invalid bool");
            writer.bool(value === "true");
            return;
        case "int":
            writer.int(Number(value));
            return;
        case "uint":
            writer.uint(Number(value));
            return;
        case "f32":
            writer.float32(Number(value));
            return;
        case "f64":
            writer.float64(Number(value));
            return;
        case "text":
            writer.text(value);
            return;
        case "bytes":
            writer.bytes(decodeHex(value));
            return;
        case "int_array": {
            const values = value.split("|").map(Number);
            writer.array(values.length);
            values.forEach(item => writer.int(item));
            return;
        }
        default:
            fail(`unknown state type: ${type}`);
    }
}

function encodeStateValues(properties, fields, idMode) {
    const count = unsigned(properties, "state.count");
    if (count !== fields.length) fail("state/Manifest count mismatch");
    const writer = new CborEncoder().map(count);
    for (let index = 0; index < count; ++index) {
        if (idMode) writer.uint(index + 1);
        else writer.text(fields[index].key);
        encodeStateValue(
            writer,
            required(properties, `state.${index}.type`),
            required(properties, `state.${index}.value`));
    }
    return writer.finish();
}

function encodeStatePage(properties, values) {
    const count = unsigned(properties, "state.count");
    return new CborEncoder()
        .map(5)
        .uint(0).uint(unsigned(properties, "state.revision"))
        .uint(1).uint(0)
        .uint(2).uint(count)
        .uint(3).uint(count)
        .uint(4).encoded(values)
        .finish();
}

function encodeStateRequest(cursor, revision) {
    const writer = new CborEncoder();
    if (cursor === 0) return writer.map(1).uint(0).uint(0).finish();
    if (revision === undefined) fail("continuation StateRequest needs revision");
    return writer.map(2).uint(0).uint(cursor).uint(1).uint(revision).finish();
}

function encodeFrame(body, sequence, idMode) {
    if (body.length > 0xffff) fail("frame body too large");
    const header = Buffer.allocUnsafe(10);
    header[0] = 0x42;
    header[1] = 0x4b;
    header[2] = 0x02;
    header[3] = 0x16;
    header[4] = 0x02 | (idMode ? 0x04 : 0);
    header[5] = 0x0a;
    header.writeUInt16BE(sequence, 6);
    header.writeUInt16BE(body.length, 8);
    return Buffer.concat([header, body]);
}

function fragment(frame, frameId, packetSize) {
    const capacity = packetSize - 4;
    if (capacity <= 0) fail("BLE packet size too small");
    const count = Math.ceil(frame.length / capacity);
    if (count < 1 || count > 256) fail("BLE fragment count out of range");
    const packets = [];
    for (let index = 0; index < count; ++index) {
        const flags = (index === 0 ? 1 : 0) | (index + 1 === count ? 2 : 0);
        packets.push(Buffer.concat([
            Buffer.from([0xb2, 0x10 | flags, frameId, index]),
            frame.subarray(index * capacity, (index + 1) * capacity)
        ]));
    }
    return packets;
}

function decodeHead(bytes, offset) {
    if (offset >= bytes.length) return "truncated_input";
    const first = bytes[offset];
    const major = first >>> 5;
    const additional = first & 0x1f;
    if (additional < 24) return { major, value: additional, next: offset + 1 };
    const size = additional === 24 ? 1 : additional === 25 ? 2 : additional === 26 ? 4 : 0;
    if (size === 0) return "invalid_encoding";
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

function validateStateRequest(bytes) {
    const root = decodeHead(bytes, 0);
    if (typeof root === "string") return root;
    if (root.major !== 5) return "invalid_encoding";
    let offset = root.next;
    let seen = 0;
    let cursor = 0;
    let hasCursor = false;
    let hasRevision = false;
    for (let pair = 0; pair < root.value; ++pair) {
        const key = decodeHead(bytes, offset);
        if (typeof key === "string") return key;
        if (key.major !== 0) return "invalid_encoding";
        offset = key.next;
        const value = decodeHead(bytes, offset);
        if (typeof value === "string") return value;
        if (value.major !== 0) return "invalid_encoding";
        offset = value.next;
        if (key.value > 1) continue;
        const bit = 1 << key.value;
        if ((seen & bit) !== 0) return "duplicate_field";
        seen |= bit;
        if (key.value === 0) {
            cursor = value.value;
            hasCursor = true;
        } else {
            hasRevision = true;
        }
    }
    if (offset !== bytes.length) return "trailing_data";
    if (!hasCursor || (cursor === 0) === hasRevision) return "invalid_encoding";
    return "ok";
}

function validateFrame(bytes) {
    if (bytes.length < 10) return "truncated_input";
    if (bytes[0] !== 0x42 || bytes[1] !== 0x4b) return "invalid_magic";
    if (bytes[2] !== 2) return "unsupported_version";
    if ((bytes[4] & ~0x0f) !== 0 || (bytes[4] & 0x08) !== 0) {
        return "unsupported_feature";
    }
    if (bytes[5] < 10) return "invalid_header";
    if (bytes.length < bytes[5]) return "truncated_input";
    const expected = bytes[5] + bytes.readUInt16BE(8);
    if (bytes.length < expected) return "truncated_input";
    if (bytes.length > expected) return "trailing_data";
    return "ok";
}

function validateBle(bytes) {
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

function verifyInvalid(properties, prefix, verifier) {
    const actual = verifier(decodeHex(required(properties, `${prefix}.hex`)));
    const expected = required(properties, `${prefix}.error`);
    if (actual !== expected) fail(`${prefix} expected ${expected}, got ${actual}`);
}

function run(properties) {
    if (required(properties, "fixture.version") !== "1") {
        fail("unsupported fixture version");
    }
    const fields = readFields(properties);
    const revision = unsigned(properties, "manifest.revision");
    const manifest = encodeManifest(revision, fields);
    const fingerprint = crypto.createHash("sha256").update(manifest).digest();
    const outputs = Object.create(null);
    outputs["expected.manifest"] = manifest.toString("hex");
    outputs["expected.manifest_sha256"] = fingerprint.toString("hex");

    const pageCapacity = unsigned(properties, "manifest.page_body_capacity");
    let cursor = 0;
    let pageIndex = 0;
    while (cursor < fields.length) {
        let selected = 0;
        for (let candidate = 0; candidate <= fields.length - cursor; ++candidate) {
            if (encodeManifestPage(
                revision, fields, fingerprint, cursor, candidate).length > pageCapacity) {
                break;
            }
            selected = candidate;
        }
        if (selected === 0) fail("Manifest page cannot advance");
        outputs[`expected.manifest_page.${pageIndex}`] = encodeManifestPage(
            revision, fields, fingerprint, cursor, selected).toString("hex");
        cursor += selected;
        ++pageIndex;
    }
    outputs["expected.manifest_page.count"] = String(pageIndex);

    outputs["expected.hello.device"] = encodeDeviceHello(
        properties,
        revision,
        fingerprint).toString("hex");
    outputs["expected.state_request.initial"] = encodeStateRequest(0).toString("hex");
    outputs["expected.state_request.continuation"] = encodeStateRequest(
        unsigned(properties, "state.request.cursor"),
        unsigned(properties, "state.request.observed_revision")).toString("hex");
    const keyedValues = encodeStateValues(properties, fields, false);
    const idValues = encodeStateValues(properties, fields, true);
    outputs["expected.state_values.keyed"] = keyedValues.toString("hex");
    outputs["expected.state_values.id"] = idValues.toString("hex");
    const keyedPage = encodeStatePage(properties, keyedValues);
    const idPage = encodeStatePage(properties, idValues);
    outputs["expected.state_page.keyed"] = keyedPage.toString("hex");
    outputs["expected.state_page.id"] = idPage.toString("hex");
    const sequence = unsigned(properties, "frame.sequence");
    const keyedFrame = encodeFrame(keyedPage, sequence, false);
    const idFrame = encodeFrame(idPage, sequence + 1, true);
    outputs["expected.frame.state_page_keyed"] = keyedFrame.toString("hex");
    outputs["expected.frame.state_page_id"] = idFrame.toString("hex");

    [20, 244].forEach(packetSize => {
        const packets = fragment(
            keyedFrame, unsigned(properties, "ble.frame_id"), packetSize);
        outputs[`expected.ble.att${packetSize}.count`] = String(packets.length);
        packets.forEach((packet, index) => {
            outputs[`expected.ble.att${packetSize}.${index}`] = packet.toString("hex");
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
        `golden server Node: ${Object.keys(outputs).length} exact outputs ` +
        "and 6 invalid vectors passed");
}

if (process.argv.length !== 3) fail("usage: node server_codec_golden.cjs <fixture>");
run(parseProperties(fs.readFileSync(process.argv[2], "utf8")));
