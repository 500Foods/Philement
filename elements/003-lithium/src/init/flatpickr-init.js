// Flatpickr Initialization Module
// Handles Flatpickr date/time picker initialization and configuration

class FlatpickrInit {
  constructor() {
    this.instances = new Map(); // Store Flatpickr instances
    this.defaultOptions = {
      dateFormat: "Y-m-d",
      enableTime: false,
      altInput: true,
      altFormat: "F j, Y",
      allowInput: true,
      clickOpens: true,
      weekNumbers: false,
      showMonths: 1,
      minDate: null,
      maxDate: null,
      defaultDate: null,
      locale: "default",
      onChange: (selectedDates, dateStr, instance) => {
        console.log('Flatpickr date changed:', dateStr);
      },
      onOpen: (selectedDates, dateStr, instance) => {
        console.log('Flatpickr opened');
      },
      onClose: (selectedDates, dateStr, instance) => {
        console.log('Flatpickr closed');
      },
      onReady: (selectedDates, dateStr, instance) => {
        console.log('Flatpickr ready');
      }
    };
  }

  /**
   * Initialize a Flatpickr instance
   * @param {string} elementId - The DOM element ID to attach the picker to
   * @param {Object} options - Custom options to override defaults
   * @returns {Flatpickr} Flatpickr instance
   */
  initPicker(elementId, options = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Merge default options with custom options
      const mergedOptions = { ...this.defaultOptions, ...options };

      // Initialize Flatpickr
      const instance = flatpickr(element, mergedOptions);

      // Store reference
      this.instances.set(elementId, instance[0]); // flatpickr returns an array

      console.log(`Flatpickr initialized on ${elementId}`);
      return instance[0];
    } catch (error) {
      console.error(`Failed to initialize Flatpickr on ${elementId}:`, error);
      throw error;
    }
  }

  /**
   * Get a Flatpickr instance by element ID
   * @param {string} elementId - The DOM element ID
   * @returns {Flatpickr|null} Flatpickr instance or null if not found
   */
  getInstance(elementId) {
    return this.instances.get(elementId) || null;
  }

  /**
   * Destroy a Flatpickr instance
   * @param {string} elementId - The DOM element ID
   */
  destroyInstance(elementId) {
    const instance = this.getInstance(elementId);
    if (instance) {
      instance.destroy();
      this.instances.delete(elementId);
    }
  }

  /**
   * Destroy all Flatpickr instances
   */
  destroyAllInstances() {
    this.instances.forEach((instance, elementId) => {
      instance.destroy();
    });
    this.instances.clear();
  }

  /**
   * Set default options for all future pickers
   * @param {Object} options - Options to set as defaults
   */
  setDefaultOptions(options) {
    this.defaultOptions = { ...this.defaultOptions, ...options };
  }

  /**
   * Get current default options
   * @returns {Object} Current default options
   */
  getDefaultOptions() {
    return { ...this.defaultOptions };
  }

  /**
   * Initialize a date picker
   * @param {string} elementId - The DOM element ID
   * @param {Object} options - Custom options
   * @returns {Flatpickr} Flatpickr instance
   */
  initDatePicker(elementId, options = {}) {
    const dateOptions = {
      ...this.defaultOptions,
      ...options,
      enableTime: false,
      dateFormat: "Y-m-d",
      altFormat: "F j, Y"
    };

    return this.initPicker(elementId, dateOptions);
  }

  /**
   * Initialize a time picker
   * @param {string} elementId - The DOM element ID
   * @param {Object} options - Custom options
   * @returns {Flatpickr} Flatpickr instance
   */
  initTimePicker(elementId, options = {}) {
    const timeOptions = {
      ...this.defaultOptions,
      ...options,
      enableTime: true,
      noCalendar: true,
      dateFormat: "H:i",
      time_24hr: false
    };

    return this.initPicker(elementId, timeOptions);
  }

  /**
   * Initialize a datetime picker
   * @param {string} elementId - The DOM element ID
   * @param {Object} options - Custom options
   * @returns {Flatpickr} Flatpickr instance
   */
  initDateTimePicker(elementId, options = {}) {
    const datetimeOptions = {
      ...this.defaultOptions,
      ...options,
      enableTime: true,
      dateFormat: "Y-m-d H:i",
      altFormat: "F j, Y h:i K"
    };

    return this.initPicker(elementId, datetimeOptions);
  }

  /**
   * Initialize a range picker
   * @param {string} elementId - The DOM element ID
   * @param {Object} options - Custom options
   * @returns {Flatpickr} Flatpickr instance
   */
  initRangePicker(elementId, options = {}) {
    const rangeOptions = {
      ...this.defaultOptions,
      ...options,
      mode: "range",
      dateFormat: "Y-m-d",
      altFormat: "F j, Y"
    };

    return this.initPicker(elementId, rangeOptions);
  }

  /**
   * Initialize a multiple dates picker
   * @param {string} elementId - The DOM element ID
   * @param {Object} options - Custom options
   * @returns {Flatpickr} Flatpickr instance
   */
  initMultiplePicker(elementId, options = {}) {
    const multipleOptions = {
      ...this.defaultOptions,
      ...options,
      mode: "multiple",
      dateFormat: "Y-m-d",
      altFormat: "F j, Y"
    };

    return this.initPicker(elementId, multipleOptions);
  }

  /**
   * Get the selected date(s) from a picker
   * @param {string} elementId - The DOM element ID
   * @returns {Array|null} Array of selected dates or null if picker not found
   */
  getSelectedDates(elementId) {
    const instance = this.getInstance(elementId);
    return instance ? instance.selectedDates : null;
  }

  /**
   * Set the selected date(s) for a picker
   * @param {string} elementId - The DOM element ID
   * @param {Array|Date} dates - Date(s) to set
   * @returns {boolean} True if successful, false otherwise
   */
  setSelectedDates(elementId, dates) {
    const instance = this.getInstance(elementId);
    if (instance) {
      try {
        instance.setDate(dates);
        return true;
      } catch (error) {
        console.error(`Failed to set dates for picker ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Clear the selected dates for a picker
   * @param {string} elementId - The DOM element ID
   * @returns {boolean} True if successful, false otherwise
   */
  clearSelectedDates(elementId) {
    const instance = this.getInstance(elementId);
    if (instance) {
      try {
        instance.clear();
        return true;
      } catch (error) {
        console.error(`Failed to clear dates for picker ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Configure localization for a picker
   * @param {string} elementId - The DOM element ID
   * @param {Object} locale - Locale configuration
   * @returns {boolean} True if successful, false otherwise
   */
  setLocale(elementId, locale) {
    const instance = this.getInstance(elementId);
    if (instance) {
      try {
        instance.set("locale", locale);
        return true;
      } catch (error) {
        console.error(`Failed to set locale for picker ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Apply custom themes to a picker
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name or CSS class
   */
  applyTheme(elementId, theme) {
    const instance = this.getInstance(elementId);
    if (instance) {
      const element = instance.element;
      element.classList.remove('flatpickr-theme-default');
      element.classList.add(`flatpickr-theme-${theme}`);
    }
  }
}

// Export singleton instance
export const flatpickrInit = new FlatpickrInit();