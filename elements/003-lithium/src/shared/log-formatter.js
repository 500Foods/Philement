/**
 * Log Formatter Utility
 * 
 * Pure functions for formatting log entries for display.
 * Used by Login Manager and Session Log Manager.
 */

import * as Flags from 'country-flag-icons/string/3x2';

/**
 * Format log entries into display text for CodeMirror viewer.
 * Expands grouped entries ({ title, items }) to multiple display lines.
 * 
 * @param {Array} entries - Array of log entries with timestamp, subsystem, description
 * @returns {string} Formatted log text
 */
export function formatLogText(entries) {
  if (!entries || entries.length === 0) {
    return '(No log entries yet)';
  }

  // Determine max subsystem width for fixed-width column alignment.
  // Bracketed entries (EventBus) are excluded from this calculation because their
  // brackets already account for the two-space column separator.
  const maxSubsystemLen = entries.reduce((max, e) => {
    const s = e.subsystem || '';
    return s.startsWith('[') ? max : Math.max(max, s.length);
  }, 0);

  // Build display lines, expanding grouped entries ({ title, items }) to multiple lines.
  // Subsystems stored with bracket notation (e.g. "[Lookups]") are EventBus entries:
  // they are rendered without extra padding because the brackets consume the two
  // separator spaces, keeping columns visually aligned with plain subsystem names.
  const lines = [];
  for (const entry of entries) {
    const date = new Date(entry.timestamp);
    const time = String(date.getHours()).padStart(2, '0') + ':' +
      String(date.getMinutes()).padStart(2, '0') + ':' +
      String(date.getSeconds()).padStart(2, '0') + '.' +
      String(date.getMilliseconds()).padStart(3, '0');
    const raw = entry.subsystem || '';
    const isBracketed = raw.startsWith('[');
    // Bracketed entries (EventBus) use a single space before and after the label.
    // The two bracket characters replace the two padding spaces from plain entries,
    // keeping columns visually aligned:
    //   time  Lookups  desc  →  pre(2) + name(7) + sep(2) = 11 chars before desc
    //   time [Lookups] desc  →  pre(1) + name(9) + sep(1) = 11 chars before desc ✓
    const subsystem = isBracketed ? raw : raw.padEnd(maxSubsystemLen);
    const pre = isBracketed ? ' ' : '  ';
    const sep = isBracketed ? ' ' : '  ';
    const desc = entry.description;

    if (desc && typeof desc === 'object' && desc.title !== undefined && Array.isArray(desc.items)) {
      // Grouped entry: render title + ― continuation lines
      lines.push(`${time}${pre}${subsystem}${sep}${desc.title}`);
      for (const item of desc.items) {
        lines.push(`${time}${pre}${subsystem}${sep}― ${item}`);
      }
    } else {
      lines.push(`${time}${pre}${subsystem}${sep}${desc}`);
    }
  }

  return lines.join('\n');
}

/**
 * Get SVG flag HTML for a country code
 * @param {string} countryCode - ISO 3166-1 alpha-2 country code
 * @param {Object} options - Configuration options
 * @param {number} options.width - SVG width (default: 28)
 * @param {number} options.height - SVG height (default: 20)
 * @param {string} options.fallbackText - Fallback text for unknown codes
 * @returns {string} SVG HTML string
 */
export function getFlagSvg(countryCode, options = {}) {
  const {
    width = 28,
    height = 20,
    fallbackText = countryCode
  } = options;

  try {
    // Check if flag exists, fallback to US if not found
    const flagSvg = Flags[countryCode] || Flags.US;
    // Adjust the SVG dimensions and styling
    return flagSvg.replace(
      /<svg /,
      `<svg width="${width}" height="${height}" style="border-radius: 3px; display: block; box-shadow: 0 1px 3px rgba(0,0,0,0.3);" `
    );
  } catch (_e) {
    // Fallback to text
    return `<span style="display: inline-block; width: ${width}px; height: ${height}px; background: var(--bg-tertiary); border-radius: 3px; text-align: center; line-height: ${height}px; font-size: 10px; font-weight: 600;">${fallbackText}</span>`;
  }
}

/**
 * Get password manager element selectors
 * Used to identify and remove password manager injected elements
 * @returns {string[]} Array of CSS selectors
 */
export function getPasswordManagerSelectors() {
  return [
    'com-1password-button',
    '[id*="1p-"]',
    '[class*="lastpass"]',
    '[class*="bitwarden"]',
    '[data-bitwarden]',
    '[class*="dashlane"]',
    '[data-dashlane]',
  ];
}
