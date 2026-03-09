/**
 * Icons - Font Awesome icon system with configurable sets and lookup integration
 *
 * Provides a mutation observer that replaces <fa> tags with proper Font Awesome
 * icon elements based on configuration. Supports:
 * - Direct icons: <fa fa-palette> → <i class="fa-duotone fa-thin fa-palette">
 * - Lookup-based icons: <fa Status> → <i class="fa-duotone fa-thin fa-flag">
 * - Preserved classes: fa-fw, fa-spin, fa-pulse, etc. are retained
 *
 * QueryRef 054 (Icons lookup) provides server-side icon overrides.
 */

import { getConfigValue } from './config.js';
import { eventBus, Events } from './event-bus.js';
import { getLookupCategory } from '../shared/lookups.js';

// Module state
let observer = null;
let iconConfig = null;
let iconLookups = null;
let isInitialized = false;

// Font Awesome class mappings
const FA_PREFIX_MAP = {
  'solid': 'fas',
  'regular': 'far',
  'light': 'fal',
  'thin': 'fat',
  'duotone': 'fad',
  'sharp-solid': 'fass',
  'sharp-regular': 'fasr',
  'sharp-light': 'fasl',
  'sharp-thin': 'fast',
  'brands': 'fab'
};

// Classes that should be preserved from the original fa-* classes
const PRESERVED_CLASSES = ['fa-fw', 'fa-spin', 'fa-pulse', 'fa-border', 'fa-pull-left', 'fa-pull-right'];

// Size classes
const SIZE_CLASSES = ['fa-xs', 'fa-sm', 'fa-lg', 'fa-xl', 'fa-2x', 'fa-2xs', 'fa-3x', 'fa-4x', 'fa-5x', 'fa-6x', 'fa-7x', 'fa-8x', 'fa-9x', 'fa-10x'];

// Animation classes
const ANIMATION_CLASSES = ['fa-spin', 'fa-pulse', 'fa-fade', 'fa-beat', 'fa-beat-fade', 'fa-bounce', 'fa-flip', 'fa-shake'];

// Utility classes (all preserved modifiers)
const UTILITY_CLASSES = [
  ...PRESERVED_CLASSES,
  ...SIZE_CLASSES,
  ...ANIMATION_CLASSES,
  'fa-rotate-90', 'fa-rotate-180', 'fa-rotate-270',
  'fa-flip-horizontal', 'fa-flip-vertical', 'fa-flip-both',
  'fa-stack', 'fa-stack-1x', 'fa-stack-2x', 'fa-inverse'
];

/**
 * Get icon configuration from config file
 * @returns {Object} Icon configuration
 */
function getIconConfig() {
  if (!iconConfig) {
    iconConfig = {
      set: getConfigValue('icons.set', 'solid'),
      weight: getConfigValue('icons.weight', ''),
      prefix: getConfigValue('icons.prefix', ''),
      fallback: getConfigValue('icons.fallback', 'solid')
    };
  }
  return iconConfig;
}

/**
 * Build the Font Awesome prefix based on configuration
 * @returns {string} The Font Awesome class prefix (e.g., 'fad', 'fas', 'fal')
 */
function buildPrefix() {
  const config = getIconConfig();

  // If explicit prefix is set, use it
  if (config.prefix) {
    return config.prefix;
  }

  // Build prefix from set and weight
  const setPrefix = FA_PREFIX_MAP[config.set] || 'fas';

  // For duotone with weight, we use the base duotone prefix
  // Font Awesome 6 Pro handles weight via separate classes
  return setPrefix;
}

/**
 * Build weight class if applicable
 * @returns {string|null} Weight class or null
 */
function buildWeightClass() {
  const config = getIconConfig();

  // Only certain sets support weight classes
  if (config.weight && ['duotone', 'sharp-solid', 'sharp-regular', 'sharp-light', 'sharp-thin'].includes(config.set)) {
    return `fa-${config.weight}`;
  }

  return null;
}

/**
 * Parse icon name from fa-* class list
 * @param {string[]} classes - Array of class names
 * @returns {string|null} Icon name or null
 */
function parseIconName(classes) {
  for (const cls of classes) {
    if (cls.startsWith('fa-') && !UTILITY_CLASSES.includes(cls)) {
      // This is likely the icon name
      return cls.slice(3); // Remove 'fa-' prefix
    }
  }
  return null;
}

/**
 * Extract utility classes from class list
 * @param {string[]} classes - Array of class names
 * @returns {string[]} Array of utility classes
 */
