'use strict';

const assert = require('node:assert');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

const serviceRoot = path.resolve(__dirname, '..', '..', '..', 'aily-blinker-services');
const protocol = require(path.join(serviceRoot, 'packages', 'device-v2-protocol', 'src'));
const { createDeviceV2BleEnrollmentSigner } = require(path.join(
  serviceRoot, 'services', 'gateway', 'src', 'device-v2', 'ble-enrollment-signer'
));

function properties(file) {
  const result = new Map();
  for (const raw of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
    if (!raw || raw.startsWith('#')) continue;
    const split = raw.indexOf('=');
    assert.ok(split > 0 && !result.has(raw.slice(0, split)), `invalid property: ${raw}`);
    result.set(raw.slice(0, split), raw.slice(split + 1));
  }
  return result;
}

const hex = value => Buffer.from(value, 'hex');
const b64 = value => hex(value).toString('base64url');

function fixedKeys() {
  const ed = crypto.createPrivateKey({ format: 'jwk', key: {
    kty: 'OKP', crv: 'Ed25519',
    x: b64('d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a'),
    d: b64('9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60')
  } });
  const p256 = crypto.createPrivateKey({ format: 'jwk', key: {
    kty: 'EC', crv: 'P-256',
    x: b64('6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296'),
    y: b64('4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5'),
    d: b64('0000000000000000000000000000000000000000000000000000000000000001')
  } });
  return { ed, p256 };
}

function main() {
  assert.strictEqual(process.argv.length, 3, 'usage: server_ble_enrollment_golden <fixture>');
  const p = properties(process.argv[2]);
  const common = {
    grantId: hex(p.get('grant_id')),
    deviceInstanceId: hex(p.get('device_instance_id')),
    setupSessionId: hex(p.get('setup_session_id')),
    setupTranscriptHash: hex(p.get('setup_transcript_hash')),
    accessEpoch: Number(p.get('access_epoch')),
    controllerId: hex(p.get('controller_id')),
    controllerSecretDigest: hex(p.get('controller_secret_digest')),
    presenceKeyVersion: Number(p.get('presence_key_version')),
    presenceKeyDigest: hex(p.get('presence_key_digest')),
    nonce: hex(p.get('nonce')),
    issuedAt: Number(p.get('issued_at')),
    expiresAt: Number(p.get('expires_at')),
    serverKeyId: Number(p.get('server_key_id')),
    securityProfile: protocol.BleEnrollmentSecurityProfile.EducationNearby
  };
  const keys = fixedKeys();
  assert.strictEqual(
    crypto.createPublicKey(keys.ed).export({ type: 'spki', format: 'der' })
      .subarray(-32).toString('hex'),
    p.get('ed25519_public_key'),
  );
  const signer = createDeviceV2BleEnrollmentSigner({ keys: [
    { keyId: 1, algorithm: protocol.ServerSignatureAlgorithm.Ed25519,
      privateKey: keys.ed, securityProfiles: [1] },
    { keyId: 1, algorithm: protocol.ServerSignatureAlgorithm.EcdsaP256Sha256Raw,
      privateKey: keys.p256, securityProfiles: [1] }
  ] });

  for (const [prefix, algorithm, key] of [
    ['ed25519', protocol.ServerSignatureAlgorithm.Ed25519, keys.ed],
    ['p256', protocol.ServerSignatureAlgorithm.EcdsaP256Sha256Raw, keys.p256]
  ]) {
    const encoded = hex(p.get(`${prefix}_grant`));
    const grant = protocol.decodeBleEnrollmentGrant(encoded);
    const transcript = protocol.encodeBleEnrollmentGrantTranscript(grant);
    const digest = protocol.bleEnrollmentGrantDigest(grant);
    assert.strictEqual(Buffer.from(transcript).toString('hex'), p.get(`${prefix}_transcript`));
    assert.strictEqual(Buffer.from(digest).toString('hex'), p.get(`${prefix}_digest`));
    assert.strictEqual(Buffer.from(protocol.encodeBleEnrollmentGrant(grant)).toString('hex'),
      p.get(`${prefix}_grant`));
    const publicKey = crypto.createPublicKey(key);
    assert.strictEqual(algorithm === protocol.ServerSignatureAlgorithm.Ed25519
      ? crypto.verify(null, digest, publicKey, grant.signature)
      : crypto.verify('sha256', transcript, { key: publicKey, dsaEncoding: 'ieee-p1363' }, grant.signature),
    true);

    const fresh = signer.sign({ ...common, signatureAlgorithm: algorithm });
    assert.strictEqual(Buffer.from(fresh.transcript).toString('hex'), p.get(`${prefix}_transcript`));
    assert.strictEqual(Buffer.from(fresh.digest).toString('hex'), p.get(`${prefix}_digest`));
    if (algorithm === protocol.ServerSignatureAlgorithm.Ed25519) {
      assert.strictEqual(Buffer.from(fresh.encodedGrant).toString('hex'), p.get('ed25519_grant'));
    } else {
      assert.strictEqual(crypto.verify('sha256', fresh.transcript,
        { key: publicKey, dsaEncoding: 'ieee-p1363' }, fresh.grant.signature), true);
    }
  }
  console.log('Server BLE enrollment golden vectors passed');
}

main();
