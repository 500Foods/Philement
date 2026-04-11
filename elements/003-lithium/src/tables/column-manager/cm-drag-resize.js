/**
 * Column Manager - Drag and Resize Module
 *
 * Popup resize handling (four corners), drag to move, and keyboard events.
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
  cm.popupStartLeft = rect.left;
  cm.popupStartTop = rect.top;

  // Determine which corner based on event target
  const handle = e.target;
  if (handle.classList.contains('col-manager-resize-handle-tl')) {
    cm.resizeCorner = 'tl';
  } else if (handle.classList.contains('col-manager-resize-handle-tr')) {
    cm.resizeCorner = 'tr';
  } else if (handle.classList.contains('col-manager-resize-handle-bl')) {
    cm.resizeCorner = 'bl';
  } else {
    cm.resizeCorner = 'br';
  }

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

  let newWidth = cm.popupStartWidth;
  let newHeight = cm.popupStartHeight;
  let newLeft = cm.popupStartLeft;
  let newTop = cm.popupStartTop;

  const minWidth = 300;
  const minHeight = 200;
  const maxWidth = window.innerWidth * 0.95;
  const maxHeight = window.innerHeight * 0.95;

  switch (cm.resizeCorner) {
    case 'br':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth + deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight + deltaY));
      break;
    case 'bl':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth - deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight + deltaY));
      if (newWidth !== cm.popupStartWidth - deltaX) {
        const actualDeltaX = cm.popupStartWidth - newWidth;
        newLeft = cm.popupStartLeft + actualDeltaX;
      } else {
        newLeft = cm.popupStartLeft + deltaX;
      }
      break;
    case 'tr':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth + deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight - deltaY));
      if (newHeight !== cm.popupStartHeight - deltaY) {
        const actualDeltaY = cm.popupStartHeight - newHeight;
        newTop = cm.popupStartTop + actualDeltaY;
      } else {
        newTop = cm.popupStartTop + deltaY;
      }
      break;
    case 'tl':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth - deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight - deltaY));
      if (newWidth !== cm.popupStartWidth - deltaX) {
        const actualDeltaX = cm.popupStartWidth - newWidth;
        newLeft = cm.popupStartLeft + actualDeltaX;
      } else {
        newLeft = cm.popupStartLeft + deltaX;
      }
      if (newHeight !== cm.popupStartHeight - deltaY) {
        const actualDeltaY = cm.popupStartHeight - newHeight;
        newTop = cm.popupStartTop + actualDeltaY;
      } else {
        newTop = cm.popupStartTop + deltaY;
      }
      break;
  }

  cm.popup.style.width = `${newWidth}px`;
  cm.popup.style.height = `${newHeight}px`;

  if (cm.resizeCorner === 'bl' || cm.resizeCorner === 'tl') {
    cm.popup.style.left = `${newLeft}px`;
  }
  if (cm.resizeCorner === 'tr' || cm.resizeCorner === 'tl') {
    cm.popup.style.top = `${newTop}px`;
  }

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
 * Handle drag start (header)
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
export function handleDragStart(cm, e) {
  // Allow dragging from the header, but not from actionable controls
  const header = cm.popup?.querySelector(`.${cm.cssPrefix}-header`);
  if (!header || !header.contains(e.target)) return;

  // Don't start drag if clicking on buttons
  if (e.target.closest('button')) return;

  cm.isDragging = true;
  cm.popup.classList.add('dragging');

  cm.dragStartX = e.clientX;
  cm.dragStartY = e.clientY;

  const rect = cm.popup.getBoundingClientRect();
  cm.popupStartX = rect.left;
  cm.popupStartY = rect.top;

  document.addEventListener('mousemove', cm.handleDragMove);
  document.addEventListener('mouseup', cm.handleDragEnd);

  log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Drag started');
  e.preventDefault();
  e.stopPropagation();
}

/**
 * Handle drag move
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
export function handleDragMove(cm, e) {
  if (!cm.isDragging) return;

  const deltaX = e.clientX - cm.dragStartX;
  const deltaY = e.clientY - cm.dragStartY;

  let newX = cm.popupStartX + deltaX;
  let newY = cm.popupStartY + deltaY;

  const rect = cm.popup.getBoundingClientRect();
  const minVisible = 50;

  newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
  newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

  cm.popup.style.left = `${newX}px`;
  cm.popup.style.top = `${newY}px`;
}

/**
 * Handle drag end
 * @param {Object} cm - ColumnManager instance
 */
export function handleDragEnd(cm) {
  cm.isDragging = false;
  cm.popup.classList.remove('dragging');

  document.removeEventListener('mousemove', cm.handleDragMove);
  document.removeEventListener('mouseup', cm.handleDragEnd);

  // Save position
  saveSetting(cm, 'x', parseInt(cm.popup.style.left, 10));
  saveSetting(cm, 'y', parseInt(cm.popup.style.top, 10));
}

/**
 * Handle keyboard events
 * @param {Object} cm - ColumnManager instance
 * @param {KeyboardEvent} e - Keyboard event
 */
export function handleKeyDown(cm, e) {
  if (e.key === 'Escape' && cm.popup?.classList.contains('visible')) {
    if (cm.columnTable?.isEditing) {
      // If editing a row, cancel the edit (stay in popup)
      e.preventDefault();
      e.stopPropagation();
      void cm.columnTable.handleCancel();
      void cm.columnTable.syncDirtyState?.();
      return;
    }
    // If not editing, close without applying changes
    e.preventDefault();
    e.stopPropagation();
    cm.close();
  }
}

/**
 * Handle click outside popup - do nothing, must use close/save/cancel buttons
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
export function handleClickOutside(cm, e) {
  // Click outside does nothing - user must explicitly close with buttons or Esc
  return;
}
