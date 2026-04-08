/**
 * Column Manager - Drag and Resize Module
 *
 * Popup resize handling and keyboard events.
 *
 * @module tables/column-manager/cm-drag-resize
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { saveSetting } from './cm-state.js';

/**
 * Handle resize start
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
export function handleResizeStart(cm, e) {
  e.preventDefault();
  e.stopPropagation();

  cm.isResizing = true;
  cm.popup.classList.add('resizing');

  cm.resizeStartX = e.clientX;
  cm.resizeStartY = e.clientY;

  const rect = cm.popup.getBoundingClientRect();
  cm.popupStartWidth = rect.width;
  cm.popupStartHeight = rect.height;

  document.addEventListener('mousemove', cm.handleResizeMove);
  document.addEventListener('mouseup', cm.handleResizeEnd);
}

/**
 * Handle resize move
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
export function handleResizeMove(cm, e) {
  if (!cm.isResizing) return;

  const deltaX = e.clientX - cm.resizeStartX;
  const deltaY = e.clientY - cm.resizeStartY;

  // Calculate new dimensions (only expand from bottom-right)
  const newWidth = Math.max(300, Math.min(window.innerWidth - 40, cm.popupStartWidth + deltaX));
  const newHeight = Math.max(200, Math.min(window.innerHeight - 100, cm.popupStartHeight + deltaY));

  cm.popup.style.width = `${newWidth}px`;
  cm.popup.style.height = `${newHeight}px`;

  // Redraw column table to fit new dimensions
  if (cm.columnTable?.table) {
    cm.columnTable.table.redraw();
  }
}

/**
 * Handle resize end
 * @param {Object} cm - ColumnManager instance
 */
export function handleResizeEnd(cm) {
  cm.isResizing = false;
  cm.popup.classList.remove('resizing');

  document.removeEventListener('mousemove', cm.handleResizeMove);
  document.removeEventListener('mouseup', cm.handleResizeEnd);

  // Save dimensions
  saveSetting(cm, 'width', parseInt(cm.popup.style.width, 10));
  saveSetting(cm, 'height', parseInt(cm.popup.style.height, 10));
}

/**
 * Handle keyboard events
 * @param {Object} cm - ColumnManager instance
 * @param {KeyboardEvent} e - Keyboard event
 */
export function handleKeyDown(cm, e) {
  if (e.key === 'Escape' && cm.popup?.classList.contains('visible')) {
    if (cm.columnTable?.isEditing) {
      e.preventDefault();
      e.stopPropagation();
      void cm.handlePrimaryCancel();
      return;
    }
    cm.close();
  }
}

/**
 * Handle click outside popup
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
export function handleClickOutside(cm, e) {
  if (cm.popup && !cm.popup.contains(e.target) && cm.popup.classList.contains('visible')) {
    // Check if click was on the anchor element (column chooser button)
    if (cm.anchorElement && cm.anchorElement.contains(e.target)) {
      return; // Don't close if clicking the anchor
    }

    // Check if click is inside any other column manager popup
    const otherPopups = document.querySelectorAll('.col-manager-popup');
    for (const popup of otherPopups) {
      if (popup !== cm.popup && popup.contains(e.target)) {
        return; // Don't close if clicking inside another column manager
      }
    }

    // Check if click is on a navigator popup item
    if (e.target.closest('.lithium-nav-popup') ||
        e.target.closest('.lithium-nav-popup-item') ||
        e.target.closest('[class*="nav-popup"]')) {
      return; // Don't close if clicking on navigator popup
    }

    cm.close();
  }
}
