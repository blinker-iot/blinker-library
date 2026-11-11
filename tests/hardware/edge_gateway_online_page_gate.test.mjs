import { test } from 'node:test';
import assert from 'node:assert/strict';
import { assertOnlinePage, assertCancelledPage, onlineRounds, observeOnlinePage } from './edge_gateway_online_page_gate.mjs';
const result = () => ({ error: '', idle: true, physicalReleased: true, counter: 0, connects: 1, closes: 1, readyMs: 3000,
  events: ['reserved', 'physical-ready', 'sdk-connect-start', 'page-ready', 'sdk-close-done', 'os-released'].map((phase, ms) => ({ phase, ms })) });
test('explicit bounded 10 -> 30 -> 100 stages, no unlimited or coerced loop count', () => {
  assert.equal(onlineRounds({}), 2);
  for (const rounds of [2, 10, 30, 100]) assert.equal(onlineRounds({ rounds }), rounds);
  for (const rounds of [0, 1, 11, 1000, '10', Infinity, NaN]) assert.throws(() => onlineRounds({ rounds }));
});
test('diagnostic page hold rejects unbounded/coerced duration before touching the App', async () => {
  for (const holdMillis of [-1, 8001, Infinity, NaN, '5000', 0.5]) {
    await assert.rejects(observeOnlinePage({ holdMillis }), /GATE_CONFIG/);
  }
  await assert.rejects(observeOnlinePage({ holdMillis: 5000, cancelBeforeReady: true }), /GATE_CONFIG/);
});
test('online normal page requires true Ready before native acquire and OS release after close', () => {
  assertOnlinePage(result(), 0);
  for (const key of ['idle', 'physicalReleased']) assert.throws(() => assertOnlinePage({ ...result(), [key]: false }, 0));
  for (const phase of result().events.map(e => e.phase)) assert.throws(() => assertOnlinePage({ ...result(), events: result().events.filter(e => e.phase !== phase) }, 0));
});
test('acceptance alone, early native connect, cached state and repeated connection are not passes', () => {
  const early = result(); [early.events[1], early.events[2]] = [early.events[2], early.events[1]];
  assert.throws(() => assertOnlinePage(early, 0));
  for (const patch of [{ connects: 2 }, { closes: 0 }, { counter: 1 }, { error: 'GATE_PAGE_DEADLINE' }]) {
    assert.throws(() => assertOnlinePage({ ...result(), ...patch }, 0));
  }
});

test('cancellation requires a real navigation boundary and no later native connection', () => {
  const cancelled = { error: '', idle: true, connects: 0, closes: 0,
    events: ['reserve-start', 'cancel-requested', 'navigation-away', 'ready-rejected', 'page-retired'].map(phase => ({ phase })) };
  assertCancelledPage(cancelled);
  for (const patch of [{ connects: 1 }, { closes: 1 }, { idle: false }, { error: 'GATE_CANCEL_RESURRECTED' },
    { events: cancelled.events.filter(e => e.phase !== 'navigation-away') },
    { events: [...cancelled.events, { phase: 'sdk-connect-start' }] }]) {
    assert.throws(() => assertCancelledPage({ ...cancelled, ...patch }));
  }
});
