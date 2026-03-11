/**
 * Toast Notification System
 *
 * A toast notification system for displaying
 * error, success, warning, and info messages to users.
 *
 * Features:
 * - Unified header button group with icon, title, subsystem, expand, close
 * - Expand button shows/hides description and keeps the toast
 * - Countdown progress bar showing time remaining before auto-dismiss
 * - Font Awesome icons (customizable per toast)
 * - Collapsible detail section for extended information
 * - Automatic error logging via the logging system
 */

import { log, logGroup, Subsystems, Status } from '../core/log.js';

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
  icon: null,            // Custom icon (Font Awesome HTML), null = use default for type
  title: null,           // Title text (if not set, uses message)
  description: null,     // Description text shown when expanded
  subsystem: null,       // Subsystem label (e.g., 'Conduit', 'Auth')
  details: null,         // Optional details text/HTML for collapsible section
  detailsTitle: 'Details', // Label for the details toggle
  showDetails: false,    // Whether details section is expanded by default
  forceDescription: true, // Always show description (defaults to "No additional information provided")
};

const DEFAULT_DESCRIPTION = 'No additional information provided';

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

    // Icon: use custom if provided, otherwise use default for type
    const icon = options.icon || this._getIcon(options.type);

    // Title: use explicit title or extract from message
    const title = options.title || message;

    // Description: use provided or default if forceDescription is enabled
    let description = options.description;
    if (options.forceDescription && (!description || description.length === 0)) {
      description = DEFAULT_DESCRIPTION;
    }

    // Content flags
    const hasDetails = options.details && options.details.length > 0;
    const hasDescription = description && description.length > 0;
    const hasExpandableContent = hasDescription || hasDetails;

    // ─── Build HTML structure ───
    let html = '';

    // Header row: unified button group style
    html += '<div class="toast-header">';
    html += '<div class="toast-header-group">';

    // Icon button (leftmost, non-clickable)
    html += `<button class="toast-header-icon" aria-label="${options.type}" tabindex="-1">${icon}</button>`;

    // Title button (flex: 1, takes remaining space)
    html += `<button class="toast-header-title" tabindex="-1">${this._escapeHtml(title)}</button>`;

    // Subsystem badge (if provided)
    if (options.subsystem) {
      html += `<button class="toast-header-subsystem" tabindex="-1">${this._escapeHtml(options.subsystem)}</button>`;
    }

    // Expand button (if there's content to expand)
    if (hasExpandableContent && options.duration > 0) {
      html += '<button class="toast-header-expand" aria-label="Expand" title="Expand to keep"><i class="fas fa-chevron-down"></i></button>';
    }

    // Close button (rightmost)
    if (options.dismissible) {
      html += '<button class="toast-header-close" aria-label="Dismiss" title="Dismiss"><i class="fas fa-times"></i></button>';
    }

    html += '</div>'; // .toast-header-group
    html += '</div>'; // .toast-header

    // Content area (description + details combined into single section)
    if (hasExpandableContent) {
      html += '<div class="toast-content">';

      // Build combined content: description followed by details (if present)
      if (hasDescription) {
        html += `<div class="toast-description">${this._escapeHtml(description)}</div>`;
      }

      // Add details directly after description (no separate toggle needed)
      if (hasDetails) {
        html += `<div class="toast-details"><div class="toast-details-content">${options.details}</div></div>`;
      }

      html += '</div>'; // .toast-content
    }

    // Countdown progress bar (only when auto-dismissing)
    if (options.duration > 0) {
      html += `
        <div class="toast-progress">
          <div class="toast-progress-bar" style="animation-duration: ${options.duration}ms"></div>
        </div>
      `;
    }

    toast.innerHTML = html;

    // ─── Event handlers ───

    // Close button
    if (options.dismissible) {
      const closeBtn = toast.querySelector('.toast-header-close');
      closeBtn?.addEventListener('click', () => this.dismiss(id));
    }

    // Expand button - toggles content visibility with smooth height animation AND keeps the toast
    const expandBtn = toast.querySelector('.toast-header-expand');
    const content = toast.querySelector('.toast-content');
    if (expandBtn && content) {
      // Initialize height for animation
      content.style.height = '0px';
      content.style.overflow = 'hidden';

      expandBtn.addEventListener('click', () => {
        const isExpanded = expandBtn.classList.contains('toast-header-expand-active');

        if (isExpanded) {
          // Collapsing - set explicit height first, then animate to 0
          const targetHeight = content.scrollHeight;
          content.style.height = targetHeight + 'px';
          // Force reflow to ensure the browser registers the height change
          content.offsetHeight;
          requestAnimationFrame(() => {
            content.style.height = '0px';
          });
          expandBtn.classList.remove('toast-header-expand-active');
          expandBtn.setAttribute('aria-expanded', 'false');
        } else {
          // Expanding - animate to full height
          expandBtn.classList.add('toast-header-expand-active');
          expandBtn.setAttribute('aria-expanded', 'true');
          content.style.height = content.scrollHeight + 'px';
        }

        // Also keep the toast when expanded
        if (!isExpanded) {
          this.keep(id);
        }
      });

      // Listen for transition end to reset overflow
      content.addEventListener('transitionend', () => {
        if (content.style.height === '0px') {
          content.style.overflow = 'hidden';
        } else {
          content.style.height = 'auto';
          content.style.overflow = 'visible';
        }
      });
    }

    return toast;
  }

  /**
   * Escape HTML special characters for safe insertion
   * @private
   */
  _escapeHtml(text) {
    if (typeof text !== 'string') return text;
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
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
   * Log a server error through the logging system
   * @private
   */
  _logServerError(message, serverError, subsystem) {
    const logSub = subsystem || Subsystems.CONDUIT;
    const items = [];
    // Support both snake_case (server) and camelCase (JS convention)
    const queryRef = serverError.query_ref ?? serverError.queryRef;
    const database = serverError.database;
    const errorMsg = serverError.error;
    
    if (queryRef != null) items.push(`Query Ref: ${queryRef}`);
    if (database) items.push(`Database: ${database}`);
    if (errorMsg) items.push(`Error: ${errorMsg}`);
    if (serverError.message && serverError.message !== message) {
      items.push(`Message: ${serverError.message}`);
    }

    if (items.length > 0) {
      logGroup(logSub, Status.ERROR, message, items);
    } else {
      log(logSub, Status.ERROR, message);
    }
  }

  /**
   * Show a toast notification
   * @param {string} message - The message to display
   * @param {Object} options - Toast options
   * @param {string} [options.type='info'] - Toast type: error, success, warning, info
   * @param {number} [options.duration=5000] - Duration in ms (0 = persistent)
   * @param {boolean} [options.dismissible=true] - Whether the toast can be dismissed
   * @param {string} [options.icon=null] - Custom Font Awesome icon HTML
   * @param {string} [options.title=null] - Optional title (if not set, uses message)
   * @param {string} [options.description=null] - Description text shown when expanded
   * @param {string} [options.subsystem=null] - Subsystem label (e.g., 'Conduit')
   * @param {string} [options.details=null] - Optional details text/HTML for collapsible section
   * @param {string} [options.detailsTitle='Details'] - Label for the details toggle button
   * @param {boolean} [options.showDetails=false] - Whether to show details expanded by default
   * @param {boolean} [options.forceDescription=true] - Always show description section (defaults to "No additional information provided")
   * @returns {string} Toast ID
   */
  show(message, options = {}) {
    const id = this._generateId();
    const config = { ...TOAST_DEFAULTS, ...options };

    // Create and append toast element
    const toastEl = this._createToastElement(id, message, config);
    this.container.appendChild(toastEl);

    // Trigger entrance animation
    requestAnimationFrame(() => {
      toastEl.classList.add('toast-visible');
    });

    // Store toast info (register before starting timer so keep() can find it)
    const toastInfo = {
      element: toastEl,
      config,
      timerId: null,
    };
    this.toasts.set(id, toastInfo);

    // Auto-dismiss if duration is set
    if (config.duration > 0) {
      toastInfo.timerId = setTimeout(() => this.dismiss(id), config.duration);
    }

    return id;
  }

  /**
   * Show an error toast with optional details and automatic logging
   * @param {string} message - The error message
   * @param {Object} options - Toast options
   * @param {string} [options.icon] - Custom icon (defaults to error icon)
   * @param {string} [options.description] - Description text shown when expanded
   * @param {string} [options.details] - Optional details to show in collapsible section
   * @param {Object} [options.serverError] - Server error object with query_ref, database, etc.
   * @param {string} [options.subsystem] - Subsystem label for badge and logging
   * @returns {string} Toast ID
   */
  error(message, options = {}) {
    // Auto-build details from server error object if provided
    let details = options.details;
    let description = options.description;
    let title = options.title ?? message;

    if (options.serverError) {
      const { serverError } = options;

      // Use serverError.error as title (e.g., "Missing Required Parameters")
      if (serverError.error) {
        title = serverError.error;
      }

      // Auto-build description from serverError.message if not provided
      // Use serverError.message as description (e.g., "Required: QUERYID...")
      if (!description && serverError.message) {
        description = serverError.message;
      }

      // Auto-build details (support both snake_case and camelCase property names)
      if (!details) {
        const detailParts = [];
        const queryRef = serverError.query_ref ?? serverError.queryRef;
        const database = serverError.database;
        
        if (queryRef != null) {
          detailParts.push(`Query Ref: ${queryRef}`);
        }
        if (database) {
          detailParts.push(`Database: ${database}`);
        }
        if (detailParts.length > 0) {
          details = detailParts.join('\n');
        }
      }
}


    return this.show(title, {
      ...options,
      type: TOAST_TYPES.ERROR,
      details,
      description,
    });
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
   * Keep (pin) a toast — cancel auto-dismiss and freeze the countdown bar
   * @param {string} id - The toast ID
   */
  keep(id) {
    const toastInfo = this.toasts.get(id);
    if (!toastInfo) return;

    // Cancel auto-dismiss timer
    if (toastInfo.timerId) {
      clearTimeout(toastInfo.timerId);
      toastInfo.timerId = null;
    }

    // Stop and fade out progress bar
    const progress = toastInfo.element.querySelector('.toast-progress');
    if (progress) {
      progress.classList.add('toast-progress-kept');
    }

    // Mark expand button as active (kept)
    const expandBtn = toastInfo.element.querySelector('.toast-header-expand');
    if (expandBtn) {
      expandBtn.classList.add('toast-header-expand-active');
    }
  }

  /**
   * Dismiss a toast by ID
   * @param {string} id - The toast ID
   */
  dismiss(id) {
    const toastInfo = this.toasts.get(id);
    if (!toastInfo) return;

    // Clear auto-dismiss timer
    if (toastInfo.timerId) {
      clearTimeout(toastInfo.timerId);
      toastInfo.timerId = null;
    }

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
