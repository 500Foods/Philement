const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./zoom-control-BOsJBiLz.js","./index-CNmg4i7Y.js","./index-ZlKPF4Cw.css","./conduit-B8Ld7ghl.js","./languages-Coru4QdC.js","./codemirror-setup-lxLGSWU-.js","./codemirror-Bg1rM8n2.js"])))=>i.map(i=>d[i]);
import { g as getMacros, l as log, S as Status, a as Subsystems, m as _registerShortcutKey, d as getTip, n as _unregisterShortcutKey, p as processIcons, e as eventBus, o as offset, q as flip, u as shift, _ as __vitePreload, w as isConfigLoaded, x as loadConfig, f as getConfigValue, y as normalizeIconHtml } from './index-CNmg4i7Y.js';
import { a as authQuery } from './conduit-B8Ld7ghl.js';
import { i as getLocaleDisplayName, h as getCountryCode, e as getFlagSvg } from './languages-Coru4QdC.js';
import './codemirror-setup-lxLGSWU-.js';

/**
 * Macro Expansion Utility
 *
 * Provides text expansion for {variable} placeholders using values
 * from the macros lookup (QueryRef 046).
 *
 * Usage:
 *   import { expandMacros } from './macro-expansion.js';
 *   const expanded = expandMacros('Welcome to {APP_NAME}!');
 */


/**
 * Expand {variable} placeholders in text with macro values
 *
 * @param {string} text - Text containing {VAR} placeholders
 * @param {Object} options - Expansion options
 * @param {Object} options.fallbacks - Additional fallback values not in macros lookup
 * @param {boolean} options.keepUnknown - Keep {VAR} if macro not found (default: true)
 * @param {string} options.unknownPlaceholder - Replace unknown vars with this (if keepUnknown is false)
 * @returns {string} Text with macros expanded
 */
function expandMacros(text, options = {}) {
  if (!text || typeof text !== 'string') {
    return text;
  }

  const {
    fallbacks = {},
    keepUnknown = true,
    unknownPlaceholder = '',
    debug = false
  } = options;

  // Get macros from lookup cache
  const macros = getMacros() || {};
  const macroKeys = Object.keys(macros);

  // DEBUG: Log what we have
  if (debug || macroKeys.length > 0) {
    log(Subsystems.UTILS, Status.DEBUG, `[MacroExpansion] Expanding text with ${macroKeys.length} macros available: ${macroKeys.join(', ')}`);
  }

  // Combine macros with fallbacks (fallbacks take precedence)
  const allMacros = { ...macros, ...fallbacks };

  // Replace {VAR} patterns
  // Matches {VAR_NAME} where VAR_NAME can contain letters, numbers, underscores, hyphens, dots
  // Note: Must include dots to match macros like {ACZ.CLIENT}
  const result = text.replace(/\{([a-zA-Z_][a-zA-Z0-9_.-]*)\}/g, (match, varName) => {
    const value = allMacros[varName];

    if (value !== undefined) {
      if (debug) {
        log(Subsystems.UTILS, Status.DEBUG, `[MacroExpansion] Replaced {${varName}} with "${value}"`);
      }
      return String(value);
    }

    // Macro not found
    if (debug) {
      log(Subsystems.UTILS, Status.WARN, `[MacroExpansion] Macro not found: {${varName}}`);
    }
    if (keepUnknown) {
      return match; // Keep the original {VAR}
    }

    return unknownPlaceholder;
  });

  return result;
}

/**
 * Check if text contains any macro placeholders
 *
 * @param {string} text - Text to check
 * @returns {boolean} True if text contains {VAR} patterns
 */
function hasMacros(text) {
  if (!text || typeof text !== 'string') {
    return false;
  }
  // Match {VAR} including dots for macros like {ACZ.CLIENT}
  return /\{[a-zA-Z_][a-zA-Z0-9_.-]*\}/.test(text);
}

/**
 * Shortcuts Registry
 *
 * Simple module to store global keyboard shortcuts.
 * Separated from manager-ui.js to avoid circular dependencies.
 *
 * @module core/shortcuts
 */

// Global keyboard shortcuts registry
const globalShortcuts = new Map();

/**
 * Register a keyboard shortcut
 * @param {string} id - Shortcut ID
 * @param {Object} data - Shortcut data { key, description, options }
 */
function registerShortcut$1(id, data) {
  globalShortcuts.set(id, data);
}

/**
 * Unregister a keyboard shortcut
 * @param {string} id - Shortcut ID
 */
function unregisterShortcut(id) {
  globalShortcuts.delete(id);
}

/**
 * Get all registered shortcuts
 * @returns {Array} Array of shortcut objects
 */
function getAllShortcuts() {
  return Array.from(globalShortcuts.entries()).map(([id, data]) => ({
    id,
    ...data,
  }));
}

/**
 * Shortcuts Manager
 *
 * Wrapper functions for keyboard shortcut registration.
 * These functions integrate with the tooltip system to display shortcuts.
 */


// ===== Tooltip Update Helper =====

/**
 * Update tooltips for elements associated with a shortcut.
 * Finds elements with data-shortcut-id matching the shortcut id
 * and appends the key combo to their tooltip content.
 */
function _updateTooltipForShortcut(id, key) {
  document.querySelectorAll(`[data-shortcut-id="${id}"]`).forEach((el) => {
    if (!el.dataset.tooltipOriginal) {
      el.dataset.tooltipOriginal = el.getAttribute('data-tooltip') || '';
    }
    const original = el.dataset.tooltipOriginal;
    if (!original) return;

    const newContent = `${original}<br><span class="li-tip-kbd">${key}</span>`;
    el.setAttribute('data-tooltip', newContent);

    const t = getTip(el);
    if (t) t.updateContent(newContent);
  });
}

// ===== Keyboard Shortcuts =====

/**
 * Register a keyboard shortcut
 * @param {string} id - Unique identifier for the shortcut
 * @param {string} key - Key combination (e.g., 'Ctrl+S', 'F1')
 * @param {string} description - Human-readable description
 * @param {Function} handler - Function to call when shortcut is triggered
 * @param {Object} options - Additional options
 * @param {string} options.managerId - Optional manager ID this shortcut belongs to
 */
function registerShortcut(id, key, description, handler, options = {}) {
  registerShortcut$1(id, { key, description, handler, options });
  _registerShortcutKey(id, key);
  _updateTooltipForShortcut(id, key);
  log(Subsystems.MANAGER, Status.INFO, `[Shortcuts] Registered shortcut: ${id} (${key})`);
}

/**
 * Unregister all shortcuts for a specific manager
 * @param {string} managerId - Manager ID
 */
function unregisterManagerShortcuts(managerId) {
  getAllShortcuts().forEach(s => {
    if (s.options?.managerId === managerId) {
      unregisterShortcut(s.id);
      _unregisterShortcutKey(s.id);
    }
  });
}

/**
 * Fullscreen Manager
 */


// ===== Fullscreen =====

/**
 * Check if fullscreen is active
 * @returns {boolean}
 */
function isFullscreen() {
  return !!(document.fullscreenElement ||
    document.webkitFullscreenElement ||
    document.mozFullScreenElement ||
    document.msFullscreenElement);
}

/**
 * Toggle fullscreen mode
 * @param {HTMLElement} element - Element to fullscreen (defaults to document.documentElement)
 */
async function toggleFullscreen(element = document.documentElement) {
  try {
    if (isFullscreen()) {
      if (document.exitFullscreen) {
        await document.exitFullscreen();
      } else if (document.webkitExitFullscreen) {
        await document.webkitExitFullscreen();
      } else if (document.mozCancelFullScreen) {
        await document.mozCancelFullScreen();
      } else if (document.msExitFullscreen) {
        await document.msExitFullscreen();
      }
    } else {
      if (element.requestFullscreen) {
        await element.requestFullscreen();
      } else if (element.webkitRequestFullscreen) {
        await element.webkitRequestFullscreen();
      } else if (element.mozRequestFullScreen) {
        await element.mozRequestFullScreen();
      } else if (element.msRequestFullscreen) {
        await element.msRequestFullscreen();
      }
    }
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `[Fullscreen] Error: ${error.message}`);
  }
}

/**
 * Update fullscreen button icon
 * @param {HTMLButtonElement} btn - The fullscreen button
 */
function updateFullscreenIcon(btn) {
  const icon = isFullscreen() ? 'fa-minimize' : 'fa-maximize';
  btn.innerHTML = `<fa ${icon}></fa>`;
  processIcons(btn);
}

// ========================================
// Video Tour State Persistence
// ========================================

const VIDEO_STORAGE_KEYS = {
  CAPTIONS: 'lithium_video_captions_enabled',
  SPEED: 'lithium_video_playback_speed',
  VOLUME: 'lithium_video_volume',
  MUTED: 'lithium_video_muted',
  X: 'lithium_video_x',
  Y: 'lithium_video_y',
  WIDTH: 'lithium_video_width',
  HEIGHT: 'lithium_video_height',
};

function loadVideoState() {
  try {
    const volume = parseFloat(localStorage.getItem(VIDEO_STORAGE_KEYS.VOLUME));
    const x = parseInt(localStorage.getItem(VIDEO_STORAGE_KEYS.X), 10);
    const y = parseInt(localStorage.getItem(VIDEO_STORAGE_KEYS.Y), 10);
    const width = parseInt(localStorage.getItem(VIDEO_STORAGE_KEYS.WIDTH), 10);
    const height = parseInt(localStorage.getItem(VIDEO_STORAGE_KEYS.HEIGHT), 10);
    return {
      captionsEnabled: localStorage.getItem(VIDEO_STORAGE_KEYS.CAPTIONS) !== 'false',
      playbackSpeed: parseFloat(localStorage.getItem(VIDEO_STORAGE_KEYS.SPEED)) || 1,
      volume: (volume !== null && !isNaN(volume)) ? volume : 1,
      muted: localStorage.getItem(VIDEO_STORAGE_KEYS.MUTED) === 'true',
      x: (!isNaN(x) && x >= 0) ? x : null,
      y: (!isNaN(y) && y >= 0) ? y : null,
      width: (!isNaN(width) && width > 0) ? width : null,
      height: (!isNaN(height) && height > 0) ? height : null,
    };
  } catch (e) {
    return null;
  }
}

function saveVideoState(state) {
  try {
    // Validate all numeric values to prevent NaN/Infinity issues
    const safeVolume = (typeof state.volume === 'number' && isFinite(state.volume)) ? state.volume : 1;
    const safeX = (typeof state.x === 'number' && isFinite(state.x)) ? Math.round(state.x) : 0;
    const safeY = (typeof state.y === 'number' && isFinite(state.y)) ? Math.round(state.y) : 0;
    // Only save valid positive dimensions
    const safeWidth = (typeof state.width === 'number' && isFinite(state.width) && state.width > 0) ? Math.round(state.width) : 0;
    const safeHeight = (typeof state.height === 'number' && isFinite(state.height) && state.height > 0) ? Math.round(state.height) : 0;
    
    localStorage.setItem(VIDEO_STORAGE_KEYS.CAPTIONS, String(state.captionsEnabled));
    localStorage.setItem(VIDEO_STORAGE_KEYS.SPEED, String(state.playbackSpeed));
    localStorage.setItem(VIDEO_STORAGE_KEYS.VOLUME, String(safeVolume));
    localStorage.setItem(VIDEO_STORAGE_KEYS.MUTED, String(state.muted ?? false));
    localStorage.setItem(VIDEO_STORAGE_KEYS.X, String(safeX));
    localStorage.setItem(VIDEO_STORAGE_KEYS.Y, String(safeY));
    localStorage.setItem(VIDEO_STORAGE_KEYS.WIDTH, String(safeWidth));
    localStorage.setItem(VIDEO_STORAGE_KEYS.HEIGHT, String(safeHeight));
  } catch (e) {
    // localStorage may not be available
  }
}

// ========================================
// State
// ========================================

let _toursCache = null;
let _activeShepherdTour = null;
let _listPopup = null;
let _documentClickHandler = null;
let _tourEnding = false;  // Guard against navigate during cancel/complete
let _tourContainer = null; // Dedicated container for tour step elements
let _videoStateCleanup = null; // Cleanup function for video state saving

/**
 * Initialize the tour container in the DOM
 * @private
 */
function initTourContainer() {
  if (_tourContainer) return _tourContainer;

  // Check if container already exists
  _tourContainer = document.getElementById('tour-container');

  if (!_tourContainer) {
    _tourContainer = document.createElement('div');
    _tourContainer.id = 'tour-container';
    _tourContainer.className = 'tour-container';
    document.body.appendChild(_tourContainer);
    log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Created tour container');
  }

  return _tourContainer;
}

/**
 * Fade out the modal overlay
 */
function fadeOutOverlay() {
  const overlay = document.querySelector('.shepherd-modal-overlay-container');
  if (overlay) {
    overlay.classList.remove('shepherd-modal-is-visible');
  }
}

/**
 * Force cleanup of any active tour state
 * Called when tour operations fail or need manual reset
 */
function cleanupActiveTour() {
  log(Subsystems.MANAGER, Status.INFO, '[Tour] Forcing tour cleanup');

  // Remove document click handler if still active
  if (_documentClickHandler) {
    document.removeEventListener('click', _documentClickHandler, true);
    _documentClickHandler = null;
  }

  _activeShepherdTour = null;
  _tourEnding = false;

  // Remove the entire tour container (all tour elements are inside it)
  if (_tourContainer) {
    _tourContainer.remove();
    _tourContainer = null;
  }

  // Remove Shepherd modal overlay if present
  document.querySelectorAll('.shepherd-modal-overlay-container').forEach(el => {
    el.remove();
  });
}

// ========================================
// Tour Loading (lazy, from Lookup #43)
// ========================================

/**
 * Get all tours from Lookup #43 (cached after first fetch)
 * @param {Object} api - The API/conduit instance
 * @returns {Promise<Array>} Array of tour objects
 */
async function getTours(api) {
  if (_toursCache) {
    return _toursCache;
  }

  log(Subsystems.MANAGER, Status.INFO, '[Tour] Fetching tours from Lookup #43');

  try {
    // QueryRef 26 with LOOKUPID 43 to get tour entries
    const entries = await authQuery(api, 26, {
      INTEGER: { LOOKUPID: 43 },
    });

    if (!entries || entries.length === 0) {
      log(Subsystems.MANAGER, Status.WARN, '[Tour] No tours found in Lookup #43');
      _toursCache = [];
      return _toursCache;
    }

    // Debug: log the first raw entry to see structure
    log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Raw entry sample: ${JSON.stringify(entries[0]).substring(0, 300)}`);

    _toursCache = entries
      .filter(e => e.collection) // Only tours with definitions
      .map(e => {
        // Parse the collection JSON string if it's a string
        let definition;
        try {
          definition = typeof e.collection === 'string'
            ? JSON.parse(e.collection)
            : e.collection;
        } catch (err) {
          log(Subsystems.MANAGER, Status.WARN, `[Tour] Failed to parse tour definition for key_idx ${e.key_idx}: ${err.message}`);
          return null;
        }
        return {
          id: e.key_idx,
          name: e.value_txt,
          code: e.code || String(e.key_idx), // Support lookup by code/name
          definition,
        };
      })
      .filter(Boolean); // Remove any null entries (parse failures)

    log(Subsystems.MANAGER, Status.INFO, `[Tour] Loaded ${_toursCache.length} tours`);
    if (_toursCache.length > 0) {
      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] First tour: ${JSON.stringify(_toursCache[0]).substring(0, 300)}`);
    }
    return _toursCache;
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to load tours: ${error.message}`);
    return [];
  }
}

/**
 * Extract numeric manager ID from a manager string (e.g., "003.Profile" -> 3)
 * @param {string} managerStr - Manager identifier string
 * @returns {number|null} Numeric ID or null if invalid
 */
function extractManagerId(managerStr) {
  if (!managerStr || typeof managerStr !== 'string') return null;
  const match = managerStr.match(/^(\d{3})\./);
  return match ? parseInt(match[1], 10) : null;
}

/**
 * Get tours for a specific manager
 * ONLY compares numeric IDs - "003.Profile" matches "003.User Profile" because both are ID 3
 * Supports both:
 * - Single manager: "023.Lookup Manager"
 * - Multiple managers: ["029.Query Manager", "023.Lookup Manager"]
 * @param {Object} api - The API/conduit instance
 * @param {string} managerName - Manager name (e.g., "023.Lookup Manager")
 * @returns {Promise<Array>} Filtered tour objects
 */
async function getToursForManager(api, managerName) {
  const tours = await getTours(api);
  const targetId = extractManagerId(managerName);

  if (targetId === null) {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] Invalid manager name format: ${managerName}`);
    return [];
  }

  return tours.filter(t => {
    const def = t.definition;
    if (!def) return false;

    // Single manager - compare numeric IDs only
    if (def.manager) {
      const tourManagerId = extractManagerId(def.manager);
      if (tourManagerId === targetId) return true;
    }

    // Multiple managers - compare numeric IDs only
    if (Array.isArray(def.managers)) {
      return def.managers.some(m => extractManagerId(m) === targetId);
    }

    return false;
  });
}

/**
 * Check if a tour is a video tour
 * @param {Object} tour - Tour object
 * @returns {boolean} True if tour has video field
 */
function isVideoTour(tour) {
  return !!(tour?.definition?.video);
}

// ========================================
// Tour Building (Shepherd)
// ========================================

/**
 * Import Shepherd.js dynamically
 * @returns {Promise<Object>} Shepherd module
 */
async function loadShepherd() {
  const mod = await __vitePreload(() => import('./shepherd-CrUPd-P1.js'),true              ?[]:void 0,import.meta.url);
  return mod.default;
}

/**
 * Create custom header HTML for a step
 * @param {Object} tour - Tour object
 * @param {Object} options - Options including onShowList callback
 * @returns {string} HTML string for the header
 */
function createHeaderHTML(tour, options) {
  // Use raw icon HTML from definition, or fallback to default fa icon
  const iconHtml = tour.definition?.icon || '<fa fa-signs-post></fa>';

  // Expand macros in tour name
  const tourName = expandMacros(tour.name);

  return `
    <div class="lithium-tour-header">
      <div class="lithium-tour-header-icon-title">
        ${iconHtml}
        <div class="lithium-tour-header-title">${tourName}</div>
      </div>
      <div class="lithium-tour-header-actions">
        <button type="button" class="lithium-tour-header-btn lithium-tour-list-btn" data-tour-action="list" title="Available Tours">
          <fa fa-signs-post></fa>
        </button>
        <button type="button" class="lithium-tour-header-btn lithium-tour-close-btn" data-tour-action="close" title="Close tour<br><span class='li-tip-kbd'>Esc</span>">
          <fa fa-xmark></fa>
        </button>
      </div>
    </div>
  `;
}

/**
 * Create custom footer HTML for a step
 * @param {number} currentStep - Current step index (0-based)
 * @param {number} totalSteps - Total number of steps
 * @returns {string} HTML string for the footer
 */
function createFooterHTML(currentStep, totalSteps) {
  const isFirst = currentStep === 0;
  const isLast = currentStep === totalSteps - 1;
  const stepCounter = `${currentStep + 1} / ${totalSteps}`;

  return `
    <div class="lithium-tour-footer">
      <button type="button"
              class="lithium-tour-footer-btn lithium-tour-back-btn"
              data-tour-action="back"
              data-tip-placement="bottom"
              title='Previous<br><span class="li-tip-kbd">← (cursor-left)</span>'
              ${isFirst ? 'disabled' : ''}>
        <fa fa-left></fa>
      </button>
      <div class="lithium-tour-step-counter" data-tip-placement="bottom" title="Step ${currentStep + 1} of ${totalSteps}">${stepCounter}</div>
      <button type="button"
              class="lithium-tour-footer-btn lithium-tour-next-btn"
              data-tour-action="next"
              data-tip-placement="bottom"
              title='${isLast ? "Finish tour<br><span class=\"li-tip-kbd\">→ (cursor-right)</span>" : "Next<br><span class=\"li-tip-kbd\">→ (cursor-right)</span>"}'>
        ${isLast ? '<fa fa-flag-checkered></fa>' : '<fa fa-right></fa>'}
      </button>
    </div>                               
  `;
}

