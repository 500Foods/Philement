/**
 * Login Password Manager
 *
 * Handles suppression and removal of password manager injected UI elements
 * (1Password, LastPass, Bitwarden, Dashlane, etc.).
 * Extracted from login.js to keep files under 1,000 lines per LITHIUM-INS.md rule #2.
 */

import { getPasswordManagerSelectors } from '../../shared/log-formatter.js';

/**
 * PasswordManager class
 *
 * Owns MutationObserver-based suppression of password manager UI elements.
 * Provides show/hide/remove lifecycle methods for use by LoginManager.
 */
export class PasswordManager {
  constructor() {
    this._observer = null;
  }

  /**
   * Apply suppression styles to a password manager element.
   * @param {HTMLElement} el - Element to suppress
   * @private
   */
  _suppressElement(el) {
    el.style.setProperty('display', 'none', 'important');
    el.style.setProperty('visibility', 'hidden', 'important');
    el.style.setProperty('opacity', '0', 'important');
    el.style.setProperty('pointer-events', 'none', 'important');
  }

  /**
   * Suppress elements matching a selector.
   * @param {string} selector - CSS selector
   * @private
   */
  _suppressSelector(selector) {
    try {
      document.querySelectorAll(selector).forEach((el) => this._suppressElement(el));
    } catch (e) {
      // Invalid selector, skip
    }
  }

  /**
   * Suppress all password manager elements and reconnect observer.
   * @param {string[]} selectors - Array of CSS selectors
   * @private
   */
  _suppressAndReconnect(selectors) {
    // Temporarily disconnect the observer while we mutate styles so that our
    // own setAttribute calls do not re-trigger this callback (infinite loop).
    // We immediately reconnect after the sweep so new 1Password injections
    // are still caught.
    this._observer?.disconnect();

    try {
      for (const selector of selectors) {
        this._suppressSelector(selector);
      }
    } finally {
      // Reconnect so future 1Password re-injections/re-shows are caught.
      // Guard against calling observe() after teardown (observer may be null).
      if (this._observer) {
        this._observer.observe(document.body, {
          childList: true,
          subtree: true,
          attributes: true,
          attributeFilter: ['style', 'class'],
        });
      }
    }
  }

  /**
   * Start password manager UI suppression with MutationObserver.
   * @param {string[]} selectors - Array of CSS selectors
   * @private
   */
  _startSuppression(selectors) {
    const suppressElements = () => this._suppressAndReconnect(selectors);

    // Create the observer (initially disconnected; suppressElements reconnects it).
    this._observer = new MutationObserver(suppressElements);

    // Run the initial suppression sweep, which also starts the observer.
    suppressElements();

    // Schedule a follow-up sweep on the next animation frame so we get the
    // last word after any 1Password re-assertion in the same microtask flush.
    requestAnimationFrame(suppressElements);
  }

  /**
   * Stop password manager UI suppression.
   * @private
   */
  _stopSuppression() {
    document.body.classList.remove('hide-password-manager-ui');
    this._observer?.disconnect();
    this._observer = null;
  }

  /**
   * Show password manager UI (stop suppression).
   */
  show() {
    this._stopSuppression();
  }

  /**
   * Hide password manager UI (start suppression).
   */
  hide() {
    const selectors = getPasswordManagerSelectors();
    this._startSuppression(selectors);
  }

  /**
   * Remove all password manager injected elements from the DOM.
   * Called after a successful login — at that point we want the elements gone
   * entirely, not just hidden, so they do not linger in the post-login view.
   */
  removeAll() {
    const selectors = getPasswordManagerSelectors();
    for (const selector of selectors) {
      try {
        document.querySelectorAll(selector).forEach((el) => el.remove());
      } catch (e) {
        // Invalid selector, skip
      }
    }
  }

  /**
   * Destroy the password manager handler.
   * Disconnects observer, removes body class, and removes any lingering elements.
   */
  destroy() {
    this._observer?.disconnect();
    this._observer = null;
    document.body.classList.remove('hide-password-manager-ui');
    this.removeAll();
  }
}
