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
    let example, current;
    try { example = sample.toFormat(t.token); } catch (_e) { example = '-'; }
    try { current = now.toFormat(t.token); } catch (_e) { current = '-'; }
    return {
      id: t.token,  // Add id field for primary key
      token: t.token,
      description: t.desc,
      group: t.group,
      example,
      current,
    };
  });
}

class TimezonePicker {
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
    this.clearButton = this.dropdown.querySelector('.df-timezone-filter-clear');
    this.listContainer = this.dropdown.querySelector('.df-timezone-list');

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
      startWidth = this.dropdown.offsetWidth;
      startHeight = this.dropdown.offsetHeight;

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
    if (this.isOpen) {
      this.close();
    } else {
      this.open();
    }
  }

  open() {
    if (this.isOpen) return;

    // Close other popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    this.isOpen = true;
    this.container.classList.add('open');

    // Prevent page scrolling while popup is open
    document.body.style.overflow = 'hidden';

    // Force show dropdown
    this.dropdown.style.display = 'block';

    // Position dropdown
    const rect = this.container.getBoundingClientRect();
    this.dropdown.style.top = `${rect.bottom + 2}px`;
    this.dropdown.style.left = `${rect.left}px`;
    this.dropdown.style.width = this.popupWidth ? `${this.popupWidth}px` : `${rect.width}px`;
    this.dropdown.style.height = this.popupHeight ? `${this.popupHeight}px` : '';
    this.dropdown.style.transformOrigin = 'top left';

    // Setup OverlayScrollbars now that element is visible
    if (window.OverlayScrollbars && !this.overlayScrollbar) {
      try {
        this.overlayScrollbar = window.OverlayScrollbars(this.listContainer, {
          scrollbars: {
            theme: 'os-theme-light',
            autoHide: 'scroll',
            autoHideDelay: 800,
          },
        });
      } catch (e) {
        // Fallback to native scrolling
        this.listContainer.style.overflowY = 'auto';
        this.overlayScrollbar = null;
      }
    } else if (!this.overlayScrollbar) {
      // Fallback to native scrolling if OverlayScrollbars not available
      this.listContainer.style.overflowY = 'auto';
    }

    // Preserve filter value, just refresh the display
    this.filterTimezones();

    const tzName = typeof this.selectedTimezone === 'object' ? this.selectedTimezone.name : this.selectedTimezone;
    if (tzName !== 'local') {
      // Scroll to the selected timezone
      const selectedItem = this.listContainer.querySelector(`[data-value="${tzName}"]`);
      if (selectedItem) {
        selectedItem.scrollIntoView({ block: 'nearest' });
      }
    }



    // Show with animation (like popup system)
    setTimeout(() => this.dropdown.classList.add('visible'), 10);

    // Focus filter
    setTimeout(() => this.filterInput.focus(), 100);

    // Check for other popups and close if they appear
    this._checkInterval = setInterval(() => {
      const allPopups = document.querySelectorAll('.manager-ui-popup.visible, .flatpickr-calendar');
      const hasOtherPopups = Array.from(allPopups).some(popup => popup !== this.dropdown);
      if (hasOtherPopups) {
        this.close();
      }
    }, 100);
  }

  close() {
    if (!this.isOpen) return;

    this.isOpen = false;
    this.container.classList.remove('open');
    this.dropdown.classList.remove('visible');
    // Wait for animation to complete before hiding
    setTimeout(() => {
      this.dropdown.style.display = 'none';
      // Restore page scrolling
      document.body.style.overflow = '';
      // Clear the check interval
      if (this._checkInterval) {
        clearInterval(this._checkInterval);
        this._checkInterval = null;
      }
    }, 350); // Match transition duration
    // Keep dropdown in DOM but hidden for reuse
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
      } catch (e) {
        displayEl.textContent = this.selectedTimezone;
      }
    }
  }

  /**
   * Filter and display timezones based on current filter input
   */
  filterTimezones() {
    const filter = this.filterInput.value.toLowerCase();
    const listContainer = this.listContainer;

    // Clear existing items
    listContainer.innerHTML = '';

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
      const browserTz = this._getBrowserTimezone();
      const item = this._createTimezoneItem('Browser Timezone', 'local');
      browserSection.appendChild(item);
    }

    if (browserSection.children.length > 1) { // Has title + at least one item
      listContainer.appendChild(browserSection);
    }

    // Create abbreviation section
    if (Object.keys(abbreviations).length > 0) {
      const abbrSection = document.createElement('div');
      abbrSection.className = 'df-timezone-group';
      abbrSection.innerHTML = '<div class="df-timezone-group-header">Abbreviations</div>';

      Object.keys(abbreviations).sort().forEach(tz => {
        if (tz.toLowerCase().includes(filter)) {
          const item = this._createTimezoneItem(tz, tz);
          abbrSection.appendChild(item);
        }
      });

      if (abbrSection.children.length > 1) { // Has title + at least one item
        listContainer.appendChild(abbrSection);
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
        listContainer.appendChild(section);
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
    let timeString = '';
    try {
      const now = DateTime.now().setZone(value);
      timeString = now.toFormat('HH:mm');
    } catch (e) {
      timeString = '--:--';
    }

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
    // Clear any check interval
    if (this._checkInterval) {
      clearInterval(this._checkInterval);
      this._checkInterval = null;
    }
    if (this.dropdown) {
      this.dropdown.remove();
    }
    if (this.overlayScrollbar) {
      this.overlayScrollbar.destroy();
    }
  }
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
    this._timezonePicker = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._ensureSectionNamed();
    this._setupEventListeners();
    this._initTimezonePicker();
    this._initFlatpickr();
    await this._initTokenTable();
    await this.loadData();
    this._startCurrentTimeUpdates();

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
  _createFlatpickrInstance(input, wrapper) {
    // Parse current input value for defaultDate
    let defaultDate = DEFAULT_SAMPLE;
    try {
      const parsed = DateTime.fromISO(input.value);
      if (parsed.isValid) defaultDate = parsed.toJSDate();
    } catch (_e) {
      // ignore
    }

    return flatpickr(input, {
      enableTime: true,
      enableSeconds: true,
      dateFormat: 'Y-m-d\\TH:i:S',
      time_24hr: true,
      allowInput: true,
      clickOpens: false,
      defaultDate,
      disableMobile: true,
      onChange: (selectedDates) => {
        if (selectedDates.length > 0) {
          const dt = DateTime.fromJSDate(selectedDates[0]);
          input.value = dt.toFormat("yyyy-MM-dd'T'HH:mm:ss");
          this.setSetting('sample', input.value);
          this._renderAllPreviews();
        }
      },
      onOpen: () => {
        // Move the calendar to our wrapper when it opens and position it
        const moveCalendar = () => {
          const calendar = document.querySelector('.flatpickr-calendar');
          if (calendar && !wrapper.contains(calendar)) {
            calendar.style.position = 'absolute';
            calendar.style.top = '0';
            calendar.style.left = '0';
            calendar.style.display = 'block';
            calendar.style.visibility = 'visible';
            calendar.style.opacity = '1';
            // Add CSS override to prevent Flatpickr from hiding it
            calendar.style.setProperty('display', 'block', 'important');
            calendar.style.setProperty('visibility', 'visible', 'important');
            wrapper.appendChild(calendar);
          } else if (!calendar) {
            // If calendar not found yet, try again
            setTimeout(moveCalendar, 10);
          }
        };
        moveCalendar();
      },
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

    btn.addEventListener('click', () => {
      // Toggle: if already open, close it
      const existing = document.querySelector('.flatpickr-popup-wrapper.visible');
      if (existing) {
        existing.classList.remove('visible');
        // Destroy instance and remove wrapper after animation completes
        setTimeout(() => {
          if (this._flatpickrInstance) {
            this._flatpickrInstance.destroy();
            this._flatpickrInstance = null;
          }
          const calendar = existing.querySelector('.flatpickr-calendar');
          if (calendar) {
            calendar.remove();
          }
          existing.remove();
        }, 350);
        return;
      }

      // Close other popups first
      document.dispatchEvent(new CustomEvent('close-all-popups'));

      // Create a wrapper that applies the manager popup styling + animation
      const wrapper = document.createElement('div');
      wrapper.className = 'manager-ui-popup manager-header-popup flatpickr-popup-wrapper';
      wrapper.style.minWidth = '313px';
      wrapper.style.minHeight = '341px';

      // Create new FlatPickr instance
      this._flatpickrInstance = this._createFlatpickrInstance(input, wrapper);

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

      // Trigger scale animation and open Flatpickr
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          wrapper.classList.add('visible');
          // Open Flatpickr after animation starts
          this._flatpickrInstance.open();
        });
      });

      // Close on outside click / escape
      const closePicker = (e) => {
        if (!wrapper.contains(e.target) && !btn.contains(e.target)) {
          wrapper.classList.remove('visible');
          // Destroy instance and remove wrapper after animation
          setTimeout(() => {
            if (this._flatpickrInstance) {
              this._flatpickrInstance.destroy();
              this._flatpickrInstance = null;
            }
            const calendar = wrapper.querySelector('.flatpickr-calendar');
            if (calendar) {
              calendar.remove();
            }
            wrapper.remove();
          }, 350);
          document.removeEventListener('click', closePicker);
          document.removeEventListener('keydown', escPicker);
          document.removeEventListener('close-all-popups', closePicker);
          if (checkInterval) {
            clearInterval(checkInterval);
            checkInterval = null;
          }
        }
      };

      const escPicker = (e) => {
        if (e.key === 'Escape') {
          wrapper.classList.remove('visible');
          // Destroy instance and remove wrapper after animation
          setTimeout(() => {
            if (this._flatpickrInstance) {
              this._flatpickrInstance.destroy();
              this._flatpickrInstance = null;
            }
            const calendar = wrapper.querySelector('.flatpickr-calendar');
            if (calendar) {
              calendar.remove();
            }
            wrapper.remove();
          }, 350);
          document.removeEventListener('click', closePicker);
          document.removeEventListener('keydown', escPicker);
          document.removeEventListener('close-all-popups', closePicker);
        }
      };

      // Check for other popups
      let checkInterval = setInterval(() => {
        const otherPopups = document.querySelectorAll('.df-timezone-dropdown.visible, .manager-ui-popup.visible:not(.flatpickr-popup-wrapper)');
        if (otherPopups.length > 0) {
          closePicker();
        }
      }, 100);

      setTimeout(() => {
        document.addEventListener('click', closePicker);
        document.addEventListener('keydown', escPicker);
        document.addEventListener('close-all-popups', closePicker);
      }, 0);
    });
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
      primaryKeyField: ['token'],
      localSearch: true,
      localSearchFields: ['token', 'description', 'group'],
      useOverlayScrollbars: true,
      onRefresh: () => this._refreshTokenTable(),
    });

    await this._tokenTable.init();
    this._tokenTable.loadStaticData(buildTokenData(sample, now), { autoSelect: true });

  }

  /**
   * Refresh the token table with updated sample/current values
   */
  _refreshTokenTable() {
    if (!this._tokenTable?.table) return;
    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();
    this._tokenTable.loadStaticData(buildTokenData(sample, now), { autoSelect: true });
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
  }

  /**
   * Add custom format
   */
  _addCustomFormat(type) {
    const container = this.container;
    if (!container) return;

    // Copy from the last custom row, or use defaults
    let name = 'Custom Format';
    let format = type === 'dates' ? 'yyyy-MM-dd' : type === 'times' ? 'HH:mm' : 'yyyy-MM-dd HH:mm:ss';

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
   * Destroy
   */
  destroy() {
    if (this._currentUpdateInterval) {
      clearInterval(this._currentUpdateInterval);
      this._currentUpdateInterval = null;
    }
    if (this._flatpickrInstance) {
      this._flatpickrInstance.destroy();
      this._flatpickrInstance = null;
    }
    // Close any open flatpickr popup
    const existing = document.querySelector('.flatpickr-popup-wrapper.visible');
    if (existing) {
      existing.classList.remove('visible');
      setTimeout(() => {
        const calendar = existing.querySelector('.flatpickr-calendar');
        if (calendar) calendar.remove();
        existing.remove();
      }, 350);
    }
    if (this._tokenTable) {
      this._tokenTable.destroy();
      this._tokenTable = null;
    }
    if (this._timezonePicker) {
      this._timezonePicker.destroy();
      this._timezonePicker = null;
    }
    super.destroy();
  }
}

export default DateFormatsPage;
