/**
 * Column Manager - UI Module
 *
 * Popup DOM creation and positioning.
 *
 * @module tables/column-manager/cm-ui
 */

import { processIcons } from '../../core/icons.js';
import { saveSetting, restoreSetting } from './cm-state.js';

/**
 * Check if this is a depth-2 column manager
 * @param {Object} cm - ColumnManager instance
 * @returns {boolean} True if depth-2
 */
export function isDepth2ColumnManager(cm) {
  if (cm.parentTable?.container) {
    return cm.parentTable.container.closest('.col-manager-popup') !== null;
  }
  return false;
}

/**
 * Create the popup container
 * @param {Object} cm - ColumnManager instance
 */
export function createPopup(cm) {
  // Create popup element
  cm.popup = document.createElement('div');
  cm.popup.className = `${cm.cssPrefix}-popup`;

  // Add depth-2 class if this is a column manager of a column manager
  if (isDepth2ColumnManager(cm)) {
    cm.popup.classList.add('depth-2');
  }

  cm.popup.setAttribute('data-manager-id', cm.managerId);

  // Restore saved dimensions or use defaults
  const savedWidth = restoreSetting(cm, 'width', 500);
  const savedHeight = restoreSetting(cm, 'height', 400);

  cm.popup.style.width = `${savedWidth}px`;
  cm.popup.style.height = `${savedHeight}px`;

  // Position relative to anchor element
  positionPopup(cm);

  // Build popup structure
  const isDepth2 = isDepth2ColumnManager(cm);
  const title = isDepth2 ? 'Column Manager Manager' : 'Column Manager';
  cm.popup.innerHTML = `
    <div class="${cm.cssPrefix}-header">
      <span class="${cm.cssPrefix}-title">${title}</span>
      <div class="${cm.cssPrefix}-mode-toggle">
        <button type="button" class="${cm.cssPrefix}-mode-btn${cm.orderingMode === 'manual' ? ' active' : ''}" data-mode="manual" title="Manual ordering (drag rows)">
          <fa fa-hand></fa> Manual
        </button>
        <button type="button" class="${cm.cssPrefix}-mode-btn${cm.orderingMode === 'auto' ? ' active' : ''}" data-mode="auto" title="Auto ordering (sort columns, edit order)">
          <fa fa-arrow-down-a-z></fa> Auto
        </button>
      </div>
      <div class="${cm.cssPrefix}-header-actions">
        <button type="button" class="${cm.cssPrefix}-save-btn" title="Save" disabled data-tooltip='Save changes<br><span class="li-tip-kbd">Ctrl+Enter</span>' data-shortcut-id="editsave" data-tooltip-original='Save changes<br><span class="li-tip-kbd">Ctrl+Enter</span>'>
          <fa fa-floppy-disk></fa>
        </button>
        <button type="button" class="${cm.cssPrefix}-cancel-btn" title="Cancel" disabled data-tooltip='Cancel changes<br><span class="li-tip-kbd">Esc</span>' data-shortcut-id="editcancel" data-tooltip-original='Cancel changes<br><span class="li-tip-kbd">Esc</span>'>
          <fa fa-ban></fa>
        </button>
        <button type="button" class="${cm.cssPrefix}-close-btn" title="Close">
          <fa fa-xmark></fa>
        </button>
      </div>
    </div>
    <div class="${cm.cssPrefix}-content">
      <div class="lithium-table-container" id="${cm.cssPrefix}-table-container-${cm.managerId}"></div>
      <div class="lithium-navigator-container" id="${cm.cssPrefix}-navigator-${cm.managerId}"></div>
    </div>
    <div class="${cm.cssPrefix}-resize-handle" title="Resize"></div>
  `;

  // Append to body (separate from table container)
  document.body.appendChild(cm.popup);

  // Store reference to this instance on the popup for cleanup
  cm.popup._columnManagerInstance = cm;

  processIcons(cm.popup);
}

/**
 * Position popup relative to anchor element
 * @param {Object} cm - ColumnManager instance
 */
export function positionPopup(cm) {
  if (!cm.anchorElement || !cm.popup) return;

  const anchorRect = cm.anchorElement.getBoundingClientRect();

  // Position: top-left corner of popup anchored to bottom-left of icon
  cm.popup.style.left = `${anchorRect.left}px`;
  cm.popup.style.top = `${anchorRect.bottom + 4}px`;

  // Ensure popup doesn't go off-screen horizontally
  const maxLeft = window.innerWidth - parseInt(cm.popup.style.width, 10) - 20;
  if (anchorRect.left > maxLeft) {
    cm.popup.style.left = `${maxLeft}px`;
  }

  // Ensure popup doesn't go off-screen vertically
  const popupHeight = parseInt(cm.popup.style.height, 10);
  const maxTop = window.innerHeight - popupHeight - 20;
  if (anchorRect.bottom + 4 > maxTop) {
    // Position above the icon instead
    cm.popup.style.top = `${anchorRect.top - popupHeight - 4}px`;
  }
}

/**
 * Show the column manager
 * @param {Object} cm - ColumnManager instance
 */
export function show(cm) {
  if (cm.popup) {
    positionPopup(cm);
    cm.popup.classList.add('visible');

    if (cm.columnTable?.table) {
      setTimeout(() => {
        cm.columnTable.table.redraw(true);
      }, 100);
    }
  }
}

/**
 * Hide the column manager
 * @param {Object} cm - ColumnManager instance
 */
export function hide(cm) {
  if (cm.popup) {
    cm.popup.classList.remove('visible');
  }
}

/**
 * Close the column manager
 * @param {Object} cm - ColumnManager instance
 */
export function close(cm) {
  hide(cm);

  // Remove event listeners
  document.removeEventListener('keydown', cm.handleKeyDown);
  document.removeEventListener('click', cm.handleClickOutside);

  cm.onClose?.();
}

/**
 * Handle table width changes from the navigator
 * @param {Object} cm - ColumnManager instance
 * @param {string} mode - Width mode
 */
export function setTableWidth(cm, mode) {
  if (!cm.popup) return;

  // Get width from CSS variables
  const widthVar = `--table-popup-width-${mode}`;
  const computedStyle = getComputedStyle(document.documentElement);
  const width = computedStyle.getPropertyValue(widthVar).trim();

  if (mode === 'auto' || !width) {
    cm.popup.style.width = '';
    cm.popup.style.maxWidth = '95vw';
  } else {
    cm.popup.style.width = width;
    cm.popup.style.maxWidth = '95vw';
  }

  // Save the width setting
  saveSetting(cm, 'width', parseInt(cm.popup.style.width, 10) || 500);

  // Redraw the table to fit new dimensions
  if (cm.columnTable?.table) {
    cm.columnTable.table.redraw();
  }
}
