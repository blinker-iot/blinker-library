import {test} from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import {runOwnerRemovalAppGate,ownerRemovalAppAction} from './owner_removal_app_gate.mjs';

function fixture({shared=false,account='owner',pending=true,fail=false,label='移除设备'}={}) {
  const calls={remove:0,dismiss:0}, session={}, devices=new Map([
    ['temporary',{id:'temporary',config:{customName:'B2a Temporary Removal test',isShared:shared,mode:'bbp2'}}],
    ['protected',{id:'protected'}]]);
  const management={async deleteDeviceV2(id){assert.equal(this,management);calls.remove++;
    if(fail)throw Error('Unknown outcome');devices.delete(id);return {status:pending?202:200,data:{logicalDeviceId:id,
      state:'deleted',cloudAuthorization:'revoked',brokerCleanupPending:pending,realtimeRefreshPending:false,
      localAccess:'not_confirmed',deletedAt:1}};}};
  const original=management.deleteDeviceV2;
  const page={loaded:true,device:devices.get('temporary'),management,isSharedDevice:shared,secretKey:'',
    async showUnbindConfirm(){this.confirmDialog={message:'不代表本地权限已清除',buttons:[{text:label,
      handler:()=>management.deleteDeviceV2('temporary')}],dismiss:async()=>calls.dismiss++};}};
  const root={dataService:{auth:{uuid:account},getDevice:id=>devices.get(id)},userService:{getAllInfo:async()=>true},
    deviceService:{deviceUi:{deviceV2:{client:{session},state:{value:'ready'}}}},
    viewService:{router:{navigateByUrl:async()=>true}}};
  const context=vm.createContext({ng:{getComponent:e=>e==='app-root'?root:page},
    document:{querySelector:selector=>selector},setTimeout});
  const action=vm.runInContext(`(${ownerRemovalAppAction.toString()})`,context);
  return {run:()=>action({logicalDeviceId:'temporary',owner:'owner',expectedName:'B2a Temporary Removal test',protectedIds:['protected']}),
    management,original,calls};
}
test('reject protected or non-temporary targets before connecting to CDP',async()=>{
  await assert.rejects(runOwnerRemovalAppGate({logicalDeviceId:'bad'}));
  const id='device_11111111-1111-4111-8111-111111111111';
  await assert.rejects(runOwnerRemovalAppGate({logicalDeviceId:id,owner:'x',expectedName:'B2a Temporary Removal test',protectedIds:[id]}));
  const ble='ble_abcdefghijklmnopqrstuv';
  for(const config of [
    {logicalDeviceId:ble,expectedName:'Old BLE device',protectedIds:[]},
    {logicalDeviceId:ble,expectedName:'B2a Temporary Removal test',protectedIds:[]},
    {logicalDeviceId:ble,expectedName:'B2b Temporary Removal test',protectedIds:[ble]},
    {logicalDeviceId:id,expectedName:'B2b Temporary Removal test',protectedIds:[]}
  ]) await assert.rejects(runOwnerRemovalAppGate({...config,owner:'x'}),/Exact temporary/);
});
test('original page handler keeps pending distinct from local erase and restores interception',async()=>{
  for(const pending of [true,false]){const f=fixture({pending}),r=await f.run();
    assert.equal(r.receipt.status,pending?202:200);assert.equal(r.receipt.localAccess,'not_confirmed');
    assert(r.targetAbsent&&r.protectedPresent&&r.sameAccountSession);
    assert.equal(f.management.deleteDeviceV2,f.original);assert.equal(f.calls.remove,1);assert.equal(f.calls.dismiss,1);
    await assert.rejects(f.run());assert.equal(f.calls.remove,1);}
});
test('wrong owner/share/unknown outcome do not escalate or retry deletion',async()=>{
  for(const opts of [{shared:true},{account:'another'},{fail:true}]){const f=fixture(opts);await assert.rejects(f.run());
    assert.equal(f.management.deleteDeviceV2,f.original);assert.equal(f.calls.remove,opts.fail?1:0);
    if(opts.fail){await assert.rejects(f.run());assert.equal(f.calls.remove,1);}}
});

test('missing current owner confirmation never substitutes a member or stale button',async()=>{
  for(const label of ['退出分享','确认解除']) { const f=fixture({label});
    await assert.rejects(f.run(),/Local-clear warning/);
    assert.equal(f.calls.remove,0); assert.equal(f.management.deleteDeviceV2,f.original);
  }
});
