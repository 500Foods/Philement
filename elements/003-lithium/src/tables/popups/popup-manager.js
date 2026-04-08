/**
 * Popup Manager Module
 *
 * Handles navigation popup menus (table options, width, layout).
 *
 * @module tables/popups/popup-manager
 */

import { log, Subsystems, Status } from '../../core/log.js';

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

  closeNavPopup(table);

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
  popup.style.top = '0px';
  popup.style.left = '0px';
  popup.style.bottom = 'auto';

  popup.classList.add('visible');

  const hostPopup = btn?.closest?.('.col-manager-popup');
  if (hostPopup) {
    const hostZIndex = parseInt(window.getComputedStyle(hostPopup).zIndex, 10);
    if (Number.isFinite(hostZIndex)) {
      popup.style.zIndex = String(hostZIndex + 10);
    }
  }

  document.body.appendChild(popup);

  const repositionPopup = () => {
    positionNavPopup(btn, popup);
  };

  repositionPopup();
  requestAnimationFrame(repositionPopup);

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
  window.addEventListener('resize', repositionPopup);
  document.addEventListener('scroll', repositionPopup, true);
}

/**
 * Position popup relative to button
 * @param {HTMLElement} btn - Button element
 * @param {HTMLElement} popup - Popup element
 */
function positionNavPopup(btn, popup) {
  if (!btn || !popup) return;

  const viewportPadding = 8;
  const gap = 4;
  const btnRect = btn.getBoundingClientRect();
  const popupRect = popup.getBoundingClientRect();
  const popupWidth = popupRect.width || popup.offsetWidth || 0;
  const popupHeight = popupRect.height || popup.offsetHeight || 0;
  const availableAbove = btnRect.top - viewportPadding;
  const availableBelow = window.innerHeight - btnRect.bottom - viewportPadding;

  let top;
  if (popupHeight <= availableAbove || availableAbove >= availableBelow) {
    top = btnRect.top - popupHeight - gap;
  } else {
    top = btnRect.bottom + gap;
  }

  let left = btnRect.left;
  if (left + popupWidth > window.innerWidth - viewportPadding) {
    left = window.innerWidth - popupWidth - viewportPadding;
  }
  if (left < viewportPadding) {
    left = viewportPadding;
  }

  const maxTop = Math.max(viewportPadding, window.innerHeight - popupHeight - viewportPadding);
  popup.style.top = `${Math.min(Math.max(top, viewportPadding), maxTop)}px`;
  popup.style.left = `${left}px`;
  popup.style.right = 'auto';
}

/**
 * Close active navigation popup
 * @param {Object} table - LithiumTable instance
 */
export function closeNavPopup(table) {
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
    case 'menu':
      return [
        { label: 'Expand All', action: () => table.expandAll?.() },
        { label: 'Collapse All', action: () => table.collapseAll?.() },
        { label: 'Toggle Row Height', action: () => table.toggleRowHeight?.() },
      ];
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
