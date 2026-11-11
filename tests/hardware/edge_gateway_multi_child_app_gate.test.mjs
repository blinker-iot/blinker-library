import {test} from 'node:test';
import assert from 'node:assert/strict';
import {multiChildAppAction} from './edge_gateway_multi_child_app_gate.mjs';

function fixture(t) {
  const previous={ng:globalThis.ng,document:globalThis.document,gate:globalThis.__blinkerMultiChildGate};
  const children=['ble_AAAAAAAAAAAAAAAAAAAAAA','ble_BBBBBBBBBBBBBBBBBBBBBB'];
  const controllers=[], counts=new Map(children.map(id=>[id,0]));
  const service={client:{session:{}},start:async()=>{},waitUntilReady:async()=>{},
    reserveDirectPriority:async(id,signal)=>{
      assert.equal(id, children[0]); let finish;
      const owner={accepted:Promise.resolve(),ready:Promise.resolve(),assertReady:()=>signal.throwIfAborted(),
        closed:new Promise(resolve=>finish=resolve),release:()=>finish('cancelled')};
      signal.addEventListener('abort',owner.release,{once:true});return owner;
    },
    snapshot:id=>({cloudReachable:true,manifestAccepted:true,stateFresh:true,values:{buttonPresses:{value:counts.get(id)}}}),
    command:async id=>{counts.set(id,counts.get(id)+1);return {};},
    acquireConnectionDemand:async(_id,signal)=>{
      let finish;const closed=new Promise(resolve=>finish=resolve);
      const owner={accepted:Promise.resolve(),closed,release:()=>finish('cancelled')};
      signal.addEventListener('abort',owner.release,{once:true});controllers.push(signal);return owner;
    }};
  globalThis.document={querySelector:()=>({})};
  globalThis.ng={getComponent:()=>({deviceService:{deviceUi:{deviceV2:service}}})};
  delete globalThis.__blinkerMultiChildGate;
  t.after(()=>{globalThis.__blinkerMultiChildGate?.close();globalThis.ng=previous.ng;
    globalThis.document=previous.document;globalThis.__blinkerMultiChildGate=previous.gate;});
  return {service,counts,controllers,act:action=>multiChildAppAction({children,action})};
}

test('two real API owners share the same session; Commands are once-only and A release preserves B',async t=>{
  const f=fixture(t);await f.act('start');
  assert.equal((await f.act('commands')).acknowledged,2);
  await assert.rejects(f.act('commands'),/GATE_COMMAND_ONCE/);
  await f.act('release-a');assert.equal(f.controllers[0].aborted,true);assert.equal(f.controllers[1].aborted,false);
  assert.equal((await f.act('b-after-a')).acknowledged,1);
  const end=await f.act('close');assert.ok(end.closed && end.sameAccountMqtt);
  assert.deepEqual(end.children.map(c=>c.counter),[1,2]);
});
test('bounded deadline and failed startup both release owners',async t=>{
  const f=fixture(t);t.mock.timers.enable({apis:['setTimeout']});await f.act('start-release-a');
  t.mock.timers.tick(90000);assert.ok((await f.act('status')).closed);assert.ok(f.controllers.every(s=>s.aborted));
  f.service.waitUntilReady=async()=>{throw Error('not ready');};
  await assert.rejects(f.act('start'),/not ready/);assert.ok(f.controllers.every(s=>s.aborted));
});
test('session replacement and unknown Action result cannot trigger an implicit retry',async t=>{
  const f=fixture(t);await f.act('start');let sent=0;
  f.service.command=async()=>{sent++;throw Error('outcome unknown');};
  await assert.rejects(f.act('b-after-a'),/outcome unknown/);
  await assert.rejects(f.act('b-after-a'),/GATE_COMMAND_ONCE/);assert.equal(sent,1);
  f.service.client.session={};await assert.rejects(f.act('commands'),/GATE_COMMAND_ONCE/);
});

test('exact A Direct reservation retains B demand and sends only one B Action',async t=>{
  const f=fixture(t);await f.act('start');
  await assert.rejects(f.act('direct-a'),/GATE_DIRECT_PRECONDITION/);
  await f.act('release-a');
  const result=await f.act('direct-a');
  assert.equal(result.physicalYieldReady,true);assert.equal(result.acknowledged,1);
  assert.deepEqual([...f.counts.values()],[0,1]);assert.equal(f.controllers[1].aborted,false);
  assert.equal(globalThis.__blinkerMultiChildGate.directController.signal.aborted,true);
  await assert.rejects(f.act('direct-a'),/GATE_DIRECT_PRECONDITION/);
});

test('rejected Direct Ready releases its reservation without sending any Action',async t=>{
  const f=fixture(t);await f.act('start-release-a');let released=false;
  f.service.reserveDirectPriority=async()=>({accepted:Promise.resolve(),
    get ready(){return Promise.reject(Error('not yielded'));},closed:Promise.resolve(),release:()=>{released=true;}});
  await assert.rejects(f.act('direct-a'),/not yielded/);
  assert.equal(released,true);assert.deepEqual([...f.counts.values()],[0,0]);
});