/**
 * Get current step index using step ID comparison
 * @param {Object} shepherd - Shepherd Tour instance
 * @returns {number} Current step index (0-based), or -1 if not found
 */
function getCurrentStepIndex(shepherd) {
  const currentStep = shepherd.getCurrentStep();
  if (!currentStep) return -1;
  return shepherd.steps.findIndex(s => s.id === currentStep.id);
}

/**
 * Setup document-level click handler for tour buttons
 * @param {Object} shepherd - Shepherd Tour instance
 * @param {number} totalSteps - Total number of steps
 * @param {Object} options - Options including onShowList callback
 */
function setupDocumentClickHandler(shepherd, totalSteps, options) {
  const handler = (e) => {
    // Find if click was on a tour action button
    const button = e.target.closest('[data-tour-action]');
    if (!button) return;

    // Verify this is part of the active tour step
    const stepEl = shepherd.getCurrentStep()?.getElement();
    if (!stepEl || !stepEl.contains(button)) return;

    const action = button.dataset.tourAction;
    e.preventDefault();
    e.stopPropagation();

    switch (action) {
      case 'close':
        cancelWithTransition(shepherd);
        break;

      case 'list':
        cancelWithTransition(shepherd);
        // Delay must exceed the fade-out + Shepherd cleanup time
        {
          const listDelay = getTransitionDuration$1() + 100;
          setTimeout(() => {
            if (options.onShowList) {
              options.onShowList();
            }
          }, listDelay);
        }
        break;

      case 'back':
        if (!button.disabled) {
          // Double-check we're not at first step - Shepherd.back() at step 0 exits the tour
          const currentIdx = getCurrentStepIndex(shepherd);
          if (currentIdx > 0) {
            navigateWithTransition(shepherd, 'back');
          } else {
            log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Back button clicked but already at first step');
          }
        }
        break;

      case 'next':
        {
          const currentIndex = getCurrentStepIndex(shepherd);
          if (currentIndex === totalSteps - 1) {
            completeWithTransition(shepherd);
          } else {
            navigateWithTransition(shepherd, 'next');
          }
        }
        break;
    }
  };

  document.addEventListener('click', handler, true); // Use capture phase for reliability
  _documentClickHandler = handler; // Store reference for potential emergency cleanup
  return () => {
    document.removeEventListener('click', handler, true);
    _documentClickHandler = null;
  };
}

/**
 * Get the CSS transition duration in milliseconds
 * @returns {number} Transition duration in ms
 */
function getTransitionDuration$1() {
  // Get the computed transition duration from CSS variable
  const duration = getComputedStyle(document.documentElement)
    .getPropertyValue('--transition-duration')
    .trim();

  // Parse duration (e.g., "0.2s" -> 200, "200ms" -> 200)
  if (duration.endsWith('ms')) {
    return parseInt(duration, 10);
  } else if (duration.endsWith('s')) {
    return parseFloat(duration) * 1000;
  }

  return 200; // Default fallback
}

/**
 * Navigate with overlapping fade transition (next step fades in before current fades out)
 * @param {Object} shepherd - Shepherd Tour instance
 * @param {string} direction - 'next' or 'back'
 */
function navigateWithTransition(shepherd, direction) {
  // Don't navigate if tour is ending
  if (_tourEnding) return;

  // CRITICAL FIX: Prevent back from causing Shepherd to exit the tour
  // Check at the START before doing anything - if at first step and going back, do nothing
  const currentIndexAtStart = getCurrentStepIndex(shepherd);
  if (direction === 'back' && currentIndexAtStart <= 0) {
    log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Back pressed at first step - blocking to prevent tour exit');
    return;
  }

  const currentStepEl = shepherd.getCurrentStep()?.getElement();
  if (!currentStepEl) {
    if (direction === 'next') shepherd.next();
    else {
      // Extra protection - still check here
      const idx = getCurrentStepIndex(shepherd);
      if (idx > 0) shepherd.back();
    }
    return;
  }

  // Get transition duration from CSS
  const transitionDuration = getTransitionDuration$1();

  // Prevent multiple rapid navigation attempts
  if (currentStepEl._isTransitioning) return;
  currentStepEl._isTransitioning = true;

  // Store the current step element reference BEFORE fading out
  const fadingOutEl = currentStepEl;

  // Overlapping transition - fade out current, but trigger navigation
  // with a reduced delay so the next step starts fading in BEFORE current finishes fading out
  const overlapDelay = 0; // Start next fade-in immediately when fade-out begins

  fadingOutEl.classList.remove('lithium-tour-visible');
  // Ensure old step stays on top during fade-out for visible crossfade
  fadingOutEl.style.zIndex = '40001';

  // Hide the old step after fade-out completes
  setTimeout(() => {
    fadingOutEl.style.display = 'none';
  }, transitionDuration);

  // Wait for shorter delay (overlapping with fade-out), then navigate
  // IMPORTANT: Use the START index (currentIndexAtStart), not a re-checked index
  // Re-checking can return -1 or wrong value if step has already started changing
  setTimeout(() => {
    fadingOutEl._isTransitioning = false;
    // Re-check guard — tour may have been cancelled during fade-out
    if (_tourEnding) return;

    // CRITICAL: For back navigation, we need to reset the target step's visibility
    // before navigating to it, otherwise the CSS transition won't re-trigger
    if (direction === 'back' && currentIndexAtStart > 0) {
      const targetStep = shepherd.steps[currentIndexAtStart - 1];
      if (targetStep) {
        const targetEl = targetStep.getElement();
        if (targetEl) {
          // Ensure it's visible for back navigation
          targetEl.style.display = 'block';
          // Remove visible class to reset opacity to 0
          targetEl.classList.remove('lithium-tour-visible');
          // Force reflow to ensure the removal is processed
          void targetEl.offsetWidth;
        }
      }
    }

    // Temporarily disable Shepherd's hide method to keep old step visible for overlapping transitions
    const oldStep = shepherd.getCurrentStep();
    const originalHide = oldStep.hide;
    oldStep.hide = () => {
      // Prevent hiding to enable smooth overlapping fade
    };

    // Use the index captured at the START of this function
    // This avoids the race condition where getCurrentStep() changes during the transition
    if (direction === 'next') {
      shepherd.next();
    } else if (currentIndexAtStart > 0) {
      shepherd.back();
    } else {
      log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Back navigation blocked - was at first step');
    }

    // Restore the original hide method
    oldStep.hide = originalHide;

    // Ensure the new step is visible (in case show event doesn't fire reliably)
    const newCurrentStep = shepherd.getCurrentStep();
    if (newCurrentStep) {
      setTimeout(() => {
        const el = newCurrentStep.getElement();
        if (el) {
          if (typeof processIcons === 'function') {
            processIcons(el);
          }
          el.classList.add('lithium-tour-visible');
        }
      }, 0);
    }
  }, overlapDelay);
}

/**
 * Cancel tour with fade-out transition
 * @param {Object} shepherd - Shepherd Tour instance
 */
function cancelWithTransition(shepherd) {
  _tourEnding = true;
  // Start overlay fade-out at the same time as step fade-out
  fadeOutOverlay();
  const currentStepEl = shepherd.getCurrentStep()?.getElement();
  if (currentStepEl) {
    const transitionDuration = getTransitionDuration$1();
    currentStepEl.classList.remove('lithium-tour-visible');
    // Wait for transition to complete before canceling
    setTimeout(() => {
      try {
        shepherd.cancel();
      } catch (err) {
        log(Subsystems.MANAGER, Status.ERROR, `[Tour] Error cancelling tour: ${err.message}`);
        // Force cleanup if cancel fails
        cleanupActiveTour();
      }
      _tourEnding = false;
    }, transitionDuration);
  } else {
    shepherd.cancel();
    _tourEnding = false;
  }
}

/**
 * Complete tour with fade-out transition
 * @param {Object} shepherd - Shepherd Tour instance
 */
function completeWithTransition(shepherd) {
  _tourEnding = true;
  // Start overlay fade-out at the same time as step fade-out
  fadeOutOverlay();
  const currentStepEl = shepherd.getCurrentStep()?.getElement();
  if (currentStepEl) {
    const transitionDuration = getTransitionDuration$1();
    currentStepEl.classList.remove('lithium-tour-visible');
    // Wait for transition to complete before completing
    setTimeout(() => {
      try {
        shepherd.complete();
      } catch (err) {
        log(Subsystems.MANAGER, Status.ERROR, `[Tour] Error completing tour: ${err.message}`);
        // Force cleanup if complete fails
        cleanupActiveTour();
      }
      _tourEnding = false;
    }, transitionDuration);
  } else {
    shepherd.complete();
    _tourEnding = false;
  }
}

/**
 * Setup drag functionality for a specific tour step element
 * @param {HTMLElement} stepElement - The tour step element
 */
function setupDragForElement(stepElement) {
  const headerEl = stepElement.querySelector('.lithium-tour-header');

  if (!headerEl) {
    // console.log('No header element found for drag setup');
    return;
  }

  // console.log('Setting up drag for step element:', stepElement);

  // Clean up any existing drag state and listeners
  if (headerEl._tourDragCleanup) {
    // console.log('Cleaning up existing drag handlers');
    headerEl._tourDragCleanup();
  }

  // Only remove dragging class, DON'T clear positioning styles
  // The styles are needed for proper step positioning (especially centered steps)
  stepElement.classList.remove('dragging');

  let isDragging = false;
  let dragStartX = 0;
  let dragStartY = 0;
  let popupStartX = 0;
  let popupStartY = 0;

          const handleDragStart = (e) => {
            // Don't drag if clicking buttons
            if (e.target.closest('.lithium-tour-header-btn')) return;

            // console.log('Tour drag start');
            isDragging = true;
            stepElement.classList.add('dragging');

            dragStartX = e.clientX;
            dragStartY = e.clientY;

            // Switch to left/top positioning at current location
            const rect = stepElement.getBoundingClientRect();
            stepElement.style.left = `${rect.left}px`;
            stepElement.style.top = `${rect.top}px`;
            stepElement.style.transform = '';

            popupStartX = rect.left;
            popupStartY = rect.top;

            document.addEventListener('mousemove', handleDragMove);
            document.addEventListener('mouseup', handleDragEnd);

            e.preventDefault();
            e.stopPropagation(); // Prevent other handlers from firing
          };

          const handleDragMove = (e) => {
            if (!isDragging) return;

            const deltaX = e.clientX - dragStartX;
            const deltaY = e.clientY - dragStartY;

            let newX = popupStartX + deltaX;
            let newY = popupStartY + deltaY;

            // Constrain to viewport
            const rect = stepElement.getBoundingClientRect();
            const minVisible = 50;

            newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
            newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

            stepElement.style.left = `${newX}px`;
            stepElement.style.top = `${newY}px`;
          };

          const handleDragEnd = () => {
            isDragging = false;
            stepElement.classList.remove('dragging');

            document.removeEventListener('mousemove', handleDragMove);
            document.removeEventListener('mouseup', handleDragEnd);
          };

          // Store cleanup function on the header element for future cleanup
          headerEl._tourDragCleanup = () => {
            document.removeEventListener('mousemove', handleDragMove);
            document.removeEventListener('mouseup', handleDragEnd);
            stepElement.classList.remove('dragging');
            // Don't clear positioning styles here - preserve Shepherd's positioning
            isDragging = false;
          };

  headerEl.addEventListener('mousedown', handleDragStart);
  // console.log('Drag handlers attached to header');
}

/**
 * Setup keyboard navigation for a tour
 * @param {Object} shepherd - Shepherd Tour instance
 * @param {number} totalSteps - Total number of steps
 * @returns {Function} Cleanup function to remove listeners
 */
function setupKeyboardNav(shepherd, totalSteps) {
  const handleKeydown = (e) => {
    // Only handle if no modifier keys pressed
    if (e.ctrlKey || e.metaKey || e.altKey) return;

    const currentStep = shepherd.getCurrentStep();
    if (!currentStep) return;

    switch (e.key) {
      case 'ArrowLeft':
        e.preventDefault();
        // Check we're not at first step - Shepherd.back() at step 0 exits the tour
        {
          const idx = getCurrentStepIndex(shepherd);
          if (idx > 0) {
            navigateWithTransition(shepherd, 'back');
          }
        }
        break;
      case 'ArrowRight':
        e.preventDefault();
        {
          const currentIndex = getCurrentStepIndex(shepherd);
          if (currentIndex === totalSteps - 1) {
            completeWithTransition(shepherd);
          } else {
            navigateWithTransition(shepherd, 'next');
          }
        }
        break;
      case 'Escape':
        e.preventDefault();
        cancelWithTransition(shepherd);
        break;
    }
  };

  document.addEventListener('keydown', handleKeydown);

  // Return cleanup function
  return () => document.removeEventListener('keydown', handleKeydown);
}

/**
 * Build a Shepherd tour from a tour definition
 * @param {Object} tour - Tour object { id, name, definition }
 * @param {Object} options - Options
 * @param {Function} options.onShowList - Callback to show tour list
 * @returns {Promise<Object>} Shepherd Tour instance
 */
