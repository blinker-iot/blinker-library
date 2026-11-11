import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Destructive hardware gate: only a newly-created, explicitly named fixture.
// All authority mutations use the original settings page / management service.
export async function runOwnerRemovalAppGate(config) {
  const wifi = /^device_[0-9a-f-]{36}$/.test(config.logicalDeviceId)
    && config.expectedName?.startsWith('B2a Temporary Removal ');
  const ble = /^ble_[A-Za-z0-9_-]{22}$/.test(config.logicalDeviceId)
    && config.expectedName?.startsWith('B2b Temporary Removal ');
  if ((!wifi && !ble)
    || !config.owner
    || !Array.isArray(config.protectedIds) || config.protectedIds.includes(config.logicalDeviceId)) {
    throw new Error('Exact temporary removal fixture required');
  }
  return runGatewayAppGate(ownerRemovalAppAction, { ...config, pagePath: '/' }, 45000);
}

export async function ownerRemovalAppAction({ logicalDeviceId: id, owner, expectedName, protectedIds }) {
  const root = ng.getComponent(document.querySelector('app-root'));
  if (root.dataService.auth?.uuid !== owner || protectedIds.includes(id)) throw new Error('Wrong removal subject');
  if (globalThis.__blinkerOwnerRemoval) throw new Error('Inspect previous removal before retrying');
  if (!await root.userService.getAllInfo()) throw new Error('Inventory unavailable');
  const target = root.dataService.getDevice(id);
  if (target?.config?.customName !== expectedName || target.config.isShared || target.config.mode !== 'bbp2')
    throw new Error('Temporary owner fixture required');
  const cloud = root.deviceService.deviceUi.deviceV2, accountSession = cloud.client.session;
  if (cloud.state.value !== 'ready') throw new Error('Original account not ready');
  const gate = globalThis.__blinkerOwnerRemoval = { phase: 'opening', id, startedAt: Date.now() };
  let page, original;
  try {
    await root.viewService.router.navigateByUrl(`/device/${encodeURIComponent(id)}/settings`);
    for (let n = 0; n < 40; n++) {
      const element = document.querySelector('app-device-settings');
      page = element && ng.getComponent(element);
      if (page?.loaded && page.device?.id === id) break;
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    if (!page?.loaded || page.device?.id !== id || page.isSharedDevice) throw new Error('Exact settings page required');
    original = page.management.deleteDeviceV2;
    page.management.deleteDeviceV2 = async function (requestedId) {
      if (requestedId !== id) throw new Error('Unexpected removal target');
      const result = await original.call(this, requestedId);
      gate.receipt = { status: result.status, ...result.data };
      return result;
    };
    await page.showUnbindConfirm();
    const dialog = page.confirmDialog;
    const confirm = dialog?.buttons.find(button => button.text === '移除设备');
    if (!confirm || !dialog.message.includes('本地权限已清除')) throw new Error('Local-clear warning required');
    gate.phase = 'requested'; gate.requestedAt = Date.now();
    await confirm.handler(); // Same handler as the visible confirmation button.
    if (!gate.receipt || gate.receipt.localAccess !== 'not_confirmed') throw new Error('Removal unconfirmed');
    await root.viewService.router.navigateByUrl('/home/device');
    if (!await root.userService.getAllInfo()) throw new Error('Refresh unconfirmed');
    gate.phase = 'confirmed'; gate.completedAt = Date.now();
    return { ...gate, targetAbsent: !root.dataService.getDevice(id),
      protectedPresent: protectedIds.every(other => !!root.dataService.getDevice(other)),
      sameAccountSession: accountSession === cloud.client.session, cloudState: cloud.state.value,
      displayedKeyCleared: page.secretKey === '' };
  } finally {
    if (original) page.management.deleteDeviceV2 = original;
    await page?.confirmDialog?.dismiss();
    // No repeated DELETE or Action on timeout. Keep gate checkpoint for inspection.
  }
}
