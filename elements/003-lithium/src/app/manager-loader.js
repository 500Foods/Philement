import { eventBus, Events } from '../core/event-bus.js';
import { logManager, logGroup, Subsystems, Status } from '../core/log.js';
import { getPermittedManagers } from '../core/permissions.js';

let MANAGER_CROSSFADE_MS = 2500;

export class ManagerLoader {
  constructor(app) {
    this.app = app;
    this.managerRegistry = {
      // Content
      7: { id: 7, name: 'Dashboard Manager' },
      8: { id: 8, name: 'Mail Manager' },
      // User Management
      9: { id: 9, name: 'Profile Manager' },
      10: { id: 10, name: 'Session Manager' },
      11: { id: 11, name: 'Version Manager' },
      // Shared
      12: { id: 12, name: 'Calendar Manager' },
      13: { id: 13, name: 'Contact Manager' },
      14: { id: 14, name: 'File Manager' },
      15: { id: 15, name: 'Document Manager' },
      16: { id: 16, name: 'Media Manager' },
      17: { id: 17, name: 'Diagram Manager' },
      // AI
      18: { id: 18, name: 'Chat Manager' },
      // Support
      19: { id: 19, name: 'Notification Manager' },
      20: { id: 20, name: 'Annotation Manager' },
      21: { id: 21, name: 'Ticketing Manager' },
      // Configuration
      22: { id: 22, name: 'Style Manager' },
      23: { id: 23, name: 'Lookup Manager' },
      24: { id: 24, name: 'Report Manager' },
      // Security
      25: { id: 25, name: 'Role Manager' },
      26: { id: 26, name: 'Security Manager' },
      27: { id: 27, name: 'AI Auditor Manager' },
      28: { id: 28, name: 'Job Manager' },
      29: { id: 29, name: 'Query Manager' },
      30: { id: 30, name: 'Sync Manager' },
      31: { id: 31, name: 'Camera Manager' },
      32: { id: 32, name: 'Terminal' },
    };

    this.transitionOverlay = null;
  }

  getTransitionDurationFromCSS() {
    const duration = getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration')
      .trim();
    if (duration.includes('ms')) {
      MANAGER_CROSSFADE_MS = parseInt(duration, 10);
    } else if (duration.includes('s')) {
      MANAGER_CROSSFADE_MS = parseFloat(duration) * 1000;
    }
    return MANAGER_CROSSFADE_MS;
  }

  getTransitionOverlay() {
    if (this.transitionOverlay) return this.transitionOverlay;
    this.transitionOverlay = document.createElement('div');
    this.transitionOverlay.className = 'manager-transition-overlay';
    document.body.appendChild(this.transitionOverlay);
    return this.transitionOverlay;
  }

  showTransitionOverlay() {
    const overlay = this.getTransitionOverlay();
    if (overlay) overlay.classList.add('active');
  }

  hideTransitionOverlay() {
    const overlay = this.getTransitionOverlay();
    if (overlay) overlay.classList.remove('active');
  }

  _logManagers() {
    const managers = Object.values(this.managerRegistry);
    logGroup(Subsystems.STARTUP, Status.INFO, `Managers available: ${managers.length}`,
      managers.map(m => `${String(m.id).padStart(4, '0')}: ${m.name}`));
  }

  async _importManager(managerId) {
    switch (managerId) {
      case 7: return import('../managers/dashboard/dashboard.js');
      case 8: return import('../managers/mail-manager/mail-manager.js');
      case 9: return import('../managers/server-profiles/server-profiles.js');
      case 10: return import('../managers/server-sessions/server-sessions.js');
      case 11: return import('../managers/version-history/version-history.js');
      case 12: return import('../managers/calendar-manager/calendar-manager.js');
      case 13: return import('../managers/contact-manager/contact-manager.js');
      case 14: return import('../managers/file-manager/file-manager.js');
      case 15: return import('../managers/document-library/document-library.js');
      case 16: return import('../managers/media-library/media-library.js');
      case 17: return import('../managers/diagram-library/diagram-library.js');
      case 18: return import('../managers/chats/chats.js');
      case 19: return import('../managers/notification-manager/notification-manager.js');
      case 20: return import('../managers/annotation-manager/annotation-manager.js');
      case 21: return import('../managers/ticketing-manager/ticketing-manager.js');
      case 22: return import('../managers/style-manager/style-manager.js');
      case 23: return import('../managers/lookups/lookups.js');
      case 24: return import('../managers/reports/reports.js');
      case 25: return import('../managers/role-manager/role-manager.js');
      case 26: return import('../managers/security-manager/security-manager.js');
      case 27: return import('../managers/ai-auditor/ai-auditor.js');
      case 28: return import('../managers/job-manager/job-manager.js');
      case 29: return import('../managers/queries/queries.js');
      case 30: return import('../managers/sync-manager/sync-manager.js');
      case 31: return import('../managers/camera-manager/camera-manager.js');
      case 32: return import('../managers/terminal/terminal.js');
      case 1: return import('../managers/login/login.js');
      case 2: return import('../managers/server-sessions/server-sessions.js');
      case 3: return import('../managers/version-history/version-history.js');
      case 4: return import('../managers/queries/queries.js');
      case 5: return import('../managers/lookups/lookups.js');
      case 6: return import('../managers/chats/chats.js');
      default: throw new Error(`No import mapping for manager ID: ${managerId}`);
    }
  }