async function buildShepherdTour(tour, options = {}) {
  const Shepherd = await loadShepherd();
  const { definition } = tour;

  log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Building tour: ${tour.name}`);
  log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Definition: ${JSON.stringify(definition).substring(0, 200)}`);

  // Get steps - handle both "steps" and "Steps" (case variation)
  const steps = definition.steps || definition.Steps || [];
  const totalSteps = steps.length;

  log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Found ${totalSteps} steps`);

  if (totalSteps === 0) {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] No steps found in tour definition`);
  }

  // Generate unique tour instance ID to prevent step ID conflicts on restart
  const tourInstanceId = `t${Date.now()}`;

  // Ensure tour container exists
  const tourContainer = initTourContainer();

  const shepherd = new Shepherd.Tour({
    useModalOverlay: true,
    exitOnEsc: false,           // We handle Escape ourselves via setupKeyboardNav
    keyboardNavigation: false,  // We handle arrow keys ourselves via setupKeyboardNav
    stepsContainer: tourContainer, // Use our dedicated container
    modalOverlayOpeningPadding: 30, // Add padding around highlighted elements
    modalOverlayOpeningRadius: 8,  // Rounded corners for cutout
    defaultStepOptions: {
      cancelIcon: { enabled: false },
      classes: 'lithium-tour-step',
      scrollTo: { behavior: 'smooth', block: 'center' },
      // Disable arrows for cleaner look, add spacing from elements
      showOn: false,
      // Floating UI middleware for consistent 30px offset and viewport containment
      floatingUIOptions: {
        middleware: [
          offset(30),                    // 30px distance from target element
          flip({                         // Flip if would go off-screen
            padding: 30,
          }),
          shift({                        // Keep within viewport
            padding: 30,                 // 30px padding from viewport edges
          }),
        ],
      },
    },
    // Custom step content handler
    steps: steps.map((stepDef, index) => {
      const element = stepDef.element || stepDef.Element;
      const position = stepDef.position || stepDef.Position || 'bottom';

      // Expand macros in step content (e.g., {APP_NAME} -> Lithium)
      const rawStepText = stepDef.text || stepDef.Text || '';
      const stepText = expandMacros(rawStepText, { debug: true });

      // Debug logging for macro expansion
      if (hasMacros(rawStepText)) {
        const macrosAvailable = getMacros();
        const macroKeys = macrosAvailable ? Object.keys(macrosAvailable) : [];
        log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Step ${index + 1} has macros. Available keys: ${macroKeys.join(', ')}`);
        if (!macrosAvailable || macroKeys.length === 0) {
          log(Subsystems.MANAGER, Status.WARN, `[Tour] Macros not loaded yet, step content may have unresolved {VARS}`);
        }
      }

      const stepOptions = {
        id: `${tourInstanceId}-step-${index}`,
        title: '',
        text: `
          ${createHeaderHTML(tour)}
          <div class="lithium-tour-content">${stepText}</div>
          ${createFooterHTML(index, totalSteps)}
        `,
        classes: 'lithium-tour-step',
      };

       if (element) {
         const el = document.querySelector(element);
         if (el) {
           stepOptions.attachTo = {
             element: el,
             on: position,
           };
            // Per-step Floating UI options with 30px offset and viewport containment
            stepOptions.floatingUIOptions = {
              middleware: [
                offset(30),                  // 30px distance from target element
                flip({                       // Flip if would go off-screen
                  padding: 30,
                }),
                shift({                      // Keep within viewport
                  padding: 30,               // 30px padding from viewport edges
                }),
              ],
            };
         } else {
           // Element not found, make it centered
           stepOptions.attachTo = null;
         }
       }

      return stepOptions;
    }),
  });

  // Store reference for cleanup
  _activeShepherdTour = shepherd;

  // Cleanup functions
  let cleanupKeyboard = null;
  let cleanupClickHandler = null;
  let cleanupContextMenu = null;
  let visibilityObserver = null;

  // Set up a mutation observer to detect when tour steps become visible or hidden
  // This ensures drag functionality works even when Shepherd reuses elements
  visibilityObserver = new MutationObserver((mutations) => {
    mutations.forEach((mutation) => {
      if (mutation.type === 'attributes' && mutation.attributeName === 'class') {
        const target = mutation.target;
        const wasVisible = mutation.oldValue && mutation.oldValue.includes('lithium-tour-visible');
        const isVisible = target.classList.contains('lithium-tour-visible');

        if (target.classList.contains('lithium-tour-step')) {
          if (isVisible && !wasVisible) {
            // console.log('Tour step became visible, ensuring drag is set up');
            // Check if this step already has drag set up
            const headerEl = target.querySelector('.lithium-tour-header');
            if (headerEl && !headerEl._tourDragCleanup) {
              // console.log('Setting up drag for newly visible step');
              setupDragForElement(target);
            }
          }
        }
      }
    });
  });

  // Observe changes to the tour container for tour step visibility
  visibilityObserver.observe(tourContainer, {
    attributes: true,
    attributeFilter: ['class'],
    attributeOldValue: true,
    subtree: true
  });

  // Track current step index manually
  let currentStepIndex = 0;

  // Single show handler for all step updates
  shepherd.on('show', (event) => {
    // console.log('Tour show event fired');
    // Get step from event or fallback to getCurrentStep
    const step = event?.step || shepherd.getCurrentStep();

    // Find index by comparing step ID
    const currentIndex = shepherd.steps.findIndex(s => s.id === step?.id);
    if (currentIndex >= 0) {
      currentStepIndex = currentIndex;
    }
    // console.log(`Tour show event for step ${currentIndex}`);

    // Element might not be ready immediately, try to get it with a small delay
    const processStep = (retryCount = 0) => {
      const currentStepEl = step?.getElement();

      if (!currentStepEl && retryCount < 50) {
        // Retry in 50ms
        // console.log(`Tour step element not ready, retry ${retryCount + 1}/50`);
        setTimeout(() => processStep(retryCount + 1), 50);
        return;
      }

      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Step show event - index: ${currentStepIndex}, element: ${currentStepEl ? 'found' : 'NOT FOUND'}, retries: ${retryCount}`);

      if (!currentStepEl) {
        log(Subsystems.MANAGER, Status.ERROR, `[Tour] Step show event but no element found after retries`);
        return;
      }

      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Processing step element:`, currentStepEl);

      // Process icons first before showing
      if (typeof processIcons === 'function') {
        processIcons(currentStepEl);
      }

      // Ensure element is visible - override any Shepherd hiding
      currentStepEl.style.visibility = 'visible';
      currentStepEl.style.opacity = '1';
      currentStepEl.style.display = 'block';
      currentStepEl.style.zIndex = '40000';

      // Add visible class for smooth transitions FIRST
      currentStepEl.classList.add('lithium-tour-visible');

      // Ensure proper positioning for centered steps (AFTER visible class is added)
      if (currentStepEl.classList.contains('shepherd-centered')) {
        // Centered step - position in viewport center
        currentStepEl.style.top = '50%';
        currentStepEl.style.left = '50%';
        currentStepEl.style.transform = 'translate(-50%, -50%)';
      } else {
        // For attached steps, ensure they stay within viewport bounds
        // Floating UI should handle this, but double-check as a safeguard
        const rect = currentStepEl.getBoundingClientRect();
        const viewportWidth = window.innerWidth;
        const viewportHeight = window.innerHeight;
        const padding = 30;

        let needsClamp = false;
        let newLeft = rect.left;
        let newTop = rect.top;

        // Check if step is off-screen and clamp if necessary
        if (rect.right > viewportWidth - padding) {
          newLeft = viewportWidth - rect.width - padding;
          needsClamp = true;
        }
        if (rect.left < padding) {
          newLeft = padding;
          needsClamp = true;
        }
        if (rect.bottom > viewportHeight - padding) {
          newTop = viewportHeight - rect.height - padding;
          needsClamp = true;
        }
        if (rect.top < padding) {
          newTop = padding;
          needsClamp = true;
        }

        if (needsClamp) {
          currentStepEl.style.left = `${newLeft}px`;
          currentStepEl.style.top = `${newTop}px`;
          // Remove transform to use absolute positioning
          currentStepEl.style.transform = '';
          log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Clamped step position to keep in viewport`);
        }
      }

      // Force visibility after a short delay to override any Shepherd changes
      setTimeout(() => {
        if (currentStepEl && currentStepEl.isConnected) {
          currentStepEl.style.opacity = '1';
          currentStepEl.style.visibility = 'visible';
        }
      }, 10);

      // console.log(`Setting up drag for step ${currentIndex}`);

      // Setup drag functionality for this step element
      setupDragForElement(currentStepEl);

      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Step ${currentStepIndex + 1} is now visible`);
    };

    processStep();
  });

  shepherd.on('start', () => {
    // console.log('Shepherd tour started');

    // Check if modal overlay was created
    setTimeout(() => {
      document.querySelector('.shepherd-modal-overlay-container');
    }, 100);

    cleanupKeyboard = setupKeyboardNav(shepherd, totalSteps);
    cleanupClickHandler = setupDocumentClickHandler(shepherd, totalSteps, options);

    // Prevent context menu on tour elements to avoid interference
    const contextMenuHandler = (e) => {
      const tourContainer = document.getElementById('tour-container');
      if (tourContainer && tourContainer.contains(e.target)) {
        // console.log('Preventing context menu on tour element');
        e.preventDefault();
        return false;
      }
    };

    document.addEventListener('contextmenu', contextMenuHandler, true);
    cleanupContextMenu = () => {
      document.removeEventListener('contextmenu', contextMenuHandler, true);
    };
  });

  shepherd.on('hide', (event) => {
    // console.log('Shepherd hide event:', event);
  });

  shepherd.on('cancel', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour cancelled: ${tour.name}`);
    // Overlay fade-out is now handled in cancelWithTransition for sync
    if (cleanupKeyboard) cleanupKeyboard();
    if (cleanupClickHandler) cleanupClickHandler();
    if (cleanupContextMenu) cleanupContextMenu();
    if (visibilityObserver) visibilityObserver.disconnect();
    _activeShepherdTour = null;
    // Clean up drag state and remove tour container after overlay fade
    setTimeout(() => cleanupActiveTour(), getTransitionDuration$1());
  });

  shepherd.on('complete', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour completed: ${tour.name}`);
    // Overlay fade-out is now handled in completeWithTransition for sync
    if (cleanupKeyboard) cleanupKeyboard();
    if (cleanupClickHandler) cleanupClickHandler();
    if (cleanupContextMenu) cleanupContextMenu();
    if (visibilityObserver) visibilityObserver.disconnect();
    _activeShepherdTour = null;
    // Clean up drag state and remove tour container after overlay fade
    setTimeout(() => cleanupActiveTour(), getTransitionDuration$1());
  });

  return shepherd;
}

/**
 * Expand the sidebar if it's collapsed
 */
function expandSidebarIfCollapsed() {
  try {
    const app = window.lithiumApp;
    const mainManager = app?.mainManagerInstance || app?._getMainManager?.();

    if (!mainManager) {
      log(Subsystems.MANAGER, Status.WARN, '[Tour] MainManager not available for sidebar expansion');
      return;
    }

    // Check both isSidebarCollapsed property and collapsed class
    const sidebar = mainManager.elements?.sidebar;
    const isCollapsedClass = sidebar?.classList?.contains('collapsed');
    const isCollapsedProp = mainManager.isSidebarCollapsed === true;
    const isCollapsed = isCollapsedClass || isCollapsedProp;

    log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Sidebar collapsed check: class=${isCollapsedClass}, prop=${isCollapsedProp}`);

    if (isCollapsed && typeof mainManager.toggleSidebarCollapse === 'function') {
      log(Subsystems.MANAGER, Status.INFO, '[Tour] Expanding collapsed sidebar');
      mainManager.toggleSidebarCollapse();
    } else {
      log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Sidebar already expanded or toggle not available');
    }
  } catch (err) {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] Could not expand sidebar: ${err.message}`);
  }
}

/**
 * Find utility manager key by numeric ID
 * @param {number} id - Manager ID (e.g., 3)
 * @returns {string|null} Utility manager key or null
 */
function findUtilityKeyById(id) {
  try {
    const app = window.lithiumApp;
    if (!app?.utilityManagerRegistry) {
      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] utilityManagerRegistry not available`);
      return null;
    }

    const registry = app.utilityManagerRegistry;
    log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Looking for utility ID ${id} in registry: ${Object.keys(registry).join(', ')}`);

    const utility = Object.values(registry).find(u => u.id === id);
    if (utility) {
      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Found utility: ${utility.key}`);
      return utility.key;
    }
    return null;
  } catch (err) {
    log(Subsystems.MANAGER, Status.ERROR, `[Tour] Error finding utility key: ${err.message}`);
    return null;
  }
}

/**
 * Switch to a manager by name
 * @param {string} managerName - Manager name (e.g., "023.Lookup Manager" or "003.Profile")
 */
