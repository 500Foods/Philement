/**
 * Formatter Utilities and Custom Calculations
 *
 * Provides formatter wrappers and custom calculation functions
 * that properly handle blank and zero values.
 *
 * @module tables/resolution/formatters
 */

import { processIcons } from '../../core/icons.js';

// ── Custom Calculation Functions ────────────────────────────────────────────

/**
 * Custom count function that counts all non-null/non-undefined values.
 * Unlike Tabulator's built-in count, this treats 0 as a valid value to count.
 * Only excludes null and undefined (not 0 or empty string).
 */
function lithiumCount(values, _data, _calcParams) {
  let count = 0;
  values.forEach((value) => {
    // Count everything except null and undefined
    // 0, empty string, false, etc. are all counted
    if (value !== null && value !== undefined) {
      count++;
    }
  });
  return count;
}

/**
 * Custom unique function that counts unique non-null/non-undefined values.
 * Unlike Tabulator's built-in unique, this treats 0 as a valid value.
 * Only excludes null and undefined (not 0 or empty string).
 */
function lithiumUnique(values, _data, _calcParams) {
  const uniqueSet = new Set();
  values.forEach((value) => {
    // Include everything except null and undefined
    if (value !== null && value !== undefined) {
      uniqueSet.add(value);
    }
  });
  return uniqueSet.size;
}

/**
 * Custom sum function that sums all numeric values.
 * Treats null/undefined as 0, includes 0 in calculations.
 */
function lithiumSum(values, _data, _calcParams) {
  let sum = 0;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      sum += num;
    }
  });
  return sum;
}

/**
 * Custom average function that averages all numeric values.
 * Treats null/undefined as 0 for sum, but excludes them from count.
 * Includes 0 in both sum and count.
 */
function lithiumAvg(values, _data, _calcParams) {
  let sum = 0;
  let count = 0;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      sum += num;
      count++;
    }
  });
  return count > 0 ? sum / count : 0;
}

/**
 * Custom min function that finds the minimum numeric value.
 * Excludes null/undefined/NaN, includes 0.
 */
function lithiumMin(values, _data, _calcParams) {
  let min = Infinity;
  let hasValue = false;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      if (!hasValue || num < min) {
        min = num;
        hasValue = true;
      }
    }
  });
  return hasValue ? min : null;
}

/**
 * Custom max function that finds the maximum numeric value.
 * Excludes null/undefined/NaN, includes 0.
 */
function lithiumMax(values, _data, _calcParams) {
  let max = -Infinity;
  let hasValue = false;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      if (!hasValue || num > max) {
        max = num;
        hasValue = true;
      }
    }
  });
  return hasValue ? max : null;
}

/**
 * Map of calculation names to custom functions.
 * These replace Tabulator's built-in calculations to properly handle 0 values.
 */
export const LITHIUM_CALCULATIONS = {
  count: lithiumCount,
  unique: lithiumUnique,
  sum: lithiumSum,
  avg: lithiumAvg,
  min: lithiumMin,
  max: lithiumMax,
};

// ── Formatter Wrapper ───────────────────────────────────────────────────────

/**
 * Format a value using the logic of a built-in Tabulator formatter.
 *
 * When wrapFormatter() wraps a built-in formatter (string name like "number"),
 * it can't delegate to Tabulator's internal formatter for non-blank/non-zero
 * values — Tabulator sees a function and uses its return value directly.
 * This utility replicates the formatting for common built-in formatters so
 * the wrapper produces identical output.
 *
 * Supported: "number", "money", "plaintext", "lookup".
 * Unsupported formatters fall through and return the raw value.
 *
 * @param {*}      value         - The cell value to format
 * @param {string} formatterName - Built-in Tabulator formatter name
 * @param {Object} params        - formatterParams for the formatter
 * @returns {string|*} Formatted display value
 */
