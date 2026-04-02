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

  // Remove any remaining tour elements from DOM
  document.querySelectorAll('.shepherd-element.lithium-tour-step').forEach(el => {
    el.remove();
  });

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
 * Extract icon name from icon HTML string
 * @param {string} iconHtml - HTML like '<fa fa-clipboard-list></fa>' or '<i class="fa-solid fa-..."></i>'
 * @returns {string} Icon name (e.g., 'clipboard-list')
 */
function extractIconName(iconHtml) {
  if (!iconHtml) return 'signs-post';

  // Handle <fa fa-icon-name></fa> format
  const faMatch = iconHtml.match(/<fa\s+fa-([\w-]+)/);
  if (faMatch) {
    return faMatch[1];
  }

  // Handle <i class="fa-solid fa-icon-name"></i> format
  const iconMatch = iconHtml.match(/fa-([\w-]+)/);
  if (iconMatch) {
    return iconMatch[1];
  }

  return 'signs-post';
}

/**
 * Create custom header HTML for a step
 * @param {Object} tour - Tour object
 * @param {Object} options - Options including onShowList callback
 * @returns {string} HTML string for the header
 */
function createHeaderHTML(tour, options) {
  const iconName = extractIconName(tour.definition?.icon);

  return `
    <div class="lithium-tour-header">
      <div class="lithium-tour-header-icon-title">
        <fa fa-${iconName}></fa>
        <div class="lithium-tour-header-title">${tour.name}</div>
      </div>
      <div class="lithium-tour-header-actions">
        <button type="button" class="lithium-tour-header-btn lithium-tour-list-btn" data-tour-action="list" title="Show all tours">
          <fa fa-signs-post></fa>
        </button>
        <button type="button" class="lithium-tour-header-btn lithium-tour-close-btn" data-tour-action="close" title="Close tour (Esc)">
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
              title="Previous (←)"
              ${isFirst ? 'disabled' : ''}>
        <fa fa-chevron-left></fa>
      </button>
      <div class="lithium-tour-step-counter" title="Step ${currentStep + 1} of ${totalSteps}">${stepCounter}</div>
      <button type="button"
              class="lithium-tour-footer-btn lithium-tour-next-btn"
              data-tour-action="next"
              title="${isLast ? 'Finish tour' : 'Next (→)'}">
        ${isLast ? '<fa fa-check></fa>' : '<fa fa-chevron-right></fa>'}
      </button>
    </div>
  `;
}

/**
 * Get current step index
 * @param {Object} shepherd - Shepherd Tour instance
 * @returns {number} Current step index
 */
function getCurrentStepIndex(shepherd) {
  return shepherd.steps.findIndex(s => s.isOpen());
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
          navigateWithTransition(shepherd, 'back');
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
 * Navigate with fade transition
 * @param {Object} shepherd - Shepherd Tour instance
 * @param {string} direction - 'next' or 'back'
 */
function navigateWithTransition(shepherd, direction) {
  // Don't navigate if tour is ending
  if (_tourEnding) return;

  const currentStepEl = shepherd.getCurrentStep()?.getElement();
  if (!currentStepEl) {
    if (direction === 'next') shepherd.next();
    else shepherd.back();
    return;
  }

  // Get transition duration from CSS
  const transitionDuration = getTransitionDuration();

  // Prevent multiple rapid navigation attempts
  if (currentStepEl._isTransitioning) return;
  currentStepEl._isTransitioning = true;

  // Fade out current step
  currentStepEl.classList.remove('lithium-tour-visible');

  // Wait for transition to complete, then navigate
  setTimeout(() => {
    currentStepEl._isTransitioning = false;
    // Re-check guard — tour may have been cancelled during fade-out
    if (_tourEnding) return;
    if (direction === 'next') shepherd.next();
    else shepherd.back();
  }, transitionDuration);
}

/**
 * Cancel tour with fade-out transition
 * @param {Object} shepherd - Shepherd Tour instance
 */
function cancelWithTransition(shepherd) {
  _tourEnding = true;
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
        navigateWithTransition(shepherd, 'back');
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

  const shepherd = new Shepherd.Tour({
    useModalOverlay: true,
    defaultStepOptions: {
      cancelIcon: { enabled: false },
      classes: 'lithium-tour-step',
      scrollTo: { behavior: 'smooth', block: 'center' },
    },
    // Custom step content handler
    steps: steps.map((stepDef, index) => {
      const element = stepDef.element || stepDef.Element;
      const position = stepDef.position || stepDef.Position || 'bottom';

      const stepOptions = {
        id: `${tourInstanceId}-step-${index}`,
        title: '',
        text: `
          ${createHeaderHTML(tour, options)}
          <div class="lithium-tour-content">${stepDef.text || stepDef.Text || ''}</div>
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

  // Track current step index manually
  let currentStepIndex = 0;

  // Single show handler for all step updates
  shepherd.on('show', (event) => {
    // Get step from event or fallback to getCurrentStep
    const step = event?.step || shepherd.getCurrentStep();

    // Find index by comparing step ID
    const currentIndex = shepherd.steps.findIndex(s => s.id === step?.id);
    if (currentIndex >= 0) {
      currentStepIndex = currentIndex;
    }

    // Element might not be ready immediately, try to get it with a small delay
    const processStep = (retryCount = 0) => {
      const currentStepEl = step?.getElement();

      if (!currentStepEl && retryCount < 5) {
        // Retry in 50ms
        setTimeout(() => processStep(retryCount + 1), 50);
        return;
      }

      log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Step show event - index: ${currentStepIndex}, element: ${currentStepEl ? 'found' : 'NOT FOUND'}, retries: ${retryCount}`);

      if (!currentStepEl) {
        log(Subsystems.MANAGER, Status.ERROR, `[Tour] Step show event but no element found after retries`);
        return;
      }

      // Process icons first before showing
      if (typeof processIcons === 'function') {
        processIcons(currentStepEl);
      }

      // Add visible class with double RAF to ensure CSS transition triggers
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          currentStepEl.classList.add('lithium-tour-visible');
          log(Subsystems.MANAGER, Status.DEBUG, `[Tour] Step ${currentStepIndex + 1} is now visible`);
        });
      });
    };

    processStep();
  });

  shepherd.on('start', () => {
    cleanupKeyboard = setupKeyboardNav(shepherd, totalSteps);
    cleanupClickHandler = setupDocumentClickHandler(shepherd, totalSteps, options);
  });

  shepherd.on('cancel', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour cancelled: ${tour.name}`);
    if (cleanupKeyboard) cleanupKeyboard();
    if (cleanupClickHandler) cleanupClickHandler();
    _activeShepherdTour = null;
    _activeTourOptions = null;
    // Ensure all Shepherd DOM artefacts are removed (overlay, step elements)
    // so they don't block subsequent UI interactions
    requestAnimationFrame(() => cleanupActiveTour());
  });

  shepherd.on('complete', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour completed: ${tour.name}`);
    if (cleanupKeyboard) cleanupKeyboard();
    if (cleanupClickHandler) cleanupClickHandler();
    _activeShepherdTour = null;
    _activeTourOptions = null;
    // Ensure all Shepherd DOM artefacts are removed (overlay, step elements)
    // so they don't block subsequent UI interactions
    requestAnimationFrame(() => cleanupActiveTour());
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
        <span>Recommended Tours</span>
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
