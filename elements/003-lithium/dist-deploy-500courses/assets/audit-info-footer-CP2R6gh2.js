import { D as DateTime } from './luxon-DbyXf6dF.js';
import { c as closeAllPopups } from './manager-ui-B8HP2uIT.js';
import { l as log, S as Status, a as Subsystems, p as processIcons } from './index-CNmg4i7Y.js';

/**
 * Audit Info Footer Component
 *
 * Displays audit information (updated_id, updated_at) in the manager footer
 * when a record with those fields is selected. Clicking opens a popup with
 * full audit history (created_id, created_at, updated_id, updated_at).
 *
 * The component creates a new audit info button after the save/cancel buttons.
 * The existing footer placeholder (with flex:1) remains to provide spacing.
 *
 * @module core/audit-info-footer
 */


class AuditInfoFooter {
  /**
   * @param {Object} manager - The manager instance
   */
  constructor(manager) {
    this.manager = manager;
    this.container = manager.container;
    this.auditButton = null;   // The transformed placeholder element
    this.userLine = null;
    this.timestampLine = null;
    this.currentRowData = null;
    this.auditPopup = null;
    this._clickHandler = null;
    this._closeAllHandler = null;
    this._outsideClickHandler = null;
    this._escapeHandler = null;
  }

  /**
   * Initialize the audit footer component
   */
  init() {
    this._transformPlaceholder();
  }