async function switchToManager(managerName) {
  if (!managerName) return;

  // Extract manager ID from name (e.g., "023.Lookup Manager" -> "23", "003.Profile" -> "3")
  const match = managerName.match(/^(\d+)\./);
  if (!match) {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] Cannot parse manager name: ${managerName}`);
    return;
  }

  const managerId = parseInt(match[1], 10);

  log(Subsystems.MANAGER, Status.INFO, `[Tour] Switching to manager: ${managerName} (ID: ${managerId})`);

  // Note: sidebar expansion is handled by launchTour() based on definition.sidebar,
  // not here — switchToManager only switches the active manager.

  const app = window.lithiumApp;
  if (!app) {
    log(Subsystems.MANAGER, Status.ERROR, '[Tour] app not available');
    return;
  }

  // Debug: log what's available
  log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Available registries: managerRegistry=${!!app.managerRegistry}, utilityManagerRegistry=${!!app.utilityManagerRegistry}`);
  if (app.managerRegistry) {
    log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Manager registry IDs: ${Object.keys(app.managerRegistry).join(', ')}`);
  }

  // Check if this is a utility manager by looking up the ID
  const utilityKey = findUtilityKeyById(managerId);
  if (utilityKey) {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Loading utility manager: ${utilityKey} (ID: ${managerId})`);
    try {
      if (typeof app.loadUtilityManager === 'function') {
        await app.loadUtilityManager(managerId);
        log(Subsystems.MANAGER, Status.INFO, `[Tour] Successfully loaded utility manager ${utilityKey}`);
      } else {
        log(Subsystems.MANAGER, Status.ERROR, '[Tour] app.loadUtilityManager not available');
      }
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to load utility manager ${utilityKey}: ${err.message}`);
    }
  } else if (app.managerRegistry?.[managerId]) {
    // Regular manager from registry
    try {
      if (typeof app.loadManager === 'function') {
        await app.loadManager(managerId);
        log(Subsystems.MANAGER, Status.INFO, `[Tour] Successfully loaded manager ${managerId}`);
      } else {
        log(Subsystems.MANAGER, Status.ERROR, '[Tour] app.loadManager not available');
      }
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to load manager ${managerId}: ${err.message}`);
    }
  } else {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] Manager ${managerId} not found in managerRegistry or utilityManagerRegistry for: ${managerName}`);
  }

  // Wait for the manager to load and UI to settle
  await new Promise(resolve => setTimeout(resolve, 600));
}

/**
 * Start a tour by ID. If not found, show the tour list instead.
 * @param {string|number} tourId - Tour ID
 * @param {Object} api - The API/conduit instance
 * @param {Object} options - Options including anchor for list popup
 */
async function launchTour(tourId, api, options = {}) {
  try {
    // End any active tour first (Shepherd or video), waiting for fade-out transition
    
    // Handle Shepherd tour
    if (_activeShepherdTour) {
      log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Cancelling active Shepherd tour before launching new one');
      const transitionDuration = getTransitionDuration$1();
      try {
        _activeShepherdTour.cancel();
      } catch (err) {
        log(Subsystems.MANAGER, Status.WARN, `[Tour] Error cancelling previous tour: ${err.message}`);
      }
      cleanupActiveTour();
      await new Promise(resolve => setTimeout(resolve, transitionDuration));
    }
    
    // Handle video tour
    if (_activeVideoPopup) {
      log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Closing active video tour before launching new one');
      closeVideoTour();
      const transitionDuration = getTransitionDuration$1();
      await new Promise(resolve => setTimeout(resolve, transitionDuration + 50));
    }

    const tours = await getTours(api);
    // Find tour by ID or by code/name
    const tour = tours.find(t =>
      String(t.id) === String(tourId) ||
      t.code === tourId ||
      t.name === tourId
    );

    if (!tour) {
      log(Subsystems.MANAGER, Status.WARN, `[Tour] Tour not found: ${tourId}, showing list`);
      // Fall back to showing the tour list
      showTourList(tours, options.anchor);
      return null;
    }

    log(Subsystems.MANAGER, Status.INFO, `[Tour] Launching tour: ${tour.name}`);

    const { definition } = tour;

    // Check for video tour - launch directly without Shepherd
    if (definition.video) {
      log(Subsystems.MANAGER, Status.INFO, `[Tour] Launching video tour: ${tour.name}`);
      await launchVideoTour(tour, options);
      return;
    }

    // Expand sidebar if tour requires it (sidebar: true in definition)
    // Use truthy check — the value may arrive as boolean true or string "true"
    const needsSidebar = definition.sidebar === true || definition.sidebar === 'true';
    log(Subsystems.MANAGER, Status.DEBUG,
      `[Tour] sidebar property: ${JSON.stringify(definition.sidebar)} (needsSidebar=${needsSidebar})`);
    if (needsSidebar) {
      expandSidebarIfCollapsed();
    }

    // Switch to target manager if specified
    if (definition.manager) {
      try {
        await switchToManager(definition.manager);
        // Give UI time to settle after manager switch
        await new Promise(resolve => setTimeout(resolve, 300));
      } catch (err) {
        log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to switch manager: ${err.message}`);
        // Continue with tour anyway - it might still work
      }
    }

    // Provide default onShowList callback if not provided
    const tourOptions = {
      ...options,
      anchor: options.anchor || document.querySelector('.lithium-footer-tours-btn'),
    };

    if (!tourOptions.onShowList) {
      tourOptions.onShowList = () => {
        showTourList(tours, tourOptions.anchor);
      };
    }

    let shepherd;
    try {
      shepherd = await buildShepherdTour(tour, tourOptions);
    } catch (buildErr) {
      log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to build tour: ${buildErr.message}`);
      throw buildErr;
    }

    // Add explicit start handler to log when tour actually shows
    shepherd.on('start', () => {
      log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour started: ${tour.name}`);
    });

    try {
      shepherd.start();
      log(Subsystems.MANAGER, Status.INFO, `[Tour] shepherd.start() called for: ${tour.name}`);
    } catch (startErr) {
      log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to start tour: ${startErr.message}`);
      throw startErr;
    }

    return shepherd;
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `[Tour] Failed to launch tour: ${error.message}`);
    console.error('[Tour] Launch error:', error);
    return null;
  }
}

// ========================================
// Tour List Popup
// ========================================

/**
 * Show the tour list popup
 * @param {Array} tours - Array of tour objects
 * @param {HTMLElement} anchor - Anchor element for positioning
 * @param {Object} options - Options
 * @param {Function} options.onSelect - Callback when a tour is selected
 */
function showTourList(tours, anchor = null, options = {}) {
  // Close existing popup
  hideTourList();

  const popup = document.createElement('div');
  popup.className = 'lithium-tour-list-popup';

  const tourItems = tours.map(t => {
    // Video tours get a play icon override (can still be customized via icon field)
    const isVideo = isVideoTour(t);
    const iconHtml = isVideo
      ? (t.definition.icon || '<fa fa-play></fa>')
      : (t.definition.icon || '<fa fa-signs-post></fa>');
    const videoIndicator = isVideo ? '<fa class="lithium-tour-list-item-video-indicator" fa-play></fa>' : '';
    return `
    <div class="lithium-tour-list-item${isVideo ? ' video-tour' : ''}" data-tour-id="${t.id}">
      <div class="lithium-tour-list-item-icon">
        ${iconHtml}
      </div>
      <div class="lithium-tour-list-item-info">
        <div class="lithium-tour-list-item-name">${t.name}</div>
      </div>
      <div class="lithium-tour-list-item-video-icon">${videoIndicator}</div>
    </div>
  `;
  }).join('');

  popup.innerHTML = `
    <div class="lithium-tour-list-header">
      <span class="lithium-tour-list-title">
        <fa fa-signs-post></fa>
        <span>Available Tours: ${tours.length}</span>
      </span>
      <button type="button" class="lithium-tour-list-close" title="Close">
        <fa fa-xmark></fa>
      </button>
    </div>
    <div class="lithium-tour-list-body">
      ${tours.length > 0 ? tourItems : '<div class="lithium-tour-list-empty">No tours available</div>'}
    </div>
  `;

  document.body.appendChild(popup);

  // Position relative to anchor or center screen
  if (anchor) {
    const rect = anchor.getBoundingClientRect();
    popup.style.bottom = `${window.innerHeight - rect.top + 8}px`;
    popup.style.right = `${window.innerWidth - rect.right}px`;
  } else {
    popup.style.top = '50%';
    popup.style.left = '50%';
    popup.style.transform = 'translate(-50%, -50%) scale(0.95)';
  }

  // Process icons
  if (typeof processIcons === 'function') {
    processIcons(popup);
  }

  // Animate in
  requestAnimationFrame(() => {
    popup.classList.add('visible');
    if (!anchor) {
      popup.style.transform = 'translate(-50%, -50%) scale(1)';
    }
  });

  _listPopup = popup;

  // Close button - fade out then remove
  popup.querySelector('.lithium-tour-list-close').addEventListener('click', () => {
    hideTourList({ fade: true });
  });

  // Tour item clicks - fade out popup then start tour
  popup.querySelectorAll('.lithium-tour-list-item').forEach(item => {
    item.addEventListener('click', () => {
      const tourId = parseInt(item.dataset.tourId, 10);
      // Fade out popup before starting tour
      hideTourList({ fade: true });
      // Use eventBus to launch - prevents double-launch when onSelect is also provided
      eventBus.emit('tour:select', { tourId });
    });
  });

  // Close on outside click — persistent handler removed by hideTourList
  const handleOutsideClick = (e) => {
    if (_listPopup && !popup.contains(e.target) && !anchor?.contains(e.target)) {
      hideTourList({ fade: true });
    }
  };
  const handleEscapeKey = (e) => {
    if (e.key === 'Escape') {
      hideTourList({ fade: true });
    }
  };

  // Store cleanup so hideTourList can remove them
  popup._cleanupOutsideClick = () => {
    document.removeEventListener('click', handleOutsideClick, true);
    document.removeEventListener('keydown', handleEscapeKey);
  };

  // Small delay to avoid immediate trigger from the opening click
  setTimeout(() => {
    document.addEventListener('click', handleOutsideClick, true);
    document.addEventListener('keydown', handleEscapeKey);
  }, 10);
}

/**
 * Hide the tour list popup
 * @param {Object} options - Options
 * @param {boolean} options.fade - Whether to fade out before removing (default: false)
 * @returns {Promise} Resolves when popup is hidden
 */
function hideTourList(options = {}) {
  return new Promise((resolve) => {
    if (!_listPopup) {
      resolve();
      return;
    }

    // Remove outside-click / escape listeners
    if (_listPopup._cleanupOutsideClick) {
      _listPopup._cleanupOutsideClick();
    }

    if (options.fade) {
      // Fade out before removing
      _listPopup.classList.remove('visible');
      const duration = getTransitionDuration$1();
      setTimeout(() => {
        if (_listPopup) {
          _listPopup.remove();
          _listPopup = null;
        }
        resolve();
      }, duration);
    } else {
      // Immediate removal
      _listPopup.remove();
      _listPopup = null;
      resolve();
    }
  });
}

// ========================================
// Video Tour Player
// ========================================

let _activeVideoPopup = null;
let _activeVideoElement = null;
let _videoTourHandleKeyDown = null;

const VIDEO_PLAYBACK_SPEEDS = [0.5, 0.75, 1, 1.25, 1.5, 2];

/**
 * Create and show video tour popup
 * @param {Object} tour - Video tour object
 * @param {Object} options - Options
 */
async function launchVideoTour(tour, options = {}) {
  const { definition } = tour;
  const videoSrc = definition.video;

  if (!videoSrc) {
    log(Subsystems.MANAGER, Status.ERROR, '[VideoTour] No video source defined');
    return;
  }

  log(Subsystems.MANAGER, Status.INFO, `[VideoTour] Launching: ${tour.name}`);

  // Close any existing tours gracefully with animation before starting new one
  
  // Handle Shepherd tour
  if (_activeShepherdTour) {
    log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] Closing active Shepherd tour');
    const transitionDuration = getTransitionDuration$1();
    try {
      _activeShepherdTour.cancel();
    } catch (err) {
      log(Subsystems.MANAGER, Status.WARN, `[VideoTour] Error cancelling Shepherd tour: ${err.message}`);
    }
    cleanupActiveTour();
    await new Promise(resolve => setTimeout(resolve, transitionDuration));
  }
  
  // Handle existing video tour
  if (_activeVideoPopup) {
    closeVideoTour();
    const duration = getTransitionDuration$1();
    await new Promise(resolve => setTimeout(resolve, duration + 50));
  }

  // Get icon from definition
  const iconHtml = definition.icon || '<fa fa-play></fa>';
  const tourName = expandMacros(tour.name);

  // Create popup element with new layout
  const popup = document.createElement('div');
  popup.className = 'lithium-video-tour-popup';
  popup.innerHTML = `
    <div class="lithium-video-tour-header">
      <div class="subpanel-header-group">
        <div class="lithium-video-tour-header-primary">
          ${iconHtml}
          <span class="lithium-video-tour-header-title">${tourName}</span>
        </div>
        <div class="lithium-video-tour-header-placeholder"></div>
        <button type="button" class="lithium-video-tour-header-btn lithium-video-tour-list-btn" data-video-action="list" title="Available Tours">
          <fa fa-signs-post></fa>
        </button>
        <button type="button" class="lithium-video-tour-header-btn lithium-video-tour-language-btn" data-video-action="language" title="Select Language" style="display: none;">
          <fa fa-earth-americas></fa>
        </button>
        <button type="button" class="lithium-video-tour-header-close" data-video-action="close" title="Close">
          <fa fa-xmark></fa>
        </button>
      </div>
    </div>
    <div class="lithium-video-tour-content">
      <video class="lithium-video-tour-player" preload="metadata" autoplay>
        <source src="${videoSrc}" type="video/mp4">
      </video>
      <div class="lithium-video-tour-play-overlay">
        <button type="button" class="lithium-video-tour-back-10-btn" data-video-action="back-10" title="Back 10 seconds" data-tip-placement="left">
          <fa fa-arrow-rotate-left-10></fa>
        </button>
        <button type="button" class="lithium-video-tour-back-30-btn" data-video-action="back-30" title="Back 30 seconds" data-tip-placement="left">
          <fa fa-arrow-rotate-left-30></fa>
        </button>
        <button type="button" class="lithium-video-tour-back-to-start-btn" data-video-action="back-to-start" title="Back to start" data-tip-placement="left">
          <fa fa-arrow-rotate-left></fa>
        </button>
        <button type="button" class="lithium-video-tour-forward-10-btn" data-video-action="forward-10" title="Forward 10 seconds" data-tip-placement="right">
          <fa fa-arrow-rotate-right-10></fa>
        </button>
        <button type="button" class="lithium-video-tour-forward-30-btn" data-video-action="forward-30" title="Forward 30 seconds" data-tip-placement="right">
          <fa fa-arrow-rotate-right-30></fa>
        </button>
        <button type="button" class="lithium-video-tour-forward-to-end-btn" data-video-action="forward-to-end" title="Forward to end" data-tip-placement="right">
          <fa fa-arrow-rotate-right></fa>
        </button>
        <div class="lithium-video-tour-play-btn-wrapper">
          <button type="button" class="lithium-video-tour-big-play-btn" data-video-action="play">
            <fa fa-pause></fa>
          </button>
        </div>
        <button type="button" class="lithium-video-tour-repeat-btn" data-video-action="repeat" title="Repeat" data-tip-placement="bottom">
          <fa fa-repeat></fa>
        </button>
        <button type="button" class="lithium-video-tour-subtitles-btn" data-video-action="subtitles" title="Subtitles" data-tip-placement="top">
          <fa fa-subtitles></fa>
        </button>
      </div>
      <div class="lithium-video-tour-captions-container"></div>
    </div>
    <div class="lithium-video-tour-scrubber-row">
      <input type="range" class="lithium-video-tour-scrubber" min="0" max="1000" value="0">
    </div>
    <div class="lithium-video-tour-controls">
      <div class="lithium-video-tour-buttons">
        <div class="lithium-video-tour-volume">
          <button type="button" class="lithium-video-tour-mute-btn" data-video-action="mute" title="Mute/Unmute">
            <fa fa-volume-high></fa>
          </button>
          <input type="range" class="lithium-video-tour-volume-slider" min="0" max="100" value="100" title="Volume">
        </div>
        <div class="lithium-video-tour-time">0:00 / 0:00</div>
        <div class="lithium-video-tour-spacer"></div>
        <button type="button" class="lithium-video-tour-skip-btn" data-video-action="skip" title="Playback Speed">
          <fa fa-rabbit-running></fa>
        </button>
        <button type="button" class="lithium-video-tour-speed-btn" data-video-action="speed" title="Playback Speed">
          <span>1x</span>
        </button>
      </div>
    </div>
    <div class="lithium-video-tour-resize-handle-br"></div>
    <div class="lithium-video-tour-resize-handle-bl"></div>
    <div class="lithium-video-tour-resize-handle-tr"></div>
    <div class="lithium-video-tour-resize-handle-tl"></div>
  `;

  document.body.appendChild(popup);

  // Load saved video tour state
  const savedState = loadVideoState();
  
  // Video aspect ratio is 2:3 (vertical video) - calculate popup dimensions with 92px overhead
  const VIDEO_ASPECT_RATIO = 2 / 3;
  const OVERHEAD = 92;
  const MIN_WIDTH = 450;
  const MIN_HEIGHT = Math.round(MIN_WIDTH / VIDEO_ASPECT_RATIO + OVERHEAD); // ~767
  const DEFAULT_WIDTH = 464;
  const DEFAULT_HEIGHT = Math.round(DEFAULT_WIDTH / VIDEO_ASPECT_RATIO + OVERHEAD); // ~780
  
  // Set initial size from saved state or defaults
  // Check for valid saved dimensions - be explicit about checking for number type
  let initialWidth = DEFAULT_WIDTH;
  let initialHeight = DEFAULT_HEIGHT;
  let hasSavedSize = false;
  
  if (savedState && typeof savedState.width === 'number' && savedState.width > 0) {
    initialWidth = savedState.width;
    hasSavedSize = true;
  }
  if (savedState && typeof savedState.height === 'number' && savedState.height > 0) {
    initialHeight = savedState.height;
  }
  
  // Only enforce minimums if no saved size (minimums should not override user's saved size)
  if (!hasSavedSize) {
    initialWidth = Math.max(initialWidth, MIN_WIDTH);
    initialHeight = Math.max(initialHeight, MIN_HEIGHT);
  }
  
  const initialX = (savedState?.x !== undefined && savedState.x !== null && !isNaN(savedState.x)) ? savedState.x : null;
  const initialY = (savedState?.y !== undefined && savedState.y !== null && !isNaN(savedState.y)) ? savedState.y : null;
  
  popup.style.width = `${initialWidth}px`;
  popup.style.height = `${initialHeight}px`;
  
  // Center in viewport or restore saved position
  const viewportWidth = window.innerWidth;
  const viewportHeight = window.innerHeight;
  if (initialX !== null && initialY !== null) {
    // Validate saved position is within viewport bounds
    const maxX = window.innerWidth - 100;
    const maxY = window.innerHeight - 100;
    popup.style.left = `${Math.max(0, Math.min(initialX, maxX))}px`;
    popup.style.top = `${Math.max(0, Math.min(initialY, maxY))}px`;
  } else {
    popup.style.left = `${Math.max(0, (viewportWidth - initialWidth) / 2)}px`;
    popup.style.top = `${Math.max(0, (viewportHeight - initialHeight) / 2)}px`;
  }

  const video = popup.querySelector('.lithium-video-tour-player');
  const scrubber = popup.querySelector('.lithium-video-tour-scrubber');
  const timeDisplay = popup.querySelector('.lithium-video-tour-time');
  const bigPlayBtn = popup.querySelector('.lithium-video-tour-big-play-btn');
  const muteBtn = popup.querySelector('.lithium-video-tour-mute-btn');
  const volumeSlider = popup.querySelector('.lithium-video-tour-volume-slider');
  const speedBtn = popup.querySelector('.lithium-video-tour-speed-btn');
  const header = popup.querySelector('.lithium-video-tour-header');
  const playOverlay = popup.querySelector('.lithium-video-tour-play-overlay');

  // Track initial aspect ratio (2:3 for vertical video + 92px overhead)
  let aspectRatio = DEFAULT_WIDTH / DEFAULT_HEIGHT; // ~0.595

  // Get video natural size for initial dimensions
  const setInitialSize = () => {
    // If we have valid saved dimensions, don't override - use those (they reflect user's drag/resize)
    // Use explicit type check
    const hasSavedWidth = savedState && typeof savedState.width === 'number' && savedState.width > 0;
    const hasSavedHeight = savedState && typeof savedState.height === 'number' && savedState.height > 0;
    if (hasSavedWidth && hasSavedHeight) {
      return;
    }
    
    if (video.videoWidth && video.videoHeight) {
      aspectRatio = video.videoWidth / video.videoHeight;
      const maxWidth = window.innerWidth * 0.9;
      const maxHeight = window.innerHeight * 0.85;
      let width = video.videoWidth;
      let height = video.videoHeight;

      // Scale down if needed
      if (width > maxWidth) {
        const ratio = maxWidth / width;
        width = maxWidth;
        height = height * ratio;
      }
      if (height > maxHeight) {
        const ratio = maxHeight / height;
        height = maxHeight;
        width = width / ratio;
      }

      // Calculate overall popup dimensions: video width + 92px overhead
      const popupWidth = Math.round(width);
      const popupHeight = Math.round(height) + OVERHEAD;
      aspectRatio = popupWidth / popupHeight;

      popup.style.width = `${Math.max(popupWidth, MIN_WIDTH)}px`;
      popup.style.height = `${Math.max(popupHeight, MIN_HEIGHT)}px`;
    } else {
      // Default size if metadata not loaded yet and no saved state
      popup.style.width = `${DEFAULT_WIDTH}px`;
      popup.style.height = `${DEFAULT_HEIGHT}px`;
      aspectRatio = DEFAULT_WIDTH / DEFAULT_HEIGHT;
    }

    // Only center if no saved position
    if (!savedState?.x || !savedState?.y) {
      centerPopup(popup);
    }
  };

  const centerPopup = (el) => {
    const rect = el.getBoundingClientRect();
    const left = (window.innerWidth - rect.width) / 2;
    const top = (window.innerHeight - rect.height) / 2;
    el.style.left = `${Math.max(0, left)}px`;
    el.style.top = `${Math.max(0, top)}px`;
  };

  // Set initial size after metadata loads (only if no saved state)
  if (!savedState?.width || savedState.width <= 0 || !savedState?.height || savedState.height <= 0) {
    video.addEventListener('loadedmetadata', setInitialSize, { once: true });
    setTimeout(() => {
      if (!video.videoWidth && (!savedState?.width || savedState.width <= 0)) {
        popup.style.width = `${DEFAULT_WIDTH}px`;
        popup.style.height = `${DEFAULT_HEIGHT}px`;
        centerPopup(popup);
      }
    }, 1000);
  }

  // Fade in after a small delay, then autoplay
  requestAnimationFrame(() => {
    popup.classList.add('visible');
    
    // Start playing after fade-in completes (200ms)
    setTimeout(() => {
      const playPromise = video.play();
      if (playPromise !== undefined) {
        playPromise.catch(err => {
          log(Subsystems.MANAGER, Status.DEBUG, `[VideoTour] Autoplay prevented: ${err.message}`);
        });
      }
    }, 200);
  });

  // Make draggable via header
  let isDragging = false;
  let dragStartX = 0;
  let dragStartY = 0;
  let popupStartX = 0;
  let popupStartY = 0;

  const handleDragStart = (e) => {
    if (e.target.closest('.lithium-video-tour-header-btn')) return;
    isDragging = true;
    popup.classList.add('dragging');
    dragStartX = e.clientX;
    dragStartY = e.clientY;
    const rect = popup.getBoundingClientRect();
    popupStartX = rect.left;
    popupStartY = rect.top;
    document.addEventListener('mousemove', handleDragMove);
    document.addEventListener('mouseup', handleDragEnd);
    e.preventDefault();
  };

  const handleDragMove = (e) => {
    if (!isDragging) return;
    const deltaX = e.clientX - dragStartX;
    const deltaY = e.clientY - dragStartY;
    let newX = popupStartX + deltaX;
    let newY = popupStartY + deltaY;

    // Constrain to viewport
    const rect = popup.getBoundingClientRect();
    const minVisible = 50;
    newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
    newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

    popup.style.left = `${newX}px`;
    popup.style.top = `${newY}px`;
  };

  const handleDragEnd = () => {
    isDragging = false;
    popup.classList.remove('dragging');
    document.removeEventListener('mousemove', handleDragMove);
    document.removeEventListener('mouseup', handleDragEnd);
    // Save state after drag ends
    saveVideoState({
      captionsEnabled,
      playbackSpeed: VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex],
      volume: video.volume,
      muted: video.muted,
      x: parseInt(popup.style.left, 10),
      y: parseInt(popup.style.top, 10),
      width: popup.offsetWidth,
      height: popup.offsetHeight,
    });
  };

  header.addEventListener('mousedown', handleDragStart);

  // Resize handles - maintain aspect ratio
  const setupResizeHandle = (handleEl, direction) => {
    let startX, startY, startWidth, startHeight, startLeft, startTop;

    const startResize = (e) => {
      e.preventDefault();
      e.stopPropagation();
      popup.classList.add('resizing');
      startX = e.clientX;
      startY = e.clientY;
      startWidth = popup.offsetWidth;
      startHeight = popup.offsetHeight;
      const rect = popup.getBoundingClientRect();
      startLeft = rect.left;
      startTop = rect.top;
      document.addEventListener('mousemove', doResize);
      document.addEventListener('mouseup', stopResize);
    };

    const doResize = (e) => {
      const deltaX = e.clientX - startX;
      e.clientY - startY;

      let newWidth = startWidth;
      let newHeight = startHeight;
      let newLeft = startLeft;
      let newTop = startTop;

      // Use the recorded aspect ratio
      const currentAspect = aspectRatio || (startWidth / startHeight);
      const startRight = startLeft + startWidth;
      const startBottom = startTop + startHeight;

      if (direction === 'br') {
        // Bottom-right: keep top-left fixed, expand right and down
        newWidth = startWidth + deltaX;
        newHeight = newWidth / currentAspect;
        // newLeft and newTop stay at start values
      } else if (direction === 'tr') {
        // Top-right: keep bottom-left fixed, expand right and up
        newWidth = startWidth + deltaX;
        newHeight = newWidth / currentAspect;
        newTop = startBottom - newHeight;
        // newLeft stays at start value
      } else if (direction === 'bl') {
        // Bottom-left: keep top-right fixed, expand left and down
        newWidth = startWidth - deltaX;
        newHeight = newWidth / currentAspect;
        newLeft = startRight - newWidth;
        // newTop stays at start value
      } else if (direction === 'tl') {
        // Top-left: keep bottom-right fixed, expand left and up
        newWidth = startWidth - deltaX;
        newHeight = newWidth / currentAspect;
        newLeft = startRight - newWidth;
        newTop = startBottom - newHeight;
      }

      // Constrain to viewport
      if (newLeft < -newWidth + 100) newLeft = -newWidth + 100;
      if (newTop < -newHeight + 100) newTop = -newHeight + 100;
      if (newLeft + newWidth > window.innerWidth - 50) {
        newWidth = window.innerWidth - newLeft - 50;
        newHeight = newWidth / currentAspect;
      }
      if (newTop + newHeight > window.innerHeight - 50) {
        newHeight = window.innerHeight - newTop - 50;
        newWidth = newHeight * currentAspect;
      }

      popup.style.width = `${Math.round(newWidth)}px`;
      popup.style.height = `${Math.round(newHeight)}px`;
      popup.style.left = `${newLeft}px`;
      popup.style.top = `${newTop}px`;
    };

    const stopResize = () => {
      popup.classList.remove('resizing');
      document.removeEventListener('mousemove', doResize);
      document.removeEventListener('mouseup', stopResize);
      // Save state after resize ends
      saveVideoState({
        captionsEnabled,
        playbackSpeed: VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex],
        volume: video.volume,
        muted: video.muted,
        x: parseInt(popup.style.left, 10),
        y: parseInt(popup.style.top, 10),
        width: popup.offsetWidth,
        height: popup.offsetHeight,
      });
    };

    handleEl.addEventListener('mousedown', startResize);
  };

  popup.querySelectorAll('.lithium-video-tour-resize-handle-br').forEach(h => setupResizeHandle(h, 'br'));
  popup.querySelectorAll('.lithium-video-tour-resize-handle-bl').forEach(h => setupResizeHandle(h, 'bl'));
  popup.querySelectorAll('.lithium-video-tour-resize-handle-tr').forEach(h => setupResizeHandle(h, 'tr'));
  popup.querySelectorAll('.lithium-video-tour-resize-handle-tl').forEach(h => setupResizeHandle(h, 'tl'));

  // Format time helper - includes tenths for precise scrubbing
  const formatTime = (seconds) => {
    if (isNaN(seconds)) return '00:00.0';
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    const tenths = Math.floor((seconds % 1) * 10);
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${tenths}`;
  };

  // Scrubber sync state
  let scrubberAnimationId = null;

  // Force time display update - called on any seek
  const updateTimeDisplay = () => {
    const current = formatTime(video.currentTime);
    const duration = formatTime(video.duration || 0);
    timeDisplay.textContent = `${current} / ${duration}`;
    if (video.duration) {
      scrubber.value = (video.currentTime / video.duration) * 1000;
    }
  };

  const startScrubberSync = () => {
    if (scrubberAnimationId) return;
    const syncLoop = () => {
      if (!video.paused && video.duration) {
        // Update both scrubber and time display every frame
        scrubber.value = (video.currentTime / video.duration) * 1000;
        const current = formatTime(video.currentTime);
        const duration = formatTime(video.duration);
        timeDisplay.textContent = `${current} / ${duration}`;
      }
      scrubberAnimationId = requestAnimationFrame(syncLoop);
    };
    scrubberAnimationId = requestAnimationFrame(syncLoop);
  };

  const stopScrubberSync = () => {
    if (scrubberAnimationId) {
      cancelAnimationFrame(scrubberAnimationId);
      scrubberAnimationId = null;
    }
  };

  // Video event handlers
  video.addEventListener('play', () => {
    startScrubberSync();
  });
  video.addEventListener('pause', () => {
    stopScrubberSync();
    updateTimeDisplay();
  });
  // Track repeat state
  let isRepeating = false;

  // Toggle repeat - when on, loops video indefinitely until paused or turned off
  const toggleRepeat = () => {
    isRepeating = !isRepeating;
    playOverlay.classList.toggle('repeat-on', isRepeating);
    log(Subsystems.MANAGER, Status.DEBUG, `[VideoTour] Repeat: ${isRepeating}`);
  };

  // Toggle captions/subtitles - when button is lit (subtitles-on), captions are OFF
  const toggleCaptionsFromOverlay = () => {
    captionsEnabled = !captionsEnabled;
    // Button is lit when captions are OFF
    playOverlay.classList.toggle('subtitles-on', !captionsEnabled);
    popup.classList.toggle('captions-off', !captionsEnabled);

    if (captionsEnabled) {
      handleCaptionTimeUpdate();
    } else {
      while (captionsContainer.firstChild) {
        captionsContainer.removeChild(captionsContainer.firstChild);
      }
    }
    saveState();
    log(Subsystems.MANAGER, Status.DEBUG, `[VideoTour] Subtitles toggle: ${captionsEnabled ? 'ON' : 'OFF'}`);
  };

  // Handle video ended - restart if repeating
  const handleVideoEnded = () => {
    stopScrubberSync();
    updateTimeDisplay();
    if (isRepeating) {
      log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] Video ended, looping (repeat on)');
      video.currentTime = 0;
      video.play();
     } else {
       log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] Video ended - show overlay');
       requestAnimationFrame(() => {
         popup.classList.add('ended');
       });
     }
  };

  video.addEventListener('ended', handleVideoEnded);
  video.addEventListener('loadedmetadata', updateTimeDisplay);

  // Play/pause toggle
  const togglePlay = () => {
    if (video.ended) {
      video.currentTime = 0;
      video.play();
    } else if (video.paused) {
      video.play();
    } else {
      video.pause();
    }
  };

  // Handle click on the big play button itself
  bigPlayBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    if (video.ended) {
      video.currentTime = 0;
      video.play();
    } else {
      togglePlay();
    }
    updateTimeDisplay();
  });

  // Also handle click on the overlay background (not on the buttons) to toggle play/pause
  playOverlay.addEventListener('click', (e) => {
    // Check if the click was on any of the 8 compass buttons - if so, ignore (they have their own handlers)
    const compassButtons = playOverlay.querySelectorAll('.lithium-video-tour-back-10-btn, .lithium-video-tour-back-30-btn, .lithium-video-tour-back-to-start-btn, .lithium-video-tour-forward-10-btn, .lithium-video-tour-forward-30-btn, .lithium-video-tour-forward-to-end-btn, .lithium-video-tour-repeat-btn, .lithium-video-tour-subtitles-btn');
    let isCompassButton = false;
    compassButtons.forEach(btn => {
      if (btn.contains(e.target)) {
        isCompassButton = true;
      }
    });
    if (isCompassButton) return;

    // Check if click was on the play button wrapper or big play button
    const wrapper = e.target.closest('.lithium-video-tour-play-btn-wrapper');
    const playBtn = e.target.closest('.lithium-video-tour-big-play-btn');
    
    // If clicked on the center wrapper or button, or on the overlay background (not on buttons)
    if (wrapper || playBtn || (e.target === playOverlay)) {
      if (video.ended) {
        video.currentTime = 0;
        video.play();
      } else {
        togglePlay();
      }
      updateTimeDisplay();
    }
  });

  video.addEventListener('play', () => {
    log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] Play event');
    bigPlayBtn.innerHTML = '<fa fa-pause></fa>';
    popup.classList.remove('paused', 'ended');
  });
  video.addEventListener('pause', () => {
    log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] Pause event');
    bigPlayBtn.innerHTML = '<fa fa-play></fa>';
    popup.classList.add('paused');
    popup.classList.remove('ended');
  });
   video.addEventListener('ended', () => {
     log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] Video ended event fired');
     bigPlayBtn.innerHTML = '<fa fa-rotate-left></fa>';
     requestAnimationFrame(() => {
       popup.classList.add('ended');
     });
     popup.classList.remove('paused');
   });

  // Scrubber seeking
  scrubber.addEventListener('input', () => {
    if (video.duration) {
      video.currentTime = (scrubber.value / 1000) * video.duration;
      // Update time display and captions immediately on scrub without fade animation
      updateTimeDisplay();
      if (captionsEnabled && captionsData.length > 0) {
        updateCaptionsForTime(video.currentTime * 1000, false);
      }
    }
    saveState();
  });

  // Drag on video to seek
  let isDraggingVideo = false;
  let videoDragStartX = 0;
  let videoDragStartTime = 0;

  const contentArea = popup.querySelector('.lithium-video-tour-content');
  
  contentArea.addEventListener('mousedown', (e) => {
    // Only handle left click, not on controls or overlay
    if (e.button !== 0) return;
    if (e.target.closest('.lithium-video-tour-controls')) return;
    if (e.target.closest('.lithium-video-tour-play-overlay')) return;
    
    isDraggingVideo = true;
    videoDragStartX = e.clientX;
    videoDragStartTime = video.currentTime;
    contentArea.style.cursor = 'grabbing';
    e.preventDefault();
  });

  document.addEventListener('mousemove', (e) => {
    if (!isDraggingVideo) return;
    
    const deltaX = e.clientX - videoDragStartX;
    const pixelsPerSecond = (video.duration || 1) / 10; // 10% of video per 100px
    let newTime = videoDragStartTime + (deltaX * pixelsPerSecond);
    
    // Clamp to valid range
    newTime = Math.max(0, Math.min(video.duration || 0, newTime));
    video.currentTime = newTime;
    
    // Update time display and captions without animation
    updateTimeDisplay();
    if (captionsEnabled && captionsData.length > 0) {
      updateCaptionsForTime(newTime * 1000, false);
    }
  });

  document.addEventListener('mouseup', () => {
    if (isDraggingVideo) {
      isDraggingVideo = false;
      contentArea.style.cursor = '';
    }
  });

  // Captions - parse and display
  const captionsContainer = popup.querySelector('.lithium-video-tour-captions-container');
  let captionsEnabled = true;

  // Parse captions from definition
  const captionsData = [];
  if (Array.isArray(definition.captions)) {
    for (let i = 0; i < definition.captions.length; i++) {
      const cap = definition.captions[i];
      if (cap.length >= 3) {
        // Format: [startSeconds, endSeconds, "text"]
        // Start/end can be like "000.500" or "2.500" - parse as float
        const startTime = parseFloat(cap[0]) * 1000; // Convert to ms
        const endTime = parseFloat(cap[1]) * 1000;
        const text = cap[2];
        if (!isNaN(startTime) && !isNaN(endTime) && text) {
          captionsData.push({ index: i, startTime, endTime, text });
        }
      }
    }
    captionsData.sort((a, b) => a.startTime - b.startTime);
  }

  const CAPTION_FADE_MS = 500; // Match CSS transition duration

  const updateCaptionsForTime = (timeMs, animate = true) => {
    if (!captionsEnabled || captionsData.length === 0) return;

    // Each caption is completely independent - fade in/out based only on its own timing
    captionsData.forEach(caption => {
      const isActive = timeMs >= caption.startTime && timeMs <= caption.endTime;
      
      // Find existing element for this caption index
      let captionEl = captionsContainer.querySelector(`.lithium-video-tour-caption[data-index="${caption.index}"]`);
      
      if (isActive) {
        if (!captionEl) {
          // Create new caption element
          captionEl = document.createElement('div');
          captionEl.className = 'lithium-video-tour-caption';
          captionEl.dataset.index = caption.index;
          captionEl.innerHTML = caption.text;
          captionsContainer.appendChild(captionEl);
          
          // Process icons in caption (e.g., <fa fa-party-horn></fa>)
          if (typeof processIcons === 'function') {
            processIcons(captionEl);
          }
          
          // Fade in
          if (animate) {
            void captionEl.offsetWidth;
            captionEl.classList.add('visible');
          } else {
            captionEl.classList.add('visible');
          }
        } else if (!captionEl.classList.contains('visible')) {
          // Already exists but hidden - fade it back in
          if (animate) {
            void captionEl.offsetWidth;
            captionEl.classList.add('visible');
          } else {
            captionEl.classList.add('visible');
          }
        }
      } else {
        // Caption is not active right now
        if (captionEl && captionEl.classList.contains('visible')) {
          // Fade it out
          captionEl.classList.remove('visible');
          // Remove after fade completes
          setTimeout(() => {
            if (captionEl.parentNode) {
              captionEl.remove();
            }
          }, CAPTION_FADE_MS);
        }
      }
    });
  };

  // Update captions on time update
  const handleCaptionTimeUpdate = () => {
    if (captionsEnabled && captionsData.length > 0) {
      updateCaptionsForTime(video.currentTime * 1000, true);
    }
  };

  video.addEventListener('timeupdate', handleCaptionTimeUpdate);

  // Volume control
  const updateVolumeIcon = () => {
    if (video.muted || video.volume === 0) {
      muteBtn.innerHTML = '<fa fa-volume-xmark></fa>';
    } else if (video.volume < 0.5) {
      muteBtn.innerHTML = '<fa fa-volume-low></fa>';
    } else {
      muteBtn.innerHTML = '<fa fa-volume-high></fa>';
    }
  };

  muteBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    video.muted = !video.muted;
    updateVolumeIcon();
    saveState();
  });

  volumeSlider.addEventListener('input', (e) => {
    e.stopPropagation();
    video.volume = volumeSlider.value / 100;
    video.muted = false;
    updateVolumeIcon();
    saveState();
  });

  // Apply saved volume and mute state
  if (savedState?.volume !== undefined) {
    video.volume = savedState.volume;
    volumeSlider.value = savedState.volume * 100;
  }
  if (savedState?.muted) {
    video.muted = true;
  }
  updateVolumeIcon();

  // Playback speed - load saved or default to 1x
  let currentSpeedIndex = VIDEO_PLAYBACK_SPEEDS.indexOf(savedState?.playbackSpeed || 1);
  if (currentSpeedIndex === -1) currentSpeedIndex = 2; // Default to 1x
  video.playbackRate = VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex];
  speedBtn.querySelector('span').textContent = `${VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex]}x`;

  speedBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    currentSpeedIndex = (currentSpeedIndex + 1) % VIDEO_PLAYBACK_SPEEDS.length;
    const newSpeed = VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex];
    video.playbackRate = newSpeed;
    speedBtn.querySelector('span').textContent = `${newSpeed}x`;
    // Save state when speed changes
    saveState();
  });

  // Captions - load saved state (default enabled)
  captionsEnabled = savedState?.captionsEnabled !== false;
  popup.classList.toggle('captions-off', !captionsEnabled);
  // Button is lit when captions are OFF
  playOverlay.classList.toggle('subtitles-on', !captionsEnabled);

  // Set up state saving on drag/resize end and close
  const saveState = () => {
    saveVideoState({
      captionsEnabled,
      playbackSpeed: VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex],
      volume: video.volume,
      muted: video.muted,
      x: parseInt(popup.style.left, 10),
      y: parseInt(popup.style.top, 10),
      width: popup.offsetWidth,
      height: popup.offsetHeight,
    });
  };

  // Keyboard handler - Space for play/pause, Left/Right for back/forward, ESC to close
  const setupKeyboardHandler = () => {
    _videoTourHandleKeyDown = (e) => {
      if (!_activeVideoPopup) {
        document.removeEventListener('keydown', _videoTourHandleKeyDown);
        _videoTourHandleKeyDown = null;
        return;
      }
      if (e.key === 'Escape') {
        closeVideoTour();
      } else if (e.key === ' ' || e.key === 'Spacebar') {
        e.preventDefault();
        togglePlay();
      } else if (e.key === 'ArrowLeft') {
        e.preventDefault();
        if (e.shiftKey) {
          video.currentTime = Math.max(0, video.currentTime - 30);
        } else {
          video.currentTime = Math.max(0, video.currentTime - 10);
        }
        updateTimeDisplay();
      } else if (e.key === 'ArrowRight') {
        e.preventDefault();
        if (e.shiftKey) {
          video.currentTime = Math.min(video.duration || 0, video.currentTime + 30);
        } else {
          video.currentTime = Math.min(video.duration || 0, video.currentTime + 10);
        }
        updateTimeDisplay();
      } else if (e.key === 'r' || e.key === 'R') {
        e.preventDefault();
        toggleRepeat();
      } else if (e.key === 's' || e.key === 'S') {
        e.preventDefault();
        toggleCaptionsFromOverlay();
      }
    };
    document.addEventListener('keydown', _videoTourHandleKeyDown);
  };

  setupKeyboardHandler();

  // Save state on resize end - inline in each stopResize function
  _videoStateCleanup = () => {
    saveState();
    video.removeEventListener('timeupdate', handleCaptionTimeUpdate);
    stopScrubberSync();
  };

  const closeBtn = popup.querySelector('.lithium-video-tour-header-close');
  closeBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    closeVideoTour();
  });

  // Button click handler
  const handleButtonClick = async (e) => {
    const button = e.target.closest('[data-video-action]');
    if (!button) return;

    // Stop propagation to prevent click from reaching playOverlay handler
    e.stopPropagation();

    const action = button.dataset.videoAction;
    switch (action) {
      case 'close':
        closeVideoTour();
        break;
      case 'list':
        closeVideoTour();
        // Delay to allow fade-out before showing list
        setTimeout(async () => {
          const api = window.lithiumApp?.api;
          if (api) {
            const tours = await getTours(api);
            showTourList(tours, null);
          }
        }, getTransitionDuration$1() + 50);
        break;
      case 'language':
        showVideoTourLanguageMenu(tour, button);
        break;
      case 'play':
        togglePlay();
        updateTimeDisplay();
        break;
      case 'back-10':
        video.currentTime = Math.max(0, video.currentTime - 10);
        updateTimeDisplay();
        break;
      case 'back-30':
        video.currentTime = Math.max(0, video.currentTime - 30);
        updateTimeDisplay();
        break;
      case 'back-to-start':
        video.currentTime = 0;
        if (!video.paused) video.play();
        updateTimeDisplay();
        break;
      case 'forward-10':
        video.currentTime = Math.min(video.duration || 0, video.currentTime + 10);
        updateTimeDisplay();
        break;
      case 'forward-30':
        video.currentTime = Math.min(video.duration || 0, video.currentTime + 30);
        updateTimeDisplay();
        break;
      case 'forward-to-end':
        video.currentTime = video.duration || 0;
        if (!video.paused) video.play();
        updateTimeDisplay();
        break;
      case 'repeat':
        toggleRepeat();
        break;
      case 'subtitles':
        toggleCaptionsFromOverlay();
        break;
      case 'mute':
        video.muted = !video.muted;
        updateVolumeIcon();
        break;
      case 'skip':
      case 'speed':
        currentSpeedIndex = (currentSpeedIndex + 1) % VIDEO_PLAYBACK_SPEEDS.length;
        const newSpeed = VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex];
        video.playbackRate = newSpeed;
        speedBtn.querySelector('span').textContent = `${newSpeed}x`;
        break;
    }
  };

  popup.addEventListener('click', handleButtonClick);

  // Show language button if this tour has alternate-language variants available
  const languageBtn = popup.querySelector('.lithium-video-tour-language-btn');
  if (languageBtn) {
    const altLanguages = getAlternateLanguagesForTour(tour);
    if (altLanguages.length > 0) {
      languageBtn.style.display = '';
    }
  }

  // Store references for cleanup
  _activeVideoPopup = popup;
  _activeVideoElement = video;

  // Process icons
  if (typeof processIcons === 'function') {
    processIcons(popup);
  }

  log(Subsystems.MANAGER, Status.INFO, `[VideoTour] Popup created for: ${tour.name}`);
}