  async _importUtilityManager(managerKey) {
    switch (managerKey) {
      case 'session-log': return import('../managers/session-log/session-log.js');
      case 'user-profile': return import('../managers/profile-manager/profile-manager.js');
      case 'terminal': return import('../managers/terminal/terminal.js');
      default: throw new Error(`No import mapping for utility manager key: ${managerKey}`);
    }
  }

  _getMainManager() {
    return this.app.mainManagerInstance || null;
  }

  _saveLastManager(type, id) {
    try {
      localStorage.setItem('lithium_last_manager', `${type}:${id}`);
    } catch (_e) {
      // Non-fatal
    }
  }

  async loadMainManager(options = {}) {
    try {
      const { default: MainManager } = await import('../managers/main/main.js');
      const container = document.getElementById('app');
      container.innerHTML = '';
      const permittedManagers = getPermittedManagers();
      const mainManager = new MainManager(this.app, container, permittedManagers);
      this.app.mainManagerInstance = mainManager;
      await mainManager.init(options);

      if (this.app.currentManager?.id == null) {
        this.app.currentManager = { name: 'main', instance: mainManager };
      }
    } catch (error) {
      console.error('[Lithium] Failed to load main manager:', error);
      this.app.showFatalError('Failed to load application. Please refresh the page.');
    }
  }

