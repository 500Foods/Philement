/**
 * Profile Manager - Date Formats Page Handler
 *
 * Handles the Date Formats settings page (index: -9)
 * Manages date/time format selection with live preview using Luxon.
 *
 * Uses the ProfileSettingsService for all persistence.
 * Section key: "-9"
 *
 * JSON structure written to the service:
 * {
 *   "_name": "Date Formats",
 *   "sample": "2020-01-01T14:03:02",
 *   "timezone": "America/New_York",
 *   "dates": {
 *     "short": "yyyy-MM-dd",
 *     "long": "MMMM d, y"
 *   },
 *   "times": {
 *     "short": "h:mm a",
 *     "medium": "h:mm:ss a",
 *     "long": "h:mm:ss a zzz"
 *   },
 *   "datetimes": {
 *     "short": "yyyy-MM-dd h:mm a",
 *     "long": "MMMM d, y 'at' h:mm:ss a"
 *   }
 * }
 */

import { BaseSettingsPage } from '../settings-page-base.js';
import { DateTime } from 'luxon';
import { log, Subsystems, Status } from '../../../../core/log.js';
import { scrollbarManager } from '../../../../core/scrollbar-manager.js';
import { closeAllPopups } from '../../../../core/manager-ui.js';
import { TimezonePicker } from '../../../../shared/timezone-picker.js';
import { FlatpickrPicker } from '../../../../shared/flatpickr-picker.js';
import { LuxonTokenTable } from '../../../../shared/luxon-token-table.js';

const DEFAULT_SAMPLE = '2020-01-01T14:03:02';

// Default built-in formats
const BUILTIN_FORMATS = {
  dates: {
    'Short Date': 'yyyy-MM-dd',
    'Medium Date': 'yyyy-MMM-dd',
    'Long Date': 'MMMM d, y',
    'Week Number': 'yyyy-\'W\'nn',
  },
  times: {
    'Short Time': 'HH:mm',
    'Medium Time': 'H:mm:ss',
    'Long Time': 'HH:mm:ss.SSS',
  },
  datetimes: {
    'Short DateTime': 'yyyy-MM-dd HH:mm:ss',
    'Medium DateTime': 'yyyy-MMM-dd (EEE) HH:mm:ss',
    'Long DateTime': 'MMMM d, y \'at\' HH:mm:ss',
  },
};



/**
 * Exhaustive list of IANA timezones, organized by region.
 * Generated from the IANA timezone database.
 */