// ========================================
// Video Tour Language Switcher
// ========================================

let _activeLanguagePopup = null;

/**
 * Get alternate-language variants for a tour, excluding 'default' and the tour's own key_idx.
 * @param {Object} tour - Tour object with { id, definition }
 * @returns {Array<{locale: string, keyIdx: number, countryCode: string, displayName: string}>}
 */
function getAlternateLanguagesForTour(tour) {
  const languages = tour?.definition?.languages;
  if (!languages || typeof languages !== 'object') return [];

  const currentId = Number(tour.id);
  const result = [];

  for (const [locale, keyIdx] of Object.entries(languages)) {
    // Exclude the 'default' pointer
    if (locale === 'default') continue;
    const idNum = Number(keyIdx);
    if (!Number.isFinite(idNum)) continue;
    // Exclude the currently-playing tour
    if (idNum === currentId) continue;

    result.push({
      locale,
      keyIdx: idNum,
      countryCode: getCountryCode(locale),
      displayName: getLocaleDisplayName(locale),
    });
  }

  // Sort alphabetically by display name for consistent ordering
  result.sort((a, b) => a.displayName.localeCompare(b.displayName));
  return result;
}

/**
 * Hide the video tour language popup if open.
 * @private
 */
function hideVideoTourLanguageMenu() {
  if (!_activeLanguagePopup) return;
  const popup = _activeLanguagePopup;
  _activeLanguagePopup = null;
  if (popup._cleanup) popup._cleanup();
  popup.classList.remove('visible');
  const duration = getTransitionDuration$1();
  setTimeout(() => {
    if (popup && popup.parentNode) popup.remove();
  }, duration);
}

/**
 * Show the language-selection popup for a video tour, anchored to a button.
 * Selecting a language closes the current tour and launches the selected one.
 * @param {Object} tour - Current tour object
 * @param {HTMLElement} anchor - Anchor element (earth-americas button)
 * @private
 */
function showVideoTourLanguageMenu(tour, anchor) {
  // Toggle off if already open
  if (_activeLanguagePopup) {
    hideVideoTourLanguageMenu();
    return;
  }

  const languages = getAlternateLanguagesForTour(tour);
  if (languages.length === 0) {
    log(Subsystems.MANAGER, Status.DEBUG, '[VideoTour] No alternate languages available');
    return;
  }

  const items = languages.map(l => `
    <div class="lithium-video-tour-language-item" data-tour-id="${l.keyIdx}" data-locale="${l.locale}">
      <span class="lithium-video-tour-language-flag">${getFlagSvg(l.countryCode, { width: 22, height: 16 })}</span>
      <span class="lithium-video-tour-language-name">${l.displayName}</span>
    </div>
  `).join('');

  const popup = document.createElement('div');
  popup.className = 'lithium-video-tour-language-popup';
  popup.innerHTML = `
    <div class="lithium-video-tour-language-header">
      <fa fa-earth-americas></fa>
      <span>Select Language</span>
    </div>
    <div class="lithium-video-tour-language-body">
      ${items}
    </div>
  `;

  document.body.appendChild(popup);

  // Position as dropdown from the anchor button (top-right of popup at bottom-right of button)
  const rect = anchor.getBoundingClientRect();
  popup.style.top = `${rect.bottom + 4}px`;
  popup.style.right = `${window.innerWidth - rect.right}px`;

  // Process icons
  if (typeof processIcons === 'function') {
    processIcons(popup);
  }

  // Animate in next frame
  requestAnimationFrame(() => {
    popup.classList.add('visible');
  });

  // Item click: close current tour, launch selected
  popup.querySelectorAll('.lithium-video-tour-language-item').forEach(item => {
    item.addEventListener('click', async (e) => {
      e.stopPropagation();
      const newTourId = parseInt(item.dataset.tourId, 10);
      if (!Number.isFinite(newTourId)) return;
      log(Subsystems.MANAGER, Status.INFO, `[VideoTour] Switching language to tour ${newTourId}`);

      hideVideoTourLanguageMenu();
      // closeVideoTour is called inside launchTour; fire via eventBus for consistency
      const api = window.lithiumApp?.api;
      if (!api) {
        log(Subsystems.MANAGER, Status.ERROR, '[VideoTour] API unavailable for language switch');
        return;
      }
      await launchTour(newTourId, api);
    });
  });

  // Dismiss on outside click / Escape
  const handleOutside = (e) => {
    if (!popup.contains(e.target) && !anchor.contains(e.target)) {
      hideVideoTourLanguageMenu();
    }
  };
  const handleEsc = (e) => {
    if (e.key === 'Escape') hideVideoTourLanguageMenu();
  };
  popup._cleanup = () => {
    document.removeEventListener('click', handleOutside, true);
    document.removeEventListener('keydown', handleEsc);
  };
  // Delay a tick to avoid catching the opening click
  setTimeout(() => {
    document.addEventListener('click', handleOutside, true);
    document.addEventListener('keydown', handleEsc);
  }, 10);

  _activeLanguagePopup = popup;
}

/**
 * Close video tour popup with fade out
 */
function closeVideoTour() {
  // Close language popup if open
  if (_activeLanguagePopup) {
    hideVideoTourLanguageMenu();
  }

  // Remove keyboard handler
  if (_videoTourHandleKeyDown) {
    document.removeEventListener('keydown', _videoTourHandleKeyDown);
    _videoTourHandleKeyDown = null;
  }

  if (_activeVideoElement) {
    _activeVideoElement.pause();
    _activeVideoElement = null;
  }

  if (_activeVideoPopup) {
    // Save state before closing
    if (_videoStateCleanup) {
      const popup = _activeVideoPopup;
      const videoEl = popup.querySelector('.lithium-video-tour-player');
      const speedIndex = VIDEO_PLAYBACK_SPEEDS.indexOf(videoEl?.playbackRate || 1);
      const captionsEnabled = !popup.classList.contains('captions-off');
      saveVideoState({
        captionsEnabled,
        playbackSpeed: VIDEO_PLAYBACK_SPEEDS[speedIndex >= 0 ? speedIndex : 2],
        volume: videoEl?.volume ?? 1,
        muted: videoEl?.muted ?? false,
        x: parseInt(popup.style.left, 10),
        y: parseInt(popup.style.top, 10),
        width: popup.offsetWidth,
        height: popup.offsetHeight,
      });
      _videoStateCleanup = null;
    }

    // Fade out then remove
    _activeVideoPopup.classList.remove('visible');
    const duration = getTransitionDuration$1();
    setTimeout(() => {
      if (_activeVideoPopup) {
        _activeVideoPopup.remove();
        _activeVideoPopup = null;
      }
    }, duration);
  }
}