  /**
   * Find the footer placeholder and create the audit info button after it.
   * The placeholder provides flex spacing; the audit button sits after save/cancel.
   */
  _transformPlaceholder() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) {
      log(Subsystems.MANAGER, Status.WARN, '[AuditInfoFooter] No manager-slot found');
      return;
    }

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) {
      log(Subsystems.MANAGER, Status.WARN, '[AuditInfoFooter] No footer found');
      return;
    }

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) {
      log(Subsystems.MANAGER, Status.WARN, '[AuditInfoFooter] No footer group found');
      return;
    }

    const placeholder = group.querySelector('.slot-footer-placeholder');
    if (!placeholder) {
      log(Subsystems.MANAGER, Status.WARN, '[AuditInfoFooter] No placeholder found');
      return;
    }

    // Create the audit button. Keep the placeholder for flex:1 spacing.
    this.auditButton = document.createElement('button');
    this.auditButton.type = 'button';
    this.auditButton.classList.add('audit-info-btn');
    this.auditButton.style.minWidth = '0'; // will be updated after content set

    // Build inner structure
    const content = document.createElement('div');
    content.className = 'audit-info-content';

    this.userLine = document.createElement('div');
    this.userLine.className = 'audit-info-user';
    this.userLine.textContent = '';

    this.timestampLine = document.createElement('div');
    this.timestampLine.className = 'audit-info-timestamp';
    this.timestampLine.textContent = '';

    content.appendChild(this.userLine);
    content.appendChild(this.timestampLine);
    this.auditButton.appendChild(content);

    // Click handler for popup
    this._clickHandler = (e) => {
      e.stopPropagation();
      this._togglePopup();
    };
    this.auditButton.addEventListener('click', this._clickHandler);

    // Insert after save/cancel buttons (before footer buttons like Crimson).
    // The placeholder remains in place for flex:1 spacing.
    const cancelBtn = group.querySelector('#manager-footer-cancel');
    const saveBtn = group.querySelector('#manager-footer-save');
    if (cancelBtn) {
      // Insert after cancel button
      group.insertBefore(this.auditButton, cancelBtn.nextSibling);
    } else if (saveBtn) {
      // Insert after save button (if no cancel)
      group.insertBefore(this.auditButton, saveBtn.nextSibling);
    } else {
      // Fallback: insert after placeholder
      group.insertBefore(this.auditButton, placeholder.nextSibling);
    }
  }

  /**
   * Update the audit display based on selected row data.
   * If rowData is null or missing required fields, clears the text.
   * @param {Object} rowData - The selected row data
   */
  update(rowData) {
    if (!rowData) {
      this._clearText();
      this.currentRowData = null;
      return;
    }

    const hasUpdatedId = rowData.updated_id !== undefined && rowData.updated_id !== null;
    const hasUpdatedAt = rowData.updated_at !== undefined && rowData.updated_at !== null;

    if (!(hasUpdatedId && hasUpdatedAt)) {
      this._clearText();
      this.currentRowData = null;
      return;
    }

    this.currentRowData = rowData;
    this._updateDisplay();
  }

  /**
   * Clear both text lines
   */
  _clearText() {
    this.userLine.textContent = '';
    this.timestampLine.textContent = '';
    if (this.auditButton) {
      this.auditButton.style.minWidth = '0';
    }
  }

  /**
   * Update the button text content from currentRowData and adjust width
   */
  _updateDisplay() {
    if (!this.currentRowData) return;

    const userId = this.currentRowData.updated_id;
    this.userLine.textContent = `UserID = ${userId}`;

    const rawTimestamp = this.currentRowData.updated_at;
    if (rawTimestamp) {
      const formatted = this._formatDateTime(rawTimestamp);
      this.timestampLine.textContent = formatted;
    } else {
      this.timestampLine.textContent = '';
    }

    this._adjustWidth();
  }

  /**
   * Measure content width and set min-width accordingly to prevent wrapping.
   */
  _adjustWidth() {
    if (!this.auditButton) return;

    const wasHidden = this.auditButton.style.display === 'none';
    if (wasHidden) this.auditButton.style.display = 'flex';

    // Force reflow
    this.auditButton.offsetHeight;

    const userWidth = this.userLine.scrollWidth;
    const timeWidth = this.timestampLine.scrollWidth;
    const needed = Math.max(userWidth, timeWidth) + 20;

    this.auditButton.style.minWidth = `${needed}px`;

    if (wasHidden) this.auditButton.style.display = 'none';
  }

  /**
   * Format datetime using user's Medium DateTime preference and timezone.
   * Handles both ISO and non-ISO formats (e.g., "2026-04-27 21:20:34.962223").
   * @param {string|Date} timestamp - The timestamp to format
   * @returns {string} Formatted datetime string
   */
  _formatDateTime(timestamp) {
    if (!timestamp) return '';

    try {
      const dateFormat = window.lithiumSettings?.get?.('datetimes.medium', 'yyyy-MMM-dd (EEE) HH:mm:ss');
      const timezone = window.lithiumSettings?.get?.('timezone', 'UTC');

      let dt;
      const rawStr = String(timestamp);

      // Clean non-ISO timestamps: replace space with T and truncate microseconds to milliseconds
      const cleaned = rawStr
        .replace(' ', 'T')
        .replace(/\.(\d{3})\d+/, '.$1'); // keep only first 3 fraction digits

      dt = DateTime.fromISO(cleaned, { zone: 'utc' });

      if (!dt.isValid) {
        // Fallback: try as Date
        const date = new Date(timestamp);
        if (!isNaN(date.getTime())) {
          dt = DateTime.fromJSDate(date, { zone: 'utc' });
        } else {
          return rawStr.substring(0, 19);
        }
      }

      return dt.setZone(timezone).toFormat(dateFormat);
    } catch (e) {
      log(Subsystems.MANAGER, Status.WARN, '[AuditInfoFooter] Format error:', e);
      return String(timestamp).substring(0, 19);
    }
  }

  /**
   * Toggle the audit history popup
   */
  _togglePopup() {
    if (!this.currentRowData) return;

    if (this.auditPopup && this.auditPopup.classList.contains('visible')) {
      this._closePopup();
    } else {
      this._openPopup();
    }
  }

  /**
   * Open the audit history popup
   */
  _openPopup() {
    if (!this.currentRowData) return;

    closeAllPopups();
    this._createPopup();
    this._positionPopup();

    requestAnimationFrame(() => {
      this.auditPopup.classList.add('visible');
    });

    this.auditButton.classList.add('popup-active');
    this._setupCloseHandlers();
  }

  /**
   * Create popup DOM
   */
  _createPopup() {
    this.auditPopup = document.createElement('div');
    this.auditPopup.className = 'manager-footer-popup audit-history-popup';
    this.auditPopup.setAttribute('aria-label', 'Audit History');

    const header = document.createElement('div');
    header.className = 'audit-popup-header';
    header.innerHTML = `
      <span class="audit-popup-title">
        <fa fa-history></fa>
        <span>Audit History</span>
      </span>
    `;

    const content = document.createElement('div');
    content.className = 'audit-popup-content';

    const createRow = (label, value) => {
      const row = document.createElement('div');
      row.className = 'audit-popup-row';
      const labelEl = document.createElement('div');
      labelEl.className = 'audit-popup-label';
      labelEl.textContent = label;
      const valueEl = document.createElement('div');
      valueEl.className = 'audit-popup-value';
      valueEl.textContent = this._formatAuditValue(value);
      row.appendChild(labelEl);
      row.appendChild(valueEl);
      return row;
    };

    const rowData = this.currentRowData;
    content.appendChild(createRow('Created By', rowData.created_by));
    content.appendChild(createRow('Created At', rowData.created_at));
    content.appendChild(createRow('Updated By', rowData.updated_id));
    content.appendChild(createRow('Updated At', rowData.updated_at));

    this.auditPopup.appendChild(header);
    this.auditPopup.appendChild(content);
    document.body.appendChild(this.auditPopup);

    processIcons(this.auditPopup);
  }

  /**
   * Format a value for popup display
   */
  _formatAuditValue(value) {
    if (value === null || value === undefined) return '—';
    if (typeof value === 'string' && value.trim() === '') return '—';
    if (this._isDateString(value)) {
      return this._formatDateTime(value);
    }
    return String(value);
  }

  /**
   * Check if string is a date/timestamp
   */
  _isDateString(value) {
    if (typeof value !== 'string') return false;
    return /^\d{4}-\d{2}-\d{2}/.test(value) || /^\d{4}\/\d{2}\/\d{2}/.test(value);
  }

  /**
   * Position popup above the button (footer-riseup style)
   */
  _positionPopup() {
    if (!this.auditPopup || !this.auditButton) return;

    const btnRect = this.auditButton.getBoundingClientRect();
    this.auditPopup.style.position = 'fixed';
    this.auditPopup.style.zIndex = '10001';
    this.auditPopup.style.right = `${window.innerWidth - btnRect.right}px`;
    this.auditPopup.style.bottom = `${window.innerHeight - btnRect.top + 1}px`;
    this.auditPopup.style.top = 'auto';
    this.auditPopup.style.left = 'auto';
  }

  /**
   * Setup close handlers for popup
   */
  _setupCloseHandlers() {
    this._closeAllHandler = () => this._closePopup();
    document.addEventListener('close-all-popups', this._closeAllHandler);

    this._outsideClickHandler = (e) => {
      if (!this.auditPopup?.contains(e.target) && !this.auditButton.contains(e.target)) {
        this._closePopup();
      }
    };
    document.addEventListener('click', this._outsideClickHandler);

    this._escapeHandler = (e) => {
      if (e.key === 'Escape') this._closePopup();
    };
    document.addEventListener('keydown', this._escapeHandler);
  }

  /**
   * Close the popup
   */
  _closePopup() {
    if (!this.auditPopup) return;

    this.auditPopup.classList.remove('visible');
    this.auditButton.classList.remove('popup-active');

    if (this._closeAllHandler) {
      document.removeEventListener('close-all-popups', this._closeAllHandler);
      this._closeAllHandler = null;
    }
    if (this._outsideClickHandler) {
      document.removeEventListener('click', this._outsideClickHandler);
      this._outsideClickHandler = null;
    }
    if (this._escapeHandler) {
      document.removeEventListener('keydown', this._escapeHandler);
      this._escapeHandler = null;
    }

    const duration = parseFloat(getComputedStyle(this.auditPopup).transitionDuration) * 1000 || 350;
    setTimeout(() => {
      if (this.auditPopup && this.auditPopup.parentNode) {
        this.auditPopup.remove();
      }
      this.auditPopup = null;
    }, duration);
  }

  /**
   * Cleanup
   */
  destroy() {
    this._closePopup();
    if (this._clickHandler && this.auditButton) {
      this.auditButton.removeEventListener('click', this._clickHandler);
    }
    this.auditButton = null;
    this.currentRowData = null;
  }
}

export { AuditInfoFooter as A };
