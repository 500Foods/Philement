/**
 * Utils - Formatting and utility functions
 *
 * Provides date, time, and number formatting using Intl.* APIs.
 * Reads user preferences from localStorage or uses sensible defaults.
 */

import { getClaims } from './jwt.js';

const PREFS_KEY = 'lithium_preferences';

// Default formatting preferences
const DEFAULT_PREFS = {
  lang: 'en-US',
  dateFormat: 'medium', // 'short' | 'medium' | 'long' | 'full'
  timeFormat: '24h',    // '12h' | '24h'
  numberFormat: {
    decimalSeparator: '.',
    groupingSeparator: ',',
    currency: 'USD',
  },
};

/**
 * Get user preferences from storage or JWT
 * @returns {Object} User preferences
 */
export function getPreferences() {
  // Try localStorage first
  try {
    const stored = localStorage.getItem(PREFS_KEY);
    if (stored) {
      return { ...DEFAULT_PREFS, ...JSON.parse(stored) };
    }
  } catch (e) {
    console.warn('Failed to parse preferences from localStorage');
  }

  // Try JWT claims
  const token = localStorage.getItem('lithium_jwt');
  if (token) {
    const claims = getClaims(token);
    if (claims?.lang) {
      return { ...DEFAULT_PREFS, lang: claims.lang };
    }
  }

  return DEFAULT_PREFS;
}

/**
 * Save user preferences to localStorage
 * @param {Object} prefs - Preferences to save
 */
export function savePreferences(prefs) {
  try {
    const current = getPreferences();
    const updated = { ...current, ...prefs };
    localStorage.setItem(PREFS_KEY, JSON.stringify(updated));
  } catch (e) {
    console.error('Failed to save preferences:', e);
  }
}

/**
 * Format a date using user preferences
 * @param {Date|string|number} date - Date to format
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted date string
 */
export function formatDate(date, options = {}) {
  const prefs = getPreferences();
  const d = date instanceof Date ? date : new Date(date);

  if (isNaN(d.getTime())) {
    return 'Invalid date';
  }

  const formatOptions = {
    dateStyle: options.dateStyle || prefs.dateFormat,
    ...options,
  };

  return new Intl.DateTimeFormat(prefs.lang, formatOptions).format(d);
}

/**
 * Format a time using user preferences
 * @param {Date|string|number} date - Date/time to format
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted time string
 */
export function formatTime(date, options = {}) {
  const prefs = getPreferences();
  const d = date instanceof Date ? date : new Date(date);

  if (isNaN(d.getTime())) {
    return 'Invalid time';
  }

  const formatOptions = {
    timeStyle: options.timeStyle || 'short',
    hour12: options.hour12 ?? prefs.timeFormat === '12h',
    ...options,
  };

  return new Intl.DateTimeFormat(prefs.lang, formatOptions).format(d);
}

/**
 * Format a date and time using user preferences
 * @param {Date|string|number} date - Date/time to format
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted datetime string
 */
export function formatDateTime(date, options = {}) {
  const prefs = getPreferences();
  const d = date instanceof Date ? date : new Date(date);

  if (isNaN(d.getTime())) {
    return 'Invalid datetime';
  }

  const formatOptions = {
    dateStyle: options.dateStyle || prefs.dateFormat,
    timeStyle: options.timeStyle || 'short',
    hour12: options.hour12 ?? prefs.timeFormat === '12h',
    ...options,
  };

  return new Intl.DateTimeFormat(prefs.lang, formatOptions).format(d);
}

/**
 * Format a number using user preferences
 * @param {number} number - Number to format
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted number string
 */
export function formatNumber(number, options = {}) {
  const prefs = getPreferences();

  if (typeof number !== 'number' || isNaN(number)) {
    return 'Invalid number';
  }

  const formatOptions = {
    minimumFractionDigits: options.minimumFractionDigits ?? 0,
    maximumFractionDigits: options.maximumFractionDigits ?? 2,
    ...options,
  };

  return new Intl.NumberFormat(prefs.lang, formatOptions).format(number);
}

/**
 * Format a currency amount using user preferences
 * @param {number} amount - Amount to format
 * @param {string} currency - Currency code (e.g., 'USD', 'EUR')
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted currency string
 */
export function formatCurrency(amount, currency, options = {}) {
  const prefs = getPreferences();
  const curr = currency || prefs.numberFormat?.currency || 'USD';

  if (typeof amount !== 'number' || isNaN(amount)) {
    return 'Invalid amount';
  }

  const formatOptions = {
    style: 'currency',
    currency: curr,
    ...options,
  };

  return new Intl.NumberFormat(prefs.lang, formatOptions).format(amount);
}

/**
 * Format a percentage using user preferences
 * @param {number} value - Value to format (0.15 for 15%)
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted percentage string
 */