// ========================================
// Event Bus Integration
// ========================================

/**
 * Initialize tour event listeners
 */
function initTours() {
  // Listen for tour:start events from manager-ui
  eventBus.on('tour:start', async (event) => {
    const { tourId } = event.detail || event;
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Event received: start tour ${tourId}`);
    // API is accessed via the app instance on window
    const api = window.lithiumApp?.api;
    if (!api) {
      log(Subsystems.MANAGER, Status.ERROR, '[Tour] API not available');
      return;
    }

    // Check if this is a video tour
    const tours = await getTours(api);
    const tour = tours.find(t =>
      String(t.id) === String(tourId) ||
      t.code === tourId ||
      t.name === tourId
    );

    if (tour && isVideoTour(tour)) {
      await launchVideoTour(tour);
    } else {
      await launchTour(tourId, api);
    }
  });

  // Listen for tour:select events from showTourList popup
  eventBus.on('tour:select', async (event) => {
    const { tourId } = event.detail || event;
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Event received: select tour ${tourId}`);
    const api = window.lithiumApp?.api;
    if (!api) {
      log(Subsystems.MANAGER, Status.ERROR, '[Tour] API not available');
      return;
    }

    // Get tours and find the selected tour
    const tours = await getTours(api);
    const tour = tours.find(t => t.id === tourId);

    if (tour) {
      if (isVideoTour(tour)) {
        await launchVideoTour(tour);
      } else {
        await launchTour(tourId, api);
      }
    } else {
      log(Subsystems.MANAGER, Status.WARN, `[Tour] Tour not found for id: ${tourId}`);
    }
  });

  log(Subsystems.MANAGER, Status.INFO, '[Tour] Initialized');
}

// Auto-initialize
initTours();

/**
 * Manager Popup State
 *
 * Centralized state management for popups.
 * Uses callback registry to avoid circular dependencies.
 */


// ===== Close Callbacks =====

const closeCallbacks = [];

/**
 * Register a callback to be called when closeAllPopups is called.
 * @param {Function} callback - Function that closes a popup (receives skipAnimation boolean)
 */
function registerCloseCallback(callback) {
  closeCallbacks.push(callback);
}

// ===== Close All Popups =====

/**
 * Close all active popups
 * @param {boolean} skipAnimation - If true, skip animation (default: false)
 */
function closeAllPopups$1(skipAnimation = false) {

  // Call registered close callbacks
  closeCallbacks.forEach(cb => cb(skipAnimation));

  // Dispatch event for other popup systems to close
  document.dispatchEvent(new CustomEvent('close-all-popups'));
}

// ===== State =====
const managerTours = new Map();

let activeShortcutsPopup = null;
let activeShortcutsButton = null;
let activeShortcutsTransitioning = false;
let activeToursPopup = null;
let activeToursButton = null;
let activeToursTransitioning = false;
let activeTrainingPopup = null;
let activeTrainingButton = null;
let activeTrainingTransitioning = false;
let activeManagerMenuPopup$1 = null;
let activeExportPopup$1 = null;

// ===== Constants =====

const DEFAULT_TRANSITION_DURATION = 350;

/**
 * Get the transition duration from computed style or default
 * @param {HTMLElement} element - Element to get transition duration from
 * @returns {number} Transition duration in ms
 */
function getTransitionDuration(element) {
  if (!element) return DEFAULT_TRANSITION_DURATION;
  const computed = getComputedStyle(element);
  const duration = parseFloat(computed.transitionDuration) * 1000;
  return duration || DEFAULT_TRANSITION_DURATION;
}

/**
 * Generic hide function for a popup with animation support
 * @param {HTMLElement} popup - Popup element to hide
 * @param {Function} clearState - Function to clear popup state
 * @param {boolean} skipAnimation - If true, remove immediately
 */
function hidePopup(popup, clearState, skipAnimation = false) {
  if (!popup) {
    clearState();
    return;
  }

  if (skipAnimation || !popup.parentNode) {
    if (popup && popup.parentNode) popup.remove();
    clearState();
    document.removeEventListener('click', handleOutsideClick);
    document.removeEventListener('keydown', handleEscapeKey);
    return;
  }

  popup.classList.remove('visible');
  const duration = getTransitionDuration(popup);

  setTimeout(() => {
    if (popup && popup.parentNode) {
      popup.remove();
    }
    clearState();
  }, duration);
}

// ===== Positioning =====

/**
 * Position a popup relative to a button
 * @param {HTMLElement} popup - Popup element
 * @param {HTMLElement} button - Reference button
 * @param {string} alignment - 'header-dropdown', 'footer-riseup', 'left', 'right', 'below-left', 'below-right'
 */
function positionPopup(popup, button, alignment = 'left') {
  const buttonRect = button.getBoundingClientRect();
  popup.style.position = 'fixed';
  popup.style.zIndex = '10001';

  // Need to wait for popup to be in DOM and rendered to get dimensions
  requestAnimationFrame(() => {
    requestAnimationFrame(() => {
      if (alignment === 'header-dropdown') {
        popup.style.top = `${buttonRect.bottom + 1}px`;
        popup.style.right = `${window.innerWidth - buttonRect.right}px`;
        popup.style.left = 'auto';
        popup.style.bottom = 'auto';
      } else if (alignment === 'footer-riseup') {
        popup.style.bottom = `${window.innerHeight - buttonRect.top + 1}px`;
        popup.style.right = `${window.innerWidth - buttonRect.right}px`;
        popup.style.top = 'auto';
        popup.style.left = 'auto';
      } else {
        // Position above the button
        const popupRect = popup.getBoundingClientRect();
        popup.style.top = `${buttonRect.top - popupRect.height - 8}px`;
        if (alignment === 'right') {
          popup.style.right = `${window.innerWidth - buttonRect.right}px`;
          popup.style.left = 'auto';
        } else {
          popup.style.left = `${buttonRect.left}px`;
          popup.style.right = 'auto';
        }
      }
    });
  });
}

// ===== Close Handlers =====

/**
 * Handle clicks outside popups
 */
function handleOutsideClick(event) {
  const popups = [
    activeShortcutsPopup,
    activeToursPopup,
    activeTrainingPopup,
    activeManagerMenuPopup$1,
    activeExportPopup$1
  ].filter(Boolean);

  // Also check for zoom popup
  const zoomPopup = document.querySelector('.zoom-popup');
  if (zoomPopup) popups.push(zoomPopup);

  const isOutside = popups.every(p => !p.contains(event.target));
  const isButton = event.target.closest('.manager-ui-btn') || event.target.closest('.zoom-btn');

  if (isOutside && !isButton) {
    // Close with animation when clicking outside
    closeAllPopups(false);
    // Hide zoom popup if open
    if (zoomPopup && zoomPopup.classList.contains('visible')) {
      __vitePreload(async () => { const {hideZoomPopup} = await import('./zoom-control-BOsJBiLz.js');return { hideZoomPopup }},true              ?__vite__mapDeps([0,1,2,3,4,5,6]):void 0,import.meta.url).then(({ hideZoomPopup }) => hideZoomPopup(false));
    }
  }
}

/**
 * Handle escape key to close popups
 */
function handleEscapeKey(event) {
  if (event.key === 'Escape') {
    closeAllPopups(false);
    // Also close zoom popup if open
    const zoomPopup = document.querySelector('.zoom-popup');
    if (zoomPopup && zoomPopup.classList.contains('visible')) {
      __vitePreload(async () => { const {hideZoomPopup} = await import('./zoom-control-BOsJBiLz.js');return { hideZoomPopup }},true              ?__vite__mapDeps([0,1,2,3,4,5,6]):void 0,import.meta.url).then(({ hideZoomPopup }) => hideZoomPopup(false));
    }
  }
}

// ===== Close All Popups =====

/**
 * Close all active popups
 * @param {boolean} skipAnimation - If true, skip animation (when opening new popup)
 */
function closeAllPopups(skipAnimation = false) {
  // Toggle off button backgrounds
  if (activeShortcutsButton) {
    activeShortcutsButton.classList.remove('popup-active');
    activeShortcutsButton = null;
  }
  if (activeToursButton) {
    activeToursButton.classList.remove('popup-active');
    activeToursButton = null;
  }
  if (activeTrainingButton) {
    activeTrainingButton.classList.remove('popup-active');
    activeTrainingButton = null;
  }

  // Use skipAnimation to determine if we animate when closing
  // When opening a new popup, skipAnimation=true for instant close
  // When clicking outside or pressing escape, skipAnimation=false for animated close
  hideShortcutsPopup(skipAnimation);
  hideToursPopup(skipAnimation);
  hideTrainingPopup(skipAnimation);
  hideManagerMenuPopup$1(skipAnimation);

  // Close zoom popup if open
  const zoomPopup = document.querySelector('.zoom-popup');
  if (zoomPopup && zoomPopup.classList.contains('visible')) {
    __vitePreload(async () => { const {hideZoomPopup} = await import('./zoom-control-BOsJBiLz.js');return { hideZoomPopup }},true              ?__vite__mapDeps([0,1,2,3,4,5,6]):void 0,import.meta.url).then(({ hideZoomPopup }) => hideZoomPopup(skipAnimation));
  }

  // Dispatch event for other popup systems
  document.dispatchEvent(new CustomEvent('close-all-popups'));
}

// ===== Shortcuts Popup =====

async function showShortcutsPopup(button) {
  if (activeShortcutsPopup) {
    hideShortcutsPopup();
    return;
  }
  if (activeShortcutsTransitioning) {
    return;
  }
  activeShortcutsTransitioning = true;
  closeAllPopups(false);

  activeShortcutsButton = button;
  button.classList.add('popup-active');

  const popup = document.createElement('div');
  popup.className = 'manager-ui-popup manager-header-popup manager-ui-shortcuts-popup';

  const shortcuts = getAllShortcuts();

  const groupedShortcuts = {};
  shortcuts.forEach(s => {
    const category = s.options?.category || 'General';
    if (!groupedShortcuts[category]) {
      groupedShortcuts[category] = [];
    }
    groupedShortcuts[category].push(s);
  });

  let shortcutsHTML = '';
  Object.entries(groupedShortcuts).forEach(([category, items]) => {
    shortcutsHTML += `<div class="manager-ui-shortcuts-group">
      <div class="manager-ui-shortcuts-group-title">${category}</div>
      ${items.map(s => `
        <div class="manager-ui-shortcuts-item">
          <span class="manager-ui-shortcuts-key">${s.key}</span>
          <span class="manager-ui-shortcuts-desc">${s.description}</span>
        </div>
      `).join('')}
    </div>`;
  });

  if (!shortcutsHTML) {
    shortcutsHTML = '<p class="manager-ui-empty">No shortcuts registered</p>';
  }

  popup.innerHTML = `
    <div class="manager-ui-popup-header">
      <span class="manager-ui-popup-title">
        <fa fa-command></fa>
        <span>Keyboard Shortcuts</span>
      </span>
    </div>
    <div class="manager-ui-popup-content">
      ${shortcutsHTML}
    </div>
  `;

  document.body.appendChild(popup);
  positionPopup(popup, button, 'header-dropdown');
  processIcons(popup);
  activeShortcutsPopup = popup;

  setTimeout(() => popup.classList.add('visible'), 10);

  setTimeout(() => {
    document.addEventListener('click', handleOutsideClick);
    document.addEventListener('keydown', handleEscapeKey);
  }, 0);
}

function hideShortcutsPopup(skipAnimation = false) {
  const clearState = () => {
    activeShortcutsPopup = null;
    if (activeShortcutsButton) {
      activeShortcutsButton.classList.remove('popup-active');
      activeShortcutsButton = null;
    }
    activeShortcutsTransitioning = false;
  };
  hidePopup(activeShortcutsPopup, clearState, skipAnimation);
}

// ===== Tours Popup =====

async function showToursPopup(button, managerId) {
  if (activeToursPopup) {
    hideToursPopup();
    return;
  }
  if (activeToursTransitioning) {
    return;
  }
  activeToursTransitioning = true;
  closeAllPopups(false);

  activeToursButton = button;
  button.classList.add('popup-active');

  const popup = document.createElement('div');
  popup.className = 'manager-ui-popup manager-footer-popup manager-ui-tours-popup';

  const tours = await getManagerTours(managerId);

  popup.innerHTML = `
    <div class="manager-ui-popup-header">
      <span class="manager-ui-popup-title">
        <button type="button" class="manager-ui-tours-all-btn" data-tour-action="all" title="Available Tours">
          <fa fa-signs-post></fa>
        </button>
        <span>Tour Suggestions</span>
      </span>
    </div>
    <div class="manager-ui-popup-content">
      ${tours.map(tour => `
        <button type="button" class="manager-ui-menu-item" data-tour-id="${tour.id}">
          ${tour.icon || '<fa fa-signs-post></fa>'}
          <span>${tour.label}</span>
        </button>
      `).join('')}
    </div>
  `;

  popup.querySelectorAll('[data-tour-id]').forEach(btn => {
    btn.addEventListener('click', async () => {
      const tourId = parseInt(btn.dataset.tourId, 10);
      const tour = tours.find(t => t.id === tourId);
      const api = window.lithiumApp?.api;
      if (tour && api) {
        if (isVideoTour(tour)) {
          await launchVideoTour(tour);
        } else {
          await launchTour(tourId, api);
        }
      }
      hideToursPopup();
    });
  });

  popup.querySelector('[data-tour-action="all"]')?.addEventListener('click', async () => {
    const api = window.lithiumApp?.api;
    if (api) {
      const allTours = await getTours(api);
      showTourList(allTours);
    }
    hideToursPopup();
  });

  document.body.appendChild(popup);
  positionPopup(popup, button, 'footer-riseup');
  processIcons(popup);
  activeToursPopup = popup;

  setTimeout(() => popup.classList.add('visible'), 10);

  setTimeout(() => {
    document.addEventListener('click', handleOutsideClick);
    document.addEventListener('keydown', handleEscapeKey);
  }, 0);

  // Listen for close-all-popups event from other popup systems
  const closeOnAllPopups = () => hideToursPopup();
  document.addEventListener('close-all-popups', closeOnAllPopups);
  popup._closeOnAllPopupsHandler = closeOnAllPopups;
}

// ===== Training Popup =====

async function showTrainingPopup(button, _managerId) {
  if (activeTrainingPopup) {
    hideTrainingPopup();
    return;
  }
  if (activeTrainingTransitioning) {
    return;
  }
  activeTrainingTransitioning = true;
  closeAllPopups(false);

  activeTrainingButton = button;
  button.classList.add('popup-active');

  const popup = document.createElement('div');
  popup.className = 'manager-ui-popup manager-footer-popup manager-ui-training-popup';

  popup.innerHTML = `
    <div class="manager-ui-popup-header">
      <span class="manager-ui-popup-title">
        <fa fa-graduation-cap></fa>
        <span>Training Courses</span>
      </span>
    </div>
    <div class="manager-ui-popup-content">
      <p class="manager-ui-empty">Training coming soon...</p>
    </div>
  `;

  document.body.appendChild(popup);
  positionPopup(popup, button, 'footer-riseup');
  processIcons(popup);
  activeTrainingPopup = popup;

  setTimeout(() => popup.classList.add('visible'), 10);

  setTimeout(() => {
    document.addEventListener('click', handleOutsideClick);
    document.addEventListener('keydown', handleEscapeKey);
  }, 0);

  // Listen for close-all-popups event from other popup systems
  const closeOnAllPopups = () => hideTrainingPopup();
  document.addEventListener('close-all-popups', closeOnAllPopups);
  popup._closeOnAllPopupsHandler = closeOnAllPopups;
}

// Helper function to get manager tours (moved from manager-ui.js)
// Uses getToursForManager from tour.js which ONLY compares numeric IDs
// "003.Profile" matches "003.User Profile" because both are ID 3
async function getManagerTours(managerId) {
  // If already cached in managerTours Map, use that
  if (managerTours.has(managerId)) {
    return managerTours.get(managerId);
  }

  const api = window.lithiumApp?.api;
  if (!api) {
    log(Subsystems.MANAGER, Status.WARN, '[Popup] API not available for getManagerTours');
    return [];
  }

  try {
    // Convert numeric ID to "003.X" format for getToursForManager
    // The actual name doesn't matter - only the numeric ID is used for matching
    let managerIdString;
    if (String(managerId).includes('.')) {
      managerIdString = managerId;
    } else {
      managerIdString = `${String(parseInt(managerId, 10)).padStart(3, '0')}.Manager`;
    }

    log(Subsystems.MANAGER, Status.DEBUG, `[Popup] getManagerTours for: ${managerIdString} (raw: ${managerId})`);

    // getToursForManager ONLY compares numeric IDs from the manager strings
    const tours = await getToursForManager(api, managerIdString);

    const toursList = tours.map(t => ({
      id: t.id,
      label: t.name,
      icon: t.definition?.icon || '',
    }));

    // Cache in managerTours Map
    if (toursList.length > 0) {
      managerTours.set(managerId, toursList);
    }

    log(Subsystems.MANAGER, Status.DEBUG, `[Popup] Found ${toursList.length} tours for manager ${managerIdString}`);
    return toursList;
  } catch (err) {
    log(Subsystems.MANAGER, Status.ERROR, `[Popup] Failed to get tours: ${err.message}`);
    return [];
  }
}

function hideToursPopup(skipAnimation = false) {
  // Remove close-all-popups listener before clearing state
  if (activeToursPopup?._closeOnAllPopupsHandler) {
    document.removeEventListener('close-all-popups', activeToursPopup._closeOnAllPopupsHandler);
  }
  const clearState = () => {
    activeToursPopup = null;
    if (activeToursButton) {
      activeToursButton.classList.remove('popup-active');
      activeToursButton = null;
    }
    activeToursTransitioning = false;
  };
  hidePopup(activeToursPopup, clearState, skipAnimation);
}

// ===== Training Popup =====

function hideTrainingPopup(skipAnimation = false) {
  // Remove close-all-popups listener before clearing state
  if (activeTrainingPopup?._closeOnAllPopupsHandler) {
    document.removeEventListener('close-all-popups', activeTrainingPopup._closeOnAllPopupsHandler);
  }
  const clearState = () => {
    activeTrainingPopup = null;
    if (activeTrainingButton) {
      activeTrainingButton.classList.remove('popup-active');
      activeTrainingButton = null;
    }
    activeTrainingTransitioning = false;
  };
  hidePopup(activeTrainingPopup, clearState, skipAnimation);
}

// ===== Manager Menu Popup =====

function hideManagerMenuPopup$1(skipAnimation = false) {
  const clearState = () => {
    activeManagerMenuPopup$1 = null;
  };
  hidePopup(activeManagerMenuPopup$1, clearState, skipAnimation);
}

 // ===== Consolidated Popup System =====

 /**
  * Unified popup state for use by consolidated functions
  */
 let activePopup = null;
 let activePopupButton = null;
 let activePopupOutsideHandler = null;
