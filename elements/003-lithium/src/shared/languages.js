/**
 * Language Support Module
 * 
 * Provides locale definitions, country-to-locale mappings, and smart language
 * detection based on browser preferences and IP geolocation.
 * 
 * Uses country-flag-icons library for SVG flag rendering.
 */

/**
 * Country to locale mapping for IP-based detection
 * @type {Object<string, string>}
 */
export const countryToLocale = {
  // English variants
  'US': 'en-US',
  'GB': 'en-GB',
  'CA': 'en-CA',
  'AU': 'en-AU',
  'IN': 'en-IN',
  'NZ': 'en-NZ',
  'ZA': 'en-ZA',
  'IE': 'en-IE',
  // French variants
  'FR': 'fr-FR',
  'BE': 'fr-BE',
  'CH': 'fr-CH',
  // Spanish variants
  'ES': 'es-ES',
  'MX': 'es-MX',
  'AR': 'es-AR',
  'CL': 'es-CL',
  'CO': 'es-CO',
  // German variants
  'DE': 'de-DE',
  'AT': 'de-AT',
  // Portuguese variants
  'BR': 'pt-BR',
  'PT': 'pt-PT',
  // Other major languages
  'IT': 'it-IT',
  'JP': 'ja-JP',
  'KR': 'ko-KR',
  'CN': 'zh-CN',
  'TW': 'zh-TW',
  'HK': 'zh-HK',
  'RU': 'ru-RU',
  'NL': 'nl-NL',
  'SE': 'sv-SE',
  'NO': 'nb-NO',
  'DK': 'da-DK',
  'FI': 'fi-FI',
  'PL': 'pl-PL',
  'TR': 'tr-TR',
  'AE': 'ar-AE',
  'SA': 'ar-SA',
  'IL': 'he-IL',
  'TH': 'th-TH',
  'VN': 'vi-VN',
  'ID': 'id-ID',
  'MY': 'ms-MY',
  'PH': 'fil-PH',
  'UA': 'uk-UA',
  'CZ': 'cs-CZ',
  'HU': 'hu-HU',
  'RO': 'ro-RO',
  'GR': 'el-GR',
};

/**
 * Supported locales list (47 common locales)
 * @type {string[]}
 */
export const supportedLocales = [
  // English variants (8)
  'en-US', 'en-CA', 'en-GB', 'en-AU', 'en-IN', 'en-NZ', 'en-ZA', 'en-IE',
  // French variants (4)
  'fr-CA', 'fr-FR', 'fr-BE', 'fr-CH',
  // Spanish variants (6)
  'es-ES', 'es-MX', 'es-AR', 'es-CL', 'es-CO', 'es-US',
  // German variants (2)
  'de-DE', 'de-AT',
  // Portuguese variants (2)
  'pt-BR', 'pt-PT',
  // Other major languages
  'it-IT', 'ja-JP', 'ko-KR', 'zh-CN', 'zh-TW', 'zh-HK', 'ru-RU',
  'nl-NL', 'sv-SE', 'nb-NO', 'da-DK', 'fi-FI', 'pl-PL', 'tr-TR',
  'ar-AE', 'ar-SA', 'he-IL', 'th-TH', 'vi-VN', 'id-ID', 'ms-MY',
  'fil-PH', 'uk-UA', 'cs-CZ', 'hu-HU', 'ro-RO', 'el-GR',
];

/**
 * Country code mapping for locales
 * Maps locale codes to ISO 3166-1 alpha-2 country codes for flag display
 * @type {Object<string, string>}
 */
