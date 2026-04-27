/**
 * FlatpickrPicker Component
 *
 * A reusable Flatpickr date/time picker component with popup positioning,
 * animations, and proper event handling for Lithium applications.
 *
 * Features:
 * - Popup positioning like header-dropdowns
 * - Scale animation matching Lithium popup system
 * - Outside click and ESC key handling
 * - Integration with close-all-popups event
 * - Proper cleanup and state management
 */

import flatpickr from 'flatpickr';
import { DateTime } from 'luxon';
import { log, Subsystems, Status } from '../core/log.js';

export class FlatpickrPicker {
  constructor(options = {}) {
    this.options = {
      enableTime: true,
      enableSeconds: true,
      dateFormat: 'Y-m-d\\TH:i:S',
      time_24hr: true,
      allowInput: true,
      clickOpens: false,
      disableMobile: true,
      ...options,
    };

    this.triggerButton = options.triggerButton || null;
    this.inputElement = options.inputElement || null;
    this.onChange = options.onChange || (() => {});
    this.onOpen = options.onOpen || (() => {});
    this.onClose = options.onClose || (() => {});

    // Internal state
    this.flatpickrInstance = null;
    this.isOpen = false;
    this.isTransitioning = false;
    this.popupWrapper = null;
    this.calendarElement = null;
    this.closeTimeout = null;

    // Event handlers
    this.outsideClickHandler = null;
    this.escKeyHandler = null;
    this.closeAllPopupsHandler = null;
  }

  /**
   * Initialize the picker with a trigger button and input element
   */
  init(triggerButton, inputElement) {
    this.triggerButton = triggerButton;
    this.inputElement = inputElement;

    if (!this.triggerButton || !this.inputElement) {
      log(Subsystems.UI, Status.ERROR, '[FlatpickrPicker] Missing trigger button or input element');
      return false;
    }

    // Set up click handler on trigger button
    this.triggerButton.addEventListener('click', () => this.toggle());

    return true;
  }

  /**
   * Toggle the picker open/closed
   */
  toggle() {
    if (this.isTransitioning) return;

    if (this.isOpen && this.popupWrapper) {
      this.close();
    } else {
      this.open();
    }
  }