let activePopupEscapeHandler = null;
  let activePopupCloseAllHandler = null;
  let activePopupRepositionHandler = null;

 /**
  * Show a popup with standardized behavior
  * - If already open on the same button, toggles it closed
  * - Handles animation from scale(0.5) to scale(1)
  * - For footer-riseup: repositions on scroll/resize
  * - Sets up event listeners for outside click, escape key, and close-all-popups
  * 
  * @param {HTMLElement} btn - Button that toggles the popup
  * @param {string} className - CSS class for the popup
  * @param {string} content - Inner HTML content
  * @param {Object} options - Options
  * @param {string} options.position - 'header-dropdown' (default) or 'footer-riseup'
  * @param {string} options.title - Optional title text
  * @param {Function} options.onSelect - Callback when item selected (receives element)
  * @param {string} options.selector - CSS selector for clickable items (default: '[data-value]')
  * @returns {HTMLElement} The created popup element
  */
 function showPopup(btn, className, content, options = {}) {
   const {
     position = 'header-dropdown',
     title,
     onSelect,
     selector = '[data-value]',
   } = options;

   // Toggle: if already open on same button, close it instead
   if (activePopup && activePopupButton === btn) {
     hideActivePopup();
     return null;
   }

   // Close any existing popup first
   hideActivePopup();
   closeAllPopups(false);

   // Create popup element
   const popup = document.createElement('div');
   popup.className = className;
   
   if (title) {
     popup.innerHTML = `
       <div class="manager-ui-popup-header">
         <span class="manager-ui-popup-title">${title}</span>
       </div>
       <div class="manager-ui-popup-content">${content}</div>
     `;
   } else {
     popup.innerHTML = content;
   }

   document.body.appendChild(popup);
   positionPopup(popup, btn, position);

   // Track state
   activePopup = popup;
   activePopupButton = btn;
   btn.classList.add('popup-active');

   // Animate in
   setTimeout(() => popup.classList.add('visible'), 10);

   // Handle item selections
   if (onSelect) {
     popup.querySelectorAll(selector).forEach(item => {
       item.addEventListener('click', () => {
         onSelect(item, item.textContent.trim());
         hideActivePopup();
       });
     });
   }

   // Set up close handlers
   activePopupOutsideHandler = (evt) => {
     if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
       hideActivePopup();
     }
   };

   activePopupEscapeHandler = (evt) => {
     if (evt.key === 'Escape') {
       hideActivePopup();
     }
   };

   activePopupCloseAllHandler = () => hideActivePopup();

   // Reposition handler for footer-riseup popups (rises up from below)
   activePopupRepositionHandler = () => {
     positionPopup(popup, btn, position);
   };

   setTimeout(() => {
     document.addEventListener('click', activePopupOutsideHandler);
     document.addEventListener('keydown', activePopupEscapeHandler);
     document.addEventListener('close-all-popups', activePopupCloseAllHandler);
     // Add repositioning for footer-riseup (rises up from below)
     if (position === 'footer-riseup') {
       window.addEventListener('resize', activePopupRepositionHandler);
       document.addEventListener('scroll', activePopupRepositionHandler, true);
     }
   }, 0);

   return popup;
 }

/**
 * Hide the active popup with proper animation
 * @param {boolean} skipAnimation - If true, remove immediately
 */
function hideActivePopup(skipAnimation = false) {
   if (!activePopup) {
     clearPopupState();
     return;
   }

   if (skipAnimation || !activePopup.parentNode) {
     if (activePopup && activePopup.parentNode) {
       activePopup.remove();
     }
     clearPopupState();
     return;
   }

   // Remove visible class to trigger reverse animation (scale down)
   activePopup.classList.remove('visible');

   const duration = getTransitionDuration(activePopup);
   setTimeout(() => {
     if (activePopup && activePopup.parentNode) {
       activePopup.remove();
     }
     clearPopupState();
   }, duration);
 }

 function clearPopupState() {
   if (activePopupButton) {
     activePopupButton.classList.remove('popup-active');
     activePopupButton = null;
   }
   if (activePopupOutsideHandler) {
     document.removeEventListener('click', activePopupOutsideHandler);
     activePopupOutsideHandler = null;
   }
   if (activePopupEscapeHandler) {
     document.removeEventListener('keydown', activePopupEscapeHandler);
     activePopupEscapeHandler = null;
   }
   if (activePopupCloseAllHandler) {
     document.removeEventListener('close-all-popups', activePopupCloseAllHandler);
     activePopupCloseAllHandler = null;
   }
   if (activePopupRepositionHandler) {
     window.removeEventListener('resize', activePopupRepositionHandler);
     document.removeEventListener('scroll', activePopupRepositionHandler, true);
     activePopupRepositionHandler = null;
   }
   activePopup = null;
 }

/**
 * Menu Service
 *
 * Fetches and caches the main menu data from QueryRef 046.
 * Menu items are grouped and include icons and count badges.
 *
 * The lithium.json managers section acts as a restrictive filter:
 * only managers listed and enabled (true) in that config will appear
 * in the menu, regardless of what the database returns.
 */


// QueryRef for main menu
const MENU_QUERY_REF = 46;

// localStorage key for caching menu data
const MENU_CACHE_KEY = 'lithium_menu_data';
const MENU_CACHE_TIMESTAMP_KEY = 'lithium_menu_cache_time';
const CACHE_TTL_MS = 5 * 60 * 1000; // 5 minutes

// In-memory cache
let menuCache = null;

// Cached set of enabled manager IDs from lithium.json
let enabledManagerIds = null;

/**
 * Parse the enabled manager IDs from lithium.json managers config.
 * Keys are in format "NNN.Name" where NNN is the numeric ID.
 * Only keys with value === true are considered enabled.
 * @returns {Set<number>} Set of enabled manager IDs
 */
/**
 * Get enabled manager IDs from lithium.json config.
 * Fetches the config directly if not already loaded.
 * @returns {Set<number>} Set of enabled manager IDs
 */
async function getEnabledManagerIdsAsync() {
  // Return cached result if available
  if (enabledManagerIds !== null && enabledManagerIds.size > 0) {
    return enabledManagerIds;
  }

  // Ensure config is loaded
  if (!isConfigLoaded()) {
    await loadConfig();
  }

  const managersConfig = getConfigValue('managers', {});
  const ids = new Set();

  if (managersConfig && typeof managersConfig === 'object') {
    for (const [key, value] of Object.entries(managersConfig)) {
      // Only include managers that are explicitly enabled (true)
      if (value === true) {
        // Parse the numeric ID from the key (e.g., "007.Query Manager" -> 7)
        const match = key.match(/^(\d+)\./);
        if (match) {
          const id = parseInt(match[1], 10);
          if (!isNaN(id) && id > 0) {
            ids.add(id);
          }
        }
      }
    }
  }

  enabledManagerIds = ids;
  log(Subsystems.MANAGER, Status.INFO, `[MenuService] Filter enabled: ${ids.size} managers`);
  
  return ids;
}

/**
 * Get enabled manager IDs (synchronous version - requires config already loaded).
 * @returns {Set<number>} Set of enabled manager IDs (empty if config not ready)
 */
function getEnabledManagerIds() {
  // Return cached result if available
  if (enabledManagerIds !== null) {
    return enabledManagerIds;
  }

  // If config not loaded, return empty - use async version instead
  if (!isConfigLoaded()) {
    return new Set();
  }

  const managersConfig = getConfigValue('managers', {});
  const ids = new Set();

  if (managersConfig && typeof managersConfig === 'object') {
    for (const [key, value] of Object.entries(managersConfig)) {
      if (value === true) {
        const match = key.match(/^(\d+)\./);
        if (match) {
          const id = parseInt(match[1], 10);
          if (!isNaN(id) && id > 0) {
            ids.add(id);
          }
        }
      }
    }
  }

  enabledManagerIds = ids;
  return ids;
}

/**
 * Menu item structure from QueryRef 046:
 * {
 *   grpname: string,      // Group name
 *   grpnum: number,       // Group ID
 *   modname: string,      // Module/manager name
 *   modnum: number,       // Module/manager ID
 *   grpsort: number,      // Group sort order
 *   modsort: number,      // Module sort order
 *   entries: number,      // Count badge value
 *   collection: string|Object // JSON containing icon info
 * }
 */

/**
 * Parse collection field to extract icon information
 * The collection contains JSON with an "icon" field (HTML <i> tag) and index"
 * @param {string|Object} collection - Collection field from query result
 * @returns {Object} Parsed icon info with fallback, includes index for filtering
 */
function parseCollection(collection) {
  const fallback = { iconHtml: '<fa fa-cube></fa>', index: 0, visible: true };

  if (!collection) {
    return fallback;
  }

  try {
    const parsed = typeof collection === 'string' ? JSON.parse(collection) : collection;
    
    const iconHtml = normalizeIconHtml(parsed.icon, fallback.iconHtml) || fallback.iconHtml;
    
    // Items with negative index should be hidden (Main Menu = -2, Login = -1)
    const index = parsed.index !== undefined ? parsed.index : 0;
    
    return {
      iconHtml,
      index,
      visible: index >= 0, // Only show items with index >= 0
    };
  } catch (_e) {
    return fallback;
  }
}

/**
 * Transform flat menu items into grouped structure.
 * Filters out managers not enabled in lithium.json config.
 * Uses the 'index' field from collection (matches lithium.json key numbers like 001, 007, etc.)
 * @param {Array} items - Raw menu items from QueryRef 046
 * @param {Set<number>} allowedManagerIds - Optional set of allowed manager IDs (if not provided, will be fetched)
 * @returns {Array} Grouped menu structure
 */
function groupMenuItems(items, allowedManagerIds = null) {
  const groups = new Map();
  // Use provided set, or fall back to sync version (which may be empty if config not loaded)
  const allowedIds = allowedManagerIds || getEnabledManagerIds();

  items.forEach((item) => {
    const groupId = item.grpnum;
    const collectionInfo = parseCollection(item.collection);
    
    // Skip items that should be hidden (negative index like Main Menu, Login)
    if (!collectionInfo.visible) {
      return;
    }

    // Filter by index from collection (matches lithium.json key numbers)
    // Index 0 means no index specified - allow through (for backwards compatibility)
    // Non-zero index must be in allowed list
    if (allowedIds.size > 0 && collectionInfo.index > 0 && !allowedIds.has(collectionInfo.index)) {
      return;
    }

    if (!groups.has(groupId)) {
      groups.set(groupId, {
        id: groupId,
        name: item.grpname,
        sortOrder: item.grpsort,
        items: [],
      });
    }

    const group = groups.get(groupId);
    group.items.push({
      managerId: collectionInfo.index || item.modnum,  // Use index (matches lithium.json keys), fallback to modnum
      name: item.modname,
      sortOrder: item.modsort,
      count: item.entries || 0,
      iconHtml: collectionInfo.iconHtml,
      index: collectionInfo.index,
    });
  });

  // Sort groups by sortOrder, then by name
  const sortedGroups = Array.from(groups.values()).sort((a, b) => {
    if (a.sortOrder !== b.sortOrder) {
      return a.sortOrder - b.sortOrder;
    }
    return a.name.localeCompare(b.name);
  });

  // Sort items within each group
  sortedGroups.forEach((group) => {
    group.items.sort((a, b) => {
      if (a.sortOrder !== b.sortOrder) {
        return a.sortOrder - b.sortOrder;
      }
      return a.name.localeCompare(b.name);
    });
  });

  return sortedGroups;
}

/**
 * Fetch menu data from server using QueryRef 046
 * @param {Object} api - API client instance
 * @returns {Promise<Array>} Grouped menu data
 */
async function fetchMenu(api) {
  try {
    log(Subsystems.MANAGER, Status.INFO, '[MenuService] Fetching menu from QueryRef 046');

    // Ensure config is loaded before filtering
    const allowedManagerIds = await getEnabledManagerIdsAsync();

    const items = await authQuery(api, MENU_QUERY_REF);

    if (!Array.isArray(items)) {
      throw new Error('Invalid menu data format');
    }

    const grouped = groupMenuItems(items, allowedManagerIds);

    // Update cache
    menuCache = grouped;
    cacheMenuData(grouped);

    log(Subsystems.MANAGER, Status.INFO, `[MenuService] Menu loaded: ${grouped.length} groups, ${items.length} items`);

    return grouped;
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `[MenuService] Failed to fetch menu: ${error.message}`);
    throw error;
  }
}

/**
 * Get cached menu data from localStorage
 * @returns {Array|null} Cached menu data or null
 */
function getCachedMenuData() {
  try {
    const cached = localStorage.getItem(MENU_CACHE_KEY);
    const timestamp = localStorage.getItem(MENU_CACHE_TIMESTAMP_KEY);

    if (!cached || !timestamp) {
      return null;
    }

    // Check TTL
    const age = Date.now() - parseInt(timestamp, 10);
    if (age > CACHE_TTL_MS) {
      clearMenuCache();
      return null;
    }

    return JSON.parse(cached);
  } catch (_e) {
    return null;
  }
}

/**
 * Cache menu data to localStorage
 * @param {Array} data - Menu data to cache
 */
function cacheMenuData(data) {
  try {
    localStorage.setItem(MENU_CACHE_KEY, JSON.stringify(data));
    localStorage.setItem(MENU_CACHE_TIMESTAMP_KEY, String(Date.now()));
  } catch (_e) {
    // Non-fatal - localStorage may be unavailable or full
    log(Subsystems.MANAGER, Status.WARN, '[MenuService] Failed to cache menu data');
  }
}

/**
 * Clear cached menu data
 */
function clearMenuCache() {
  menuCache = null;
  try {
    localStorage.removeItem(MENU_CACHE_KEY);
    localStorage.removeItem(MENU_CACHE_TIMESTAMP_KEY);
  } catch (_e) {
    // Non-fatal
  }
}

/**
 * Get menu data - returns cached data if available, otherwise fetches
 * @param {Object} api - API client instance
 * @param {Object} options - Options
 * @param {boolean} options.refresh - Force refresh from server
 * @returns {Promise<Array>} Menu data
 */
async function getMenu(api, options = {}) {
  // Return in-memory cache if available
  if (menuCache && !options.refresh) {
    return menuCache;
  }

  // Try localStorage cache
  const cached = getCachedMenuData();
  if (cached && !options.refresh) {
    menuCache = cached;
    return cached;
  }

  // Fetch from server
  return fetchMenu(api);
}

/**
 * Build manager icons registry from menu data
 * @param {Array} menuData - Grouped menu data
 * @returns {Object} Manager icons registry (managerId -> {iconHtml, name})
 */
function buildManagerIconsRegistry(menuData) {
  const registry = {};

  if (!menuData || !Array.isArray(menuData)) {
    return registry;
  }

  menuData.forEach((group) => {
    group.items.forEach((item) => {
      registry[item.managerId] = {
        iconHtml: item.iconHtml || '<fa fa-cube></fa>',
        name: item.name,
      };
    });
  });

  return registry;
}

/**
 * Manager Menu
 *
 * Handles the manager menu popup (toolbox icon) with profile navigation.
 */


// ===== State =====

let activeManagerMenuPopup = null;
let menuDataCache = null;

// ===== Profile Menu Items =====

/**
 * Get profile menu items for a manager
 * @param {string|number} managerId - The manager ID
 * @param {Object} api - API client
 * @returns {string} HTML content for menu items
 */
async function getManagerProfileMenuItems(managerId, api) {
  try {
    if (menuDataCache) {
      log(Subsystems.MANAGER, Status.DEBUG, `[ManagerMenu] Using cached menu data for manager ${managerId}`);
      const cached = menuDataCache[managerId];
      if (cached) {
        return cached.html;
      }
    }

    log(Subsystems.MANAGER, Status.INFO, `[ManagerMenu] Fetching profile menu items for manager ${managerId}`);

    const allLookupRows = await authQuery(api, 26, {
      INTEGER: { LOOKUPID: 60 }
    });

    const menuData = allLookupRows?.find(row => row.key_idx === 1);

    if (!menuData) {
      log(Subsystems.MANAGER, Status.WARN, `[ManagerMenu] No data for Lookup 60 key_idx 1`);
      return '';
    }
    if (!menuData.collection) {
      log(Subsystems.MANAGER, Status.WARN, `[ManagerMenu] No collection in menuData`);
      return '';
    }

    let menuObject;
    try {
      menuObject = JSON.parse(menuData.collection);
    } catch (_e) {
      log(Subsystems.MANAGER, Status.WARN, `[ManagerMenu] Invalid JSON in Lookup 60 key_idx 1`);
      return '';
    }

    if (!menuObject || typeof menuObject !== 'object') {
      log(Subsystems.MANAGER, Status.WARN, `[ManagerMenu] menuObject is not an object`);
      return '';
    }

    const managerKey = String(managerId).padStart(3, '0');
    const menuArray = menuObject[managerKey];

    if (!Array.isArray(menuArray) || menuArray.length < 2) {
      log(Subsystems.MANAGER, Status.WARN, `[ManagerMenu] Invalid menu array for manager ${managerKey}`);
      return '';
    }

    const sectionIndices = menuArray.slice(1);

    const mainMenuData = await getMenu(api);
    const managerIcons = buildManagerIconsRegistry(mainMenuData);

    const sectionRows = allLookupRows?.filter(row => row.key_idx !== 1 && row.collection) || [];

    const sectionMap = new Map();
    sectionRows.forEach(row => {
      try {
        const sectionData = JSON.parse(row.collection);
        if (Array.isArray(sectionData) && sectionData.length >= 3) {
          const [, icon, label] = sectionData;
          sectionMap.set(row.key_idx, { icon, label });
        }
      } catch (_e) {
        // Ignore invalid rows
      }
    });

    const defaultSections = [
      { index: -1, icon: '<fa fa-id-card></fa>', label: 'Account' },
      { index: -2, icon: '<fa fa-id-badge></fa>', label: 'Names' },
      { index: -3, icon: '<fa fa-address-book></fa>', label: 'Addresses' },
      { index: -4, icon: '<fa fa-at></fa>', label: 'E-Mail' },
      { index: -5, icon: '<fa fa-phone></fa>', label: 'Phone' },
      { index: -6, icon: '<fa fa-key></fa>', label: 'Authentication' },
      { index: -7, icon: '<fa fa-user-key></fa>', label: 'Tokens' },
      { index: -8, icon: '<fa fa-globe></fa>', label: 'Language' },
      { index: -9, icon: '<fa fa-calendar></fa>', label: 'Date Formats' },
      { index: -10, icon: '<fa fa-00></fa>', label: 'Number Formats' },
      { index: -11, icon: '<fa fa-atom-simple></fa>', label: 'Startup' },
      { index: -12, icon: '<fa fa-bell></fa>', label: 'Notifications' },
      { index: -13, icon: '<fa fa-bell-concierge></fa>', label: 'Concierge' },
      { index: -14, icon: '<fa fa-note-sticky></fa>', label: 'Annotations' },
      { index: -15, icon: '<fa fa-signs-post></fa>', label: 'Tours' },
      { index: -16, icon: '<fa fa-graduation-cap></fa>', label: 'Training' },
      { index: -17, icon: '<fa fa-receipt></fa>', label: 'Login History' },
    ];

    defaultSections.forEach(def => {
      if (!sectionMap.has(def.index)) {
        sectionMap.set(def.index, { icon: def.icon, label: def.label });
      }
    });

    Object.entries(managerIcons).forEach(([id, info]) => {
      const managerId = parseInt(id, 10);
      if (sectionMap.has(managerId)) {
        sectionMap.set(managerId, {
          icon: info.iconHtml || sectionMap.get(managerId).icon,
          label: info.name || sectionMap.get(managerId).label
        });
      } else {
        sectionMap.set(managerId, { icon: info.iconHtml || '<fa fa-cube></fa>', label: info.name || `Manager ${id}` });
      }
    });

    let html = '';
    sectionIndices.forEach(index => {
      const section = sectionMap.get(index);
      if (section) {
        html += `
          <button type="button" class="manager-ui-menu-item" data-value="profile:${index}">
            ${section.icon}
            <span>${section.label}</span>
          </button>
        `;
      }
    });

    if (!menuDataCache) menuDataCache = {};
    menuDataCache[managerId] = { html };

    return html;
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `[ManagerMenu] Error loading profile menu items:`, error);
    return '';
  }
}

