/**
 * Tour Manager
 *
 * Provides Shepherd.js guided tours loaded from Lookup #43.
 * Tours are lazy-loaded on first request and cached.
 */

import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { eventBus, Events } from '../../core/event-bus.js';
import { processIcons } from '../../core/icons.js';
import { expandMacros, hasMacros } from '../../core/macro-expansion.js';
import { getMacros } from '../../shared/lookups.js';
import { offset, flip, shift } from '@floating-ui/dom';
import './tour.css';

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
let _activeTourOptions = null;
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
  _activeTourOptions = null;
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
export async function getTours(api) {
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
 * Force refresh tours cache
 * @param {Object} api - The API/conduit instance
 */
export async function refreshTours(api) {
  _toursCache = null;
  return getTours(api);
}

/**
 * Get tours for a specific manager
 * Supports both:
 * - Single manager: "023.Lookup Manager" (exact match)
 * - Multiple managers: ["029.Query Manager", "023.Lookup Manager"] (array match)
 * @param {Object} api - The API/conduit instance
 * @param {string} managerName - Manager name (e.g., "023.Lookup Manager")
 * @returns {Promise<Array>} Filtered tour objects
 */
export async function getToursForManager(api, managerName) {
  const tours = await getTours(api);
  return tours.filter(t => {
    const def = t.definition;
    if (!def) return false;
    // Single manager (existing format)
    if (def.manager === managerName) return true;
    // Multiple managers (new array format)
    if (Array.isArray(def.managers)) {
      return def.managers.includes(managerName);
    }
    return false;
  });
}

/**
 * Check if a tour is a video tour
 * @param {Object} tour - Tour object
 * @returns {boolean} True if tour has video field
 */
export function isVideoTour(tour) {
  return !!(tour?.definition?.video);
}

/**
 * Get all video tours
 * @param {Object} api - The API/conduit instance
 * @returns {Promise<Array>} Array of video tour objects
 */
export async function getVideoTours(api) {
  const tours = await getTours(api);
  return tours.filter(t => isVideoTour(t));
}

/**
 * Get video tours for a specific manager
 * @param {Object} api - The API/conduit instance
 * @param {string} managerName - Manager name (e.g., "023.Lookup Manager")
 * @returns {Promise<Array>} Filtered video tour objects
 */
export async function getVideoToursForManager(api, managerName) {
  const allVideoTours = await getVideoTours(api);
  return allVideoTours.filter(t => {
    const def = t.definition;
    if (!def) return false;
    // Single manager (existing format)
    if (def.manager === managerName) return true;
    // Multiple managers (new array format)
    if (Array.isArray(def.managers)) {
      return def.managers.includes(managerName);
    }
    return false;
  });
}

// ========================================
// Tour Building (Shepherd)
// ========================================

/**
 * Import Shepherd.js dynamically
 * @returns {Promise<Object>} Shepherd module
 */
async function loadShepherd() {
  const mod = await import('shepherd.js');
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
          const listDelay = getTransitionDuration() + 100;
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
function getTransitionDuration() {
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
  const transitionDuration = getTransitionDuration();

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
    const transitionDuration = getTransitionDuration();
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
    const transitionDuration = getTransitionDuration();
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
export async function buildShepherdTour(tour, options = {}) {
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
          ${createHeaderHTML(tour, options)}
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
  _activeTourOptions = options;

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
          } else if (wasVisible && !isVisible) {
            // console.log('Tour step became hidden - this should not happen during normal operation!');
            // console.log('Step element:', target);
            // console.log('Step classes:', target.className);
            // console.trace('Visibility change trace');
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
      const overlay = document.querySelector('.shepherd-modal-overlay-container');
      // console.log('Modal overlay element:', overlay);
      if (overlay) {
        // console.log('Modal overlay styles:', getComputedStyle(overlay));
      } else {
        // console.log('No modal overlay found!');
      }
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
    _activeTourOptions = null;
    // Clean up drag state and remove tour container after overlay fade
    setTimeout(() => cleanupActiveTour(), getTransitionDuration());
  });

  shepherd.on('complete', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour completed: ${tour.name}`);
    // Overlay fade-out is now handled in completeWithTransition for sync
    if (cleanupKeyboard) cleanupKeyboard();
    if (cleanupClickHandler) cleanupClickHandler();
    if (cleanupContextMenu) cleanupContextMenu();
    if (visibilityObserver) visibilityObserver.disconnect();
    _activeShepherdTour = null;
    _activeTourOptions = null;
    // Clean up drag state and remove tour container after overlay fade
    setTimeout(() => cleanupActiveTour(), getTransitionDuration());
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
export async function launchTour(tourId, api, options = {}) {
  try {
    // End any active tour first (Shepherd or video), waiting for fade-out transition
    
    // Handle Shepherd tour
    if (_activeShepherdTour) {
      log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Cancelling active Shepherd tour before launching new one');
      const transitionDuration = getTransitionDuration();
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
      const transitionDuration = getTransitionDuration();
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
export function showTourList(tours, anchor = null, options = {}) {
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
export function hideTourList(options = {}) {
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
      const duration = getTransitionDuration();
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
export async function launchVideoTour(tour, options = {}) {
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
    const transitionDuration = getTransitionDuration();
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
    const duration = getTransitionDuration();
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
        <button type="button" class="lithium-video-tour-back-10-btn" data-video-action="back-10" title="Back 10 seconds">
          <fa fa-arrow-rotate-left-10></fa>
        </button>
        <button type="button" class="lithium-video-tour-back-30-btn" data-video-action="back-30" title="Back 30 seconds">
          <fa fa-arrow-rotate-left-30></fa>
        </button>
        <button type="button" class="lithium-video-tour-back-to-start-btn" data-video-action="back-to-start" title="Back to start">
          <fa fa-arrow-rotate-left></fa>
        </button>
        <button type="button" class="lithium-video-tour-forward-10-btn" data-video-action="forward-10" title="Forward 10 seconds">
          <fa fa-arrow-rotate-right-10></fa>
        </button>
        <button type="button" class="lithium-video-tour-forward-30-btn" data-video-action="forward-30" title="Forward 30 seconds">
          <fa fa-arrow-rotate-right-30></fa>
        </button>
        <button type="button" class="lithium-video-tour-forward-to-end-btn" data-video-action="forward-to-end" title="Forward to end">
          <fa fa-arrow-rotate-right></fa>
        </button>
        <div class="lithium-video-tour-play-btn-wrapper">
          <button type="button" class="lithium-video-tour-big-play-btn" data-video-action="play">
            <fa fa-pause></fa>
          </button>
        </div>
        <button type="button" class="lithium-video-tour-repeat-btn" data-video-action="repeat" title="Repeat">
          <fa fa-repeat></fa>
        </button>
        <button type="button" class="lithium-video-tour-subtitles-btn" data-video-action="subtitles" title="Subtitles">
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
          <button type="button" class="lithium-video-tour-mute-btn" data-video-action="mute">
            <fa fa-volume-high></fa>
          </button>
          <input type="range" class="lithium-video-tour-volume-slider" min="0" max="100" value="100">
        </div>
        <div class="lithium-video-tour-time">0:00 / 0:00</div>
        <div class="lithium-video-tour-spacer"></div>
        <button type="button" class="lithium-video-tour-speed-btn" data-video-action="speed">
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
      const deltaY = e.clientY - startY;

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
      popup.classList.add('ended');
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
    popup.classList.add('ended');
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
        }, getTransitionDuration() + 50);
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
      case 'speed':
        currentSpeedIndex = (currentSpeedIndex + 1) % VIDEO_PLAYBACK_SPEEDS.length;
        const newSpeed = VIDEO_PLAYBACK_SPEEDS[currentSpeedIndex];
        video.playbackRate = newSpeed;
        speedBtn.querySelector('span').textContent = `${newSpeed}x`;
        break;
      case 'captions':
        // Toggle handled by dedicated click listener
        break;
    }
  };

  popup.addEventListener('click', handleButtonClick);

  // Store references for cleanup
  _activeVideoPopup = popup;
  _activeVideoElement = video;

  // Process icons
  if (typeof processIcons === 'function') {
    processIcons(popup);
  }

  log(Subsystems.MANAGER, Status.INFO, `[VideoTour] Popup created for: ${tour.name}`);
}

/**
 * Close video tour popup with fade out
 */
export function closeVideoTour() {
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
    const duration = getTransitionDuration();
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
export function initTours() {
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
