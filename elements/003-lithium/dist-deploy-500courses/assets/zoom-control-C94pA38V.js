import { a as closeAllPopups, p as positionPopup, h as hidePopup } from './manager-ui-BnjjCmoo.js';
import './index-CEuz7QhE.js';
import './conduit-B8Ld7ghl.js';
import './languages-Coru4QdC.js';
import './codemirror-setup-lxLGSWU-.js';
import './codemirror-Bg1rM8n2.js';

/**
 * Zoom Control Module
 *
 * Provides global page zoom functionality with a slider popup.
 * Zoom level is persisted to localStorage.
 *
 * Implementation uses CSS transform: scale() on the app container
 * with counter-scaling to prevent layout expansion.
 */

 
// localStorage key for zoom level
const ZOOM_LEVEL_KEY = 'lithium_zoom_level';
 
// Default zoom level (100%)
const DEFAULT_ZOOM = 1;
 
// Zoom constraints
const MIN_ZOOM = 0.5;  // 50%
const MAX_ZOOM = 3;    // 300% (was 400%)
 
// Detent values (every 25% from 50% to 400%)
const DETENTS = [
  0.5,   // 50%
  0.75,  // 75%
  1,     // 100%
  1.25,  // 125%
  1.5,   // 150%
  1.75,  // 175%
  2,     // 200%
  2.25,  // 225%
  2.5,   // 250%
  2.75,  // 275%
  3,     // 300%
  3.25,  // 325%
  3.5,   // 350%
  3.75,  // 375%
  4,     // 400%
];
 
// Track active zoom popup and its button
let activePopup = null;
let activeButton = null;
let currentZoomLevel = DEFAULT_ZOOM;
 
// Callbacks for zoom change notifications
const zoomChangeCallbacks = new Set();
 
/**
 * Initialize the zoom system
 * Loads saved zoom level from localStorage and applies it
 */
function initZoom() {
  loadZoomLevel();
  applyZoom(currentZoomLevel);
}
 
/**
 * Set the zoom level
 * @param {number} level - Zoom level (0.5 to 4)
 * @param {boolean} save - Whether to save to localStorage (default: true)
 */
function setZoomLevel(level, save = true) {
  // Clamp to valid range
  const zoom = Math.max(MIN_ZOOM, Math.min(MAX_ZOOM, level));
  currentZoomLevel = zoom;
 
  applyZoom(zoom);
 
  if (save) {
    saveZoomLevel(zoom);
  }
 
  // Notify all registered callbacks
  zoomChangeCallbacks.forEach(callback => {
    try {
      callback(zoom);
    } catch (_e) {
      // Ignore callback errors
    }
  });
 
  return zoom;
}
 
/**
 * Apply zoom level via CSS custom properties
 * Uses transform: scale() with counter-scaling for layout
 * @param {number} zoom - Zoom level to apply
 */
function applyZoom(zoom) {
  const root = document.documentElement;
 
  // Set CSS custom properties for zoom
  root.style.setProperty('--zoom', zoom);
  root.style.setProperty('--zoom-inv', 1 / zoom);
 
  // Apply transform to main layout (contains both sidebar and manager area)
  // This ensures consistent scaling across the entire UI
  const mainLayout = document.getElementById('main-layout');
  if (mainLayout) {
    // Add transition class for smooth animation
    mainLayout.classList.add('zoom-transition');
 
    // Apply zoom transform
    mainLayout.style.transform = `scale(${zoom})`;
    mainLayout.style.transformOrigin = '0 0';
    mainLayout.style.width = `${100 / zoom}%`;
    mainLayout.style.height = `${100 / zoom}vh`;
 
    // Remove transition class after animation completes
    // to prevent transition on initial load
    setTimeout(() => {
      mainLayout.classList.remove('zoom-transition');
    }, 210);
  }
 
  // Update any active slider to reflect new zoom level
  updateActiveSlider(zoom);
}
 
/**
 * Load zoom level from localStorage
 */
