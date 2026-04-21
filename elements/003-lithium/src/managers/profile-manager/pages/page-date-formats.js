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

import { BaseSettingsPage } from './settings-page-base.js';
import { DateTime } from 'luxon';
import flatpickr from 'flatpickr';
import { LithiumTable } from '../../../tables/lithium-table-main.js';
import { log, Subsystems, Status } from '../../../core/log.js';

const DEFAULT_SAMPLE = '2020-01-01T14:03:02';

const BUILTIN_FORMATS = {
  dates: {
    short: { name: 'Short Date', format: 'yyyy-MM-dd', setting: 'dates.short' },
    long: { name: 'Long Date', format: 'MMMM d, y', setting: 'dates.long' },
  },
  times: {
    short: { name: 'Short Time', format: 'h:mm a', setting: 'times.short' },
    medium: { name: 'Medium Time', format: 'h:mm:ss a', setting: 'times.medium' },
    long: { name: 'Long Time', format: 'h:mm:ss a zzz', setting: 'times.long' },
  },
  datetimes: {
    short: { name: 'Short DateTime', format: 'yyyy-MM-dd h:mm a', setting: 'datetimes.short' },
    long: { name: 'Long DateTime', format: "MMMM d, y 'at' h:mm:ss a", setting: 'datetimes.long' },
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
  // UTC
  'UTC', 'GMT', 'Etc/UTC', 'Etc/GMT', 'Etc/GMT+0', 'Etc/GMT+1', 'Etc/GMT+2', 'Etc/GMT+3',
  'Etc/GMT+4', 'Etc/GMT+5', 'Etc/GMT+6', 'Etc/GMT+7', 'Etc/GMT+8', 'Etc/GMT+9', 'Etc/GMT+10',
  'Etc/GMT+11', 'Etc/GMT+12', 'Etc/GMT-0', 'Etc/GMT-1', 'Etc/GMT-2', 'Etc/GMT-3', 'Etc/GMT-4',
  'Etc/GMT-5', 'Etc/GMT-6', 'Etc/GMT-7', 'Etc/GMT-8', 'Etc/GMT-9', 'Etc/GMT-10', 'Etc/GMT-11',
  'Etc/GMT-12', 'Etc/GMT-13', 'Etc/GMT-14',
];

/**
 * Build static token data for the LithiumTable.
 * Returns an array of row objects with token, description, group, example, current.
 */
function buildTokenData(sample, now) {
  const tokens = [
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

  return tokens.map(t => {
    let example = '-';
    let current = '-';
    try { example = sample.toFormat(t.token); } catch (_e) { example = '-'; }
    try { current = now.toFormat(t.token); } catch (_e) { current = '-'; }
    return {
      token: t.token,
      description: t.desc,
      group: t.group,
      example,
      current,
    };
  });
}

export class DateFormatsPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -9,
    });

    this._currentUpdateInterval = null;
    this._flatpickrInstance = null;
    this._tokenTable = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._ensureSectionNamed();
    this._setupEventListeners();
    this._initTimezoneDropdown();
    this._initFlatpickr();
    await this._initTokenTable();
    await this.loadData();

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
    return this.getSetting('timezone', 'local');
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

    // Timezone dropdown
    const tzSelect = container.querySelector('#df-timezone-select');
    if (tzSelect) {
      tzSelect.addEventListener('change', () => {
        this.setSetting('timezone', tzSelect.value);
        this._renderAllPreviews();
      });
    }

    // Timezone filter
    const tzFilter = container.querySelector('#df-tz-filter');
    if (tzFilter) {
      tzFilter.addEventListener('input', () => {
        this._filterTimezones(tzFilter.value);
      });
    }

    // Add custom format buttons
    container.querySelectorAll('.df-add-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const type = e.currentTarget.dataset.type;
        this._addCustomFormat(type);
      });
    });

    // Delete buttons (delegated, for builtins these are just visual locks)
    container.querySelectorAll('.df-delete-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const item = e.currentTarget.closest('.df-format-item');
        if (item && item.classList.contains('df-custom-item') && confirm('Delete this custom format?')) {
          item.remove();
          this._persistCustomFormats();
        }
      });
    });
  }

  /**
   * Filter timezone dropdown options based on search input
   */
  _filterTimezones(query) {
    const container = this.container;
    if (!container) return;

    const select = container.querySelector('#df-timezone-select');
    if (!select) return;

    const q = query.toLowerCase().trim();
    const allOptions = select.querySelectorAll('option');
    const allGroups = select.querySelectorAll('optgroup');

    allOptions.forEach(opt => {
      if (opt.value === 'local') {
        opt.style.display = '';
        return;
      }
      const searchText = opt.dataset.search || opt.textContent.toLowerCase();
      const match = searchText.includes(q);
      opt.style.display = match ? '' : 'none';
    });

    // Hide empty optgroups
    allGroups.forEach(group => {
      const visible = group.querySelectorAll('option:not([style*="none"])');
      group.style.display = visible.length > 0 ? '' : 'none';
    });
  }

  /**
   * Initialize FlatPickr on the sample input
   *
   * Positioned like a header popup: top-right corner of calendar is placed
   * just below the bottom-right corner of the trigger button.  The calendar
   * is wrapped in our popup container so it gets the same scale(0.5)→scale(1)
   * animation used by the rest of the manager UI.
   */
  _initFlatpickr() {
    const container = this.container;
    if (!container) return;

    const input = container.querySelector('#df-sample-input');
    const btn = container.querySelector('#df-picker-btn');
    if (!input || !btn) return;

    // Parse current input value for defaultDate
    let defaultDate = DEFAULT_SAMPLE;
    try {
      const parsed = DateTime.fromISO(input.value);
      if (parsed.isValid) defaultDate = parsed.toJSDate();
    } catch (_e) {
      // ignore
    }

    // Create a wrapper that applies the manager popup styling + animation
    const wrapper = document.createElement('div');
    wrapper.className = 'manager-ui-popup manager-header-popup flatpickr-popup-wrapper';

    this._flatpickrInstance = flatpickr(input, {
      enableTime: true,
      dateFormat: 'Y-m-d\\TH:i:S',
      time_24hr: true,
      allowInput: true,
      clickOpens: false,
      defaultDate,
      appendTo: wrapper,
      static: true,
      disableMobile: true,
      onChange: (selectedDates) => {
        if (selectedDates.length > 0) {
          const dt = DateTime.fromJSDate(selectedDates[0]);
          input.value = dt.toFormat("yyyy-MM-dd'T'HH:mm:ss");
          this.setSetting('sample', input.value);
          this._renderAllPreviews();
        }
      },
    });

    btn.addEventListener('click', () => {
      if (!this._flatpickrInstance) return;

      // Toggle: if already open, close it
      const existing = document.querySelector('.flatpickr-popup-wrapper.visible');
      if (existing) {
        existing.classList.remove('visible');
        setTimeout(() => existing.remove(), 350);
        return;
      }

      // Update the date inside the picker without firing onChange
      this._flatpickrInstance.setDate(input.value, false);

      // Position wrapper like a header-dropdown popup:
      // top-right of popup is 1px below bottom-right of button
      const btnRect = btn.getBoundingClientRect();
      wrapper.style.top = `${btnRect.bottom + 1}px`;
      wrapper.style.right = `${window.innerWidth - btnRect.right}px`;
      wrapper.style.left = 'auto';
      wrapper.style.bottom = 'auto';

      document.body.appendChild(wrapper);

      // Trigger scale animation
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          wrapper.classList.add('visible');
        });
      });

      // Close on outside click / escape
      const closePicker = (e) => {
        if (!wrapper.contains(e.target) && !btn.contains(e.target)) {
          wrapper.classList.remove('visible');
          setTimeout(() => wrapper.remove(), 350);
          document.removeEventListener('click', closePicker);
          document.removeEventListener('keydown', escPicker);
          document.removeEventListener('close-all-popups', closePicker);
        }
      };

      const escPicker = (e) => {
        if (e.key === 'Escape') {
          wrapper.classList.remove('visible');
          setTimeout(() => wrapper.remove(), 350);
          document.removeEventListener('click', closePicker);
          document.removeEventListener('keydown', escPicker);
          document.removeEventListener('close-all-popups', closePicker);
        }
      };

      setTimeout(() => {
        document.addEventListener('click', closePicker);
        document.addEventListener('keydown', escPicker);
        document.addEventListener('close-all-popups', closePicker);
      }, 0);
    });
  }

  /**
   * Populate the timezone dropdown with all IANA zones plus common abbreviations
   */
  _initTimezoneDropdown() {
    const container = this.container;
    if (!container) return;

    const select = container.querySelector('#df-timezone-select');
    if (!select) return;

    const currentTz = this._getTimezone();
    const fragment = document.createDocumentFragment();

    // Common abbreviations mapped to IANA zones
    const COMMON_ABBREVIATIONS = [
      { abbr: 'PST', name: 'Pacific Standard Time', iana: 'America/Los_Angeles' },
      { abbr: 'PDT', name: 'Pacific Daylight Time', iana: 'America/Los_Angeles' },
      { abbr: 'MST', name: 'Mountain Standard Time', iana: 'America/Denver' },
      { abbr: 'MDT', name: 'Mountain Daylight Time', iana: 'America/Denver' },
      { abbr: 'CST', name: 'Central Standard Time', iana: 'America/Chicago' },
      { abbr: 'CDT', name: 'Central Daylight Time', iana: 'America/Chicago' },
      { abbr: 'EST', name: 'Eastern Standard Time', iana: 'America/New_York' },
      { abbr: 'EDT', name: 'Eastern Daylight Time', iana: 'America/New_York' },
      { abbr: 'AST', name: 'Atlantic Standard Time', iana: 'America/Halifax' },
      { abbr: 'ADT', name: 'Atlantic Daylight Time', iana: 'America/Halifax' },
      { abbr: 'GMT', name: 'Greenwich Mean Time', iana: 'GMT' },
      { abbr: 'UTC', name: 'Coordinated Universal Time', iana: 'UTC' },
      { abbr: 'BST', name: 'British Summer Time', iana: 'Europe/London' },
      { abbr: 'CET', name: 'Central European Time', iana: 'Europe/Paris' },
      { abbr: 'CEST', name: 'Central European Summer Time', iana: 'Europe/Paris' },
      { abbr: 'EET', name: 'Eastern European Time', iana: 'Europe/Athens' },
      { abbr: 'EEST', name: 'Eastern European Summer Time', iana: 'Europe/Athens' },
      { abbr: 'JST', name: 'Japan Standard Time', iana: 'Asia/Tokyo' },
      { abbr: 'KST', name: 'Korea Standard Time', iana: 'Asia/Seoul' },
      { abbr: 'IST', name: 'India Standard Time', iana: 'Asia/Kolkata' },
      { abbr: 'HKT', name: 'Hong Kong Time', iana: 'Asia/Hong_Kong' },
      { abbr: 'SGT', name: 'Singapore Time', iana: 'Asia/Singapore' },
      { abbr: 'AEST', name: 'Australian Eastern Standard Time', iana: 'Australia/Sydney' },
      { abbr: 'AEDT', name: 'Australian Eastern Daylight Time', iana: 'Australia/Sydney' },
      { abbr: 'NZST', name: 'New Zealand Standard Time', iana: 'Pacific/Auckland' },
      { abbr: 'NZDT', name: 'New Zealand Daylight Time', iana: 'Pacific/Auckland' },
    ];

    // Add Common Abbreviations optgroup
    const abbrGroup = document.createElement('optgroup');
    abbrGroup.label = 'Common Abbreviations';
    COMMON_ABBREVIATIONS.forEach(({ abbr, name, iana }) => {
      const option = document.createElement('option');
      option.value = iana;
      option.textContent = `${abbr} — ${name}`;
      option.dataset.search = `${abbr} ${name} ${iana}`.toLowerCase();
      if (iana === currentTz) {
        option.selected = true;
      }
      abbrGroup.appendChild(option);
    });
    fragment.appendChild(abbrGroup);

    // Group IANA zones by region
    const regions = {};
    IANA_TIMEZONES.forEach(tz => {
      const region = tz.split('/')[0];
      if (!regions[region]) regions[region] = [];
      regions[region].push(tz);
    });

    Object.keys(regions).sort().forEach(region => {
      const optgroup = document.createElement('optgroup');
      optgroup.label = region;
      regions[region].forEach(tz => {
        const option = document.createElement('option');
        option.value = tz;
        option.textContent = tz;
        option.dataset.search = tz.toLowerCase();
        if (tz === currentTz) {
          option.selected = true;
        }
        optgroup.appendChild(option);
      });
      fragment.appendChild(optgroup);
    });

    select.appendChild(fragment);

    // If currentTz is a specific zone, select it
    if (currentTz !== 'local') {
      select.value = currentTz;
    }
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
    const now = this._getDateTimeNow();

    this._tokenTable = new LithiumTable({
      container: tableContainer,
      navigatorContainer: navContainer,
      tablePath: 'profile-manager/luxon-tokens',
      lookupKeyIdx: 16,
      cssPrefix: 'profile-luxon-tokens',
      storageKey: 'profile_luxon_tokens',
      app: null,
      readonly: true,
      localSearch: true,
      localSearchFields: ['token', 'description', 'group'],
      onRefresh: () => this._refreshTokenTable(),
    });

    await this._tokenTable.init();
    this._tokenTable.loadStaticData(buildTokenData(sample, now), { autoSelect: false });
  }

  /**
   * Refresh the token table with updated sample/current values
   */
  _refreshTokenTable() {
    if (!this._tokenTable?.table) return;
    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();
    this._tokenTable.loadStaticData(buildTokenData(sample, now), { autoSelect: false });
  }

  /**
   * Render current time previews for format tables
   */
  _renderCurrentPreviews() {
    const container = this.container;
    if (!container) return;

    const now = this._getDateTimeNow();

    // Update the current datetime display at top
    const currentDisplay = container.querySelector('#df-current-datetime');
    if (currentDisplay) {
      currentDisplay.textContent = now.toFormat("yyyy-MM-dd'T'HH:mm:ss");
    }

    // Format tables
    container.querySelectorAll('.df-current').forEach(el => {
      const format = el.dataset.format;
      if (format) {
        try {
          el.textContent = now.toFormat(format);
        } catch (_e) {
          el.textContent = 'Invalid';
        }
      }
    });
  }

  /**
   * Render example previews for format tables
   */
  _renderExamplePreviews() {
    const container = this.container;
    if (!container) return;

    const sample = this._getSampleDateTime();

    // Format tables
    container.querySelectorAll('.df-example').forEach(el => {
      const row = el.closest('.df-format-item');
      if (!row) return;
      const input = row.querySelector('.df-format-input');
      if (!input) return;
      try {
        el.textContent = sample.toFormat(input.value);
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
    const now = this._getDateTimeNow();

    try {
      if (exampleEl) exampleEl.textContent = sample.toFormat(format);
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
    const now = this._getDateTimeNow();

    const item = document.createElement('tr');
    item.className = 'df-format-item df-custom-item';
    item.innerHTML = `
      <td><input type="text" class="df-format-name-input" value="${this._escapeHtml(name)}" placeholder="Name"></td>
      <td><input type="text" class="form-input df-format-input" value="${this._escapeHtml(format)}" placeholder="Luxon format"></td>
      <td><span class="df-example">${this._safeFormat(sample, format)}</span></td>
      <td><span class="df-current" data-format="${this._escapeHtml(format)}">${this._safeFormat(now, format)}</span></td>
      <td><button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button></td>
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
      if (confirm('Delete this custom format?')) {
        item.remove();
        this._persistCustomFormats();
      }
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

    ['dates', 'times', 'datetime'].forEach(type => {
      const custom = this._gatherCustomFormats(type);
      const settingKey = type === 'datetime' ? 'datetimes.custom' : `${type}.custom`;
      if (Object.keys(custom).length > 0) {
        this.setSetting(settingKey, custom);
      } else {
        this.setSetting(settingKey, undefined);
      }
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

    // Timezone
    const tzSelect = container.querySelector('#df-timezone-select');
    if (tzSelect) {
      const savedTz = this.getSetting('timezone', 'local');
      tzSelect.value = savedTz;
    }

    // Built-in formats
    this._loadBuiltinFormats('dates');
    this._loadBuiltinFormats('times');
    this._loadBuiltinFormats('datetimes');

    // Custom formats
    this._loadCustomFormats('dates');
    this._loadCustomFormats('times');
    this._loadCustomFormats('datetime');

    this._renderAllPreviews();
  }

  /**
   * Load built-in formats from settings service
   */
  _loadBuiltinFormats(type) {
    const container = this.container;
    if (!container) return;

    const builtins = BUILTIN_FORMATS[type];
    if (!builtins) return;

    Object.entries(builtins).forEach(([key, config]) => {
      const input = container.querySelector(`[data-setting="${config.setting}"]`);
      if (input) {
        input.value = this.getSetting(config.setting, config.format);
      }
    });
  }

  /**
   * Load custom formats from settings service
   */
  _loadCustomFormats(type) {
    const container = this.container;
    if (!container) return;

    const listId = type === 'datetime' ? 'df-datetime-custom' : `df-${type}-custom`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    list.innerHTML = '';

    const settingKey = type === 'datetime' ? 'datetimes.custom' : `${type}.custom`;
    const customData = this.getSetting(settingKey, null);
    if (!customData || typeof customData !== 'object') return;

    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();

    Object.entries(customData).forEach(([id, data]) => {
      if (typeof data !== 'object' || !data.format) return;

      const item = document.createElement('tr');
      item.className = 'df-format-item df-custom-item';
      item.dataset.id = id;
      item.innerHTML = `
        <td><input type="text" class="df-format-name-input" value="${this._escapeHtml(data.name || id)}" placeholder="Name"></td>
        <td><input type="text" class="form-input df-format-input" value="${this._escapeHtml(data.format)}" placeholder="Luxon format"></td>
        <td><span class="df-example">${this._safeFormat(sample, data.format)}</span></td>
        <td><span class="df-current" data-format="${this._escapeHtml(data.format)}">${this._safeFormat(now, data.format)}</span></td>
        <td><button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button></td>
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
        if (confirm('Delete this custom format?')) {
          item.remove();
          this._persistCustomFormats();
        }
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

    const listId = type === 'datetime' ? 'df-datetime-custom' : `df-${type}-custom`;
    const list = container?.querySelector(`#${listId}`);
    if (!list) return custom;

    list.querySelectorAll('.df-custom-item').forEach(item => {
      const nameInput = item.querySelector('.df-format-name-input');
      const input = item.querySelector('.df-format-input');
      if (nameInput && input) {
        const id = item.dataset.id || `custom-${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
        custom[id] = {
          name: nameInput.value,
          format: input.value,
        };
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
   * Destroy
   */
  destroy() {
    if (this._flatpickrInstance) {
      this._flatpickrInstance.destroy();
      this._flatpickrInstance = null;
    }
    if (this._tokenTable) {
      this._tokenTable.destroy();
      this._tokenTable = null;
    }
    super.destroy();
  }
}

export default DateFormatsPage;
