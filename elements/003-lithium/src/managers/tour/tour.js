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
let _listPopup = null;

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
 * @param {number} currentStep - Current step index (0-based)
 * @param {number} totalSteps - Total number of steps
 * @param {Object} tour - Tour object
 * @param {Object} options - Options including onShowList callback
 * @returns {string} HTML string for the header
 */
function createHeaderHTML(currentStep, totalSteps, tour, options) {
  const listBtn = options.onShowList 
    ? `<button type="button" class="lithium-tour-header-btn lithium-tour-header-list" data-tour-action="list" data-tooltip="Show all tours">
         <fa fa-list></fa>
       </button>` 
    : '';
  
  return `
    <div class="lithium-tour-header">
      <div class="lithium-tour-header-title">${tour.name}</div>
      <div class="lithium-tour-header-actions">
        ${listBtn}
        <button type="button" class="lithium-tour-header-btn lithium-tour-header-close" data-tour-action="close" data-tooltip="Close tour (Esc)">
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
              class="lithium-tour-footer-btn lithium-tour-footer-back" 
              data-tour-action="back" 
              data-tooltip="Previous (←)"
              ${isFirst ? 'disabled' : ''}>
        <fa fa-chevron-left></fa>
        <span>Back</span>
      </button>
      <div class="lithium-tour-step-counter" title="Step ${currentStep + 1} of ${totalSteps}">${stepCounter}</div>
      <button type="button" 
              class="lithium-tour-footer-btn lithium-tour-footer-next" 
              data-tour-action="next"
              data-tooltip="${isLast ? 'Finish tour' : 'Next (→)'}">
        <span>${isLast ? 'Done' : 'Next'}</span>
        <fa fa-${isLast ? 'check' : 'chevron-right'}></fa>
      </button>
    </div>
  `;
}

/**
 * Setup keyboard navigation for a tour
 * @param {Object} shepherd - Shepherd Tour instance
 * @param {Object} options - Options including onShowList callback
 * @returns {Function} Cleanup function to remove listeners
 */
function setupKeyboardNav(shepherd, options) {
  const handleKeydown = (e) => {
    // Only handle if no modifier keys pressed
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    
    const currentStep = shepherd.getCurrentStep();
    if (!currentStep) return;
    
    switch (e.key) {
      case 'ArrowLeft':
        e.preventDefault();
        shepherd.back();
        break;
      case 'ArrowRight':
        e.preventDefault();
        shepherd.next();
        break;
      case 'Escape':
        e.preventDefault();
        shepherd.cancel();
        break;
    }
  };

  document.addEventListener('keydown', handleKeydown);
  
  // Return cleanup function
  return () => document.removeEventListener('keydown', handleKeydown);
}

/**
 * Process icons and tooltips in an element
 * @param {HTMLElement} el - Element to process
 */
function processTourIcons(el) {
  if (el && typeof processIcons === 'function') {
    processIcons(el);
  }
  // Process tooltips
  if (el) {
    el.querySelectorAll('[data-tooltip]').forEach(tooltipEl => {
      // Simple tooltip via title attribute for now
      // Full tooltip system integration can be added later
    });
  }
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
        id: `step-${index}`,
        title: '',
        text: `
          ${createHeaderHTML(index, totalSteps, tour, options)}
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

  // Setup keyboard navigation
  let cleanupKeyboard = null;

  shepherd.on('show', () => {
    // Update step counter and button states after show
    const currentIndex = shepherd.steps.findIndex(s => s.isOpen());
    const currentStepEl = shepherd.getCurrentStep()?.getElement();
    if (currentIndex >= 0 && currentStepEl) {
      const counter = currentStepEl.querySelector('.lithium-tour-step-counter');
      if (counter) {
        counter.textContent = `${currentIndex + 1} / ${totalSteps}`;
        counter.title = `Step ${currentIndex + 1} of ${totalSteps}`;
      }
      
      // Update button states
      const backBtn = currentStepEl.querySelector('.lithium-tour-footer-back');
      const nextBtn = currentStepEl.querySelector('.lithium-tour-footer-next');
      
      if (backBtn) backBtn.disabled = currentIndex === 0;
      if (nextBtn) {
        const isLast = currentIndex === totalSteps - 1;
        nextBtn.innerHTML = isLast 
          ? '<span>Done</span><fa fa-check></fa>' 
          : '<span>Next</span><fa fa-chevron-right></fa>';
        nextBtn.dataset.tooltip = isLast ? 'Finish tour' : 'Next (→)';
      }
    }
    
    // Process icons in the new step and add fade-in class
    if (currentStepEl) {
      processTourIcons(currentStepEl);
      currentStepEl.classList.add('lithium-tour-visible');
    }
  });

  shepherd.on('start', () => {
    cleanupKeyboard = setupKeyboardNav(shepherd, options);
  });

  shepherd.on('cancel', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour cancelled: ${tour.name}`);
    if (cleanupKeyboard) cleanupKeyboard();
    _activeShepherdTour = null;
  });

  shepherd.on('complete', () => {
    log(Subsystems.MANAGER, Status.INFO, `[Tour] Tour completed: ${tour.name}`);
    if (cleanupKeyboard) cleanupKeyboard();
    _activeShepherdTour = null;
  });

  // Setup button click handlers after tour is created
  shepherd.on('show', () => {
    const el = shepherd.getCurrentStep()?.getElement();
    if (!el) return;
    
    // Close button
    const closeBtn = el.querySelector('[data-tour-action="close"]');
    if (closeBtn && !closeBtn._bound) {
      closeBtn._bound = true;
      closeBtn.addEventListener('click', () => shepherd.cancel());
    }
    
    // List button
    const listBtn = el.querySelector('[data-tour-action="list"]');
    if (listBtn && !listBtn._bound) {
      listBtn._bound = true;
      listBtn.addEventListener('click', () => {
        shepherd.cancel();
        if (options.onShowList) options.onShowList();
      });
    }
    
    // Back button
    const backBtn = el.querySelector('[data-tour-action="back"]');
    if (backBtn && !backBtn._bound) {
      backBtn._bound = true;
      backBtn.addEventListener('click', () => shepherd.back());
    }
    
    // Next/Done button
    const nextBtn = el.querySelector('[data-tour-action="next"]');
    if (nextBtn && !nextBtn._bound) {
      nextBtn._bound = true;
      nextBtn.addEventListener('click', () => {
        const currentIndex = shepherd.steps.findIndex(s => s.isOpen());
        if (currentIndex === totalSteps - 1) {
          shepherd.complete();
        } else {
          shepherd.next();
        }
      });
    }
  });

  return shepherd;
}

