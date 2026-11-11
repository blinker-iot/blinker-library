import assert from 'node:assert/strict';
import { randomBytes } from 'node:crypto';

const PUBLIC_FIELDS = new Set([
  'renewalOperationId', 'topologyOperationId', 'tenantId',
  'edgeHubLogicalDeviceId', 'childLogicalDeviceId',
  'expectedCredentialVersion', 'credentialVersion', 'topologyVersion',
  'state', 'pendingStorageRevision', 'activeStorageRevision',
  'credentialExpiresAt', 'deliveredAt', 'rotatedAt',
  'gatewayProvenAt', 'finalizedAt', 'updatedAt', 'replayed'
]);
const TERMINAL_FAILURES = new Set(['cancelled', 'expired']);

function parseArguments(values) {
  const result = new Map();
  for (let index = 0; index < values.length; index += 2) {
    const name = values[index];
    const value = values[index + 1];
    if (!name?.startsWith('--') || value === undefined) {
      throw new Error(`Invalid argument near ${name || '<end>'}`);
    }
    result.set(name.slice(2), value);
  }
  return result;
}

function positiveInteger(value, name, fallback) {
  if (value === undefined) return fallback;
  const parsed = Number(value);
  if (!Number.isSafeInteger(parsed) || parsed <= 0) {
    throw new Error(`${name} must be a positive integer`);
  }
  return parsed;
}

function exactOperationId(value, name) {
  if (typeof value !== 'string' || !/^[A-Za-z0-9_-]{22}$/.test(value)) {
    throw new Error(`${name} must be an unpadded 16-byte base64url ID`);
  }
  const decoded = Buffer.from(value, 'base64url');
  if (decoded.length !== 16 || decoded.every(byte => byte === 0)
      || decoded.toString('base64url') !== value) {
    throw new Error(`${name} is not canonical`);
  }
  return decoded;
}

function delay(milliseconds) {
  return new Promise(resolve => setTimeout(resolve, milliseconds));
}

async function requestJson(url, options, expectedStatus) {
  const response = await fetch(url, {
    ...options,
    signal: AbortSignal.timeout(15_000)
  });
  const text = await response.text();
  let body;
  try {
    body = JSON.parse(text);
  } catch {
    throw new Error(`${options.method || 'GET'} ${url} returned non-JSON`);
  }
  if (response.status !== expectedStatus) {
    throw new Error(
      `${options.method || 'GET'} ${url} returned ${response.status}: `
        + JSON.stringify(body)
    );
  }
  return { response, body };
}

function assertPublicOperation(data, expectedIds) {
  assert(data && typeof data === 'object' && !Array.isArray(data));
  for (const field of Object.keys(data)) {
    assert(PUBLIC_FIELDS.has(field), `unexpected public operation field: ${field}`);
  }
  assert.equal(data.renewalOperationId, expectedIds.renewalOperationId);
  assert.equal(data.topologyOperationId, expectedIds.topologyOperationId);
  assert.equal(typeof data.replayed, 'boolean');
  return data;
}

function assertCompleted(current, previous, expectedInitialVersion) {
  assert.equal(current.state, 'completed');
  assert.equal(current.credentialVersion, current.expectedCredentialVersion + 1);
  assert(Number.isSafeInteger(current.pendingStorageRevision)
    && current.pendingStorageRevision > 0);
  assert(Number.isSafeInteger(current.activeStorageRevision)
    && current.activeStorageRevision > 0);
  for (const field of [
    'deliveredAt', 'rotatedAt', 'gatewayProvenAt', 'finalizedAt', 'updatedAt'
  ]) {
    assert(Number.isSafeInteger(current[field]) && current[field] > 0,
      `${field} was not durably recorded`);
  }
  if (previous) {
    assert.equal(current.expectedCredentialVersion, previous.credentialVersion);
    assert.equal(current.topologyVersion, previous.topologyVersion + 1);
    assert.equal(current.activeStorageRevision, previous.activeStorageRevision + 1);
  } else if (expectedInitialVersion !== undefined) {
    assert.equal(current.expectedCredentialVersion, expectedInitialVersion);
  }
}

async function brokerRecord(context, path, expectedStatus = 200) {
  const { body } = await requestJson(`${context.brokerUrl}${path}`, {
    headers: {
      Authorization: `Bearer ${context.brokerToken}`,
      'X-Aily-Shard-Id': String(context.shardId)
    }
  }, expectedStatus);
  return body;
}