export function formatBuiltinValue(value, formatterName, params) {
  switch (formatterName) {
    case 'number': {
      if (value == null || value === '') return '';
      const num = Number(value);
      if (isNaN(num)) return String(value);

      const precision = params?.precision ?? 0;
      let formatted = num.toFixed(precision);

      // Thousands separator - check for !== undefined (empty string "" disables separator)
      if (params?.thousand !== undefined) {
        const parts = formatted.split('.');
        if (params.thousand) {
          parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, params.thousand);
        }
        formatted = parts.join('.');
      }

      // Decimal character override
      if (params?.decimal && formatted.includes('.')) {
        formatted = formatted.replace('.', params.decimal);
      }

      // Prefix / suffix
      if (params?.prefix) formatted = params.prefix + formatted;
      if (params?.suffix) formatted = formatted + params.suffix;

      return formatted;
    }

    case 'money': {
      if (value == null || value === '') return '';
      const num = Number(value);
      if (isNaN(num)) return String(value);

      const precision = params?.precision ?? 2;
      let formatted = num.toFixed(precision);

      // Thousands separator - check for !== undefined (empty string "" disables separator)
      if (params?.thousand !== undefined) {
        const parts = formatted.split('.');
        if (params.thousand) {
          parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, params.thousand);
        }
        formatted = parts.join(params?.decimal || '.');
      } else if (params?.decimal && formatted.includes('.')) {
        formatted = formatted.replace('.', params.decimal);
      }

      // Currency symbol
      if (params?.symbol) {
        formatted = params.symbolAfter
          ? formatted + params.symbol
          : params.symbol + formatted;
      }

      return formatted;
    }

    case 'plaintext':
      return value != null ? String(value) : '';

    case 'html':
    case 'lookup': {
      if (value == null || value === '') return '';
      const lookup = params?.lookup || {};
      return lookup[value] !== undefined ? lookup[value] : String(value);
    }

    default:
      // For formatters we don't replicate (datetime, tickCross, link, etc.),
      // return the raw value. The built-in formatting won't apply when wrapped,
      // but this is an acceptable fallback — the value is still displayed.
      return value;
  }
}

/**
 * Create a custom HTML formatter that processes Lithium icons.
 * This formatter renders HTML content and calls processIcons on the cell
 * to convert <fa> tags to Font Awesome SVG icons.
 *
 * NOTE: This formatter relies on the global MutationObserver in icons.js
 * to process <fa> tags. The onRendered callback ensures icons are processed
 * after the cell is added to the DOM.
 *
 * @returns {Function} Tabulator formatter function
 */
export function createHtmlFormatter() {
  return function(cell, _formatterParams, onRendered) {
    const value = cell.getValue();
    if (value == null || value === '') return '';

    // Process icons after the cell is rendered and added to DOM
    // This ensures the MutationObserver in icons.js catches the new <fa> elements
    if (typeof onRendered === 'function') {
      onRendered(function() {
        // Double requestAnimationFrame ensures DOM is fully updated
        requestAnimationFrame(() => {
          requestAnimationFrame(() => {
            const cellEl = cell.getElement();
            if (cellEl) {
              processIcons(cellEl);
            }
          });
        });
      });
    }

    return String(value);
  };
}

/**
 * Wrap a Tabulator formatter to intercept blank and zero values.
 *
 * - If the cell value is null, undefined, or "" → return blankValue
 * - If the cell value is 0 and zeroValue differs from null → return zeroValue
 * - Otherwise → delegate to the original formatter
 *
 * For built-in formatters (string names like "number"), the wrapper
 * replicates the formatting via formatBuiltinValue() so that thousand
 * separators, precision, currency symbols, etc. are preserved.
 *
 * @param {string|Function} formatter     - Tabulator formatter name or function
 * @param {Object}          formatterParams - Params for the formatter
 * @param {*}               blankValue    - What to show for blank values
 * @param {*}               zeroValue     - What to show for zero (null = show '0')
 * @returns {string|Function} The original formatter (when no wrapping needed)
 *                            or a Tabulator formatter function
 */
export function wrapFormatter(formatter, formatterParams, blankValue, zeroValue) {
  // If no special handling needed, return the original formatter as-is
  if (!needsBlankZeroWrapper(blankValue, zeroValue)) {
    return formatter;
  }

  // For 'html' formatter, use our custom formatter that processes icons
  if (formatter === 'html') {
    return createHtmlFormatter();
  }

  // Return a wrapper function that handles blank/zero before delegating
  return function(cell, onRendered) {
    const value = cell.getValue();

    // Handle blank (null, undefined, empty string)
    if (value === null || value === undefined || value === '') {
      if (blankValue != null) return blankValue;
      if (blankValue === '') return '';
    }

    // Handle zero
    if (value === 0 && zeroValue !== null && zeroValue !== undefined) {
      return zeroValue;
    }

    // For built-in formatters (string name), replicate their formatting
    // so the wrapper produces the same output (thousand seps, precision, etc.)
    if (typeof formatter === 'string') {
      return formatBuiltinValue(value, formatter, formatterParams);
    }

    // For custom formatter functions, call them directly
    if (typeof formatter === 'function') {
      return formatter(cell, formatterParams, onRendered);
    }

    // Fallback: return raw value
    return value;
  };
}

/**
 * Check if a column needs the blank/zero wrapper at all.
 * If blank is null and zero is null, no wrapper needed.
 *
 * @param {*} blankValue - coltype/override blank value
 * @param {*} zeroValue  - coltype/override zero value
 * @returns {boolean}
 */
export function needsBlankZeroWrapper(blankValue, zeroValue) {
  // blank="" means "show nothing for blanks" — that's active
  // blank=null means "no special handling" — inactive
  // zero="" means "show nothing for zeros" — active
  // zero=null means "no special handling (show 0)" — inactive
  return blankValue !== null || zeroValue !== null;
}
