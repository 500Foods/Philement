/**
 * Profile Manager - Date Formats Page Handler
 *
 * Handles the Date Formats settings page (index: -9)
 * Manages date/time format selection with live preview using Luxon.
 *
 * Uses the ProfileSettingsService for all persistence.
 * Section key: "-9"
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { DateTime } from 'luxon';
import { log, Subsystems, Status } from '../../../core/log.js';

const DEFAULT_SAMPLE = '2020-01-01T14:03:02';

const BUILTIN_FORMATS = {
  dates: {
    short: { name: 'Short Date', format: 'yyyy-MM-dd' },
    long: { name: 'Long Date', format: 'MMMM d, y' },
  },
  times: {
    short: { name: 'Short Time', format: 'h:mm a' },
    medium: { name: 'Medium Time', format: 'h:mm:ss a' },
    long: { name: 'Long Time', format: 'h:mm:ss a zzz' },
  },
  datetime: {
    short: { name: 'Short DateTime', format: 'yyyy-MM-dd h:mm a' },
    long: { name: 'Long DateTime', format: "MMMM d, y 'at' h:mm:ss a" },
  },
};

export class DateFormatsPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -9,
      formSelector: '.df-content',
    });

    this._originalData = {};
    this._currentUpdateInterval = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._setupEventListeners();
    await this.loadData();
    this._startCurrentTimeUpdates();

    log(Subsystems.MANAGER, Status.DEBUG, '[DateFormatsPage] Initialized');
  }

  /**
   * Setup event listeners
   */
  _setupEventListeners() {
    const container = this.container;
    if (!container) return;

    const formatInputs = container.querySelectorAll('.df-format-input');
    formatInputs.forEach(input => {
      input.addEventListener('input', () => {
        this._updatePreview(input);
        this.setDirty(true);
      });
    });

    container.querySelectorAll('.df-add-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const type = e.currentTarget.dataset.type;
        this._addCustomFormat(type);
      });
    });

    container.querySelectorAll('.df-delete-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const item = e.currentTarget.closest('.df-format-item');
        if (item && confirm('Delete this custom format?')) {
          item.remove();
          this.setDirty(true);
        }
      });
    });
  }

  /**
   * Start updating "Current" column every second
   */
  _startCurrentTimeUpdates() {
    if (this._currentUpdateInterval) {
      clearInterval(this._currentUpdateInterval);
    }
    this._currentUpdateInterval = setInterval(() => {
      this._renderCurrentPreviews();
    }, 1000);
  }

  /**
   * Render current time previews
   */
  _renderCurrentPreviews() {
    const container = this.container;
    if (!container) return;

    const now = DateTime.now().toUTC();

    // Update the current datetime display at top
    const currentDisplay = container.querySelector('#df-current-datetime');
    if (currentDisplay) {
      currentDisplay.textContent = now.toFormat("yyyy-MM-dd'T'HH:mm:ss");
    }

    container.querySelectorAll('.df-current').forEach(el => {
      const format = el.dataset.format;
      if (format) {
        try {
          el.textContent = now.toFormat(format);
        } catch (e) {
          el.textContent = 'Invalid';
        }
      }
    });
  }

  /**
   * Update single preview
   */
  _updatePreview(input) {
    const format = input.value;
    const row = input.closest('.df-format-item');
    if (!row) return;

    const exampleEl = row.querySelector('.df-example');
    const currentEl = row.querySelector('.df-current');

    const sample = this._getSampleDateTime();
    const now = DateTime.now().toUTC();

    try {
      if (exampleEl) {
        exampleEl.textContent = sample.toFormat(format);
      }
    } catch (e) {
      if (exampleEl) exampleEl.textContent = 'Invalid format';
    }

    try {
      if (currentEl) {
        currentEl.textContent = now.toFormat(format);
      }
    } catch (e) {
      if (currentEl) currentEl.textContent = 'Invalid';
    }
  }

  /**
   * Get sample DateTime
   */
  _getSampleDateTime() {
    const input = this.container?.querySelector('#df-sample-input');
    const sampleStr = input?.value || DEFAULT_SAMPLE;
    return DateTime.fromISO(sampleStr, { zone: 'utc' });
  }

  /**
   * Render all previews
   */
  _renderAllPreviews() {
    const container = this.container;
    if (!container) return;

    const formatInputs = container.querySelectorAll('.df-format-input');
    formatInputs.forEach(input => this._updatePreview(input));
  }

  /**
   * Add custom format
   */
  _addCustomFormat(type) {
    const container = this.container;
    if (!container) return;

    const name = prompt('Enter format name:');
    if (!name) return;

    const format = prompt('Enter Luxon format string:');
    if (!format) return;

    const listId = type === 'datetime' ? 'df-datetime-custom' : `df-${type}-custom`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    const sample = this._getSampleDateTime();
    const now = DateTime.now().toUTC();

    const item = document.createElement('div');
    item.className = 'df-format-item df-custom-item';
    item.innerHTML = `
      <input type="text" class="df-format-name-input" value="${this._escapeHtml(name)}" placeholder="Name">
      <input type="text" class="form-input df-format-input" value="${this._escapeHtml(format)}" placeholder="Luxon format">
      <span class="df-example">${this._safeFormat(sample, format)}</span>
      <span class="df-current" data-format="${this._escapeHtml(format)}">${this._safeFormat(now, format)}</span>
      <button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button>
    `;

    item.querySelector('.df-format-name-input')?.addEventListener('input', () => this.setDirty(true));
    item.querySelector('.df-format-input')?.addEventListener('input', () => {
      this._updatePreview(item.querySelector('.df-format-input'));
      this.setDirty(true);
    });
    item.querySelector('.df-delete-btn')?.addEventListener('click', () => {
      if (confirm('Delete this custom format?')) {
        item.remove();
        this.setDirty(true);
      }
    });

    list.appendChild(item);
    this.setDirty(true);
  }

  /**
   * Safe format helper
   */
  _safeFormat(dt, format) {
    try {
      return dt.toFormat(format);
    } catch (e) {
      return 'Invalid';
    }
  }

  /**
   * Escape HTML
   */
  _escapeHtml(str) {
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
  }

  /**
   * Load data
   */
  async loadData() {
    const section = this.getSectionData();
    const hasData = section && Object.keys(section).length > 1;

    const container = this.container;
    if (!container) return;

    const sampleInput = container.querySelector('#df-sample-input');
    if (sampleInput) {
      sampleInput.value = hasData && section.sample ? section.sample : DEFAULT_SAMPLE;
    }

    this._loadBuiltinFormats('dates', section.dates);
    this._loadBuiltinFormats('times', section.times);
    this._loadBuiltinFormats('datetime', section.datetime);

    this._loadCustomFormats('dates', section.dates?.custom);
    this._loadCustomFormats('times', section.times?.custom);
    this._loadCustomFormats('datetime', section.datetime?.custom);

    this._renderAllPreviews();
    this._renderCurrentPreviews();
  }

  /**
   * Load built-in formats
   */
  _loadBuiltinFormats(type, data) {
    const container = this.container;
    if (!container) return;

    const builtins = BUILTIN_FORMATS[type];
    if (!builtins) return;

    Object.entries(builtins).forEach(([key, config]) => {
      const input = container.querySelector(`[name="${type}_${key}"]`);
      if (input) {
        input.value = data?.[key] || config.format;
      }
    });
  }

  /**
   * Load custom formats
   */
  _loadCustomFormats(type, customData) {
    const container = this.container;
    if (!container) return;

    const listId = type === 'datetime' ? 'df-datetime-custom' : `df-${type}-custom`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    list.innerHTML = '';

    if (!customData) return;

    const sample = this._getSampleDateTime();
    const now = DateTime.now().toUTC();

    Object.entries(customData).forEach(([id, data]) => {
      if (typeof data !== 'object' || !data.format) return;

      const item = document.createElement('div');
      item.className = 'df-format-item df-custom-item';
      item.dataset.id = id;
      item.innerHTML = `
        <input type="text" class="df-format-name-input" value="${this._escapeHtml(data.name || id)}" placeholder="Name">
        <input type="text" class="form-input df-format-input" value="${this._escapeHtml(data.format)}" placeholder="Luxon format">
        <span class="df-example">${this._safeFormat(sample, data.format)}</span>
        <span class="df-current" data-format="${this._escapeHtml(data.format)}">${this._safeFormat(now, data.format)}</span>
        <button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button>
      `;

      item.querySelector('.df-format-name-input')?.addEventListener('input', () => this.setDirty(true));
      item.querySelector('.df-format-input')?.addEventListener('input', () => {
        this._updatePreview(item.querySelector('.df-format-input'));
        this.setDirty(true);
      });
      item.querySelector('.df-delete-btn')?.addEventListener('click', () => {
        if (confirm('Delete this custom format?')) {
          item.remove();
          this.setDirty(true);
        }
      });

      list.appendChild(item);
    });
  }

  /**
   * Save
   */
  async save() {
    const container = this.container;
    if (!container) {
      this.setDirty(false);
      return { success: true };
    }

    const sample = container.querySelector('#df-sample-input')?.value || DEFAULT_SAMPLE;

    const data = {
      sample,
      dates: {
        short: container.querySelector('[name="dates_short"]')?.value || BUILTIN_FORMATS.dates.short.format,
        long: container.querySelector('[name="dates_long"]')?.value || BUILTIN_FORMATS.dates.long.format,
        ...(this._gatherCustomFormats('dates').length > 0 ? { custom: this._gatherCustomFormats('dates')[0] } : {}),
      },
      times: {
        short: container.querySelector('[name="times_short"]')?.value || BUILTIN_FORMATS.times.short.format,
        medium: container.querySelector('[name="times_medium"]')?.value || BUILTIN_FORMATS.times.medium.format,
        long: container.querySelector('[name="times_long"]')?.value || BUILTIN_FORMATS.times.long.format,
        ...(this._gatherCustomFormats('times').length > 0 ? { custom: this._gatherCustomFormats('times')[0] } : {}),
      },
      datetime: {
        short: container.querySelector('[name="datetime_short"]')?.value || BUILTIN_FORMATS.datetime.short.format,
        long: container.querySelector('[name="datetime_long"]')?.value || BUILTIN_FORMATS.datetime.long.format,
        ...(this._gatherCustomFormats('datetime').length > 0 ? { custom: this._gatherCustomFormats('datetime')[0] } : {}),
      },
    };

    if (this._gatherCustomFormats('dates').length > 1) {
      data.dates.custom = this._gatherCustomFormats('dates');
    }
    if (this._gatherCustomFormats('times').length > 1) {
      data.times.custom = this._gatherCustomFormats('times');
    }
    if (this._gatherCustomFormats('datetime').length > 1) {
      data.datetime.custom = this._gatherCustomFormats('datetime');
    }

    this.setSectionData(data, 'Date Formats');
    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[DateFormatsPage] Saved formats');
    return { success: true, data };
  }

  /**
   * Gather custom formats
   */
  _gatherCustomFormats(type) {
    const container = this.container;
    const custom = {};

    const listId = type === 'datetime' ? 'df-datetime-custom' : `df-${type}-custom`;
    const list = container?.querySelector(`#${listId}`);
    if (!list) return custom;

    list.querySelectorAll('.df-custom-item').forEach(item => {
      const nameInput = item.querySelector('.df-format-name-input');
      const input = item.querySelector('.df-format-input');
      if (nameInput && input) {
        const id = item.dataset.id || `custom-${Date.now()}-${Math.random().toString(36).slice(2)}`;
        custom[id] = {
          name: nameInput.value,
          format: input.value,
        };
      }
    });

    return custom;
  }

  /**
   * Cancel
   */
  cancel() {
    this.loadData();
    this.setDirty(false);
  }

  /**
   * Check if dirty
   */
  isDirty() {
    const container = this.container;
    if (!container) return false;

    const sample = container.querySelector('#df-sample-input')?.value || '';
    const datesShort = container.querySelector('[name="dates_short"]')?.value || '';
    const datesLong = container.querySelector('[name="dates_long"]')?.value || '';
    const timesShort = container.querySelector('[name="times_short"]')?.value || '';
    const timesMedium = container.querySelector('[name="times_medium"]')?.value || '';
    const timesLong = container.querySelector('[name="times_long"]')?.value || '';
    const datetimeShort = container.querySelector('[name="datetime_short"]')?.value || '';
    const datetimeLong = container.querySelector('[name="datetime_long"]')?.value || '';

    const currentData = JSON.stringify({
      sample, datesShort, datesLong, timesShort, timesMedium, timesLong, datetimeShort, datetimeLong,
      datesCustom: this._gatherCustomFormats('dates'),
      timesCustom: this._gatherCustomFormats('times'),
      datetimeCustom: this._gatherCustomFormats('datetime'),
    });

    return currentData !== JSON.stringify(this._originalData);
  }

  /**
   * On show
   */
  onShow() {
    this._renderCurrentPreviews();
  }

  /**
   * Destroy
   */
  destroy() {
    if (this._currentUpdateInterval) {
      clearInterval(this._currentUpdateInterval);
      this._currentUpdateInterval = null;
    }
    super.destroy();
  }
}

export default DateFormatsPage;