function extractUtilityClasses(classes) {
  return classes.filter(cls => UTILITY_CLASSES.includes(cls));
}

/**
 * Get icon from lookups by name
 * @param {string} name - Icon lookup name (e.g., 'Status')
 * @returns {Object|null} Icon data with icon class, or null
 */
function getLookupIcon(name) {
  if (!iconLookups) {
    iconLookups = getLookupCategory('icons');
  }

  if (!iconLookups) {
    return null;
  }

  // Lookups can be an array or object depending on structure
  const lookupArray = Array.isArray(iconLookups) ? iconLookups : Object.values(iconLookups);

  // Find the lookup entry by value/lookup_key
  const lookup = lookupArray.find(item => {
    const key = item.lookup_value || item.value || item.name || item.lookup_key;
    return key === name;
  });

  if (!lookup) {
    return null;
  }

  // Parse the icon HTML from value_txt or icon field
  const iconHtml = lookup.value_txt || lookup.icon || lookup.html;

  if (!iconHtml) {
    return null;
  }

  // Extract icon class from HTML like "<i class='fa fa-fw fa-flag'></i>"
  const match = iconHtml.match(/class=['"]([^'"]*)['"]/);
  if (match) {
    const classes = match[1].split(/\s+/);
    const iconName = parseIconName(classes);
    const utilities = extractUtilityClasses(classes);

    return {
      icon: iconName,
      utilities: utilities
    };
  }

  return null;
}

/**
 * Process a single <fa> element and replace it with an <i> element
 * @param {HTMLElement} faElement - The <fa> element to process
 */
function processIconElement(faElement) {
  const attributes = Array.from(faElement.attributes);
  const classes = [];
  let lookupName = null;
  let iconName = null;

  // Parse attributes
  for (const attr of attributes) {
    const name = attr.name;
    const value = attr.value;

    if (name === 'class') {
      // Split class attribute
      classes.push(...value.split(/\s+/).filter(Boolean));
    } else if (name === 'icon') {
      // Icon name specified as attribute value
      iconName = value;
    } else if (name.startsWith('fa-')) {
      // fa-* class as attribute
      classes.push(name);
    } else if (name.startsWith('data-')) {
      // Data attributes - preserve them
      continue;
    } else {
      // Check if it's an icon name or lookup reference
      const cleanName = name.toLowerCase();

      // Check if it's a Font Awesome icon class pattern
      if (cleanName.startsWith('fa')) {
        classes.push(cleanName);
      } else {
        // Treat as lookup name (like "Status")
        lookupName = name;
      }
    }
  }

  // Also check text content for icon name (legacy support)
  if (!lookupName && !iconName && faElement.textContent.trim()) {
    const text = faElement.textContent.trim();
    if (text.startsWith('fa-')) {
      classes.push(text);
    } else if (/^[a-z-]+$/i.test(text)) {
      // Could be a lookup name or icon name
      lookupName = text;
    }
  }

  // Determine the icon to use
  let finalIconName = null;
  let utilityClasses = [];

  if (lookupName) {
    // Try to get icon from lookups
    const lookupData = getLookupIcon(lookupName);
    if (lookupData) {
      finalIconName = lookupData.icon;
      utilityClasses = lookupData.utilities;
    }
  }

  // If no lookup match, try to parse icon from classes
  if (!finalIconName) {
    finalIconName = parseIconName(classes);
  }

  // Extract additional utility classes from the element
  const elementUtilities = extractUtilityClasses(classes);
  utilityClasses = [...new Set([...utilityClasses, ...elementUtilities])];

  // Create the <i> element
  const iElement = document.createElement('i');

  // Build the class list
  const prefix = buildPrefix();
  const weightClass = buildWeightClass();
  const classList = [prefix];

  if (weightClass) {
    classList.push(weightClass);
  }

  if (finalIconName) {
    classList.push(`fa-${finalIconName}`);
  }

  // Add utility classes
  classList.push(...utilityClasses);

  // Set the class attribute
  iElement.className = classList.join(' ');

  // Copy any data attributes
  for (const attr of attributes) {
    if (attr.name.startsWith('data-')) {
      iElement.setAttribute(attr.name, attr.value);
    }
  }

  // Copy inline styles
  if (faElement.style.cssText) {
    iElement.style.cssText = faElement.style.cssText;
  }

  // Copy id if present
  if (faElement.id) {
    iElement.id = faElement.id;
  }

  // Copy title for accessibility
  if (faElement.title) {
    iElement.title = faElement.title;
  }

  // Copy aria attributes
  for (const attr of attributes) {
    if (attr.name.startsWith('aria-')) {
      iElement.setAttribute(attr.name, attr.value);
    }
  }

  // Replace the <fa> element with the <i> element
  faElement.parentNode.replaceChild(iElement, faElement);

  return iElement;
}

