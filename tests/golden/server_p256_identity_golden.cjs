'use strict';

const crypto = require('crypto');
const fs = require('fs');

function fail(message) {
  throw new Error(message);
}

function loadProperties(path) {
  const properties = new Map();
  for (const rawLine of fs.readFileSync(path, 'utf8').split(/\r?\n/u)) {
    const line = rawLine.trim();
    if (line.length === 0 || line.startsWith('#')) continue;
    const separator = line.indexOf('=');
    if (separator <= 0) fail(`invalid property line: ${rawLine}`);
    properties.set(line.slice(0, separator), line.slice(separator + 1));
  }
  return properties;
}

function required(properties, key) {
  const value = properties.get(key);
  if (value === undefined) fail(`missing property: ${key}`);
  return value;
}

function base64url(bytes) {
  return bytes.toString('base64')
    .replace(/=/gu, '')
    .replace(/\+/gu, '-')
    .replace(/\//gu, '_');
}

function compareBigEndian(first, second) {
  return Buffer.compare(first, second);
}

const fixturePath = process.argv[2];
if (!fixturePath) fail('usage: node server_p256_identity_golden.cjs FIXTURE');
const properties = loadProperties(fixturePath);
if (required(properties, 'fixture.version') !== '1') {
  fail('unsupported fixture version');
}

const message = Buffer.from(required(properties, 'message'), 'utf8');
const privateKey = Buffer.from(required(properties, 'private_key'), 'hex');
const publicKey = Buffer.from(required(properties, 'public_key'), 'hex');
const digest = Buffer.from(required(properties, 'digest'), 'hex');
const signature = Buffer.from(required(properties, 'signature'), 'hex');
const proofAlgorithm = Number(required(properties, 'proof_algorithm'));
const record = Buffer.from(required(properties, 'identity_record'), 'hex');

if (privateKey.length !== 32 || publicKey.length !== 65 ||
    publicKey[0] !== 0x04 || digest.length !== 32 || signature.length !== 64) {
  fail('invalid fixed P-256 width');
}
const expectedDigest = crypto.createHash('sha256').update(message).digest();
if (!crypto.timingSafeEqual(digest, expectedDigest)) fail('digest mismatch');

const key = crypto.createPublicKey({
  key: {
    kty: 'EC',
    crv: 'P-256',
    x: base64url(publicKey.subarray(1, 33)),
    y: base64url(publicKey.subarray(33, 65)),
  },
  format: 'jwk',
});
if (!crypto.verify(
  'sha256',
  message,
  {key, dsaEncoding: 'ieee-p1363'},
  signature)) {
  fail('P-256 signature rejected');
}
const mutated = Buffer.from(signature);
mutated[0] ^= 0x01;
if (crypto.verify(
  'sha256',
  message,
  {key, dsaEncoding: 'ieee-p1363'},
  mutated)) {
  fail('mutated signature accepted');
}

const order = Buffer.from(
  'ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551',
  'hex');
const halfOrder = Buffer.from(
  '7fffffff800000007fffffffffffffffde737d56d38bcf4279dce5617e3192a8',
  'hex');
const r = signature.subarray(0, 32);
const s = signature.subarray(32, 64);
if (r.equals(Buffer.alloc(32)) || s.equals(Buffer.alloc(32)) ||
    compareBigEndian(r, order) >= 0 || compareBigEndian(s, halfOrder) > 0) {
  fail('signature is not canonical low-S raw r||s');
}

const physicalDigest = crypto.createHash('sha256')
  .update(Buffer.from('blinker.developer-id.v1', 'ascii'))
  .update(Buffer.from([proofAlgorithm]))
  .update(publicKey)
  .digest();
const physicalId = `dev_${physicalDigest.subarray(0, 16).toString('hex')}`;
if (physicalId !== required(properties, 'physical_device_id')) {
  fail('physical device ID mismatch');
}

if (record.length !== 72 ||
    !record.subarray(0, 8).equals(Buffer.from('4249443201030000', 'hex')) ||
    !record.subarray(8, 40).equals(privateKey)) {
  fail('identity record prefix/private key mismatch');
}
const recordChecksum = crypto.createHash('sha256')
  .update(Buffer.from('blinker.developer-key-record.v1', 'ascii'))
  .update(record.subarray(0, 40))
  .digest();
if (!crypto.timingSafeEqual(recordChecksum, record.subarray(40))) {
  fail('identity record checksum mismatch');
}

console.log('server P-256 identity golden passed');