// ===== Show/Hide Manager Menu Popup =====

/**
 * Show manager menu popup
 * @param {HTMLElement} button - The button that triggered the popup
 * @param {string|number} managerId - The manager ID
 * @param {Object} api - API client
 */
async function showManagerMenuPopup(button, managerId, api) {
  closeAllPopups$1();

  let content = `
    <button type="button" class="manager-ui-menu-item" data-value="refresh">
      <fa fa-sync></fa>
      <span>Refresh Manager</span>
    </button>
  `;

  if (managerId && api) {
    try {
      const profileItems = await getManagerProfileMenuItems(managerId, api);
      content += profileItems;
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, `[ManagerMenu] Failed to load profile menu items:`, error);
    }
  }

  activeManagerMenuPopup = showPopup(button, 'manager-ui-popup manager-header-popup', content, {
    position: 'header-dropdown',
    title: 'Manager Menu',
    onSelect: (item) => {
      const action = item.dataset.value;
      if (action === 'refresh') {
        eventBus.emit('manager:refresh');
      } else if (action.startsWith('profile:')) {
        const sectionIndex = parseInt(action.replace('profile:', ''), 10);
        openUserProfileSection(sectionIndex);
      }
      hideActivePopup();
    },
  });

  if (activeManagerMenuPopup) {
    processIcons(activeManagerMenuPopup);
  }
}

/**
 * Open User Profile Manager and select a specific section
 * @param {number} sectionIndex - The section index to select
 */
function openUserProfileSection(sectionIndex) {
  log(Subsystems.MANAGER, Status.INFO, `[ManagerMenu] Opening User Profile Manager for section ${sectionIndex}`);

  if (window.lithiumApp) {
    window.lithiumApp.loadUtilityManager('user-profile').then(() => {
      const loadedEntry = window.lithiumApp.loadedManagers.get('utility:user-profile');
      const manager = loadedEntry?.instance;
      if (manager && manager.selectSection) {
        manager.selectSection(sectionIndex);
      } else {
        log(Subsystems.MANAGER, Status.ERROR, `[ManagerMenu] Profile Manager not found or selectSection not available`);
      }
    }).catch(error => {
      log(Subsystems.MANAGER, Status.ERROR, `[ManagerMenu] Failed to open User Profile Manager: ${error?.message || error}`);
    });
  } else {
    log(Subsystems.MANAGER, Status.ERROR, `[ManagerMenu] window.lithiumApp not available`);
  }
}

/**
 * Hide manager menu popup
 * @param {boolean} skipAnimation - If true, remove immediately without animation
 */
function hideManagerMenuPopup(skipAnimation = false) {
  if (activeManagerMenuPopup) {
    hideActivePopup(skipAnimation);
    activeManagerMenuPopup = null;
  }
}

// Register close callback for closeAllPopups
registerCloseCallback((skipAnimation) => {
  hideManagerMenuPopup(skipAnimation);
});

/**
 * Manager Export
 *
 * Handles export popup and related functionality.
 */


// ===== Export Options =====

const DEFAULT_EXPORT_OPTIONS = [
  { value: 'pdf', label: 'PDF', icon: 'fa-file-pdf fa-fw', enabled: true },
  { value: 'csv', label: 'CSV', icon: 'fa-file-csv fa-fw', enabled: true },
  { value: 'xls', label: 'XLS', icon: 'fa-file-excel fa-fw', enabled: true },
  { value: 'json', label: 'JSON', icon: 'fa-brackets-curly fa-fw', enabled: true },
  { value: 'html', label: 'HTML', icon: 'fa-file-html fa-fw', enabled: true },
  { value: 'markdown', label: 'Markdown', icon: 'fa-file-code fa-fw', enabled: true },
  { value: 'txt', label: 'Text', icon: 'fa-file-lines fa-fw', enabled: true },
];

const EXPORT_FORMAT_ICONS = {
  pdf: 'fa-file-pdf',
  csv: 'fa-file-csv',
  xls: 'fa-file-excel',
  json: 'fa-brackets-curly',
  html: 'fa-file-html',
  markdown: 'fa-file-code',
  txt: 'fa-file-lines',
};

// ===== Export Popup State =====

let activeExportPopup = null;
let activeExportButton = null;
let activeExportCloseHandler = null;

// ===== Export Button Format =====

/**
 * Set the export button icon based on the selected format
 * @param {HTMLElement} exportBtn - The export button element
 * @param {string} format - The export format value
 */
function setExportButtonFormat(exportBtn, format) {
  if (!exportBtn) return;

  const icon = EXPORT_FORMAT_ICONS[format];
  if (icon) {
    exportBtn.innerHTML = `<fa ${icon}></fa>`;
    processIcons(exportBtn);
  }
}

// ===== Show Export Popup =====

/**
 * Create and show the export popup menu
 * @param {HTMLElement} btn - The export button element
 * @param {Array} exportOptions - Array of export option configs
 * @param {Function} onExportAction - Callback with (value, label) when option selected
 * @param {Object} extraOptions - Extra options like { versionSummary: { label, action } }
 * @param {Function} options.onFormatSelected - Callback when format is selected (for updating icon)
 */
function showExportPopup(btn, exportOptions, onExportAction, extraOptions = {}, options = {}) {
  log(Subsystems.UI, Status.DEBUG, `[Export] showExportPopup called`);

  if (activeExportPopup && activeExportButton === btn) {
    closeExportPopup(false);
    return;
  }

  closeExportPopup(false);
  closeAllPopups$1(false);

  activeExportButton = btn;
  btn.classList.add('popup-active');

  const popup = document.createElement('div');
  popup.className = 'manager-footer-export-popup manager-footer-popup';
  popup.style.transition = 'opacity var(--transition-duration, 350ms) ease, transform var(--transition-duration, 350ms) ease, visibility var(--transition-duration, 350ms)';

  const opts = exportOptions || DEFAULT_EXPORT_OPTIONS;

  opts.forEach(opt => {
    const row = document.createElement('button');
    row.type = 'button';
    row.className = 'manager-footer-export-popup-item';
    row.dataset.value = opt.value;
    row.disabled = opt.enabled === false;

    const iconHtml = opt.icon ? `<fa class="manager-footer-export-icon ${opt.icon}"></fa>` : '';
    row.innerHTML = `${iconHtml}<span>${opt.label}</span>`;

    if (!row.disabled) {
      row.addEventListener('click', () => {
        closeExportPopup();
        if (options.onFormatSelected) {
          options.onFormatSelected(opt.value);
        }
      });
    }

    popup.appendChild(row);
  });

  if (extraOptions.versionSummary) {
    const sep = document.createElement('div');
    sep.className = 'manager-footer-export-popup-separator';
    popup.appendChild(sep);

    const vsRow = document.createElement('button');
    vsRow.type = 'button';
    vsRow.className = 'manager-footer-export-popup-item version-summary-item';
    vsRow.innerHTML = `<fa class="manager-footer-export-icon fa-list"></fa><span>${extraOptions.versionSummary.label}</span>`;
    vsRow.addEventListener('click', () => {
      closeExportPopup();
      extraOptions.versionSummary.action();
    });
    popup.appendChild(vsRow);
  }

  const btnRect = btn.getBoundingClientRect();
  document.body.appendChild(popup);

  requestAnimationFrame(() => {
    popup.style.position = 'fixed';
    popup.style.zIndex = '10001';
    popup.style.bottom = `${window.innerHeight - btnRect.top + 1}px`;
    popup.style.right = `${window.innerWidth - btnRect.right}px`;
    popup.style.top = 'auto';
    popup.style.left = 'auto';
  });

  requestAnimationFrame(() => {
    popup.classList.add('visible');
  });

  activeExportPopup = popup;

  const handleOutsideClick = (evt) => {
    if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
      closeExportPopup(false);
    }
  };

  const handleEscapeKey = (evt) => {
    if (evt.key === 'Escape') {
      closeExportPopup(false);
    }
  };

  activeExportCloseHandler = handleOutsideClick;

  setTimeout(() => {
    document.addEventListener('click', handleOutsideClick);
    document.addEventListener('keydown', handleEscapeKey);
  }, 0);

  const closeOnAllPopups = () => closeExportPopup();
  document.addEventListener('close-all-popups', closeOnAllPopups);

  popup._closeOnAllPopupsHandler = closeOnAllPopups;

  processIcons(popup);
}

// ===== Close Export Popup =====

/**
 * Close the active export popup
 * @param {boolean} skipAnimation - If true, remove immediately without animation
 */
function closeExportPopup(skipAnimation = false) {
  if (activeExportPopup) {
    if (skipAnimation) {
      activeExportPopup.remove();
      activeExportPopup = null;
    } else {
      activeExportPopup.classList.remove('visible');

      const transitionDuration = parseFloat(getComputedStyle(activeExportPopup).transitionDuration) * 1000 || 350;
      setTimeout(() => {
        if (activeExportPopup && activeExportPopup.parentNode) {
          activeExportPopup.remove();
        }
        activeExportPopup = null;
      }, transitionDuration);
    }
  }

  if (activeExportButton) {
    activeExportButton.classList.remove('popup-active');
    activeExportButton = null;
  }

  if (activeExportCloseHandler) {
    document.removeEventListener('click', activeExportCloseHandler);
    activeExportCloseHandler = null;
  }
  if (activeExportPopup && activeExportPopup._closeOnAllPopupsHandler) {
    document.removeEventListener('close-all-popups', activeExportPopup._closeOnAllPopupsHandler);
    activeExportPopup._closeOnAllPopupsHandler = null;
  }
}

// ===== Report Options =====

const DEFAULT_REPORT_OPTIONS = [
  { value: 'user-profile', label: 'User Profile Report' },
  { value: 'view', label: 'View' },
  { value: 'data', label: 'Data' },
];

// ===== Footer Storage =====

const FOOTER_STORAGE_KEYS = {
  exportFormat: 'lithium_footer_export_format',
  reportSelection: 'lithium_footer_report_selection',
};

/**
 * Load saved export format from localStorage
 * @returns {string|null}
 */
function loadSavedExportFormat() {
  try {
    return localStorage.getItem(FOOTER_STORAGE_KEYS.exportFormat);
  } catch {
    return null;
  }
}

/**
 * Save export format to localStorage
 * @param {string} format
 */
function saveExportFormat(format) {
  try {
    localStorage.setItem(FOOTER_STORAGE_KEYS.exportFormat, format);
  } catch {
    // Ignore storage errors
  }
}

/**
 * Load saved report selection from localStorage
 * @returns {string|null}
 */
function loadSavedReportSelection() {
  try {
    return localStorage.getItem(FOOTER_STORAGE_KEYS.reportSelection);
  } catch {
    return null;
  }
}

/**
 * Save report selection to localStorage
 * @param {string} report
 */
function saveReportSelection(report) {
  try {
    localStorage.setItem(FOOTER_STORAGE_KEYS.reportSelection, report);
  } catch {
    // Ignore storage errors
  }
}

// ===== Setup Manager Footer Icons =====

/**
 * Setup common manager footer icons (Print, Email, Export, Reports select, Save/Cancel, Dummy).
 * @param {HTMLElement} footerGroup - The footer subpanel-header-group element
 * @param {Object} options - Configuration options
 * @returns {Object} References to created elements
 */
function setupManagerFooterIcons(footerGroup, options = {}) {
  const {
    onPrint,
    onEmail,
    onDownload,
    onExport,
    reportOptions = DEFAULT_REPORT_OPTIONS,
    anchor,
    showSaveCancel = false,
    onSave,
    onCancel,
    exportOptions,
    extraExportOptions,
    showClipboard = false,
    onClipboard,
  } = options;

  if (!footerGroup) {
    log(Subsystems.MANAGER, Status.ERROR, '[Export] Footer group not provided to setupManagerFooterIcons');
    return {};
  }

  const elements = {};

  const makeBtn = (id, icon, tooltip, shortcutId) => {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'subpanel-header-btn subpanel-header-close';
    btn.id = id;
    btn.dataset.tooltip = tooltip;
    if (shortcutId) btn.dataset.shortcutId = shortcutId;
    btn.innerHTML = `<fa ${icon}></fa>`;
    return btn;
  };

  const makeReportSelect = (id, options) => {
    const select = document.createElement('select');
    select.id = id;
    select.title = 'Data Source';
    select.className = 'manager-footer-reports-select';
    select.innerHTML = options.map(opt =>
      `<option value="${opt.value}">${opt.label}</option>`
    ).join('');
    return select;
  };

  elements.printBtn = makeBtn('manager-footer-print', 'fa-print', 'Print', 'print');
  elements.clipboardBtn = makeBtn('manager-footer-clipboard', 'fa-clipboard', 'Copy to Clipboard');
  elements.emailBtn = makeBtn('manager-footer-email', 'fa-envelope', 'E-Mail');
  elements.downloadBtn = makeBtn('manager-footer-download', 'fa-square-down', 'Download');
  elements.exportBtn = makeBtn('manager-footer-export', 'fa-file-circle-question', 'Export Format');
  elements.exportBtn.classList.add('manager-ui-btn');
  elements.reportSelect = makeReportSelect('manager-footer-reports', reportOptions);

  const savedFormat = loadSavedExportFormat();
  const availableFormats = (exportOptions || DEFAULT_EXPORT_OPTIONS).map(o => o.value);
  let selectedExportFormat = savedFormat && availableFormats.includes(savedFormat)
    ? savedFormat
    : (exportOptions?.[0]?.value || 'pdf');
  elements.exportFormat = selectedExportFormat;

  setExportButtonFormat(elements.exportBtn, selectedExportFormat);

  const savedReport = loadSavedReportSelection();
  const availableReports = reportOptions.map(o => o.value);
  if (savedReport && availableReports.includes(savedReport)) {
    elements.reportSelect.value = savedReport;
  } else {
    elements.reportSelect.value = reportOptions[0]?.value || 'user-profile';
  }

  const insertBeforeAnchor = (el) => {
    if (anchor && footerGroup.contains(anchor)) {
      footerGroup.insertBefore(el, anchor);
    } else {
      footerGroup.appendChild(el);
    }
  };

  insertBeforeAnchor(elements.printBtn);
  if (showClipboard) {
    insertBeforeAnchor(elements.clipboardBtn);
  }
  insertBeforeAnchor(elements.emailBtn);
  insertBeforeAnchor(elements.downloadBtn);
  insertBeforeAnchor(elements.exportBtn);
  insertBeforeAnchor(elements.reportSelect);

  if (onPrint) {
    elements.printBtn.addEventListener('click', onPrint);
  }
  if (showClipboard && onClipboard) {
    elements.clipboardBtn.addEventListener('click', () => {
      onClipboard('clipboard', 'Clipboard');
    });
  }
  if (onEmail) {
    elements.emailBtn.addEventListener('click', onEmail);
  }
  if (onDownload) {
    elements.downloadBtn.addEventListener('click', onDownload);
  }
  if (onExport) {
    elements.exportBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      const useCommonExport = exportOptions !== undefined || extraExportOptions !== undefined;

      if (useCommonExport) {
        const opts = exportOptions || DEFAULT_EXPORT_OPTIONS;
        showExportPopup(elements.exportBtn, opts, null, {}, {
          onFormatSelected: (format) => {
            selectedExportFormat = format;
            elements.exportFormat = format;
            setExportButtonFormat(elements.exportBtn, format);
            saveExportFormat(format);
          },
        });
      } else {
        onExport(e);
      }
    });
  }

  elements.reportSelect.addEventListener('change', () => {
    saveReportSelection(elements.reportSelect.value);
  });

  if (showSaveCancel) {
    elements.saveBtn = makeBtn('manager-footer-save', 'fa-square-full', 'Save');
    elements.saveBtn.classList.add('manager-footer-save-btn');
    elements.saveBtn.disabled = true;

    elements.cancelBtn = makeBtn('manager-footer-cancel', 'fa-octagon', 'Cancel');
    elements.cancelBtn.classList.add('manager-footer-cancel-btn');
    elements.cancelBtn.disabled = true;

    elements.dummyBtn = document.createElement('button');
    elements.dummyBtn.type = 'button';
    elements.dummyBtn.className = 'subpanel-header-btn subpanel-header-close manager-footer-dummy-btn';
    elements.dummyBtn.disabled = true;
    elements.dummyBtn.setAttribute('aria-hidden', 'true');
    elements.dummyBtn.setAttribute('tabindex', '-1');

    if (anchor && footerGroup.contains(anchor)) {
      footerGroup.insertBefore(elements.saveBtn, anchor.nextSibling);
      footerGroup.insertBefore(elements.cancelBtn, elements.saveBtn.nextSibling);
      footerGroup.insertBefore(elements.dummyBtn, elements.cancelBtn.nextSibling);
    } else {
      footerGroup.appendChild(elements.saveBtn);
      footerGroup.appendChild(elements.cancelBtn);
      footerGroup.appendChild(elements.dummyBtn);
    }

    if (onSave) {
      elements.saveBtn.addEventListener('click', onSave);
    }
    if (onCancel) {
      elements.cancelBtn.addEventListener('click', onCancel);
    }
  }

  processIcons(footerGroup);

  return elements;
}

// Register close callback for closeAllPopups
registerCloseCallback((skipAnimation) => {
  closeExportPopup(skipAnimation);
});

/**
 * Manager UI - Main Entry Point
 *
 * Re-exports from all manager UI sub-modules.
 * This file replaces the original monolithic manager-ui.js (2490 lines)
 * which has been split into smaller, more maintainable modules.
 */


// ===== Initialization =====

/**
 * Initialize global keyboard shortcut handler
 */
function initGlobalShortcuts() {
  document.addEventListener('keydown', (e) => {
    const key = [];
    if (e.ctrlKey || e.metaKey) key.push('Ctrl');
    if (e.shiftKey) key.push('Shift');
    if (e.altKey) key.push('Alt');
    key.push(e.key);

    const keyCombo = key.join('+');

    const matchingShortcut = getAllShortcuts().find(s => s.key === keyCombo);

    if (matchingShortcut) {
      const activeElement = document.activeElement;
      const isTyping = activeElement && (
        activeElement.tagName === 'INPUT' ||
        activeElement.tagName === 'TEXTAREA' ||
        activeElement.isContentEditable
      );

      if (!isTyping) {
        e.preventDefault();
        matchingShortcut.handler();
      }
    }
  });
}

// Initialize on module load
initGlobalShortcuts();

// Register default global shortcuts
registerShortcut('logout', 'Ctrl+Shift+L', 'Open logout panel', () => {
  eventBus.emit('logout:show');
});

registerShortcut('reload', 'Ctrl+Shift+R', 'Reload page', () => {
  window.location.reload(true);
});

registerShortcut('editsave', 'Ctrl+Enter', 'Save changes', () => {
  eventBus.emit('edit:save');
});

registerShortcut('editcancel', 'Esc', 'Cancel changes', () => {
  eventBus.emit('edit:cancel');
});

registerShortcut('fullscreen', 'F11', 'Toggle fullscreen', () => {
  toggleFullscreen();
});

registerShortcut('shortcuts', 'Ctrl+Shift+?', 'Show shortcuts', () => {
  eventBus.emit('shortcuts');
});

log(Subsystems.MANAGER, Status.INFO, '[ManagerUI] Initialized');

export { closeAllPopups as a, showShortcutsPopup as b, closeAllPopups$1 as c, showManagerMenuPopup as d, expandMacros as e, showTrainingPopup as f, showToursPopup as g, hidePopup as h, unregisterManagerShortcuts as i, getMenu as j, buildManagerIconsRegistry as k, clearMenuCache as l, positionPopup as p, registerShortcut as r, setupManagerFooterIcons as s, toggleFullscreen as t, updateFullscreenIcon as u };