/**
 * Process all <fa> elements within a container
 * @param {HTMLElement} container - Container to search for <fa> elements
 */
export function processIcons(container = document) {
  const faElements = container.querySelectorAll('fa');

  faElements.forEach((element) => {
    try {
      processIconElement(element);
    } catch (error) {
      // Silently ignore icon processing errors
    }
  });

  return faElements.length;
}

/**
 * Set up mutation observer to watch for new <fa> elements
 */
function setupMutationObserver() {
  if (observer) {
    return; // Already set up
  }

  observer = new MutationObserver((mutations) => {
    let shouldProcess = false;

    for (const mutation of mutations) {
      // Check if any added nodes contain <fa> elements
      for (const node of mutation.addedNodes) {
        if (node.nodeType === Node.ELEMENT_NODE) {
          if (node.tagName.toLowerCase() === 'fa' || node.querySelector?.('fa')) {
            shouldProcess = true;
            break;
          }
        }
      }

      if (shouldProcess) break;
    }

    if (shouldProcess) {
      // Process any new <fa> elements
      processIcons();
    }
  });

  // Start observing the document body
  observer.observe(document.body, {
    childList: true,
    subtree: true
  });
}

/**
 * Refresh icon lookups from the server
 */
export function refreshIconLookups() {
  iconLookups = getLookupCategory('icons');
}

/**
 * Get the icon configuration
 * @returns {Object} Current icon configuration
 */
export function getIconConfiguration() {
  return { ...getIconConfig() };
}

/**
 * Programmatically set an icon on an element
 * @param {HTMLElement} element - The element to set the icon on
 * @param {string} iconName - Icon name (e.g., 'user', 'cog')
 * @param {Object} options - Options for the icon
 * @param {string} options.set - Override the icon set
 * @param {string} options.weight - Override the weight
 * @param {string[]} options.classes - Additional classes to add
 */
export function setIcon(element, iconName, options = {}) {
  if (!element) {
    return;
  }

  const config = getIconConfig();
  const set = options.set || config.set;
  const weight = options.weight || config.weight;

  // Build prefix
  const prefix = FA_PREFIX_MAP[set] || 'fas';

  // Build class list
  const classList = [prefix];

  // Add weight class if applicable
  if (weight && ['duotone', 'sharp-solid', 'sharp-regular', 'sharp-light', 'sharp-thin'].includes(set)) {
    classList.push(`fa-${weight}`);
  }

  // Add icon class
  classList.push(`fa-${iconName}`);

  // Add additional classes
  if (options.classes) {
    classList.push(...options.classes);
  }

  element.className = classList.join(' ');
}

/**
 * Create an icon element programmatically
 * @param {string} iconName - Icon name (e.g., 'user', 'cog')
 * @param {Object} options - Options for the icon
 * @returns {HTMLElement} The created <i> element
 */
export function createIcon(iconName, options = {}) {
  const element = document.createElement('i');
  setIcon(element, iconName, options);
  return element;
}

/**
 * Initialize the icon system
 * @param {Object} options - Initialization options
 * @param {boolean} options.observe - Whether to set up mutation observer
 */
export function init(options = {}) {
  if (isInitialized) {
    return;
  }

  const { observe = true } = options;

  // Process any existing <fa> elements
  if (document.body) {
    processIcons();
  }

  // Set up mutation observer if requested
  if (observe) {
    setupMutationObserver();
  }

  // Listen for lookups loaded events to refresh icon data
  eventBus.on(Events.LOOKUPS_LOADED, () => {
    refreshIconLookups();
    // Re-process icons in case lookups changed any icon mappings
    processIcons();
  });

  // Also listen for specific icon lookup events
  eventBus.on('lookups:icons:loaded', () => {
    refreshIconLookups();
    processIcons();
  });

  isInitialized = true;
}

/**
 * Tear down the icon system
 */
export function teardown() {
  if (observer) {
    observer.disconnect();
    observer = null;
  }

  isInitialized = false;
  iconConfig = null;
  iconLookups = null;
}

/**
 * Check if the icon system is initialized
 * @returns {boolean} True if initialized
 */
export function isReady() {
  return isInitialized;
}

// Default export
export default {
  init,
  teardown,
  processIcons,
  refreshIconLookups,
  getIconConfiguration,
  setIcon,
  createIcon,
  isReady
};