export const localeToCountryCode = {
  'en-US': 'US',
  'en-CA': 'CA',
  'en-GB': 'GB',
  'en-AU': 'AU',
  'en-IN': 'IN',
  'en-NZ': 'NZ',
  'en-ZA': 'ZA',
  'en-IE': 'IE',
  'fr-CA': 'CA',
  'fr-FR': 'FR',
  'fr-BE': 'BE',
  'fr-CH': 'CH',
  'es-ES': 'ES',
  'es-MX': 'MX',
  'es-AR': 'AR',
  'es-CL': 'CL',
  'es-CO': 'CO',
  'es-US': 'US',
  'de-DE': 'DE',
  'de-AT': 'AT',
  'pt-BR': 'BR',
  'pt-PT': 'PT',
  'it-IT': 'IT',
  'ja-JP': 'JP',
  'ko-KR': 'KR',
  'zh-CN': 'CN',
  'zh-TW': 'TW',
  'zh-HK': 'HK',
  'ru-RU': 'RU',
  'nl-NL': 'NL',
  'sv-SE': 'SE',
  'nb-NO': 'NO',
  'da-DK': 'DK',
  'fi-FI': 'FI',
  'pl-PL': 'PL',
  'tr-TR': 'TR',
  'ar-AE': 'AE',
  'ar-SA': 'SA',
  'he-IL': 'IL',
  'th-TH': 'TH',
  'vi-VN': 'VN',
  'id-ID': 'ID',
  'ms-MY': 'MY',
  'fil-PH': 'PH',
  'uk-UA': 'UA',
  'cs-CZ': 'CZ',
  'hu-HU': 'HU',
  'ro-RO': 'RO',
  'el-GR': 'GR',
};

/**
 * Get the country code for a locale
 * @param {string} locale - Locale code (e.g., 'en-US')
 * @returns {string} Country code (e.g., 'US') or 'US' as fallback
 */
export function getCountryCode(locale) {
  return localeToCountryCode[locale] || 'US';
}

/**
 * Get language name for a locale in English (without country)
 * @param {string} locale - Locale code (e.g., 'en-US')
 * @returns {string} Language name (e.g., 'English')
 */
export function getLanguageName(locale) {
  try {
    const langCode = locale.split('-')[0];
    return new Intl.DisplayNames(['en'], { type: 'language' }).of(langCode);
  } catch (e) {
    // Fallback to language code
    return locale.split('-')[0];
  }
}

/**
 * Get country name for a locale in English
 * @param {string} locale - Locale code (e.g., 'en-US')
 * @returns {string} Country name (e.g., 'United States') or empty string if no country
 */
export function getCountryName(locale) {
  try {
    const region = locale.split('-')[1];
    if (!region) return '';
    return new Intl.DisplayNames(['en'], { type: 'region' }).of(region);
  } catch (e) {
    // Fallback to region code
    const region = locale.split('-')[1];
    return region || '';
  }
}

/**
 * Get display name for a locale in English
 * @param {string} locale - Locale code (e.g., 'en-US')
 * @returns {string} Display name (e.g., 'English (United States)')
 * @deprecated Use getLanguageName and getCountryName separately
 */
export function getLocaleDisplayName(locale) {
  try {
    const langPart = new Intl.DisplayNames(['en'], { type: 'language' }).of(locale);
    const region = locale.split('-')[1];
    if (!region) return langPart;
    const regionPart = new Intl.DisplayNames(['en'], { type: 'region' }).of(region);
    return `${langPart} (${regionPart})`;
  } catch (e) {
    // Fallback to simple formatting
    const [lang, region] = locale.split('-');
    return `${lang.toUpperCase()} (${region})`;
  }
}

/**
 * Get native display name for a locale
 * @param {string} locale - Locale code (e.g., 'fr-CA')
 * @returns {string} Native display name (e.g., 'Français (Canada)')
 */
export function getLocaleNativeName(locale) {
  try {
    const langPart = new Intl.DisplayNames([locale], { type: 'language' }).of(locale);
    const region = locale.split('-')[1];
    if (!region) return langPart;
    const regionPart = new Intl.DisplayNames([locale], { type: 'region' }).of(region);
    return `${langPart} (${regionPart})`;
  } catch (e) {
    return getLocaleDisplayName(locale);
  }
}

/**
 * Get all supported language data as an array of objects
 * @returns {Array<{locale: string, countryCode: string, language: string, country: string, nativeName: string}>}
 */
