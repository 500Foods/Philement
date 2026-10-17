/**
 * ManagerLoader lifecycle. Phase 1 of CATCHUP.
 *
 * closeManager calls one hook: destroy, otherwise cleanup, otherwise teardown.
 * showManager calls onDeactivate and onActivate after the crossfade.
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';

vi.mock('../../../src/core/log.js', () => ({
  log: vi.fn(),
  logManager: vi.fn(),
  logGroup: vi.fn(),
  Subsystems: { STARTUP: 'Startup', MANAGER: 'Manager' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR', SUCCESS: 'SUCCESS' },
}));

import { logManager, Status } from '../../../src/core/log.js';
import { ManagerLoader } from '../../../src/app/manager-loader.js';

function slot() {
  const el = document.createElement('div');
  document.body.appendChild(el);
  return el;
}

function harness(instance, id = 29, name = 'Query Manager') {
  const slotEl = slot();
  const loadedManagers = new Map([
    [id, { id, name, instance, slotEl, workspaceEl: slotEl }],
  ]);
  const app = {
    loadedManagers,
    openManagers: new Set([id]),
    currentManager: { id, name, instance },
    mainManagerInstance: { updateOpenMenuItems: vi.fn() },
    utilityManagerRegistry: {},
  };
  return { app, loader: new ManagerLoader(app), slotEl };
}

describe('ManagerLoader lifecycle', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    document.body.innerHTML = '';
  });

  it('calls cleanup when that is the only close hook', async () => {
    const instance = { cleanup: vi.fn(), onDeactivate: vi.fn() };
    const { app, loader } = harness(instance);

    await loader.closeManager(29);

    expect(instance.cleanup).toHaveBeenCalledOnce();
    expect(instance.onDeactivate).not.toHaveBeenCalled();
    expect(app.loadedManagers.has(29)).toBe(false);
    expect(app.currentManager).toBeNull();
  });

  it('calls destroy and skips cleanup and teardown when destroy exists', async () => {
    const instance = {
      destroy: vi.fn(),
      cleanup: vi.fn(),
      teardown: vi.fn(),
    };
    const { loader } = harness(instance, 32, 'Terminal');

    await loader.closeManager(32);

    expect(instance.destroy).toHaveBeenCalledOnce();
    expect(instance.cleanup).not.toHaveBeenCalled();
    expect(instance.teardown).not.toHaveBeenCalled();
  });

  it('calls teardown when destroy and cleanup are absent', async () => {
    const instance = { teardown: vi.fn(async () => {}) };
    const { app, loader } = harness(instance, 7, 'Dashboard Manager');

    await loader.closeManager(7);

    expect(instance.teardown).toHaveBeenCalledOnce();
    expect(app.loadedManagers.has(7)).toBe(false);
  });

  it('logs a close failure and still drops the manager', async () => {
    const instance = {
      cleanup: vi.fn(() => {
        throw new Error('table still open');
      }),
    };
    const { app, loader } = harness(instance);

    await loader.closeManager(29);

    expect(app.loadedManagers.has(29)).toBe(false);
    expect(logManager).toHaveBeenCalledWith(
      Status.WARN,
      'cleanup failed for Query Manager: table still open'
    );
  });

  it('calls onDeactivate then onActivate after the crossfade', async () => {
    const order = [];
    const outgoing = {
      onDeactivate: vi.fn(() => {
        order.push('deactivate');
      }),
    };
    const incoming = {
      onActivate: vi.fn(() => {
        order.push('activate');
      }),
    };
    const { app, loader } = harness(outgoing, 29, 'Query Manager');
    const incomingSlot = slot();
    app.loadedManagers.set(23, {
      id: 23,
      name: 'Lookup Manager',
      instance: incoming,
      slotEl: incomingSlot,
      workspaceEl: incomingSlot,
    });
    loader._crossfadeSlots = vi.fn(async (_outgoing, _incoming, onComplete) => {
      order.push('fade');
      onComplete();
    });

    await loader.showManager(23);

    expect(order).toEqual(['fade', 'deactivate', 'activate']);
    expect(app.currentManager.instance).toBe(incoming);
  });

  it('does not activate a manager that is already visible', async () => {
    const instance = { onActivate: vi.fn(), onDeactivate: vi.fn() };
    const { loader } = harness(instance);

    await loader.showManager(29);

    expect(instance.onActivate).not.toHaveBeenCalled();
    expect(instance.onDeactivate).not.toHaveBeenCalled();
  });

  it('rejects the retired import ids 1 through 6', async () => {
    const { loader } = harness({});

    await expect(loader._importManager(1)).rejects.toThrow('No import mapping for manager ID: 1');
    await expect(loader._importManager(6)).rejects.toThrow('No import mapping for manager ID: 6');
  });
});
