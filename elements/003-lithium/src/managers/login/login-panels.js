/**
 * Login Panels
 *
 * Manages panel switching, crossfade transitions, and overlay state
 * for the Login Manager. Extracted from login.js to keep files under
 * 1,000 lines per LITHIUM-INS.md rule #2.
 */

import { getTransitionDuration, waitForTransition } from '../../core/transitions.js';

/**
 * LoginPanels class
 *
 * Owns panel visibility, crossfade transitions, and the transition overlay.
 * Delegates panel-specific init/cleanup to the LoginManager via callbacks.
 */
export class LoginPanels {
  /**
   * @param {Object} options
   * @param {Object} options.panels - Map of panel names to DOM elements
   * @param {HTMLElement} options.loginPanel - The login panel element (reference for width/display rules)
   * @param {HTMLElement} options.overlay - The transition overlay element
   * @param {Function} [options.onBeforeSwitch] - Called before transition: (targetPanel) => void
   * @param {Function} [options.onAfterSwitch] - Called after transition: (fromPanel, toPanel, durationMs) => void
   */
  constructor({ panels, loginPanel, overlay, onBeforeSwitch, onAfterSwitch }) {
    this._panels = panels || {};
    this._loginPanel = loginPanel;
    this._overlay = overlay;
    this._onBeforeSwitch = onBeforeSwitch || (() => {});
    this._onAfterSwitch = onAfterSwitch || (() => {});
    this._currentPanel = 'login';
    this._focusTimer = null;
  }

  /** @returns {string} */
  get currentPanel() {
    return this._currentPanel;
  }

  /** @param {string} value */
  set currentPanel(value) {
    this._currentPanel = value;
  }

  /**
   * Switch to a different panel with crossfade transition
   * @param {string} targetPanel - Panel name to switch to
   */
  async switchPanel(targetPanel) {
    if (targetPanel === this._currentPanel) return;

    const fromPanel = this._panels[this._currentPanel];
    const toPanel = this._panels[targetPanel];

    if (!toPanel) return;

    // Cancel any pending focus timer
    this.cancelPendingFocus();

    // Let the owner do panel-specific prep (populate logs, init language, etc.)
    this._onBeforeSwitch(targetPanel);

    // Block interactions during transition
    this.enableOverlay();

    // Perform crossfade transition
    await this._performTransition(fromPanel, toPanel);

    // Update state
    const fromPanelName = this._currentPanel;
    this._currentPanel = targetPanel;
    const panelDuration = 0; // Timing is now the caller's responsibility

    // Re-enable interactions
    this.disableOverlay();

    // Let the owner do post-switch work (logging, focus management)
    this._onAfterSwitch(fromPanelName, targetPanel, panelDuration);
  }

  /**
   * Cancel any pending focus timer
   */
  cancelPendingFocus() {
    if (this._focusTimer !== null) {
      clearTimeout(this._focusTimer);
      this._focusTimer = null;
    }
  }

  /**
   * Schedule a focus action after a delay, with automatic cancellation
   * if another switch happens first.
   * @param {Function} focusFn - Function to call after delay
   * @param {number} [delay] - Delay in ms (defaults to transition duration)
   */
  scheduleFocus(focusFn, delay) {
    this.cancelPendingFocus();
    const actualDelay = delay ?? getTransitionDuration();
    this._focusTimer = setTimeout(() => {
      this._focusTimer = null;
      if (typeof focusFn === 'function') {
        focusFn();
      }
    }, actualDelay);
  }

  /**
   * Enable the transition overlay to block interactions
   */
  enableOverlay() {
    this._overlay?.classList.add('active');
  }

  /**
   * Disable the transition overlay
   */
  disableOverlay() {
    this._overlay?.classList.remove('active');
  }

  /**
   * Perform the actual panel transition animation with crossfade
   * @param {HTMLElement|null} fromElement
   * @param {HTMLElement|null} toElement
   * @private
   */
  async _performTransition(fromElement, toElement) {
    const duration = getTransitionDuration();

    // Determine the correct display value for the target panel
    const isLoginPanel = toElement === this._loginPanel;
    const targetDisplay = isLoginPanel ? 'block' : 'flex';

    // If we have both elements, do a true crossfade with absolute positioning
    if (fromElement && toElement) {
      // Get current dimensions before we modify anything
      const fromWidth = fromElement.offsetWidth;
      const fromHeight = fromElement.offsetHeight;
      const toWidth = toElement.offsetWidth || fromWidth; // fallback if not yet rendered
      const toHeight = toElement.offsetHeight || fromHeight;

      // Position both panels absolutely in the center of the container
      const centerStyles = `
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        margin: 0;
      `;

      // Apply absolute positioning to from element (keep visible during fade)
      fromElement.style.cssText = `
        ${centerStyles}
        width: ${fromWidth}px;
        height: ${fromHeight}px;
        display: ${fromElement.style.display || (fromElement === this._loginPanel ? 'block' : 'flex')};
        opacity: 1;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 1;
      `;

      // Prepare target element - also absolutely positioned in center
      let toCssWidth;
      if (toElement === this._loginPanel) {
        toCssWidth = '360px';
      } else if (toElement.id === 'language-panel') {
        toCssWidth = '484px'; // Language panel is wider
      } else {
        toCssWidth = '384px'; // Default subpanel width
      }
      toElement.style.cssText = `
        ${centerStyles}
        width: ${toCssWidth};
        display: ${targetDisplay};
        opacity: 0;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 2;
      `;

      // Force reflow
      void toElement.offsetHeight;

      // Start crossfade
      requestAnimationFrame(() => {
        // Fade out current panel
        fromElement.style.opacity = '0';

        // Fade in new panel (with slight delay for overlap effect)
        setTimeout(() => {
          toElement.style.opacity = '1';
        }, duration / 4);
      });

      // Wait for transition to complete
      await waitForTransition(duration + duration / 4 + 50);

      // Clean up from element
      fromElement.style.cssText = 'display: none;';

      // Restore target element to normal flow
      toElement.style.cssText = '';
      toElement.style.display = targetDisplay;
      toElement.style.opacity = '1';
      toElement.classList.add('visible');
    } else if (toElement) {
      // No from element, just fade in normally
      toElement.style.display = targetDisplay;
      void toElement.offsetHeight;
      requestAnimationFrame(() => {
        toElement.classList.add('visible');
      });
      await waitForTransition(duration);
    }
  }

  /**
   * Teardown the panels system
   */
  teardown() {
    this.cancelPendingFocus();
    this._panels = {};
    this._loginPanel = null;
    this._overlay = null;
    this._currentPanel = 'login';
  }
}
