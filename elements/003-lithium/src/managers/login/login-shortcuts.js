/**
 * Login Shortcuts
 *
 * Owns the document-level keyboard handlers for the login screen:
 *
 *   - ESC: clear username/password (on login panel) or close subpanel (on
 *     theme/logs/language/help panels).
 *   - Ctrl+Shift+U: focus + select-all on username.
 *   - Ctrl+Shift+P: focus + select-all on password.
 *   - F1:           click the help button.
 *   - Ctrl+Shift+I: click the language button.
 *   - Ctrl+Shift+T: click the theme button.
 *   - Ctrl+Shift+L: click the logs button.
 *   - CAPS LOCK indicator: toggles `caps-lock-active` on the password field.
 *
 * Extracted from login.js to keep files under 1,000 lines per LITHIUM-INS.md
 * rule #2. Designed as a self-contained module with explicit callbacks for
 * ESC behaviour; the rest reads from the supplied `elements` map directly.
 */

import { log, Subsystems, Status } from '../../core/log.js';

const LOGIN = Subsystems.LOGIN;

export class LoginShortcuts {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element refs. Required:
   *   `username`, `password`. Optional: `helpBtn`, `languageBtn`, `themeBtn`,
   *   `logsBtn` (any missing btn is silently ignored).
   * @param {Function} options.getCurrentPanel - Returns the currently active
   *   panel name (e.g. `'login'`, `'theme'`, …). Used to gate non-ESC
   *   shortcuts to the login panel.
   * @param {Function} options.onClearUsername - Called when ESC fires on the
   *   login panel.
   * @param {Function} options.onSwitchToLogin - Called when ESC fires on a
   *   subpanel.
   * @param {Document} [options.doc] - Document override for tests.
   */
  constructor({ elements, getCurrentPanel, onClearUsername, onSwitchToLogin, doc } = {}) {
    this.elements = elements || {};
    this._getCurrentPanel = getCurrentPanel || (() => 'login');
    this._onClearUsername = onClearUsername || (() => {});
    this._onSwitchToLogin = onSwitchToLogin || (() => {});
    this._doc = doc || (typeof document !== 'undefined' ? document : null);

    this.isCapsLockOn = false;
    this._handleKeyDown = null;
    this._handleKeyUp = null;
  }

  /**
   * Attach `keydown`/`keyup` listeners on the document. Idempotent: a second
   * call before `detach()` is a no-op.
   */
  attach() {
    if (this._handleKeyDown || !this._doc) return;

    this._handleKeyDown = (e) => {
      this.checkCapsLock(e);
      this.handleKeyboardShortcuts(e);
    };
    this._handleKeyUp = (e) => this.checkCapsLock(e);

    this._doc.addEventListener('keydown', this._handleKeyDown);
    this._doc.addEventListener('keyup', this._handleKeyUp);
  }

  /**
   * Remove the document-level listeners.
   */
  detach() {
    if (!this._doc) return;
    if (this._handleKeyDown) {
      this._doc.removeEventListener('keydown', this._handleKeyDown);
      this._handleKeyDown = null;
    }
    if (this._handleKeyUp) {
      this._doc.removeEventListener('keyup', this._handleKeyUp);
      this._handleKeyUp = null;
    }
  }

  /**
   * Dispatch a keyboard event to the matching shortcut handler. Public so
   * tests can drive it without a real document listener.
   * @param {KeyboardEvent} event
   */
  handleKeyboardShortcuts(event) {
    const isOnLoginPanel = this._getCurrentPanel() === 'login';

    if (event.key === 'Escape') {
      this._handleEscapeKey(event, isOnLoginPanel);
      return;
    }

    if (!isOnLoginPanel) return;

    const shortcuts = [
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'u', handler: () => this._focusUsername() },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'p', handler: () => this._focusPassword() },
      { check: (e) => e.key === 'F1', handler: () => this._clickButton('helpBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'i', handler: () => this._clickButton('languageBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 't', handler: () => this._clickButton('themeBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'l', handler: () => this._clickButton('logsBtn') },
    ];

    for (const shortcut of shortcuts) {
      if (shortcut.check(event)) {
        shortcut.handler();
        event.preventDefault();
        return;
      }
    }
  }

  /**
   * Update the CAPS-LOCK indicator class on the password input. Public so
   * tests can drive it directly.
   * @param {KeyboardEvent} event
   */
  checkCapsLock(event) {
    const capsLockOn = event.getModifierState && event.getModifierState('CapsLock');
    if (this.isCapsLockOn !== capsLockOn) {
      this.isCapsLockOn = capsLockOn;
      this._updateCapsLockIndicator(capsLockOn);
    }
  }

  // -------- private helpers --------

  /** @private */
  _handleEscapeKey(event, isOnLoginPanel) {
    if (isOnLoginPanel) {
      this._onClearUsername();
    } else {
      log(LOGIN, Status.INFO, `Keyboard: ESC closed ${this._getCurrentPanel()} panel`);
      this._onSwitchToLogin();
    }
    event.preventDefault();
  }

  /** @private */
  _focusUsername() {
    if (this.elements.username) {
      log(LOGIN, Status.INFO, 'Keyboard: Ctrl+Shift+U focused username field');
      this.elements.username.focus();
      this.elements.username.select();
    }
  }

  /** @private */
  _focusPassword() {
    if (this.elements.password) {
      log(LOGIN, Status.INFO, 'Keyboard: Ctrl+Shift+P focused password field');
      this.elements.password.focus();
      this.elements.password.select();
    }
  }

  /** @private */
  _clickButton(btnName) {
    const btn = this.elements[btnName];
    if (btn && !btn.disabled) btn.click();
  }

  /** @private */
  _updateCapsLockIndicator(isCapsLockOn) {
    if (!this.elements.password) return;
    if (isCapsLockOn) {
      this.elements.password.classList.add('caps-lock-active');
    } else {
      this.elements.password.classList.remove('caps-lock-active');
    }
  }
}
