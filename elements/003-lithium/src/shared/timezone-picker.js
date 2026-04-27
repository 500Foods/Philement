/**
 * TimezonePicker Component
 *
 * A resizable, searchable timezone dropdown picker with grouped display.
 * Used by the Date Formats page for timezone selection.
 */

import { DateTime } from 'luxon';
import { log, Subsystems, Status } from '../core/log.js';
import { scrollbarManager } from '../core/scrollbar-manager.js';

/**
 * Common timezone abbreviations mapped to IANA identifiers and full names
 */
const TIMEZONE_ABBREVIATIONS = {
  // North America
  'PST': { iana: 'America/Los_Angeles', fullName: 'Pacific Standard Time' },
  'PDT': { iana: 'America/Los_Angeles', fullName: 'Pacific Daylight Time' },
  'MST': { iana: 'America/Denver', fullName: 'Mountain Standard Time' },
  'MDT': { iana: 'America/Denver', fullName: 'Mountain Daylight Time' },
  'CST': { iana: 'America/Chicago', fullName: 'Central Standard Time' },
  'CDT': { iana: 'America/Chicago', fullName: 'Central Daylight Time' },
  'EST': { iana: 'America/New_York', fullName: 'Eastern Standard Time' },
  'EDT': { iana: 'America/New_York', fullName: 'Eastern Daylight Time' },
  'AKST': { iana: 'America/Anchorage', fullName: 'Alaska Standard Time' },
  'AKDT': { iana: 'America/Anchorage', fullName: 'Alaska Daylight Time' },
  'HST': { iana: 'Pacific/Honolulu', fullName: 'Hawaii Standard Time' },

  // Europe
  'GMT': { iana: 'Europe/London', fullName: 'Greenwich Mean Time' },
  'BST': { iana: 'Europe/London', fullName: 'British Summer Time' },
  'CET': { iana: 'Europe/Paris', fullName: 'Central European Time' },
  'CEST': { iana: 'Europe/Paris', fullName: 'Central European Summer Time' },

  // Asia
  'IST': { iana: 'Asia/Kolkata', fullName: 'India Standard Time' },
  'JST': { iana: 'Asia/Tokyo', fullName: 'Japan Standard Time' },
  'KST': { iana: 'Asia/Seoul', fullName: 'Korea Standard Time' },
  'CST_ASIA': { iana: 'Asia/Shanghai', fullName: 'China Standard Time' }, // Note: CST conflicts with US Central

  // Oceania
  'AEST': { iana: 'Australia/Sydney', fullName: 'Australian Eastern Standard Time' },
  'AEDT': { iana: 'Australia/Sydney', fullName: 'Australian Eastern Daylight Time' },
  'ACST': { iana: 'Australia/Adelaide', fullName: 'Australian Central Standard Time' },
  'ACDT': { iana: 'Australia/Adelaide', fullName: 'Australian Central Daylight Time' },
  'AWST': { iana: 'Australia/Perth', fullName: 'Australian Western Standard Time' },

  // UTC
  'UTC': { iana: 'UTC', fullName: 'Coordinated Universal Time' },
};

/**
 * Map IANA identifiers to user-friendly display names for abbreviated timezones
 */
const DISPLAY_NAME_MAP = {
  // UTC and GMT
  'UTC': 'Coordinated Universal Time',
  'GMT': 'Greenwich Mean Time',

  // US Timezones
  'America/Los_Angeles': 'Pacific Time',
  'America/Denver': 'Mountain Time',
  'America/Chicago': 'Central Time',
  'America/New_York': 'Eastern Time',
  'America/Anchorage': 'Alaska Time',
  'Pacific/Honolulu': 'Hawaii Time',

  // European Timezones
  'Europe/Paris': 'Central European Time',
  'Europe/London': 'British Time',
  'Europe/Berlin': 'Central European Time',
  'Europe/Rome': 'Central European Time',
  'Europe/Madrid': 'Central European Time',

  // Asian Timezones
  'Asia/Kolkata': 'India Time',
  'Asia/Tokyo': 'Japan Time',
  'Asia/Shanghai': 'China Time',
  'Asia/Seoul': 'Korea Time',
  'Asia/Bangkok': 'Indochina Time',
  'Asia/Manila': 'Philippine Time',
  'Asia/Singapore': 'Singapore Time',

  // Australian Timezones
  'Australia/Sydney': 'Australian Eastern Time',
  'Australia/Adelaide': 'Australian Central Time',
  'Australia/Perth': 'Australian Western Time',

  // Other major cities
  'Pacific/Auckland': 'New Zealand Time',
  'Africa/Johannesburg': 'South Africa Time',
  'Europe/Moscow': 'Moscow Time',
  'Asia/Yekaterinburg': 'Yekaterinburg Time'
};