function loadZoomLevel() {
  try {
    const stored = localStorage.getItem(ZOOM_LEVEL_KEY);
    if (stored) {
      const zoom = parseFloat(stored);
      if (!isNaN(zoom) && zoom >= MIN_ZOOM && zoom <= MAX_ZOOM) {
        currentZoomLevel = zoom;
      }
    }
  } catch (_e) {
    // Non-fatal: use default
    currentZoomLevel = DEFAULT_ZOOM;
  }
}
 
/**
 * Save zoom level to localStorage
 * @param {number} zoom - Zoom level to save
 */
function saveZoomLevel(zoom) {
  try {
    localStorage.setItem(ZOOM_LEVEL_KEY, String(zoom));
  } catch (_e) {
    // Non-fatal: ignore storage errors
  }
}
 
/**
 * Create and show the zoom popup for a button
 * @param {HTMLElement} button - The button that triggered the popup
 * @returns {HTMLElement} The popup element
 */
function showZoomPopup(button) {
  // Close any existing popup
  hideZoomPopup();

  // Close other manager popups
  closeAllPopups();

  // Track button and set toggle state
  activeButton = button;
  button.classList.add('popup-active');

  // Create popup container - use manager-ui-popup classes for consistent styling/animation
  const popup = document.createElement('div');
  popup.className = 'manager-ui-popup manager-header-popup zoom-popup';
  
  // Create popup content
  popup.innerHTML = `
    <div class="zoom-popup-header">
      <span class="zoom-popup-title">Zoom</span>
      <span class="zoom-popup-value">${Math.round(currentZoomLevel * 100)}%</span>
    </div>
    <div class="zoom-popup-slider-container">
      <input type="range" class="zoom-popup-slider" 
             min="${MIN_ZOOM}" max="${MAX_ZOOM}" 
             step="0.01" value="${currentZoomLevel}">
      <div class="zoom-popup-detents">
        ${DETENTS.map(d => `
          <div class="zoom-popup-detent ${Math.abs(d - currentZoomLevel) < 0.01 ? 'active' : ''}" 
               data-zoom="${d}" 
               title="${Math.round(d * 100)}%">
            <div class="zoom-popup-detent-mark"></div>
          </div>
        `).join('')}
      </div>
    </div>
    <div class="zoom-popup-presets">
      <button type="button" class="zoom-popup-preset" data-zoom="0.8">80%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="1">100%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="1.2">120%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="1.4">140%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="1.6">160%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="1.8">180%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="2">200%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="2.2">220%</button>
      <button type="button" class="zoom-popup-preset" data-zoom="2.4">240%</button>
    </div>
  `;
  
  // Position popup below the button, with top-right aligned to bottom-right of button
  // Use positionPopup for consistent positioning with other header popups
  popup.style.zIndex = '10001';
  document.body.appendChild(popup);
  positionPopup(popup, button, 'header-dropdown');
  
  // Add event listeners
  const slider = popup.querySelector('.zoom-popup-slider');
  const valueDisplay = popup.querySelector('.zoom-popup-value');
  const detents = popup.querySelectorAll('.zoom-popup-detent');
  const presets = popup.querySelectorAll('.zoom-popup-preset');
  
  // Slider input - snap to 10% increments
  slider.addEventListener('input', (e) => {
    const rawZoom = parseFloat(e.target.value);
    // Snap to nearest 10% (0.1)
    const zoom = Math.round(rawZoom * 10) / 10;
    setZoomLevel(zoom);
    valueDisplay.textContent = `${Math.round(zoom * 100)}%`;
    updateDetentVisuals(detents, zoom);
  });
  
  // Detent clicks
  detents.forEach(detent => {
    detent.addEventListener('click', () => {
      const zoom = parseFloat(detent.dataset.zoom);
      setZoomLevel(zoom);
      slider.value = zoom;
      valueDisplay.textContent = `${Math.round(zoom * 100)}%`;
      updateDetentVisuals(detents, zoom);
    });
  });
  
  // Preset buttons
  presets.forEach(preset => {
    preset.addEventListener('click', () => {
      const zoom = parseFloat(preset.dataset.zoom);
      setZoomLevel(zoom);
      slider.value = zoom;
      valueDisplay.textContent = `${Math.round(zoom * 100)}%`;
      updateDetentVisuals(detents, zoom);
    });
  });
  
  // Close on outside click
  setTimeout(() => {
    document.addEventListener('click', handleOutsideClick);
  }, 0);
  
  // Close on escape key and handle arrow navigation
  document.addEventListener('keydown', handlePopupKeydown);
  
  activePopup = popup;
  
  // Trigger animation after a small delay to ensure DOM is ready
  setTimeout(() => {
    popup.classList.add('visible');
  }, 10);
  
  return popup;
}
 
