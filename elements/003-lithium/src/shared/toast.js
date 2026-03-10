/**
 * Toast Notification System
 *
 * A lightweight toast notification system for displaying
 * error, success, warning, and info messages to users.
 */

const TOAST_TYPES = {
  ERROR: 'error',
  SUCCESS: 'success',
  WARNING: 'warning',
  INFO: 'info',
};

const TOAST_DEFAULTS = {
  duration: 5000,
  dismissible: true,
  type: TOAST_TYPES.INFO,
};

class ToastManager {
  constructor() {
    this.container = null;
    this.toasts = new Map();
    this._idCounter = 0;
    this._initContainer();
  }

  /**
   * Initialize the toast container in the DOM
   * @private
   */
  _initContainer() {
    // Check if container already exists
    this.container = document.getElementById('toast-container');

    if (!this.container) {
      this.container = document.createElement('div');
      this.container.id = 'toast-container';
      this.container.className = 'toast-container';
      document.body.appendChild(this.container);
    }
  }

  /**
   * Generate a unique ID for each toast
   * @private
   */
  _generateId() {
    return `toast-${++this._idCounter}`;
  }

  /**
   * Create a toast element
   * @private
   */
  _createToastElement(id, message, options) {
    const toast = document.createElement('div');
    toast.className = `toast toast-${options.type}`;
    toast.dataset.id = id;

    // Icon based on type
    const icon = this._getIcon(options.type);

    // Content HTML
    toast.innerHTML = `
      <span class="toast-icon">${icon}</span>
      <span class="toast-message">${message}</span>
      ${options.dismissible ? '<button class="toast-dismiss" aria-label="Dismiss">&times;</button>' : ''}
    `;

    // Add dismiss handler
    if (options.dismissible) {
      const dismissBtn = toast.querySelector('.toast-dismiss');
      dismissBtn?.addEventListener('click', () => this.dismiss(id));
    }

    return toast;
  }

  /**
   * Get icon HTML for toast type
   * @private
   */
  _getIcon(type) {
    const icons = {
      [TOAST_TYPES.ERROR]: '<i class="fas fa-times-circle"></i>',
      [TOAST_TYPES.SUCCESS]: '<i class="fas fa-check-circle"></i>',
      [TOAST_TYPES.WARNING]: '<i class="fas fa-exclamation-triangle"></i>',
      [TOAST_TYPES.INFO]: '<i class="fas fa-info-circle"></i>',
    };
    return icons[type] || icons[TOAST_TYPES.INFO];
  }

  /**
   * Show a toast notification
   * @param {string} message - The message to display
   * @param {Object} options - Toast options
   * @param {string} [options.type='info'] - Toast type: error, success, warning, info
   * @param {number} [options.duration=5000] - Duration in ms (0 = persistent)
   * @param {boolean} [options.dismissible=true] - Whether the toast can be dismissed
   * @returns {string} Toast ID
   */
  show(message, options = {}) {
    const id = this._generateId();
    const config = { ...TOAST_DEFAULTS, ...options };

    // Create and append toast element
    const toastEl = this._createToastElement(id, message, config);
    this.container.appendChild(toastEl);

    // Trigger animation
    requestAnimationFrame(() => {
      toastEl.classList.add('toast-visible');
    });

    // Store toast info
    this.toasts.set(id, {
      element: toastEl,
      config,
    });

    // Auto-dismiss if duration is set
    if (config.duration > 0) {
      setTimeout(() => this.dismiss(id), config.duration);
    }

    return id;
  }

  /**
   * Show an error toast
   * @param {string} message - The error message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  error(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.ERROR });
  }

  /**
   * Show a success toast
   * @param {string} message - The success message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  success(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.SUCCESS });
  }

  /**
   * Show a warning toast
   * @param {string} message - The warning message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  warning(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.WARNING });
  }

  /**
   * Show an info toast
   * @param {string} message - The info message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  info(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.INFO });
  }

  /**
   * Dismiss a toast by ID
   * @param {string} id - The toast ID
   */
  dismiss(id) {
    const toastInfo = this.toasts.get(id);
    if (!toastInfo) return;

    const { element } = toastInfo;

    // Trigger exit animation
    element.classList.remove('toast-visible');
    element.classList.add('toast-exit');

    // Remove from DOM after animation
    setTimeout(() => {
      element.remove();
      this.toasts.delete(id);
    }, 300);
  }

  /**
   * Dismiss all toasts
   */
  dismissAll() {
    const ids = Array.from(this.toasts.keys());
    ids.forEach(id => this.dismiss(id));
  }
}

// Create a singleton instance
const toast = new ToastManager();

export { toast, TOAST_TYPES };
export default toast;