/**
 * Switch to a manager by name
 * @param {string} managerName - Manager name (e.g., "023.Lookup Manager")
 */
async function switchToManager(managerName) {
  if (!managerName) return;
  
  // Extract manager ID from name (e.g., "023.Lookup Manager" -> "23")
  const match = managerName.match(/^(\d+)\./);
  if (!match) {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] Cannot parse manager name: ${managerName}`);
    return;
  }
  
  const managerId = parseInt(match[1], 10);
  log(Subsystems.MANAGER, Status.INFO, `[Tour] Switching to manager: ${managerName} (ID: ${managerId})`);
  
  // Use the app's loadManager if available
  const app = window.lithiumApp;
  if (app && typeof app.loadManager === 'function') {
    app.loadManager(managerId);
    // Wait a bit for the manager to load
    await new Promise(resolve => setTimeout(resolve, 500));
  }
}

/**
 * Start a tour by ID. If not found, show the tour list instead.
 * @param {string|number} tourId - Tour ID
 * @param {Object} api - The API/conduit instance
 * @param {Object} options - Options including anchor for list popup
 */
export async function launchTour(tourId, api, options = {}) {
  // End any active tour first
  if (_activeShepherdTour) {
    _activeShepherdTour.cancel();
  }

  const tours = await getTours(api);
  const tour = tours.find(t => String(t.id) === String(tourId));

  if (!tour) {
    log(Subsystems.MANAGER, Status.WARN, `[Tour] Tour not found: ${tourId}, showing list`);
    // Fall back to showing the tour list
    showTourList(tours, options.anchor, {
      onSelect: (selectedId) => launchTour(selectedId, api, options),
    });
    return null;
  }

  log(Subsystems.MANAGER, Status.INFO, `[Tour] Launching tour: ${tour.name}`);

  // Switch to target manager if specified
  const { definition } = tour;
  if (definition.manager) {
    await switchToManager(definition.manager);
  }

  const shepherd = await buildShepherdTour(tour, options);
  shepherd.start();

  return shepherd;
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
        <div class="lithium-tour-list-item-manager">${t.definition.manager || ''}</div>
      </div>
    </div>
  `).join('');

  popup.innerHTML = `
    <div class="lithium-tour-list-header">
      <span class="lithium-tour-list-title">
        <fa fa-route></fa>
        <span>Available Tours</span>
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
  processIcons(popup);

  // Animate in
  requestAnimationFrame(() => {
    popup.classList.add('visible');
    if (!anchor) {
      popup.style.transform = 'translate(-50%, -50%) scale(1)';
    }
  });

  _listPopup = popup;

  // Close button
  popup.querySelector('.lithium-tour-list-close').addEventListener('click', () => {
    hideTourList();
  });

  // Tour item clicks
  popup.querySelectorAll('.lithium-tour-list-item').forEach(item => {
    item.addEventListener('click', () => {
      const tourId = item.dataset.tourId;
      hideTourList();
      if (options.onSelect) {
        options.onSelect(tourId);
      }
      eventBus.emit('tour:select', { tourId });
    });
  });

  // Close on outside click
  const handleOutsideClick = (e) => {
    if (!popup.contains(e.target) && !anchor?.contains(e.target)) {
      hideTourList();
      document.removeEventListener('click', handleOutsideClick);
    }
  };
  setTimeout(() => document.addEventListener('click', handleOutsideClick), 0);

  // Close on escape
  const handleEscape = (e) => {
    if (e.key === 'Escape') {
      hideTourList();
      document.removeEventListener('keydown', handleEscape);
    }
  };
  document.addEventListener('keydown', handleEscape);
}

/**
 * Hide the tour list popup
 */
export function hideTourList() {
  if (_listPopup) {
    _listPopup.remove();
    _listPopup = null;
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
    await launchTour(tourId, api);
  });

  log(Subsystems.MANAGER, Status.INFO, '[Tour] Initialized');
}

// Auto-initialize
initTours();