export function formatPercent(value, options = {}) {
  const prefs = getPreferences();

  if (typeof value !== 'number' || isNaN(value)) {
    return 'Invalid percentage';
  }

  const formatOptions = {
    style: 'percent',
    minimumFractionDigits: options.minimumFractionDigits ?? 0,
    maximumFractionDigits: options.maximumFractionDigits ?? 0,
    ...options,
  };

  return new Intl.NumberFormat(prefs.lang, formatOptions).format(value);
}

/**
 * Format a relative time (e.g., "2 hours ago")
 * @param {Date|string|number} date - Date to format relative to now
 * @param {Object} options - Optional formatting overrides
 * @returns {string} Formatted relative time string
 */
export function formatRelativeTime(date, options = {}) {
  const prefs = getPreferences();
  const d = date instanceof Date ? date : new Date(date);

  if (isNaN(d.getTime())) {
    return 'Invalid date';
  }

  const now = new Date();
  const diffInSeconds = Math.floor((d.getTime() - now.getTime()) / 1000);

  // Use Intl.RelativeTimeFormat if available
  if (typeof Intl.RelativeTimeFormat !== 'undefined') {
    const rtf = new Intl.RelativeTimeFormat(prefs.lang, {
      numeric: options.numeric || 'auto',
      ...options,
    });

    // Determine the appropriate unit
    const absSeconds = Math.abs(diffInSeconds);
    if (absSeconds < 60) {
      return rtf.format(Math.round(diffInSeconds / 1), 'second');
    }
    if (absSeconds < 3600) {
      return rtf.format(Math.round(diffInSeconds / 60), 'minute');
    }
    if (absSeconds < 86400) {
      return rtf.format(Math.round(diffInSeconds / 3600), 'hour');
    }
    if (absSeconds < 2592000) {
      return rtf.format(Math.round(diffInSeconds / 86400), 'day');
    }
    if (absSeconds < 31536000) {
      return rtf.format(Math.round(diffInSeconds / 2592000), 'month');
    }
    return rtf.format(Math.round(diffInSeconds / 31536000), 'year');
  }

  // Fallback for browsers without RelativeTimeFormat
  return formatDateTime(d);
}

/**
 * Debounce a function
 * @param {Function} func - Function to debounce
 * @param {number} wait - Milliseconds to wait
 * @returns {Function} Debounced function
 */
export function debounce(func, wait = 300) {
  let timeout;
  return function executedFunction(...args) {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  };
}

/**
 * Throttle a function
 * @param {Function} func - Function to throttle
 * @param {number} limit - Milliseconds between calls
 * @returns {Function} Throttled function
 */
export function throttle(func, limit = 300) {
  let inThrottle;
  return function executedFunction(...args) {
    if (!inThrottle) {
      func(...args);
      inThrottle = true;
      setTimeout(() => (inThrottle = false), limit);
    }
  };
}

/**
 * Generate a unique ID
 * @param {string} prefix - Optional prefix for the ID
 * @returns {string} Unique ID
 */
export function generateId(prefix = 'id') {
  const randomPart = _getRandomString(9);
  return `${prefix}-${Date.now()}-${randomPart}`;
}

/**
 * Generate a cryptographically secure random string
 * @param {number} length - Length of the string to generate
 * @returns {string} Random alphanumeric string
 */
function _getRandomString(length) {
  const chars = 'abcdefghijklmnopqrstuvwxyz0123456789';
  let result = '';
  const randomValues = new Uint8Array(length);
  
  // Use crypto.getRandomValues for cryptographically secure randomness
  crypto.getRandomValues(randomValues);
  for (let i = 0; i < length; i++) {
    result += chars[randomValues[i] % chars.length];
  }
  return result;
}

/**
 * Deep clone an object
 * @param {*} obj - Object to clone
 * @returns {*} Cloned object
 */
export function deepClone(obj) {
  if (obj === null || typeof obj !== 'object') {
    return obj;
  }
  if (obj instanceof Date) {
    return new Date(obj.getTime());
  }
  if (Array.isArray(obj)) {
    return obj.map(deepClone);
  }
  const cloned = {};
  for (const key in obj) {
    if (obj.hasOwnProperty(key)) {
      cloned[key] = deepClone(obj[key]);
    }
  }
  return cloned;
}

/**
 * Escape HTML special characters
 * @param {string} text - Text to escape
 * @returns {string} Escaped text
 */
export function escapeHtml(text) {
  if (typeof text !== 'string') {
    return '';
  }
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

// Default export
export default {
  getPreferences,
  savePreferences,
  formatDate,
  formatTime,
  formatDateTime,
  formatNumber,
  formatCurrency,
  formatPercent,
  formatRelativeTime,
  debounce,
  throttle,
  generateId,
  deepClone,
  escapeHtml,
};
