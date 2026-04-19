/**
 * Popup Manager Module
 *
 * Handles navigation popup menus (table options, width, layout).
 *
 * @module tables/popups/popup-manager
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { closeGroupingPopupImmediate } from './grouping-popup.js';

/**
 * Toggle navigation popup
 * @param {Object} table - LithiumTable instance
 * @param {Event} e - Click event
 * @param {string} popupId - Popup identifier
 */
export function toggleNavPopup(table, e, popupId) {
  e.stopPropagation();

  if (table.activeNavPopup && table.activeNavPopupId === popupId) {
    closeNavPopup(table);
    return;
  }

  // Close any existing popup immediately (no animation) before opening new one
  closeNavPopupImmediate(table);

  // Dispatch event to close all manager popups
  document.dispatchEvent(new CustomEvent('close-all-popups'));

  const btn = e.currentTarget;
  const popup = buildStandardNavPopup(table, getPopupItems(table, popupId));

  if (!popup) return;

  showNavPopup(table, btn, popup, popupId);
}

/**
 * Build standard navigation popup
 * @param {Object} table - LithiumTable instance
 * @param {Array} items - Popup items
 * @returns {HTMLElement} Popup element
 */
function buildStandardNavPopup(table, items) {
  if (!items.length) return null;

  const popup = document.createElement('div');
  popup.className = 'lithium-nav-popup';

  items.forEach((item) => {
    if (item.isSeparator) {
      const separator = document.createElement('div');
      separator.className = 'lithium-nav-popup-separator';
      popup.appendChild(separator);
      return;
    }

    const row = document.createElement('button');
    row.type = 'button';
    row.className = 'lithium-nav-popup-item';

    if (typeof item.checked === 'boolean') {
      row.innerHTML = `
        <span class="lithium-nav-popup-check">${item.checked ? '&#10003;' : ''}</span>
        <span class="lithium-nav-popup-label">${item.label}</span>
      `;
    } else {
      row.textContent = item.label;
    }

    row.addEventListener('click', () => {
      closeNavPopup(table);
      item.action();
    });
    popup.appendChild(row);
  });

  return popup;
}

/**
 * Show popup positioned near button
 * @param {Object} table - LithiumTable instance
 * @param {HTMLElement} btn - Button element
 * @param {HTMLElement} popup - Popup element
 * @param {string} popupId - Popup identifier
 */
function showNavPopup(table, btn, popup, popupId) {
  popup.style.position = 'fixed';
  // Footer-riseup positioning: bottom-right of popup 1px above top-right of button
  popup.style.bottom = 'auto';
  popup.style.top = 'auto';
  popup.style.left = 'auto';
  popup.style.right = 'auto';

  // Add popup-active class to button for toggle styling
  btn.classList.add('popup-active');

  // Check for Column Manager popup
  const hostPopup = btn?.closest?.('.col-manager-popup');
  if (hostPopup) {
    const hostZIndex = parseInt(window.getComputedStyle(hostPopup).zIndex, 10);
    if (Number.isFinite(hostZIndex)) {
      popup.style.zIndex = String(hostZIndex + 10);
    }
  } else {
    // Check for Crimson popup
    const crimsonPopup = btn?.closest?.('.crimson-citation-popup');
    if (crimsonPopup) {
      const crimsonZIndex = parseInt(window.getComputedStyle(crimsonPopup).zIndex, 10);
      if (Number.isFinite(crimsonZIndex)) {
        popup.style.zIndex = String(crimsonZIndex + 10);
      }
    }
  }

  document.body.appendChild(popup);

  const repositionPopup = () => {
    positionNavPopup(btn, popup);
  };

  repositionPopup();
  requestAnimationFrame(repositionPopup);

  // Trigger animation after positioning
  requestAnimationFrame(() => {
    popup.classList.add('visible');
  });

  table.activeNavPopup = popup;
  table.activeNavPopupId = popupId;
  table.activeNavPopupButton = btn;
  table.navPopupRepositionHandler = repositionPopup;

  table.navPopupCloseHandler = (evt) => {
    if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
      closeNavPopup(table);
    }
  };
  document.addEventListener('click', table.navPopupCloseHandler);

  // ESC key handler
  table.navPopupKeyHandler = (evt) => {
    if (evt.key === 'Escape') {
      closeNavPopup(table);
    }
  };
  document.addEventListener('keydown', table.navPopupKeyHandler);

  // Listen for close-all-popups from manager menus
  table.navPopupGlobalCloseHandler = () => {
    closeNavPopup(table);
  };
  document.addEventListener('close-all-popups', table.navPopupGlobalCloseHandler);

  window.addEventListener('resize', repositionPopup);
  document.addEventListener('scroll', repositionPopup, true);
}