const IANA_TIMEZONES = [
  // Africa
  'Africa/Abidjan', 'Africa/Accra', 'Africa/Addis_Ababa', 'Africa/Algiers', 'Africa/Asmara',
  'Africa/Bamako', 'Africa/Bangui', 'Africa/Banjul', 'Africa/Bissau', 'Africa/Blantyre',
  'Africa/Brazzaville', 'Africa/Bujumbura', 'Africa/Cairo', 'Africa/Casablanca', 'Africa/Ceuta',
  'Africa/Conakry', 'Africa/Dakar', 'Africa/Dar_es_Salaam', 'Africa/Djibouti', 'Africa/Douala',
  'Africa/El_Aaiun', 'Africa/Freetown', 'Africa/Gaborone', 'Africa/Harare', 'Africa/Johannesburg',
  'Africa/Juba', 'Africa/Kampala', 'Africa/Khartoum', 'Africa/Kigali', 'Africa/Kinshasa',
  'Africa/Lagos', 'Africa/Libreville', 'Africa/Lome', 'Africa/Luanda', 'Africa/Lubumbashi',
  'Africa/Lusaka', 'Africa/Malabo', 'Africa/Maputo', 'Africa/Maseru', 'Africa/Mbabane',
  'Africa/Mogadishu', 'Africa/Monrovia', 'Africa/Nairobi', 'Africa/Ndjamena', 'Africa/Niamey',
  'Africa/Nouakchott', 'Africa/Ouagadougou', 'Africa/Porto-Novo', 'Africa/Sao_Tome', 'Africa/Tripoli',
  'Africa/Tunis', 'Africa/Windhoek',
  // America
  'America/Adak', 'America/Anchorage', 'America/Anguilla', 'America/Antigua', 'America/Araguaina',
  'America/Argentina/Buenos_Aires', 'America/Argentina/Catamarca', 'America/Argentina/Cordoba',
  'America/Argentina/Jujuy', 'America/Argentina/La_Rioja', 'America/Argentina/Mendoza',
  'America/Argentina/Rio_Gallegos', 'America/Argentina/Salta', 'America/Argentina/San_Juan',
  'America/Argentina/San_Luis', 'America/Argentina/Tucuman', 'America/Argentina/Ushuaia',
  'America/Aruba', 'America/Asuncion', 'America/Atikokan', 'America/Bahia', 'America/Bahia_Banderas',
  'America/Barbados', 'America/Belem', 'America/Belize', 'America/Blanc-Sablon', 'America/Boa_Vista',
  'America/Bogota', 'America/Boise', 'America/Cambridge_Bay', 'America/Campo_Grande', 'America/Cancun',
  'America/Caracas', 'America/Cayenne', 'America/Cayman', 'America/Chicago', 'America/Chihuahua',
  'America/Costa_Rica', 'America/Creston', 'America/Cuiaba', 'America/Curacao', 'America/Danmarkshavn',
  'America/Dawson', 'America/Dawson_Creek', 'America/Denver', 'America/Detroit', 'America/Dominica',
  'America/Edmonton', 'America/Eirunepe', 'America/El_Salvador', 'America/Fort_Nelson',
  'America/Fortaleza', 'America/Glace_Bay', 'America/Goose_Bay', 'America/Grand_Turk',
  'America/Grenada', 'America/Guadeloupe', 'America/Guatemala', 'America/Guayaquil', 'America/Guyana',
  'America/Halifax', 'America/Havana', 'America/Hermosillo', 'America/Indiana/Indianapolis',
  'America/Indiana/Knox', 'America/Indiana/Marengo', 'America/Indiana/Petersburg',
  'America/Indiana/Tell_City', 'America/Indiana/Vevay', 'America/Indiana/Vincennes',
  'America/Indiana/Winamac', 'America/Inuvik', 'America/Iqaluit', 'America/Jamaica',
  'America/Juneau', 'America/Kentucky/Louisville', 'America/Kentucky/Monticello',
  'America/Kralendijk', 'America/La_Paz', 'America/Lima', 'America/Los_Angeles',
  'America/Lower_Princes', 'America/Maceio', 'America/Managua', 'America/Manaus', 'America/Marigot',
  'America/Martinique', 'America/Matamoros', 'America/Mazatlan', 'America/Menominee',
  'America/Merida', 'America/Metlakatla', 'America/Mexico_City', 'America/Miquelon',
  'America/Moncton', 'America/Monterrey', 'America/Montevideo', 'America/Montserrat',
  'America/Nassau', 'America/New_York', 'America/Nipigon', 'America/Nome', 'America/Noronha',
  'America/North_Dakota/Beulah', 'America/North_Dakota/Center', 'America/North_Dakota/New_Salem',
  'America/Nuuk', 'America/Ojinaga', 'America/Panama', 'America/Pangnirtung', 'America/Paramaribo',
  'America/Phoenix', 'America/Port-au-Prince', 'America/Port_of_Spain', 'America/Porto_Velho',
  'America/Puerto_Rico', 'America/Punta_Arenas', 'America/Rainy_River', 'America/Rankin_Inlet',
  'America/Recife', 'America/Regina', 'America/Resolute', 'America/Rio_Branco', 'America/Santarem',
  'America/Santiago', 'America/Santo_Domingo', 'America/Sao_Paulo', 'America/Scoresbysund',
  'America/Sitka', 'America/St_Barthelemy', 'America/St_Johns', 'America/St_Kitts',
  'America/St_Lucia', 'America/St_Thomas', 'America/St_Vincent', 'America/Swift_Current',
  'America/Tegucigalpa', 'America/Thule', 'America/Thunder_Bay', 'America/Tijuana',
  'America/Toronto', 'America/Tortola', 'America/Vancouver', 'America/Whitehorse', 'America/Winnipeg',
  'America/Yakutat', 'America/Yellowknife',
  // Antarctica
  'Antarctica/Casey', 'Antarctica/Davis', 'Antarctica/DumontDUrville', 'Antarctica/Macquarie',
  'Antarctica/Mawson', 'Antarctica/McMurdo', 'Antarctica/Palmer', 'Antarctica/Rothera',
  'Antarctica/Syowa', 'Antarctica/Troll', 'Antarctica/Vostok',
  // Arctic
  'Arctic/Longyearbyen',
  // Asia
  'Asia/Aden', 'Asia/Almaty', 'Asia/Amman', 'Asia/Anadyr', 'Asia/Aqtau', 'Asia/Aqtobe',
  'Asia/Ashgabat', 'Asia/Atyrau', 'Asia/Baghdad', 'Asia/Bahrain', 'Asia/Baku', 'Asia/Bangkok',
  'Asia/Barnaul', 'Asia/Beirut', 'Asia/Bishkek', 'Asia/Brunei', 'Asia/Chita', 'Asia/Choibalsan',
  'Asia/Colombo', 'Asia/Damascus', 'Asia/Dhaka', 'Asia/Dili', 'Asia/Dubai', 'Asia/Dushanbe',
  'Asia/Famagusta', 'Asia/Gaza', 'Asia/Hebron', 'Asia/Ho_Chi_Minh', 'Asia/Hong_Kong', 'Asia/Hovd',
  'Asia/Irkutsk', 'Asia/Jakarta', 'Asia/Jayapura', 'Asia/Jerusalem', 'Asia/Kabul',
  'Asia/Kamchatka', 'Asia/Karachi', 'Asia/Kathmandu', 'Asia/Khandyga', 'Asia/Kolkata',
  'Asia/Krasnoyarsk', 'Asia/Kuala_Lumpur', 'Asia/Kuching', 'Asia/Kuwait', 'Asia/Macau',
  'Asia/Magadan', 'Asia/Makassar', 'Asia/Manila', 'Asia/Muscat', 'Asia/Nicosia', 'Asia/Novokuznetsk',
  'Asia/Novosibirsk', 'Asia/Omsk', 'Asia/Oral', 'Asia/Phnom_Penh', 'Asia/Pontianak',
  'Asia/Pyongyang', 'Asia/Qatar', 'Asia/Qostanay', 'Asia/Qyzylorda', 'Asia/Riyadh',
  'Asia/Sakhalin', 'Asia/Samarkand', 'Asia/Seoul', 'Asia/Shanghai', 'Asia/Singapore',
  'Asia/Srednekolymsk', 'Asia/Taipei', 'Asia/Tashkent', 'Asia/Tbilisi', 'Asia/Tehran',
  'Asia/Thimphu', 'Asia/Tokyo', 'Asia/Tomsk', 'Asia/Ulaanbaatar', 'Asia/Urumqi', 'Asia/Ust-Nera',
  'Asia/Vientiane', 'Asia/Vladivostok', 'Asia/Yakutsk', 'Asia/Yangon', 'Asia/Yekaterinburg',
  'Asia/Yerevan',
  // Atlantic
  'Atlantic/Azores', 'Atlantic/Bermuda', 'Atlantic/Canary', 'Atlantic/Cape_Verde',
  'Atlantic/Faroe', 'Atlantic/Madeira', 'Atlantic/Reykjavik', 'Atlantic/South_Georgia',
  'Atlantic/St_Helena', 'Atlantic/Stanley',
  // Australia
  'Australia/Adelaide', 'Australia/Brisbane', 'Australia/Broken_Hill', 'Australia/Currie',
  'Australia/Darwin', 'Australia/Eucla', 'Australia/Hobart', 'Australia/Lindeman',
  'Australia/Lord_Howe', 'Australia/Melbourne', 'Australia/Perth', 'Australia/Sydney',
  // Europe
  'Europe/Amsterdam', 'Europe/Andorra', 'Europe/Astrakhan', 'Europe/Athens', 'Europe/Belgrade',
  'Europe/Berlin', 'Europe/Bratislava', 'Europe/Brussels', 'Europe/Bucharest', 'Europe/Budapest',
  'Europe/Busingen', 'Europe/Chisinau', 'Europe/Copenhagen', 'Europe/Dublin', 'Europe/Gibraltar',
  'Europe/Guernsey', 'Europe/Helsinki', 'Europe/Isle_of_Man', 'Europe/Istanbul', 'Europe/Jersey',
  'Europe/Kaliningrad', 'Europe/Kiev', 'Europe/Kirov', 'Europe/Lisbon', 'Europe/Ljubljana',
  'Europe/London', 'Europe/Luxembourg', 'Europe/Madrid', 'Europe/Malta', 'Europe/Mariehamn',
  'Europe/Minsk', 'Europe/Monaco', 'Europe/Moscow', 'Europe/Oslo', 'Europe/Paris',
  'Europe/Podgorica', 'Europe/Prague', 'Europe/Riga', 'Europe/Rome', 'Europe/Samara',
  'Europe/San_Marino', 'Europe/Sarajevo', 'Europe/Saratov', 'Europe/Simferopol', 'Europe/Skopje',
  'Europe/Sofia', 'Europe/Stockholm', 'Europe/Tallinn', 'Europe/Tirane', 'Europe/Ulyanovsk',
  'Europe/Uzhgorod', 'Europe/Vaduz', 'Europe/Vatican', 'Europe/Vienna', 'Europe/Vilnius',
  'Europe/Volgograd', 'Europe/Warsaw', 'Europe/Zagreb', 'Europe/Zaporozhye', 'Europe/Zurich',
  // Indian
  'Indian/Antananarivo', 'Indian/Chagos', 'Indian/Christmas', 'Indian/Cocos', 'Indian/Comoro',
  'Indian/Kerguelen', 'Indian/Mahe', 'Indian/Maldives', 'Indian/Mauritius', 'Indian/Mayotte',
  'Indian/Reunion',
  // Pacific
  'Pacific/Apia', 'Pacific/Auckland', 'Pacific/Bougainville', 'Pacific/Chatham', 'Pacific/Chuuk',
  'Pacific/Easter', 'Pacific/Efate', 'Pacific/Fakaofo', 'Pacific/Fiji', 'Pacific/Funafuti',
  'Pacific/Galapagos', 'Pacific/Gambier', 'Pacific/Guadalcanal', 'Pacific/Guam',
  'Pacific/Honolulu', 'Pacific/Kanton', 'Pacific/Kiritimati', 'Pacific/Kosrae', 'Pacific/Kwajalein',
  'Pacific/Majuro', 'Pacific/Marquesas', 'Pacific/Midway', 'Pacific/Nauru', 'Pacific/Niue',
  'Pacific/Norfolk', 'Pacific/Noumea', 'Pacific/Pago_Pago', 'Pacific/Palau', 'Pacific/Pitcairn',
  'Pacific/Pohnpei', 'Pacific/Port_Moresby', 'Pacific/Rarotonga', 'Pacific/Saipan',
  'Pacific/Tahiti', 'Pacific/Tarawa', 'Pacific/Tongatapu', 'Pacific/Wake', 'Pacific/Wallis',
  // UTC and common abbreviations (mapped to IANA identifiers)
  'UTC', 'GMT',
  // US timezones (using IANA identifiers)
  'America/Los_Angeles', 'America/Denver', 'America/Chicago', 'America/New_York',
  'America/Anchorage', 'Pacific/Honolulu',
  // European timezones (using IANA identifiers)
  'Europe/Paris', 'Europe/London', 'Europe/Berlin', 'Europe/Rome', 'Europe/Madrid',
  // Asian timezones (using IANA identifiers)
  'Asia/Kolkata', 'Asia/Tokyo', 'Asia/Shanghai', 'Asia/Seoul',
  'Asia/Bangkok', 'Asia/Manila', 'Asia/Singapore',
  // Australian timezones (using IANA identifiers)
  'Australia/Sydney', 'Australia/Adelaide', 'Australia/Perth',
  // Other major cities
  'Pacific/Auckland', 'Africa/Johannesburg', 'Europe/Moscow', 'Asia/Yekaterinburg',
  // Etc zones for completeness
  'Etc/UTC', 'Etc/GMT', 'Etc/GMT+0', 'Etc/GMT+1', 'Etc/GMT+2', 'Etc/GMT+3',
  'Etc/GMT+4', 'Etc/GMT+5', 'Etc/GMT+6', 'Etc/GMT+7', 'Etc/GMT+8', 'Etc/GMT+9', 'Etc/GMT+10',
  'Etc/GMT+11', 'Etc/GMT+12', 'Etc/GMT-0', 'Etc/GMT-1', 'Etc/GMT-2', 'Etc/GMT-3', 'Etc/GMT-4',
  'Etc/GMT-5', 'Etc/GMT-6', 'Etc/GMT-7', 'Etc/GMT-8', 'Etc/GMT-9', 'Etc/GMT-10', 'Etc/GMT-11',
  'Etc/GMT-12', 'Etc/GMT-13', 'Etc/GMT-14',
];





