/**
 * WebSocket and Radar for MainManager
 * Handles WebSocket connection and radar icon toggle between header/sidebar
 */

import { getAppWS, ConnectionState } from '../../shared/app-ws.js';
import { initRadar, setActiveRadar, wsConnected, wsFlaky, wsDisconnected, onHeartbeat, destroyRadar, setRadarTooltip, getRadarTooltip } from '../../shared/radar-controller.js';
import { getTip, initTooltips } from '../../core/tooltip-api.js';
import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Methods to be mixed into MainManager prototype
 */
export const websocketMethods = {
  /**
   * Initialize the app-wide WebSocket connection
   * Called after successful login
   */
  async initWebSocket() {
    const ws = getAppWS({
      onStateChange: (newState, oldState) => {
        this.updateWSStatus(newState);
      },
      onSendActivity: () => {
        this.flashWSStatus();
      },
    });

    // Set initial status
    this.updateWSStatus(ws.getState());

    // Connect to the WebSocket server
    try {
      await ws.connect();
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, `[MainManager] WebSocket connection failed: ${error.message}`);
      // Not fatal - connection will be retried when needed
    }

    return ws;
  },

  /**
   * Update the radar status icon based on WebSocket connection state
   * @param {string} state - Connection state from ConnectionState
   */
  updateWSStatus(state) {
    const radarIcon = this.elements.radarIcon;
    if (!radarIcon) return;

    let tooltipText = 'Status: Disconnected';
    switch (state) {
      case ConnectionState.CONNECTED:
        wsConnected();
        tooltipText = 'Status: Connected';
        break;
      case ConnectionState.CONNECTING:
        wsFlaky();
        tooltipText = 'Status: Connecting...';
        break;
      case ConnectionState.ERROR:
        wsDisconnected();
        tooltipText = 'Status: Error';
        break;
      case ConnectionState.DISCONNECTED:
      default:
        wsDisconnected();
        tooltipText = 'Status: Disconnected';
        break;
    }

    // Update tooltip on active radar element and track for sync
    radarIcon.dataset.tooltip = tooltipText;
    const tipInstance = getTip(radarIcon);
    if (tipInstance) tipInstance.updateContent(tooltipText);
    setRadarTooltip(tooltipText);
  },

  /**
   * Flash the radar sweep line (send activity / heartbeat)
   */
  flashWSStatus() {
    onHeartbeat();
  },

  /**
   * Toggle radar position between header and sidebar
   * Uses --transition-duration to animate opacity
   */
  _toggleRadarPosition() {
    const headerRadar = this.elements.radarIcon;
    const sidebarRadar = this.elements.radarIconSidebar;
    if (!headerRadar || !sidebarRadar) return;
  
    this._isRadarInSidebar = !this._isRadarInSidebar;
  
    const duration = this.getTransitionDuration() || 300;
    const transitionStr = `opacity ${duration}ms ease`;
  
    // Ensure transition is set (safe to call every toggle)
    headerRadar.style.transition = transitionStr;
    sidebarRadar.style.transition = transitionStr;
  
    // Disable pointer events during the whole animation
    headerRadar.style.pointerEvents = 'none';
    sidebarRadar.style.pointerEvents = 'none';
  
    if (this._isRadarInSidebar) {
      // === MOVING TO SIDEBAR ===
      setActiveRadar('sidebar');

      // Sync tooltip text to both radars
      const tooltipText = getRadarTooltip();
      headerRadar.dataset.tooltip = tooltipText;
      sidebarRadar.dataset.tooltip = tooltipText;
      const headerTip = getTip(headerRadar);
      const sidebarTip = getTip(sidebarRadar);
      if (headerTip && headerTip.updateContent) headerTip.updateContent(tooltipText);
      if (sidebarTip && sidebarTip.updateContent) sidebarTip.updateContent(tooltipText);
   
      // 1. Bring sidebar into the render tree at opacity 0
      sidebarRadar.style.display = 'block';
      sidebarRadar.style.opacity = '0';
   
      // Start fade-out on header immediately (it was already visible)
      headerRadar.style.opacity = '0';
   
      // Double RAF = the trick that makes SVGs fade IN reliably
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          sidebarRadar.style.opacity = '1';
        });
      });
   
      // Cleanup: hide header once its fade-out finishes
      headerRadar.addEventListener('transitionend', function handler(e) {
        if (e.propertyName === 'opacity') {
          headerRadar.style.display = 'none';
          headerRadar.removeEventListener('transitionend', handler);
        }
      }, { once: true });
   
    } else {
      // === MOVING BACK TO HEADER ===
      setActiveRadar('header');

      // Sync tooltip text to both radars
      const tooltipText = getRadarTooltip();
      headerRadar.dataset.tooltip = tooltipText;
      sidebarRadar.dataset.tooltip = tooltipText;
      const headerTip = getTip(headerRadar);
      const sidebarTip = getTip(sidebarRadar);
      if (headerTip && headerTip.updateContent) headerTip.updateContent(tooltipText);
      if (sidebarTip && sidebarTip.updateContent) sidebarTip.updateContent(tooltipText);
   
      // 1. Bring header into the render tree at opacity 0
      headerRadar.style.display = 'block';
      headerRadar.style.opacity = '0';
   
      // Start fade-out on sidebar immediately
      sidebarRadar.style.opacity = '0';
   
      // Double RAF for the incoming header SVG
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          headerRadar.style.opacity = '1';
        });
      });
  
      // Cleanup: hide sidebar once its fade-out finishes
      sidebarRadar.addEventListener('transitionend', function handler(e) {
        if (e.propertyName === 'opacity') {
          sidebarRadar.style.display = 'none';
          sidebarRadar.removeEventListener('transitionend', handler);
        }
      }, { once: true });
    }
  
    // Re-enable pointer events once everything is definitely done
    setTimeout(() => {
      headerRadar.style.pointerEvents = '';
      sidebarRadar.style.pointerEvents = '';
    }, duration + 50);

    log(Subsystems.MANAGER, Status.INFO, `[MainManager] Radar moved to ${this._isRadarInSidebar ? 'sidebar' : 'header'}`);
  },
};