  /**
   * Open the picker
   */
  open() {
    if (this.isOpen || this.isTransitioning || !this.triggerButton || !this.inputElement) return;

    this.isTransitioning = true;
    log(Subsystems.UI, Status.DEBUG, '[FlatpickrPicker] Opening picker');

    // Cancel any pending close timeout
    if (this.closeTimeout) {
      clearTimeout(this.closeTimeout);
      this.closeTimeout = null;
    }

    // Close other popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    // Create popup wrapper
    this.popupWrapper = document.createElement('div');
    this.popupWrapper.className = 'manager-ui-popup manager-header-popup flatpickr-popup-wrapper';
    this.popupWrapper.style.minWidth = '313px';
    this.popupWrapper.style.minHeight = '341px';

    this.isOpen = true;

    // Create Flatpickr instance
    this.flatpickrInstance = this._createFlatpickrInstance();

    // Position wrapper like a header-dropdown popup
    const btnRect = this.triggerButton.getBoundingClientRect();
    this.popupWrapper.style.top = `${btnRect.bottom + 1}px`;
    this.popupWrapper.style.right = `${window.innerWidth - btnRect.right}px`;
    this.popupWrapper.style.left = 'auto';
    this.popupWrapper.style.bottom = 'auto';

    document.body.appendChild(this.popupWrapper);

    // Trigger scale animation and open Flatpickr
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        this.popupWrapper.classList.add('visible');
        // Open Flatpickr after animation starts
        setTimeout(() => {
          this.flatpickrInstance.open();
        }, 50);
      });
    });

    // Clear transitioning after animation completes
    setTimeout(() => {
      this.isTransitioning = false;
    }, 350);

    // Set up event handlers
    this._setupEventHandlers();

    // Call onOpen callback
    this.onOpen();
  }

  /**
   * Close the picker
   */
  close() {
    if (!this.isOpen || this.isTransitioning) return;

    this.isTransitioning = true;

    log(Subsystems.UI, Status.DEBUG, '[FlatpickrPicker] Closing picker');

    // Remove visible class for animation
    if (this.popupWrapper && this.popupWrapper.parentNode) {
      this.popupWrapper.classList.remove('visible');
    }

    // Remove event listeners
    this._removeEventHandlers();

    // Wait for animation to complete before cleanup
    this.closeTimeout = setTimeout(() => {
      this._cleanup();
      this.isTransitioning = false;
      this.isOpen = false;

      // Call onClose callback
      this.onClose();
    }, 350);
  }

  /**
   * Set the date programmatically without triggering onChange
   */
  setDate(date, triggerChange = false) {
    if (this.flatpickrInstance && date) {
      this.flatpickrInstance.setDate(date, triggerChange);
    }
  }

  /**
   * Get the current selected date
   */
  getDate() {
    return this.flatpickrInstance ? this.flatpickrInstance.selectedDates[0] : null;
  }

  /**
   * Check if picker is currently open
   */
  getIsOpen() {
    return this.isOpen;
  }

  /**
   * Destroy the picker and clean up all resources
   */
  destroy() {
    this.close();

    // Additional cleanup for complete destruction
    if (this.flatpickrInstance) {
      this.flatpickrInstance.destroy();
      this.flatpickrInstance = null;
    }

    // Remove popup wrapper if it still exists
    const existing = document.querySelector('.flatpickr-popup-wrapper');
    if (existing) {
      existing.remove();
    }

    // Clear references
    this.popupWrapper = null;
    this.calendarElement = null;
    this.triggerButton = null;
    this.inputElement = null;
  }

  /**
   * Create the Flatpickr instance with custom configuration
   * @private
   */
  _createFlatpickrInstance() {
    // Parse current input value for defaultDate
    let defaultDate = null;
    try {
      const parsed = DateTime.fromISO(this.inputElement.value);
      if (parsed.isValid) defaultDate = parsed.toJSDate();
    } catch (_e) {
      // Use default from options or null
    }

    const config = {
      ...this.options,
      defaultDate,
      onChange: (selectedDates) => {
        if (selectedDates.length > 0) {
          const dt = DateTime.fromJSDate(selectedDates[0]);
          this.inputElement.value = dt.toFormat("yyyy-MM-dd'T'HH:mm:ss");
          this.onChange(selectedDates[0], dt.toFormat("yyyy-MM-dd'T'HH:mm:ss"));
        }
      },
      onOpen: () => {
        // Move the calendar to our wrapper when it opens
        this._moveCalendarToWrapper();
      },
    };

    return flatpickr(this.inputElement, config);
  }

  /**
   * Move the Flatpickr calendar to our popup wrapper
   * @private
   */
  _moveCalendarToWrapper() {
    const moveCalendar = () => {
      // Find all calendars and take the last one (most recently created)
      const calendars = document.querySelectorAll('.flatpickr-calendar');
      const calendar = calendars[calendars.length - 1];

      if (calendar && !this.popupWrapper.contains(calendar)) {
        calendar.style.position = 'absolute';
        calendar.style.top = '0';
        calendar.style.left = '0';
        calendar.style.display = 'block';
        calendar.style.visibility = 'visible';
        calendar.style.opacity = '1';
        // Add CSS override to prevent Flatpickr from hiding it
        calendar.style.setProperty('display', 'block', 'important');
        calendar.style.setProperty('visibility', 'visible', 'important');
        this.popupWrapper.appendChild(calendar);
        // Store reference to the calendar for cleanup
        this.calendarElement = calendar;
      } else if (!calendar) {
        // If calendar not found yet, try again
        setTimeout(moveCalendar, 10);
      }
    };
    moveCalendar();
  }

  /**
   * Set up event handlers for outside clicks, ESC key, etc.
   * @private
   */
  _setupEventHandlers() {
    // Remove any existing handlers
    this._removeEventHandlers();

    // Outside click handler
    this.outsideClickHandler = (e) => {
      if (!this.popupWrapper.contains(e.target) && !this.triggerButton.contains(e.target)) {
        if (this.isTransitioning) return;
        this.close();
      }
    };

    // ESC key handler
    this.escKeyHandler = (e) => {
      if (e.key === 'Escape') {
        if (this.isTransitioning) return;
        this.close();
      }
    };

    // close-all-popups handler
    this.closeAllPopupsHandler = () => this.close();

    // Use setTimeout to attach listeners after current click event finishes bubbling
    setTimeout(() => {
      document.addEventListener('click', this.outsideClickHandler);
      document.addEventListener('keydown', this.escKeyHandler);
      document.addEventListener('close-all-popups', this.closeAllPopupsHandler);
    }, 0);
  }

  /**
   * Remove event handlers
   * @private
   */
  _removeEventHandlers() {
    if (this.outsideClickHandler) {
      document.removeEventListener('click', this.outsideClickHandler);
      this.outsideClickHandler = null;
    }
    if (this.escKeyHandler) {
      document.removeEventListener('keydown', this.escKeyHandler);
      this.escKeyHandler = null;
    }
    if (this.closeAllPopupsHandler) {
      document.removeEventListener('close-all-popups', this.closeAllPopupsHandler);
      this.closeAllPopupsHandler = null;
    }
  }

  /**
   * Clean up resources after closing
   * @private
   */
  _cleanup() {
    if (this.flatpickrInstance) {
      this.flatpickrInstance.destroy();
      this.flatpickrInstance = null;
    }

    // Remove the calendar if we have a reference to it
    if (this.calendarElement && this.calendarElement.parentNode) {
      this.calendarElement.remove();
      this.calendarElement = null;
    }

    // Remove popup wrapper
    if (this.popupWrapper && this.popupWrapper.parentNode) {
      this.popupWrapper.remove();
    }

    this.popupWrapper = null;
  }
}