export function getLanguageData() {
  return supportedLocales.map(locale => ({
    locale,
    countryCode: getCountryCode(locale),
    language: getLanguageName(locale),
    country: getCountryName(locale),
    nativeName: getLocaleNativeName(locale),
  }));
}

/**
 * Get the best guess locale based on browser preference and IP geolocation
 * Priority order:
 * 1. Saved user choice (localStorage)
 * 2. Browser preference (navigator.languages or navigator.language)
 * 3. IP geolocation (if available)
 * 4. Fallback to 'en-US'
 * 
 * @param {Object} options - Options object
 * @param {string} options.ipinfoToken - Optional ipinfo.io API token
 * @returns {Promise<string>} The best guess locale
 */
export async function getBestGuessLocale(options = {}) {
  const { ipinfoToken = null } = options;

  // 1. Check saved preference first
  const savedLocale = localStorage.getItem('lithium_user_locale');
  if (savedLocale && supportedLocales.includes(savedLocale)) {
    return savedLocale;
  }

  // 2. Check browser preference
  const browserLocale = _getBrowserLocale();
  if (browserLocale) return browserLocale;

  // 3. IP geolocation fallback
  if (ipinfoToken) {
    const ipLocale = await _getIpGeolocationLocale(ipinfoToken);
    if (ipLocale) return ipLocale;
  }

  // 4. Ultimate fallback
  return 'en-US';
}

/**
 * Get locale from browser preferences
 * @returns {string|null} Browser locale or null if not found
 * @private
 */
function _getBrowserLocale() {
  const browserLang = navigator.languages?.[0] || navigator.language;
  if (!browserLang) return null;

  // Try exact match first
  if (supportedLocales.includes(browserLang)) {
    return browserLang;
  }
  
  // Try base language match (e.g., 'fr' from 'fr-CA')
  const baseLang = browserLang.split('-')[0];
  return supportedLocales.find(l => l.startsWith(`${baseLang}-`)) || null;
}

/**
 * Get locale from IP geolocation
 * @param {string} ipinfoToken - API token for ipinfo.io
 * @returns {Promise<string|null>} Locale from IP or null
 * @private
 */
async function _getIpGeolocationLocale(ipinfoToken) {
  try {
    const response = await fetch(`https://ipinfo.io/json?token=${ipinfoToken}`);
    if (!response.ok) return null;
    
    const data = await response.json();
    return _resolveLocaleFromCountry(data.country, data.region);
  } catch (e) {
    // Network/VPN fail silently
    return null;
  }
}

/**
 * Resolve locale from country and region
 * @param {string} country - Country code
 * @param {string} region - Region/state code
 * @returns {string|null} Resolved locale or null
 * @private
 */
function _resolveLocaleFromCountry(country, region) {
  // Special case for Canada - check region
  if (country === 'CA' && region) {
    const caLocale = (region === 'QC' || region.includes('Quebec')) ? 'fr-CA' : 'en-CA';
    return supportedLocales.includes(caLocale) ? caLocale : null;
  }

  // General country mapping
  const ipLocale = countryToLocale[country];
  return (ipLocale && supportedLocales.includes(ipLocale)) ? ipLocale : null;
}

/**
 * Get all languages sorted by language name then country name
 * @returns {Array<{locale: string, countryCode: string, language: string, country: string, nativeName: string}>}
 */
export function getSortedLanguageList() {
  const allLanguages = getLanguageData();

  allLanguages.sort((a, b) => {
    const langCompare = a.language.localeCompare(b.language);
    if (langCompare !== 0) return langCompare;
    return a.country.localeCompare(b.country);
  });

  return allLanguages;
}

/**
 * Save user locale preference to localStorage
 * @param {string} locale - The selected locale
 */
export function saveLocalePreference(locale) {
  try {
    localStorage.setItem('lithium_user_locale', locale);
  } catch (e) {
    console.warn('[Languages] Could not save locale preference:', e);
  }
}

/**
 * Get saved locale preference from localStorage
 * @returns {string|null} The saved locale or null
 */
export function getSavedLocale() {
  try {
    return localStorage.getItem('lithium_user_locale');
  } catch (e) {
    return null;
  }
}
