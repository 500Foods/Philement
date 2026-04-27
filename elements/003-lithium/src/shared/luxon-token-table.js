/**
 * LuxonTokenTable Component
 *
 * A reusable component for displaying Luxon date/time format tokens in a table.
 * Provides a comprehensive reference of all available format tokens with examples.
 *
 * Features:
 * - Complete Luxon format token reference
 * - Live examples using sample and current datetime
 * - Grouped by category (Timezones, Years, Months, etc.)
 * - Integrated with LithiumTable for navigation and search
 * - Auto-updates current time examples
 */

import { DateTime } from 'luxon';
import { LithiumTable } from '../tables/lithium-table-main.js';
import { log, Subsystems, Status } from '../core/log.js';
import { closeAllPopups } from '../core/manager-ui.js';

/**
 * Comprehensive list of Luxon format tokens organized by category
 */
const LUXON_FORMAT_TOKENS = [
  // Timezones
  { token: 'Z', desc: 'Narrow offset (+5)', group: 'Timezones' },
  { token: 'ZZ', desc: 'Short offset (+05:00)', group: 'Timezones' },
  { token: 'ZZZ', desc: 'Techie offset (+0500)', group: 'Timezones' },
  { token: 'ZZZZ', desc: 'Abbreviated named offset', group: 'Timezones' },
  { token: 'ZZZZZ', desc: 'Full named offset', group: 'Timezones' },
  { token: 'z', desc: 'IANA zone name', group: 'Timezones' },
  // Eras
  { token: 'G', desc: 'Abbreviated era', group: 'Eras' },
  { token: 'GG', desc: 'Full era', group: 'Eras' },
  { token: 'GGGGG', desc: 'One-letter era', group: 'Eras' },
  // Years
  { token: 'y', desc: 'Year, unpadded', group: 'Years' },
  { token: 'yy', desc: 'Two-digit year', group: 'Years' },
  { token: 'yyyy', desc: 'Four-digit year', group: 'Years' },
  { token: 'kk', desc: 'ISO week year, unpadded', group: 'Years' },
  { token: 'kkkk', desc: 'ISO week year, padded', group: 'Years' },
  { token: 'ii', desc: 'Local week year, unpadded', group: 'Years' },
  { token: 'iiii', desc: 'Local week year, padded', group: 'Years' },
  // Months
  { token: 'M', desc: 'Month, unpadded', group: 'Months' },
  { token: 'MM', desc: 'Month, padded to 2', group: 'Months' },
  { token: 'MMM', desc: 'Abbreviated month', group: 'Months' },
  { token: 'MMMM', desc: 'Full month name', group: 'Months' },
  { token: 'MMMMM', desc: 'Month as single letter', group: 'Months' },
  // Weeks
  { token: 'W', desc: 'ISO week number', group: 'Weeks' },
  { token: 'WW', desc: 'Padded ISO week', group: 'Weeks' },
  { token: 'n', desc: 'Local week number', group: 'Weeks' },
  { token: 'nn', desc: 'Padded local week', group: 'Weeks' },
  // Days
  { token: 'd', desc: 'Day of month, unpadded', group: 'Days' },
  { token: 'dd', desc: 'Day of month, padded', group: 'Days' },
  { token: 'E', desc: 'Weekday as number (1-7)', group: 'Days' },
  { token: 'EEE', desc: 'Abbreviated weekday', group: 'Days' },
  { token: 'EEEE', desc: 'Full weekday name', group: 'Days' },
  { token: 'EEEEE', desc: 'Weekday as single letter', group: 'Days' },
  // Ordinal / Quarter
  { token: 'o', desc: 'Ordinal day of year', group: 'Ordinal / Quarter' },
  { token: 'ooo', desc: 'Padded ordinal day', group: 'Ordinal / Quarter' },
  { token: 'q', desc: 'Quarter, unpadded', group: 'Ordinal / Quarter' },
  { token: 'qq', desc: 'Quarter, padded', group: 'Ordinal / Quarter' },
  // Hours
  { token: 'H', desc: '24-hour, unpadded', group: 'Hours' },
  { token: 'HH', desc: '24-hour, padded', group: 'Hours' },
  { token: 'h', desc: '12-hour, unpadded', group: 'Hours' },
  { token: 'hh', desc: '12-hour, padded', group: 'Hours' },
  { token: 'a', desc: 'Meridiem (AM/PM)', group: 'Hours' },
  // Minutes
  { token: 'm', desc: 'Minutes, unpadded', group: 'Minutes' },
  { token: 'mm', desc: 'Minutes, padded', group: 'Minutes' },
  // Seconds
  { token: 's', desc: 'Seconds, unpadded', group: 'Seconds' },
  { token: 'ss', desc: 'Seconds, padded', group: 'Seconds' },
  // Subseconds
  { token: 'S', desc: 'Millisecond, unpadded', group: 'Subseconds' },
  { token: 'SSS', desc: 'Millisecond, padded to 3', group: 'Subseconds' },
  { token: 'u', desc: 'Fractional seconds (3 digits)', group: 'Subseconds' },
  { token: 'uu', desc: 'Fractional seconds (2 digits)', group: 'Subseconds' },
  { token: 'uuu', desc: 'Fractional seconds (1 digit)', group: 'Subseconds' },
  // Timestamps
  { token: 'X', desc: 'Unix timestamp (seconds)', group: 'Timestamps' },
  { token: 'x', desc: 'Unix timestamp (ms)', group: 'Timestamps' },
  // Locale Date
  { token: 'D', desc: 'Localized numeric date', group: 'Locale Date' },
  { token: 'DD', desc: 'Localized date with abbreviated month', group: 'Locale Date' },
  { token: 'DDD', desc: 'Localized date with full month', group: 'Locale Date' },
  { token: 'DDDD', desc: 'Localized date with weekday', group: 'Locale Date' },
  // Locale Time (12-hour)
  { token: 't', desc: 'Localized time', group: 'Locale Time (12-hour)' },
  { token: 'tt', desc: 'Localized time with seconds', group: 'Locale Time (12-hour)' },
  { token: 'ttt', desc: 'Localized time with abbreviated offset', group: 'Locale Time (12-hour)' },
  { token: 'tttt', desc: 'Localized time with full offset', group: 'Locale Time (12-hour)' },
  // Locale Time (24-hour)
  { token: 'T', desc: 'Localized 24-hour time', group: 'Locale Time (24-hour)' },
  { token: 'TT', desc: 'Localized 24-hour time with seconds', group: 'Locale Time (24-hour)' },
  { token: 'TTT', desc: 'Localized 24-hour time with abbreviated offset', group: 'Locale Time (24-hour)' },
  { token: 'TTTT', desc: 'Localized 24-hour time with full offset', group: 'Locale Time (24-hour)' },
  // Locale DateTime
  { token: 'f', desc: 'Short localized date & time', group: 'Locale DateTime' },
  { token: 'ff', desc: 'Less short localized date & time', group: 'Locale DateTime' },
  { token: 'fff', desc: 'Verbose localized date & time', group: 'Locale DateTime' },
  { token: 'ffff', desc: 'Extra verbose localized date & time', group: 'Locale DateTime' },
  // Locale DateTime with Seconds
  { token: 'F', desc: 'Short localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  { token: 'FF', desc: 'Less short localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  { token: 'FFF', desc: 'Verbose localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  { token: 'FFFF', desc: 'Extra verbose localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  // Escaping
  { token: "'text'", desc: 'Literal text in single quotes', group: 'Escaping' },
];

export class LuxonTokenTable {
  constructor(options = {}) {
    this.container = options.container || null;
    this.navigatorContainer = options.navigatorContainer || null;
    this.sampleDateTime = options.sampleDateTime || DateTime.fromISO('2020-01-01T14:03:02');
    this.currentDateTime = options.currentDateTime || DateTime.now();
    this.tablePath = options.tablePath || 'luxon-tokens';
    this.lookupKeyIdx = options.lookupKeyIdx || 16;
    this.cssPrefix = options.cssPrefix || 'luxon-tokens';
    this.storageKey = options.storageKey || 'luxon_tokens';
    this.onTokenSelected = options.onTokenSelected || (() => {});

    this.table = null;
    this.updateInterval = null;
    this.isDestroyed = false;
  }

  /**
   * Initialize the token table
   */
  async init() {
    if (!this.container) {
      log(Subsystems.UI, Status.ERROR, '[LuxonTokenTable] No container provided');
      return false;
    }

    this.table = new LithiumTable({
      container: this.container,
      navigatorContainer: this.navigatorContainer,
      tablePath: this.tablePath,
      lookupKeyIdx: this.lookupKeyIdx,
      cssPrefix: this.cssPrefix,
      storageKey: this.storageKey,
      app: null,
      readonly: true,
      primaryKeyField: ['token'],
      localSearch: true,
      localSearchFields: ['token', 'description', 'group'],
      useOverlayScrollbars: true,
      onRefresh: () => this.refresh(),
      onRowSelected: () => {
        closeAllPopups();
        this.onTokenSelected();
      },
    });

    await this.table.init();
    this.loadData();
    // Removed automatic updates - table only updates when explicitly requested

    log(Subsystems.UI, Status.DEBUG, '[LuxonTokenTable] Initialized');
    return true;
  }

  /**
   * Load token data into the table
   */
  loadData() {
    if (!this.table) return;

    const data = this.buildTokenData(this.sampleDateTime, this.currentDateTime);
    this.table.loadStaticData(data, { autoSelect: true });
  }

  /**
   * Refresh the table with updated datetime values
   */
  refresh() {
    if (this.isDestroyed || !this.table) return;

    this.currentDateTime = DateTime.now();
    this.loadData();
  }

  /**
   * Update the sample datetime and refresh the table
   */
  updateSample(sampleDateTime) {
    this.sampleDateTime = sampleDateTime;
    this.refresh();
  }

  /**
   * Start automatic updates of current time examples
   */
  startUpdates() {
    if (this.updateInterval) return;

    this.updateInterval = setInterval(() => {
      this.refresh();
    }, 1000);
  }

  /**
   * Stop automatic updates
   */
  stopUpdates() {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
      this.updateInterval = null;
    }
  }

  /**
   * Build the token data array with examples
   */
  buildTokenData(sample, now) {
    return LUXON_FORMAT_TOKENS.map(t => {
      let example, current;
      try {
        example = sample.toFormat(t.token);
      } catch (_e) {
        example = '-';
      }
      try {
        current = now.toFormat(t.token);
      } catch (_e) {
        current = '-';
      }
      return {
        id: t.token,  // Primary key
        token: t.token,
        description: t.desc,
        group: t.group,
        example,
        current,
      };
    });
  }

  /**
   * Get the current table instance
   */
  getTable() {
    return this.table;
  }

  /**
   * Destroy the component and clean up resources
   */
  destroy() {
    this.isDestroyed = true;
    this.stopUpdates();

    if (this.table) {
      this.table.destroy();
      this.table = null;
    }

    log(Subsystems.UI, Status.DEBUG, '[LuxonTokenTable] Destroyed');
  }
}