/**
 * Position popup relative to button using footer-riseup style
 * Bottom-right corner of popup is 1px above top-right corner of button
 * @param {HTMLElement} btn - Button element
 * @param {HTMLElement} popup - Popup element
 */
function positionNavPopup(btn, popup) {
  if (!btn || !popup) return;

  const btnRect = btn.getBoundingClientRect();

  // Footer-riseup positioning: bottom-right of popup 1px above top-right of button
  popup.style.bottom = `${window.innerHeight - btnRect.top + 1}px`;
  popup.style.right = `${window.innerWidth - btnRect.right}px`;
  popup.style.top = 'auto';
  popup.style.left = 'auto';
}

/**
 * Close active navigation popup immediately (no animation)
 * Used when opening a new popup to avoid animation conflicts
 * @param {Object} table - LithiumTable instance
 */
export function closeNavPopupImmediate(table) {
  // Handle grouping popup specifically if that's what's open
  if (table.activeNavPopupId === 'grouping') {
    closeGroupingPopupImmediate(table);
    return;
  }

  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }
  if (table.activeNavPopup) {
    table.activeNavPopup.remove();
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupGlobalCloseHandler) {
    document.removeEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
    table.navPopupGlobalCloseHandler = null;
  }
  if (table.navPopupRepositionHandler) {
    window.removeEventListener('resize', table.navPopupRepositionHandler);
    document.removeEventListener('scroll', table.navPopupRepositionHandler, true);
    table.navPopupRepositionHandler = null;
  }
}

/**
 * Close active navigation popup with animation
 * @param {Object} table - LithiumTable instance
 */
export function closeNavPopup(table) {
  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }
  if (table.activeNavPopup) {
    table.activeNavPopup.classList.remove('visible');
    // Remove after animation completes
    const duration = 200; // Match transition duration
    setTimeout(() => {
      if (table.activeNavPopup && table.activeNavPopup.parentNode) {
        table.activeNavPopup.remove();
      }
    }, duration);
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupRepositionHandler) {
    window.removeEventListener('resize', table.navPopupRepositionHandler);
    document.removeEventListener('scroll', table.navPopupRepositionHandler, true);
    table.navPopupRepositionHandler = null;
  }
}

/**
 * Close all transient popups
 * @param {Object} table - LithiumTable instance
 */
export function closeTransientPopups(table) {
  closeNavPopup(table);
  if (typeof table.closeColumnChooser === 'function') {
    table.closeColumnChooser();
  }
}

/**
 * Get popup menu items
 * @param {Object} table - LithiumTable instance
 * @param {string} popupId - Popup identifier
 * @returns {Array} Menu items
 */
function getPopupItems(table, popupId) {
  switch (popupId) {
    case 'width':
      return [
        { label: 'Narrow', checked: table.tableWidthMode === 'narrow', action: () => table.setTableWidth?.('narrow') },
        { label: 'Compact', checked: table.tableWidthMode === 'compact', action: () => table.setTableWidth?.('compact') },
        { label: 'Normal', checked: table.tableWidthMode === 'normal', action: () => table.setTableWidth?.('normal') },
        { label: 'Wide', checked: table.tableWidthMode === 'wide', action: () => table.setTableWidth?.('wide') },
        { label: 'Auto', checked: table.tableWidthMode === 'auto', action: () => table.setTableWidth?.('auto') },
      ];
    case 'layout':
      return [
        { label: 'Fit Columns', checked: table.tableLayoutMode === 'fitColumns', action: () => table.setTableLayout?.('fitColumns') },
        { label: 'Fit Data', checked: table.tableLayoutMode === 'fitData', action: () => table.setTableLayout?.('fitData') },
        { label: 'Fit Fill', checked: table.tableLayoutMode === 'fitDataFill', action: () => table.setTableLayout?.('fitDataFill') },
        { label: 'Fit Stretch', checked: table.tableLayoutMode === 'fitDataStretch', action: () => table.setTableLayout?.('fitDataStretch') },
        { label: 'Fit Table', checked: table.tableLayoutMode === 'fitDataTable', action: () => table.setTableLayout?.('fitDataTable') },
      ];
    default:
      return [];
  }
}

/**
 * Create a popup separator element
 * @returns {HTMLElement} Separator element
 */
export function createPopupSeparator() {
  const separator = document.createElement('div');
  separator.className = 'lithium-nav-popup-separator';
  return separator;
}