export class TimezonePicker {
  constructor(container, timezones, onSelect, onPositionChange = null) {
    this.container = container;
    this.timezones = timezones;
    this.onSelect = onSelect;
    this.onPositionChange = onPositionChange;
    this.selectedTimezone = 'local';
    this.isOpen = false;
    this.popupWidth = null;
    this.popupHeight = null;
    this.isResizing = false;
    this._closeTimeout = null;
    this._outsideClickHandler = null;
    this._escKeyHandler = null;
    this._transitioning = false;

    // Create dropdown element
    this.dropdown = document.createElement('div');
    this.dropdown.className = 'df-timezone-dropdown manager-ui-popup';
    this.dropdown.innerHTML = `
      <div class="df-timezone-filter">
        <input type="text" class="df-timezone-filter-input" placeholder="Search timezones...">
        <button type="button" class="df-timezone-filter-clear" title="Clear">&times;</button>
      </div>
      <div class="df-timezone-list"></div>
      <div class="df-timezone-resize-handle"></div>
    `;

    // Cache DOM references
    this.filterInput = this.dropdown.querySelector('.df-timezone-filter-input');
    this.listContainer = this.dropdown.querySelector('.df-timezone-list');
    this.clearButton = this.dropdown.querySelector('.df-timezone-filter-clear');

    // Setup event listeners
    this.container.addEventListener('click', () => this.toggle());
    this.filterInput.addEventListener('input', () => this.filterTimezones());
    this.clearButton.addEventListener('click', () => {
      this.filterInput.value = '';
      this.filterTimezones();
      this.filterInput.focus();
    });

    // Setup resize functionality
    this.setupResize();

    // OverlayScrollbars will be initialized in open() when element is visible

    // Hide dropdown initially
    this.dropdown.style.display = 'none';
    document.body.appendChild(this.dropdown);

    // Set initial display
    this._updateDisplay();

    // Listen for close-all-popups
    document.addEventListener('close-all-popups', () => this.close());
  }

  /**
   * Get the browser timezone IANA name
   */
  _getBrowserTimezone() {
    try {
      return Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC';
    } catch (_e) {
      return 'UTC';
    }
  }

  /**
   * Setup resize functionality
   */
  setupResize() {
    const resizeHandle = this.dropdown.querySelector('.df-timezone-resize-handle');
    if (!resizeHandle) return;

    let startX, startY, startWidth, startHeight;

    const startResize = (e) => {
      this.isResizing = true;
      startX = e.clientX;
      startY = e.clientY;
      startWidth = this.listContainerWrapper.offsetWidth;
      startHeight = this.listContainerWrapper.offsetHeight;

      document.body.style.cursor = 'nw-resize';
      document.body.style.userSelect = 'none';

      document.addEventListener('mousemove', resize);
      document.addEventListener('mouseup', stopResize);
      e.preventDefault();
    };

    const resize = (e) => {
      if (!this.isResizing) return;

      const newWidth = Math.max(250, startWidth + (e.clientX - startX));
      const newHeight = Math.max(200, startHeight + (e.clientY - startY));

      this.dropdown.style.width = `${newWidth}px`;
      this.dropdown.style.height = `${newHeight}px`;

      // Store the new size
      this.popupWidth = newWidth;
      this.popupHeight = newHeight;

      // Notify position change
      if (this.onPositionChange) {
        this.onPositionChange({ width: newWidth, height: newHeight });
      }
    };

    const stopResize = () => {
      this.isResizing = false;
      document.body.style.cursor = '';
      document.body.style.userSelect = '';

      document.removeEventListener('mousemove', resize);
      document.removeEventListener('mouseup', stopResize);
    };

    resizeHandle.addEventListener('mousedown', startResize);
  }