/**
 * Hide the zoom popup with animation
 * @param {boolean} skipAnimation - If true, skip animation
 */
function hideZoomPopup(skipAnimation = false) {
  if (!activePopup) return;

  // Remove button active state immediately so it looks normal
  if (activeButton) {
    activeButton.classList.remove('popup-active');
    activeButton = null;
  }

  // Then animate the popup close
  hidePopup(activePopup, () => {
    activePopup = null;
  }, skipAnimation);

  document.removeEventListener('click', handleOutsideClick);
  document.removeEventListener('keydown', handlePopupKeydown);
}
 
/**
 * Toggle the zoom popup for a button
 * @param {HTMLElement} button - The button that triggered the popup
 * @returns {HTMLElement|null} The popup element or null if hidden
 */
function toggleZoomPopup(button) {
  if (activePopup) {
    // Pass false to play close animation when toggling off
    hideZoomPopup(false);
    return null;
  }
  return showZoomPopup(button);
}
 
/**
 * Update detent visual state
 * @param {NodeList} detents - Detent elements
 * @param {number} currentZoom - Current zoom level
 */
function updateDetentVisuals(detents, currentZoom) {
  detents.forEach(detent => {
    const detentZoom = parseFloat(detent.dataset.zoom);
    const isActive = Math.abs(detentZoom - currentZoom) < 0.01;
    detent.classList.toggle('active', isActive);
  });
}
 
/**
 * Update the active slider value
 * @param {number} zoom - Current zoom level
 */
function updateActiveSlider(zoom) {
  if (activePopup) {
    const slider = activePopup.querySelector('.zoom-popup-slider');
    const valueDisplay = activePopup.querySelector('.zoom-popup-value');
    const detents = activePopup.querySelectorAll('.zoom-popup-detent');
    
    if (slider) slider.value = zoom;
    if (valueDisplay) valueDisplay.textContent = `${Math.round(zoom * 100)}%`;
    if (detents) updateDetentVisuals(detents, zoom);
  }
}
 
/**
 * Handle clicks outside the popup
 * @param {Event} event 
 */
function handleOutsideClick(event) {
  if (activePopup && !activePopup.contains(event.target) && 
      !event.target.closest('.zoom-btn')) {
    // Pass false to play close animation
    hideZoomPopup(false);
  }
}

/**
 * Handle keyboard navigation in popup
 * @param {KeyboardEvent} event
 */
function handlePopupKeydown(event) {
  if (!activePopup) return;
  
  // Left/Right arrows to change zoom level
  if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') {
    event.preventDefault();
    const direction = event.key === 'ArrowRight' ? 1 : -1;
    
    // Find current index and move one step
    const currentIndex = DETENTS.findIndex(d => Math.abs(d - currentZoomLevel) < 0.01);
    const newIndex = Math.max(0, Math.min(DETENTS.length - 1, currentIndex + direction));
    setZoomLevel(DETENTS[newIndex]);
    
    // Update popup UI
    const slider = activePopup.querySelector('.zoom-popup-slider');
    const valueDisplay = activePopup.querySelector('.zoom-popup-value');
    const detents = activePopup.querySelectorAll('.zoom-popup-detent');
    if (slider) slider.value = DETENTS[newIndex];
    if (valueDisplay) valueDisplay.textContent = `${Math.round(DETENTS[newIndex] * 100)}%`;
    updateDetentVisuals(detents, DETENTS[newIndex]);
    return;
  }
  
  // Escape to close
  if (event.key === 'Escape') {
    hideZoomPopup(false);
  }
}
 
// Initialize zoom on module load
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initZoom);
} else {
  initZoom();
}

export { hideZoomPopup, initZoom, setZoomLevel, showZoomPopup, toggleZoomPopup };