  async loadManager(managerId) {
    const managerDef = this.managerRegistry[managerId];
    if (!managerDef) {
      logManager(Status.ERROR, `Unknown manager ID: ${managerId}`);
      return;
    }

    if (this.app.loadedManagers.has(managerId)) {
      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.app.currentManager?.id, to: managerId });
      logManager(Status.INFO, `Switched to already-loaded manager: ${managerDef.name}`);
      return;
    }

    try {
      logManager(Status.INFO,'────────────────────');
      logManager(Status.INFO, `Loading manager: ${managerDef.name}`);
      const loadStart = Date.now();
      const module = await this._importManager(managerId);
      const mainMgr = this._getMainManager();
      if (!mainMgr) throw new Error('MainManager not ready');

      const iconInfo = mainMgr.managerIcons?.[managerId] || { iconHtml: '<fa fa-cube></fa>', name: managerDef.name };
      const slotId = mainMgr._slotId(managerId);
      const slotResult = mainMgr.createSlot(slotId, iconInfo.iconHtml, iconInfo.name, managerId);
      if (!slotResult) throw new Error('Failed to create manager slot');
      const { slotEl, workspaceEl } = slotResult;

      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this.app, workspaceEl);
      await managerInstance.init();

      this.app.loadedManagers.set(managerId, {
        id: managerId,
        name: managerDef.name,
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      this.app.openManagers.add(managerId);
      mainMgr.updateOpenMenuItems();
      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_LOADED, { managerId });
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.app.currentManager?.id, to: managerId });
      logManager(Status.SUCCESS, `Manager ${managerDef.name} loaded`, Date.now() - loadStart);
      logManager(Status.INFO, '────────────────────');
    } catch (error) {
      logManager(Status.ERROR, `Failed to load manager ${managerDef?.name}: ${error.message}`);
    }
  }

  async showManager(managerId) {
    const managerData = this.app.loadedManagers.get(managerId) || this._getUtilityManagerData(managerId);
    if (!managerData) {
      logManager(Status.ERROR, `No manager data found for ID: ${managerId}`);
      return;
    }

    const mainMgr = this._getMainManager();
    if (!mainMgr) {
      logManager(Status.ERROR, 'MainManager not loaded');
      return;
    }

    const outgoingSlot = this.app.currentManager ? this._getSlotForCurrent() : null;
    const incomingSlot = managerData.slotEl || null;

    if (outgoingSlot === incomingSlot) {
      logManager(Status.INFO, `Manager ${managerData.name} is already visible`);
      return;
    }

    this.showTransitionOverlay();
    this._saveLastManager('manager', managerId);
    this.getTransitionDurationFromCSS();

    await this._crossfadeSlots(outgoingSlot, incomingSlot, () => {
      this.app.currentManager = { id: managerId, name: managerData.name, instance: managerData.instance };
      if (managerData.id) {
        this.app.openManagers.add(managerId);
        mainMgr.updateOpenMenuItems();
      }
      this.hideTransitionOverlay();
    });
  }

  _getSlotForCurrent() {
    if (!this.app.currentManager) return null;
    if (this.app.currentManager.name === 'main') return null;
    if (this.app.currentManager.name === 'login') return null;
    const data = this.app.loadedManagers.get(this.app.currentManager.id);
    return data ? data.slotEl : null;
  }

  _getUtilityManagerData(key) {
    const def = this.app.utilityManagerRegistry[key];
    if (!def) return null;
    return this.app.loadedManagers.get(def.id) || null;
  }

  async loadUtilityManager(key) {
    const def = this.app.utilityManagerRegistry[key];
    if (!def) {
      logManager(Status.ERROR, `Unknown utility manager key: ${key}`);
      return;
    }

    if (this.app.loadedManagers.has(def.id)) {
      await this.showManager(def.id);
      return;
    }

    try {
      logManager(Status.INFO, `Loading utility manager: ${def.name}`);
      const module = await this._importUtilityManager(key);
      const mainMgr = this._getMainManager();
      if (!mainMgr) throw new Error('MainManager not ready');

      const slotId = `utility-slot-${key}`;
      const slotResult = mainMgr.createSlot(slotId, '', def.name, def.id);
      if (!slotResult) throw new Error('Failed to create utility slot');
      const { slotEl, workspaceEl } = slotResult;

      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this.app, workspaceEl);
      await managerInstance.init();

      this.app.loadedManagers.set(def.id, {
        id: def.id,
        name: def.name,
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      this.app.openManagers.add(def.id);
      mainMgr.updateOpenMenuItems();
      await this.showManager(def.id);

      logManager(Status.SUCCESS, `Utility manager ${def.name} loaded`);
    } catch (error) {
      logManager(Status.ERROR, `Failed to load utility manager ${def?.name}: ${error.message}`);
    }
  }

  async _crossfadeSlots(outgoing, incoming, onComplete) {
    const duration = MANAGER_CROSSFADE_MS;
    const steps = [];

    if (outgoing) {
      steps.push(new Promise(resolve => {
        outgoing.classList.add('slot-fading-out');
        setTimeout(() => {
          outgoing.classList.remove('slot-visible', 'slot-fading-out');
          resolve();
        }, duration);
      }));
    }

    if (incoming) {
      steps.push(new Promise(resolve => {
        incoming.classList.add('slot-visible');
        setTimeout(resolve, duration);
      }));
    }

    await Promise.all(steps);
    if (onComplete) onComplete();
  }

  /**
   * Close a manager by ID.
   * Removes the slot from DOM, cleans up the manager instance, and updates UI.
   * @param {number|string} managerId - Manager ID or utility key
   */
  async closeManager(managerId) {
    // Handle both numeric IDs and string keys
    let numericId = managerId;
    let managerData = this.app.loadedManagers.get(managerId);

    // If not found by direct key, try to find by numeric ID
    if (!managerData && typeof managerId === 'number') {
      managerData = this.app.loadedManagers.get(managerId);
    }

    // Try utility manager registry
    if (!managerData && this.app.utilityManagerRegistry[managerId]) {
      const def = this.app.utilityManagerRegistry[managerId];
      managerData = this.app.loadedManagers.get(def.id);
      numericId = def.id;
    }

    if (!managerData) {
      logManager(Status.WARN, `closeManager: No manager found for ID ${managerId}`);
      return;
    }

    logManager(Status.INFO, `Closing manager: ${managerData.name}`);

    // Remove slot from DOM
    if (managerData.slotEl && managerData.slotEl.parentNode) {
      managerData.slotEl.remove();
    }

    // Call destroy/cleanup on manager instance if it exists
    if (managerData.instance && typeof managerData.instance.destroy === 'function') {
      try {
        await managerData.instance.destroy();
      } catch (err) {
        logManager(Status.WARN, `Error destroying manager ${managerData.name}: ${err.message}`);
      }
    }

    // Remove from tracking
    this.app.loadedManagers.delete(numericId);
    this.app.openManagers.delete(numericId);

    // Update main manager UI
    const mainMgr = this._getMainManager();
    if (mainMgr) {
      mainMgr.updateOpenMenuItems();
    }

    // Clear currentManager if it was this manager
    if (this.app.currentManager && this.app.currentManager.id === numericId) {
      this.app.currentManager = null;
    }
  }
}