  toggle() {
    // Prevent toggle if we're in a transition state
    if (this._transitioning) return;

    if (this.isOpen) {
      this.close();
    } else {
      this.open();
    }
  }

  open() {
    if (this.isOpen || this._transitioning) return;

    this._transitioning = true;
    log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] Opening popup');

    // Cancel any pending close timeout (popup is being reopened)
    if (this._closeTimeout) {
      clearTimeout(this._closeTimeout);
      this._closeTimeout = null;
      log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] Cancelled pending close timeout');
    }

    // Recreate dropdown element if it was removed from DOM
    if (!this.dropdown || !this.dropdown.parentNode) {
      this.dropdown = document.createElement('div');
      this.dropdown.className = 'df-timezone-dropdown manager-ui-popup';
      this.dropdown.innerHTML = `
        <div class="df-timezone-filter">
          <input type="text" class="df-timezone-filter-input" placeholder="Search timezones...">
          <button type="button" class="df-timezone-filter-clear" title="Clear">&times;</button>
        </div>
        <div class="df-timezone-list"></div>
        <div class="df-timezone-resize-handle"></div>
      `;

      // Cache DOM references
      this.filterInput = this.dropdown.querySelector('.df-timezone-filter-input');
      this.listContainer = this.dropdown.querySelector('.df-timezone-list');
      this.clearButton = this.dropdown.querySelector('.df-timezone-filter-clear');

      // Setup event listeners
      this.container.addEventListener('click', () => this.toggle());
      this.filterInput.addEventListener('input', () => this.filterTimezones());
      this.clearButton.addEventListener('click', () => {
        this.filterInput.value = '';
        this.filterTimezones();
        this.filterInput.focus();
      });

      // Setup resize functionality
      this.setupResize();

      document.body.appendChild(this.dropdown);
      log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] Recreated dropdown element');
    }

    // Close other popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    this.isOpen = true;
    this.container.classList.add('open');

    // Position dropdown first
    const rect = this.container.getBoundingClientRect();
    this.dropdown.style.top = `${rect.bottom + 2}px`;
    this.dropdown.style.left = `${rect.left}px`;
    this.dropdown.style.width = this.popupWidth ? `${this.popupWidth}px` : `${rect.width}px`;
    this.dropdown.style.height = this.popupHeight ? `${this.popupHeight}px` : '';

    // Force show dropdown
    this.dropdown.style.display = 'flex';

    // Populate the timezone list initially
    this.filterTimezones();

    // Initialize OSB after content is populated
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        if (this.listContainer) {
          this.overlayScrollbar = scrollbarManager.initPopup(this.listContainer);
          log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] OSB initialized on list container');
        }
      });
    });

    // Setup outside click handler and ESC key support (similar to Flatpickr pattern)
    this._outsideClickHandler = (e) => {
      // Prevent handling if we're transitioning or if click is inside dropdown/container
      if (this._transitioning || !this.dropdown || !this.container) return;
      if (this.dropdown.contains(e.target) || this.container.contains(e.target)) return;

      log(Subsystems.UI, Status.DEBUG, '[TimezonePicker] Outside click detected, closing');
      this.close();
    };

    this._escKeyHandler = (e) => {
      if (e.key === 'Escape' && this.isOpen && !this._transitioning) {
        log(Subsystems.UI, Status.DEBUG, '[TimezonePicker] ESC key detected, closing');
        this.close();
      }
    };

    // Use setTimeout to attach listeners after current click event finishes bubbling
    setTimeout(() => {
      if (!this._outsideClickHandler || !this._escKeyHandler) return;
      document.addEventListener('click', this._outsideClickHandler);
      document.addEventListener('keydown', this._escKeyHandler);
      this._transitioning = false; // Clear transition flag after handlers are set up
    }, 0);

    // Show with animation
    setTimeout(() => this.dropdown.classList.add('visible'), 50);
  }

  close() {
    if (!this.isOpen || this._transitioning) return;

    this._transitioning = true;
    this.isOpen = false;
    this.container.classList.remove('open');
    this.dropdown.classList.remove('visible');

    // Cancel any pending close timeout
    if (this._closeTimeout) {
      clearTimeout(this._closeTimeout);
      this._closeTimeout = null;
    }

    // Remove outside click handler and ESC key handler immediately
    if (this._outsideClickHandler) {
      document.removeEventListener('click', this._outsideClickHandler);
      this._outsideClickHandler = null;
    }
    if (this._escKeyHandler) {
      document.removeEventListener('keydown', this._escKeyHandler);
      this._escKeyHandler = null;
    }

    // Wait for animation to complete before removing from DOM
    this._closeTimeout = setTimeout(() => {
      // Clean up OSB instance
      if (this.overlayScrollbar) {
        log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.close] Destroying OSB instance');
        scrollbarManager.destroy(this.overlayScrollbar);
        this.overlayScrollbar = null;
      }
      // Remove dropdown from DOM completely
      if (this.dropdown && this.dropdown.parentNode) {
        this.dropdown.parentNode.removeChild(this.dropdown);
        log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.close] Removed dropdown from DOM');
      }
      this._transitioning = false; // Clear transition flag after cleanup
      this._closeTimeout = null;
    }, 350); // Match transition duration
  }

  setTimezone(timezone) {
    this.selectedTimezone = timezone;
    this._updateDisplay();
  }

  /**
   * Update the display text based on selected timezone
   */
  _updateDisplay() {
    const displayEl = this.container.querySelector('#df-selected-timezone');
    if (!displayEl) return;

    if (this.selectedTimezone === 'local') {
      const browserTz = this._getBrowserTimezone();
      displayEl.textContent = `Browser Timezone (${browserTz})`;
    } else {
      try {
        const now = DateTime.now().setZone(this.selectedTimezone);
        const abbr = now.toFormat('ZZZZ');
        displayEl.textContent = `${this.selectedTimezone} (${abbr})`;
      } catch (_e) {
        displayEl.textContent = this.selectedTimezone;
      }
    }
  }

  /**
   * Filter and display timezones based on current filter input
   */
  filterTimezones() {
    const filter = this.filterInput.value.toLowerCase();

    log(Subsystems.UI, Status.DEBUG, `[filterTimezones] Called, filter: "${filter}"`);

    // Get the correct target container
    // After OSB initialization on dropdown, content is inside .os-content
    let targetContainer = this.listContainer;
    if (this.overlayScrollbar && !this.overlayScrollbar.destroyed) {
      const elements = this.overlayScrollbar.elements();
      if (elements && elements.content) {
        targetContainer = elements.content;
        log(Subsystems.UI, Status.DEBUG, `[filterTimezones] Using OSB content element`);
      } else {
        log(Subsystems.UI, Status.WARN, `[filterTimezones] OSB instance exists but no content element found`);
      }
    } else {
      log(Subsystems.UI, Status.DEBUG, `[filterTimezones] Using listContainer directly, hasOSB: ${!!this.overlayScrollbar}`);
    }

    log(Subsystems.UI, Status.DEBUG, `[filterTimezones] targetContainer classes: ${targetContainer.className}`);

    // Clear existing items
    targetContainer.innerHTML = '';

    // Group timezones by region
    const grouped = {};
    const abbreviations = {};

    this.timezones.forEach(tz => {
      const parts = tz.split('/');
      const region = parts[0];

      // Handle abbreviations (like UTC, GMT)
      if (parts.length === 1) {
        abbreviations[tz] = tz;
        return;
      }

      if (!grouped[region]) {
        grouped[region] = [];
      }
      grouped[region].push(tz);
    });

    // Sort regions
    const sortedRegions = Object.keys(grouped).sort();

    // Create browser timezone section first
    const browserSection = document.createElement('div');
    browserSection.className = 'df-timezone-group';
    browserSection.innerHTML = '<div class="df-timezone-group-header">Current</div>';

    if ('Browser Timezone'.toLowerCase().includes(filter)) {
      const item = this._createTimezoneItem('Browser Timezone', 'local');
      browserSection.appendChild(item);
    }

    if (browserSection.children.length > 1) { // Has title + at least one item
      targetContainer.appendChild(browserSection);
    }

    // Create abbreviated timezones section
    const abbreviatedSection = document.createElement('div');
    abbreviatedSection.className = 'df-timezone-group';
    abbreviatedSection.innerHTML = '<div class="df-timezone-group-header">Abbreviated Timezones</div>';

    Object.keys(TIMEZONE_ABBREVIATIONS).sort().forEach(abbrev => {
      if (abbrev.toLowerCase().includes(filter) ||
          TIMEZONE_ABBREVIATIONS[abbrev].fullName.toLowerCase().includes(filter)) {
        const mapping = TIMEZONE_ABBREVIATIONS[abbrev];
        const displayName = `${abbrev} (${mapping.fullName})`;
        const item = this._createTimezoneItem(displayName, mapping.iana);
        abbreviatedSection.appendChild(item);
      }
    });

    if (abbreviatedSection.children.length > 1) { // Has title + at least one item
      targetContainer.appendChild(abbreviatedSection);
    }

    // Create major timezones section
    if (Object.keys(abbreviations).length > 0) {
      const majorSection = document.createElement('div');
      majorSection.className = 'df-timezone-group';
      majorSection.innerHTML = '<div class="df-timezone-group-header">Major Timezones</div>';

      Object.keys(abbreviations).sort().forEach(tz => {
        if (tz.toLowerCase().includes(filter)) {
          const friendlyName = DISPLAY_NAME_MAP[tz] || tz.split('/').pop().replace(/_/g, ' ');
          const displayName = `${friendlyName}`;
          const item = this._createTimezoneItem(displayName, tz);
          majorSection.appendChild(item);
        }
      });

      if (majorSection.children.length > 1) { // Has title + at least one item
        targetContainer.appendChild(majorSection);
      }
    }

    // Create region sections
    sortedRegions.forEach(region => {
      const timezones = grouped[region];
      const section = document.createElement('div');
      section.className = 'df-timezone-group';
      section.innerHTML = `<div class="df-timezone-group-header">${region}</div>`;

      timezones.forEach(tz => {
        if (tz.toLowerCase().includes(filter)) {
          const item = this._createTimezoneItem(tz, tz);
          section.appendChild(item);
        }
      });

      if (section.children.length > 1) { // Has title + at least one item
        targetContainer.appendChild(section);
      }
    });
  }

  /**
   * Create a timezone list item
   */
  _createTimezoneItem(displayName, value) {
    const item = document.createElement('div');
    item.className = 'df-timezone-item';
    item.dataset.value = value;

    const tzName = typeof this.selectedTimezone === 'object' ? this.selectedTimezone.name : this.selectedTimezone;
    if (value === tzName) {
      item.classList.add('selected');
    }

    // Get current time in this timezone
    const timeString = (() => {
      try {
        const now = DateTime.now().setZone(value);
        return now.toFormat('HH:mm');
      } catch (_e) {
        return '--:--';
      }
    })();

    item.innerHTML = `
      <span class="df-timezone-name">${displayName}</span>
      <span class="df-timezone-time">${timeString}</span>
    `;

    item.addEventListener('click', () => {
      this.selectedTimezone = value;
      this.onSelect(value);
      this.close();
    });

    return item;
  }

  destroy() {
    // Restore page scrolling in case popup was open during destroy
    document.body.style.overflow = '';
    if (this.overlay) {
      this.overlay.remove();
    }
    if (this.dropdown) {
      this.dropdown.remove();
    }
    if (this.overlayScrollbar) {
      this.overlayScrollbar.destroy();
    }
  }
}