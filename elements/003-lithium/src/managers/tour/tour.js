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
// State
// ========================================

let _toursCache = null;
let _activeShepherdTour = null;
let _activeTourOptions = null;
let _listPopup = null;
let _documentClickHandler = null;
let _tourEnding = false;  // Guard against navigate during cancel/complete
let _tourContainer = null; // Dedicated container for tour step elements

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
 * @param {Object} api - The API/conduit instance
 * @param {string} managerName - Manager name (e.g., "023.Lookup Manager")
 * @returns {Promise<Array>} Filtered tour objects
 */
export async function getToursForManager(api, managerName) {
  const tours = await getTours(api);
  return tours.filter(t => t.definition.manager === managerName);
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
    // End any active tour first, waiting for fade-out transition
    if (_activeShepherdTour) {
      log(Subsystems.MANAGER, Status.DEBUG, '[Tour] Cancelling active tour before launching new one');
      const transitionDuration = getTransitionDuration();
      try {
        _activeShepherdTour.cancel();
      } catch (err) {
        log(Subsystems.MANAGER, Status.WARN, `[Tour] Error cancelling previous tour: ${err.message}`);
      }
      // Force-clean any Shepherd artefacts and wait for fade-out
      cleanupActiveTour();
      await new Promise(resolve => setTimeout(resolve, transitionDuration));
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
      showTourList(tours, options.anchor, {
        onSelect: (selectedId) => launchTour(selectedId, api, options),
      });
      return null;
    }

    log(Subsystems.MANAGER, Status.INFO, `[Tour] Launching tour: ${tour.name}`);

    const { definition } = tour;

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
        showTourList(tours, tourOptions.anchor, {
          onSelect: (selectedId) => launchTour(selectedId, api, tourOptions),
        });
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

  const tourItems = tours.map(t => `
    <div class="lithium-tour-list-item" data-tour-id="${t.id}">
      <div class="lithium-tour-list-item-icon">
        ${t.definition.icon || '<fa fa-signs-post></fa>'}
      </div>
      <div class="lithium-tour-list-item-info">
        <div class="lithium-tour-list-item-name">${t.name}</div>
      </div>
    </div>
  `).join('');

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
      hideTourList({ fade: true }).then(() => {
        if (options.onSelect) {
          options.onSelect(tourId);
        }
        eventBus.emit('tour:select', { tourId });
      });
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
    await launchTour(tourId, api);
  });

  log(Subsystems.MANAGER, Status.INFO, '[Tour] Initialized');
}

// Auto-initialize
initTours();