export class DateFormatsPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -9,
    });

    this._currentUpdateInterval = null;
    this._flatpickrPicker = null;
    this._tokenTable = null;
    this._timezonePicker = null;
    this._contentScrollbarInstance = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._ensureSectionNamed();
    this._setupEventListeners();
    this._initTimezonePicker();
    this._initFlatpickrPicker();
    await this._initTokenTable();
    await this.loadData();
    this._startCurrentTimeUpdates();
    this._initContentScrollbars();

    log(Subsystems.MANAGER, Status.DEBUG, '[DateFormatsPage] Initialized');
  }

  /**
   * Ensure the section has a _name so the JSON is self-describing
   */
  _ensureSectionNamed() {
    const section = this.getSectionData();
    if (!section._name) {
      this.setSetting('_name', 'Date Formats');
    }
  }

  /**
   * Get the effective timezone — either the saved preference or browser local
   */
  _getTimezone() {
    const tz = this.getSetting('timezone', 'local');
    if (tz === 'local') return 'local';
    if (typeof tz === 'object' && tz.name) {
      if (tz.name === 'local') return 'local';
      return tz.name;
    }
    if (typeof tz === 'string') return tz; // backward compatibility
    return 'local';
  }

  /**
   * Get a DateTime in the effective timezone
   */
  _getDateTimeNow() {
    const tz = this._getTimezone();
    if (tz === 'local') {
      return DateTime.now();
    }
    return DateTime.now().setZone(tz);
  }

  /**
   * Setup event listeners for live save
   */
  _setupEventListeners() {
    const container = this.container;
    if (!container) return;

    // Built-in format inputs: live save on input
    container.querySelectorAll('.df-format-input[data-setting]').forEach(input => {
      input.addEventListener('input', () => {
        this._updatePreview(input);
        const settingPath = input.dataset.setting;
        if (settingPath) {
          this.setSetting(settingPath, input.value);
        }
      });
    });

    // Sample input: live save
    const sampleInput = container.querySelector('#df-sample-input');
    if (sampleInput) {
      sampleInput.addEventListener('input', () => {
        this.setSetting('sample', sampleInput.value);
        this._renderAllPreviews();
      });
    }



    // Add custom format buttons
    container.querySelectorAll('.df-add-header-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const type = e.currentTarget.dataset.type;
        this._addCustomFormat(type);
      });
    });

    // Delete buttons (delegated, for builtins these are just visual locks)
    container.querySelectorAll('.df-delete-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const item = e.currentTarget.closest('.df-format-item');
        if (item && item.classList.contains('df-custom-item')) {
          item.remove();
          this._persistCustomFormats();
        }
      });
    });
  }



  /**
   * Create a new FlatPickr instance for the sample input
   */


  /**
   * Initialize FlatpickrPicker on the sample input
   */
  _initFlatpickrPicker() {
    const container = this.container;
    if (!container) return;

    const input = container.querySelector('#df-sample-input');
    const btn = container.querySelector('#df-picker-btn');
    if (!input || !btn) return;

    // Create FlatpickrPicker instance
    this._flatpickrPicker = new FlatpickrPicker({
      onChange: (date, formattedValue) => {
        this.setSetting('sample', formattedValue);
        this._renderAllPreviews();
      },
    });

    // Initialize with trigger button and input
    this._flatpickrPicker.init(btn, input);
  }

  /**
   * Initialize the custom timezone picker
   */
  _initTimezonePicker() {
    const container = this.container;
    if (!container) return;

    const pickerContainer = container.querySelector('#df-timezone-picker');
    if (!pickerContainer) return;

    this._timezonePicker = new TimezonePicker(
      pickerContainer,
      IANA_TIMEZONES,
      (timezone) => {
        this.setSetting('timezone', timezone);
        this._timezonePicker.setTimezone(timezone); // Update picker display
        this._renderAllPreviews();
      },
      (position) => {
        this.setSetting('timezonePopupPosition', position);
      }
    );

    // Load saved popup position
    const savedPosition = this.getSetting('timezonePopupPosition');
    if (savedPosition && typeof savedPosition === 'object') {
      this._timezonePicker.popupWidth = savedPosition.width;
      this._timezonePicker.popupHeight = savedPosition.height;
    }

    // Set initial timezone
    const savedTz = this.getSetting('timezone', 'local');
    this._timezonePicker.setTimezone(savedTz);
  }

  /**
   * Initialize the Luxon tokens LithiumTable
   */
  async _initTokenTable() {
    const container = this.container;
    if (!container) return;

    const tableContainer = container.querySelector('#df-token-table-container');
    const navContainer = container.querySelector('#df-token-navigator');
    if (!tableContainer) return;

    const sample = this._getSampleDateTime();

    this._tokenTable = new LuxonTokenTable({
      container: tableContainer,
      navigatorContainer: navContainer,
      sampleDateTime: sample,
      tablePath: 'profile-manager/luxon-tokens',
      lookupKeyIdx: 16,
      cssPrefix: 'profile-luxon-tokens',
      storageKey: 'profile_luxon_tokens',
      onTokenSelected: () => closeAllPopups(),
    });

    await this._tokenTable.init();
  }

  /**
   * Initialize OverlayScrollbars on the main content area
   */
  _initContentScrollbars() {
    const container = this.container;
    if (!container) return;

    const contentElement = container.querySelector('.df-content');
    if (!contentElement) return;

    // Initialize OSB on the content area using the same styling as other instances
    this._contentScrollbarInstance = scrollbarManager.initGeneric(contentElement);
  }



  /**
   * Start updating current time previews every second
   */
  _startCurrentTimeUpdates() {
    this._currentUpdateInterval = setInterval(() => {
      this._renderCurrentPreviews();
    }, 1000);
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
   * Get the alternate timezone (override if different from browser, else UTC)
   */
  _getAlternateTimezone() {
    const overrideTz = this._getTimezone();
    const browserTz = this._getBrowserTimezone();

    if (overrideTz !== 'local' && overrideTz !== browserTz) {
      return overrideTz;
    }
    return 'UTC';
  }

  /**
   * Render current time previews for format tables
   */
  _renderCurrentPreviews() {
    const container = this.container;
    if (!container) return;

    const browserTz = this._getBrowserTimezone();
    const overrideTz = this._getTimezone();
    const altTz = this._getAlternateTimezone();

    // Primary: always browser timezone
    const nowPrimary = DateTime.now().setZone(browserTz);

    // Alternate: override if different from browser, else UTC
    const nowAlt = altTz === 'UTC' ? DateTime.now().toUTC() : DateTime.now().setZone(altTz);

    // Update the current datetime display at top (primary = browser)
    const currentDisplay = container.querySelector('#df-current-datetime');
    if (currentDisplay) {
      currentDisplay.textContent = nowPrimary.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ '('ZZZZ')'");
    }

    // Update alternate current datetime
    const currentAltDisplay = container.querySelector('#df-current-datetime-alt');
    if (currentAltDisplay) {
      currentAltDisplay.textContent = nowAlt.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ '('ZZZZ')'");
    }

    // Update browser timezone display
    const browserTzDisplay = container.querySelector('#df-browser-timezone');
    if (browserTzDisplay) {
      const abbr = nowPrimary.toFormat('ZZZZ');
      browserTzDisplay.textContent = `${browserTz} (${abbr})`;
    }

    // Update sample datetime display
    this._renderSampleDatetime();

    // Format tables - use override timezone for compatibility
    const nowForTables = overrideTz === 'local' ? DateTime.now() : DateTime.now().setZone(overrideTz);
    container.querySelectorAll('.df-current').forEach(el => {
      const format = el.dataset.format;
      if (format) {
        try {
          el.textContent = nowForTables.toFormat(format);
        } catch (_e) {
          el.textContent = 'Invalid';
        }
      }
    });
  }

  /**
   * Render sample datetime display
   */
  _renderSampleDatetime() {
    const container = this.container;
    if (!container) return;

    const sample = this._getSampleDateTime();
    const browserTz = this._getBrowserTimezone();
    const altTz = this._getAlternateTimezone();

    // Get current time in browser timezone for consistent abbreviation
    const nowPrimary = DateTime.now().setZone(browserTz);
    const nowAlt = altTz === 'UTC' ? DateTime.now().toUTC() : DateTime.now().setZone(altTz);

    // Primary: sample in browser timezone
    const dtPrimary = sample.setZone(browserTz);

    // Alternate: sample in alternate timezone
    const dtAlt = altTz === 'UTC' ? sample.toUTC() : sample.setZone(altTz);

    // Primary sample display - use current time's abbreviation for consistency
    const sampleDisplay = container.querySelector('#df-sample-datetime');
    if (sampleDisplay) {
      sampleDisplay.textContent = dtPrimary.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ") + ' (' + nowPrimary.toFormat('ZZZZ') + ')';
    }

    // Alternate sample display - use current time's abbreviation for consistency
    const sampleAltDisplay = container.querySelector('#df-sample-datetime-alt');
    if (sampleAltDisplay) {
      sampleAltDisplay.textContent = dtAlt.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ") + ' (' + nowAlt.toFormat('ZZZZ') + ')';
    }
  }

  /**
   * Render example previews for format tables
   */
  _renderExamplePreviews() {
    const container = this.container;
    if (!container) return;

    const sample = this._getSampleDateTime();
    const tz = this._getTimezone();
    const sampleInTz = tz === 'local' ? sample : sample.setZone(tz);

    // Format tables
    container.querySelectorAll('.df-example').forEach(el => {
      const row = el.closest('.df-format-item');
      if (!row) return;
      const input = row.querySelector('.df-format-input');
      if (!input) return;
      try {
        el.textContent = sampleInTz.toFormat(input.value);
      } catch (_e) {
        el.textContent = 'Invalid format';
      }
    });
  }

  /**
   * Update single preview (example + current)
   */
  _updatePreview(input) {
    const row = input.closest('.df-format-item');
    if (!row) return;

    const exampleEl = row.querySelector('.df-example');
    const currentEl = row.querySelector('.df-current');
    const format = input.value;

    const sample = this._getSampleDateTime();
    const tz = this._getTimezone();
    const sampleInTz = tz === 'local' ? sample : sample.setZone(tz);
    const now = this._getDateTimeNow();

    try {
      if (exampleEl) exampleEl.textContent = sampleInTz.toFormat(format);
    } catch (_e) {
      if (exampleEl) exampleEl.textContent = 'Invalid format';
    }

    try {
      if (currentEl) {
        currentEl.textContent = now.toFormat(format);
        currentEl.dataset.format = format;
      }
    } catch (_e) {
      if (currentEl) currentEl.textContent = 'Invalid';
    }
  }

  /**
   * Get sample DateTime
   */
  _getSampleDateTime() {
    const input = this.container?.querySelector('#df-sample-input');
    const sampleStr = input?.value || DEFAULT_SAMPLE;
    return DateTime.fromISO(sampleStr);
  }

  /**
   * Render all previews (example + current)
   */
  _renderAllPreviews() {
    this._renderExamplePreviews();
    this._renderCurrentPreviews();

    // Update token table with new sample datetime
    if (this._tokenTable) {
      const sample = this._getSampleDateTime();
      this._tokenTable.updateSample(sample);
    }
  }

  /**
   * Add custom format
   */
  _addCustomFormat(type) {
    const container = this.container;
    if (!container) return;

    // Copy from the last custom row, or use defaults
    const name = 'Custom Format';
    const format = type === 'dates' ? 'yyyy-MM-dd' : type === 'times' ? 'HH:mm' : 'yyyy-MM-dd HH:mm:ss';

    const listId = `df-${type}-all`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();

    // Generate unique name
    const settingKey = type === 'datetime' ? 'datetimes' : type;
    const existing = this.getSetting(settingKey, {});
    let uniqueName = name;
    let counter = 1;
    while (existing[uniqueName]) {
      uniqueName = `${name} ${counter}`;
      counter++;
    }

    const item = document.createElement('tr');
    item.className = 'df-format-item df-custom-item';
    item.dataset.id = uniqueName;
    item.innerHTML = `
      <td class="df-col-icon"><button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button></td>
      <td class="df-col-name"><input type="text" class="df-format-name-input" value="${this._escapeHtml(uniqueName)}" placeholder="Name"></td>
      <td class="df-col-format"><input type="text" class="form-input df-format-input" value="${this._escapeHtml(format)}" placeholder="Luxon format"></td>
      <td class="df-col-example"><span class="df-example">${this._safeFormat(sample, format)}</span></td>
      <td class="df-col-current"><span class="df-current" data-format="${this._escapeHtml(format)}">${this._safeFormat(now, format)}</span></td>
    `;

    const nameInput = item.querySelector('.df-format-name-input');
    const formatInput = item.querySelector('.df-format-input');
    const deleteBtn = item.querySelector('.df-delete-btn');

    nameInput?.addEventListener('input', () => this._persistCustomFormats());
    formatInput?.addEventListener('input', () => {
      this._updatePreview(formatInput);
      this._persistCustomFormats();
    });
    deleteBtn?.addEventListener('click', () => {
      item.remove();
      this._persistCustomFormats();
    });

    list.appendChild(item);
    this._persistCustomFormats();
  }

  /**
   * Persist all custom formats to the settings service
   */
  _persistCustomFormats() {
    const container = this.container;
    if (!container) return;

    ['dates', 'times', 'datetimes'].forEach(type => {
      const custom = this._gatherCustomFormats(type);
      const settingKey = type;

      // Get current stored formats for this type
      const currentStored = this.getSetting(settingKey, {});
      const merged = { ...currentStored };

      // Remove old custom formats (anything not in builtin defaults)
      if (BUILTIN_FORMATS[type]) {
        Object.keys(merged).forEach(key => {
          if (!(key in BUILTIN_FORMATS[type])) {
            delete merged[key];
          }
        });
      }

      // Add current custom formats
      Object.assign(merged, custom);

      // Update the setting
      this.setSetting(settingKey, merged);
    });
  }

  /**
   * Safe format helper
   */
  _safeFormat(dt, format) {
    try {
      return dt.toFormat(format);
    } catch (_e) {
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
   * Load data from settings service
   */
  async loadData() {
    const container = this.container;
    if (!container) return;

    // Sample
    const sampleInput = container.querySelector('#df-sample-input');
    if (sampleInput) {
      sampleInput.value = this.getSetting('sample', DEFAULT_SAMPLE);
    }

    // Timezone picker
    if (this._timezonePicker) {
      const savedTz = this.getSetting('timezone', 'local');
      this._timezonePicker.setTimezone(savedTz);
    }

    // Built-in formats
    this._loadBuiltinFormats('dates');
    this._loadBuiltinFormats('times');
    this._loadBuiltinFormats('datetimes');

    // Custom formats
    this._loadCustomFormats('dates');
    this._loadCustomFormats('times');
    this._loadCustomFormats('datetimes');

    this._renderAllPreviews();
  }

  /**
   * Load built-in formats from settings service
   */
  _loadBuiltinFormats(type) {
    const container = this.container;
    if (!container) return;

    let allFormats = this.getSetting(type, {});

    // If no formats exist for this type, populate with defaults
    if (Object.keys(allFormats).length === 0 && BUILTIN_FORMATS[type]) {
      allFormats = { ...BUILTIN_FORMATS[type] };
      // Note: Defaults are now initialized by Profile Manager, but keep as fallback
      this.setSetting(type, allFormats);
    }

    // Load all formats for this type (both builtin and custom)
    Object.entries(allFormats).forEach(([key, format]) => {
      const input = container.querySelector(`[data-setting="${type}.${key}"]`);
      if (input && typeof format === 'string') {
        input.value = format;
        this._updatePreview(input);
      }
    });
  }

  /**
   * Load custom formats from settings service
   */
   _loadCustomFormats(type) {
    const container = this.container;
    if (!container) return;

    const listId = `df-${type}-all`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    // Remove existing custom items
    list.querySelectorAll('.df-custom-item').forEach(item => item.remove());

    const settingKey = type;
    const allFormats = this.getSetting(settingKey, {});

    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();

    // Only load formats that are not builtins
    Object.entries(allFormats).forEach(([name, format]) => {
      if (typeof format !== 'string') return;

      // Skip if this is a builtin format
      if (BUILTIN_FORMATS[type] && name in BUILTIN_FORMATS[type]) {
        return;
      }

      const item = document.createElement('tr');
      item.className = 'df-format-item df-custom-item';
      item.dataset.id = name; // Use name as ID for simplicity
      item.innerHTML = `
        <td class="df-col-icon"><button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button></td>
        <td class="df-col-name"><input type="text" class="df-format-name-input" value="${this._escapeHtml(name)}" placeholder="Name"></td>
        <td class="df-col-format"><input type="text" class="form-input df-format-input" value="${this._escapeHtml(format)}" placeholder="Luxon format"></td>
        <td class="df-col-example"><span class="df-example">${this._safeFormat(sample, format)}</span></td>
        <td class="df-col-current"><span class="df-current" data-format="${this._escapeHtml(format)}">${this._safeFormat(now, format)}</span></td>
      `;

      const nameInput = item.querySelector('.df-format-name-input');
      const formatInput = item.querySelector('.df-format-input');
      const deleteBtn = item.querySelector('.df-delete-btn');

      nameInput?.addEventListener('input', () => this._persistCustomFormats());
      formatInput?.addEventListener('input', () => {
        this._updatePreview(formatInput);
        this._persistCustomFormats();
      });
      deleteBtn?.addEventListener('click', () => {
        item.remove();
        this._persistCustomFormats();
      });

      list.appendChild(item);
    });
  }

  /**
   * Gather custom formats from DOM
   */
  _gatherCustomFormats(type) {
    const container = this.container;
    const custom = {};

    const listId = `df-${type}-all`;
    const list = container?.querySelector(`#${listId}`);
    if (!list) return custom;

    list.querySelectorAll('.df-custom-item').forEach(item => {
      const nameInput = item.querySelector('.df-format-name-input');
      const input = item.querySelector('.df-format-input');
      if (nameInput && input && nameInput.value.trim()) {
        // Use the name as the key, format as the value
        custom[nameInput.value.trim()] = input.value;
      }
    });

    return custom;
  }

  /**
   * Save — no-op for live-save page, but required by base class
   */
  async save() {
    // All changes are saved live via setSetting
    return { success: true };
  }

  /**
   * Cancel — reload from service
   */
  cancel() {
    this.loadData();
  }

  /**
   * Always clean for live-save page
   */
  isDirty() {
    return false;
  }

  /**
   * On show
   */
  onShow() {
    this._renderCurrentPreviews();
  }

  /**
   * Called when the page is hidden
   */
  onHide() {
    // Close any open popups when switching away from this page
    if (this._timezonePicker && this._timezonePicker.isOpen) {
      this._timezonePicker.close();
    }
    if (this._flatpickrPicker && this._flatpickrPicker.getIsOpen()) {
      this._flatpickrPicker.close();
    }
  }

  /**
   * Destroy the page
   */
  destroy() {
    if (this._currentUpdateInterval) {
      clearInterval(this._currentUpdateInterval);
      this._currentUpdateInterval = null;
    }
    if (this._flatpickrPicker) {
      this._flatpickrPicker.destroy();
      this._flatpickrPicker = null;
    }
    if (this._tokenTable) {
      this._tokenTable.destroy();
      this._tokenTable = null;
    }
    if (this._timezonePicker) {
      this._timezonePicker.destroy();
      this._timezonePicker = null;
    }
    if (this._contentScrollbarInstance) {
      scrollbarManager.destroy(this._contentScrollbarInstance);
      this._contentScrollbarInstance = null;
    }
    super.destroy();
  }
}

export default DateFormatsPage;
