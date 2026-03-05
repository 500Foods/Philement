/**
 * Config - Application configuration loader
 * 
 * Loads and provides access to the lithium.json configuration file.
 * Falls back to sensible defaults if config is missing or partial.
 */

// Default configuration values
const DEFAULT_CONFIG = {
  server: {
    url: 'http://localhost:8080',
    api_prefix: '/api',
    websocket_url: 'ws://localhost:8080/ws',
  },
  auth: {
    api_key: '',
    default_database: 'Demo_PG',
    session_timeout_minutes: 30,
  },
  app: {
    name: 'Lithium',
    tagline: 'A Philement Project',
    version: '1.0.0',
    logo: '/assets/images/logo-li.svg',
    default_theme: 'dark',
    default_language: 'en-US',
  },
  features: {
    offline_mode: true,
    install_prompt: true,
    debug_logging: false,
  },
};

let cachedConfig = null;

/**
 * Deep merge two objects
 * @param {Object} target - Target object
 * @param {Object} source - Source object
 * @returns {Object} Merged object
 */
function deepMerge(target, source) {
  const result = { ...target };
  
  for (const key in source) {
    if (source[key] && typeof source[key] === 'object' && !Array.isArray(source[key])) {
      result[key] = deepMerge(result[key] || {}, source[key]);
    } else {
      result[key] = source[key];
    }
  }
  
  return result;
}

/**
 * Load configuration from server
 * @returns {Promise<Object>} Configuration object
 */
export async function loadConfig() {
  // Return cached config if available
  if (cachedConfig) {
    return cachedConfig;
  }

  try {
    const response = await fetch('/config/lithium.json');
    
    if (!response.ok) {
      console.warn('Config file not found, using defaults');
      cachedConfig = DEFAULT_CONFIG;
      return cachedConfig;
    }

    const config = await response.json();
    
    // Merge with defaults
    cachedConfig = deepMerge(DEFAULT_CONFIG, config);
    
    console.log('Configuration loaded successfully');
    return cachedConfig;
  } catch (error) {
    console.warn('Failed to load config, using defaults:', error.message);
    cachedConfig = DEFAULT_CONFIG;
    return cachedConfig;
  }
}

/**
 * Get current configuration
 * @returns {Object} Configuration object (may be defaults if not loaded)
 */
export function getConfig() {
  if (!cachedConfig) {
    console.warn('Config not loaded yet, returning defaults');
    return DEFAULT_CONFIG;
  }
  return cachedConfig;
}

/**
 * Get a specific config value by path
 * @param {string} path - Dot-notation path (e.g., 'server.url')
 * @param {*} defaultValue - Default value if path not found
 * @returns {*} Config value or default
 */
export function getConfigValue(path, defaultValue = null) {
  const config = getConfig();
  const parts = path.split('.');
  
  let current = config;
  for (const part of parts) {
    if (current === null || current === undefined || !(part in current)) {
      return defaultValue;
    }
    current = current[part];
  }
  
  return current !== undefined ? current : defaultValue;
}

/**
 * Clear cached configuration (useful for testing)
 */
export function clearConfig() {
  cachedConfig = null;
}

/**
 * Check if config has been loaded
 * @returns {boolean}
 */
export function isConfigLoaded() {
  return cachedConfig !== null;
}

// Default export
export default {
  load: loadConfig,
  get: getConfig,
  getValue: getConfigValue,
  clear: clearConfig,
  isLoaded: isConfigLoaded,
};