async function assertBrokerLedger(context, completed, renewalHex) {
  const prefix = '/internal/v2/edge-hub-management/renewals';
  const delivery = await brokerRecord(
    context, `${prefix}/deliveries/${renewalHex}`
  );
  assert.equal(delivery.message, 1000);
  assert.equal(delivery.detail.status, 'acknowledged');
  assert.equal(delivery.detail.pendingStorageRevision,
    completed.pendingStorageRevision);
  assert(delivery.detail.attemptCount >= 1);

  const expected = new Map([
    [1, 'nonce_ready'], [2, 'rotated'], [3, 'finalized']
  ]);
  for (const [phase, status] of expected) {
    const record = await brokerRecord(
      context,
      `${prefix}/phases/${renewalHex}/${completed.topologyVersion}/${phase}`
    );
    assert.equal(record.message, 1000);
    assert.equal(record.detail.phase, phase);
    assert.equal(record.detail.status, status);
    assert.equal(record.detail.credentialVersion, completed.credentialVersion);
    assert(record.detail.attemptCount >= 1);
  }
  const cancel = await brokerRecord(
    context,
    `${prefix}/phases/${renewalHex}/${completed.topologyVersion}/4`,
    404
  );
  assert.equal(cancel.detail?.code, 'EDGE_GATEWAY_RENEWAL_NOT_FOUND');
}

async function runOne(context, previous, index) {
  const renewal = randomBytes(16);
  const ids = {
    renewalOperationId: renewal.toString('base64url'),
    topologyOperationId: context.topologyId
  };
  const headers = {
    Authorization: `Bearer ${context.gatewayToken}`,
    'Content-Type': 'application/json'
  };
  const collection = `${context.gatewayUrl}/internal/v2/edge-gateway/renewals`;
  const created = await requestJson(collection, {
    method: 'POST', headers, body: JSON.stringify(ids)
  }, 202);
  assert.equal(created.response.headers.get('cache-control'), 'no-store');
  assert.equal(assertPublicOperation(created.body.data, ids).replayed, false);

  const item = `${collection}/${ids.renewalOperationId}`;
  const deadline = Date.now() + context.operationTimeoutMs;
  let completed;
  while (Date.now() < deadline) {
    await delay(context.pollIntervalMs);
    const current = await requestJson(item, { headers }, 200);
    assert.equal(current.response.headers.get('cache-control'), 'no-store');
    const operation = assertPublicOperation(current.body.data, ids);
    if (operation.state === 'completed') {
      completed = operation;
      break;
    }
    if (TERMINAL_FAILURES.has(operation.state)) {
      throw new Error(`renewal ${index} entered ${operation.state}`);
    }
  }
  if (!completed) throw new Error(`renewal ${index} timed out`);
  assertCompleted(completed, previous, context.expectedInitialVersion);

  const replay = await requestJson(collection, {
    method: 'POST', headers, body: JSON.stringify(ids)
  }, 200);
  assert.equal(assertPublicOperation(replay.body.data, ids).replayed, true);
  assert.equal(replay.body.data.state, 'completed');
  assert.equal(replay.body.data.credentialVersion, completed.credentialVersion);

  await assertBrokerLedger(context, completed, renewal.toString('hex'));
  console.log(
    `[${index}/${context.count}] credential `
      + `${completed.expectedCredentialVersion}->${completed.credentialVersion}, `
      + `topology ${completed.topologyVersion}->${completed.topologyVersion + 1}, `
      + `storage ${completed.activeStorageRevision}`
  );
  renewal.fill(0);
  return completed;
}

async function main() {
  const args = parseArguments(process.argv.slice(2));
  const topology = exactOperationId(args.get('topology'), 'topology');
  const gatewayToken = process.env.GATEWAY_ADMIN_TOKEN;
  const brokerToken = process.env.BROKER_INTERNAL_TOKEN;
  if (!gatewayToken || !brokerToken) {
    throw new Error('GATEWAY_ADMIN_TOKEN and BROKER_INTERNAL_TOKEN are required');
  }
  const context = {
    topologyId: topology.toString('base64url'),
    gatewayUrl: (args.get('gateway') || 'http://127.0.0.1:17001').replace(/\/$/, ''),
    brokerUrl: (args.get('broker') || 'http://127.0.0.1:3001').replace(/\/$/, ''),
    gatewayToken,
    brokerToken,
    shardId: positiveInteger(args.get('shard'), 'shard', 1),
    count: positiveInteger(args.get('count'), 'count', 10),
    pollIntervalMs: positiveInteger(args.get('poll-ms'), 'poll-ms', 500),
    operationTimeoutMs: positiveInteger(
      args.get('operation-timeout-ms'), 'operation-timeout-ms', 180_000
    ),
    expectedInitialVersion: args.has('expected-version')
      ? positiveInteger(args.get('expected-version'), 'expected-version')
      : undefined
  };
  topology.fill(0);

  let previous;
  const startedAt = Date.now();
  for (let index = 1; index <= context.count; index += 1) {
    previous = await runOne(context, previous, index);
  }
  console.log(
    `PASS ${context.count} exact renewals in ${Date.now() - startedAt} ms; `
      + `credential=${previous.credentialVersion}, `
      + `topology=${previous.topologyVersion + 1}, `
      + `storage=${previous.activeStorageRevision}`
  );
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
