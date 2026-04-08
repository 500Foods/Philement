/**
 * Crimson Events Module
 *
 * Adds drag and resize event handling to CrimsonManager:
 * - Main popup drag (header)
 * - Main popup resize (4 corners)
 * - Citation popup drag and resize
 * - Debug/reasoning panel resize (splitters)
 * - Send button and input handling
 *
 * @module managers/crimson/crimson-events
 */

import { CrimsonManager } from './crimson-core.js';

// ==================== MAIN POPUP DRAG ====================

/**
 * Handle drag start on header
 */
CrimsonManager.prototype.handleDragStart = function(e) {
  // Allow dragging from the title/status area, but not from actionable header controls
  if (e.target.closest([
    '.crimson-header-close',
    '.crimson-reset-btn',
    '.crimson-debug-btn',
    '.crimson-streaming-btn',
    '.crimson-reasoning-btn',
    '.crimson-citation-header-btn',
  ].join(', '))) return;

  this.isDragging = true;
  this.popup.classList.add('dragging');

  this.dragStartX = e.clientX;
  this.dragStartY = e.clientY;

  const rect = this.popup.getBoundingClientRect();
  this.popupStartX = rect.left;
  this.popupStartY = rect.top;

  document.addEventListener('mousemove', this.handleDragMove);
  document.addEventListener('mouseup', this.handleDragEnd);

  e.preventDefault();
};

/**
 * Handle drag move
 */
CrimsonManager.prototype.handleDragMove = function(e) {
  if (!this.isDragging) return;

  const deltaX = e.clientX - this.dragStartX;
  const deltaY = e.clientY - this.dragStartY;

  let newX = this.popupStartX + deltaX;
  let newY = this.popupStartY + deltaY;

  const rect = this.popup.getBoundingClientRect();
  const minVisible = 50;

  newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
  newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

  this.popup.style.left = `${newX}px`;
  this.popup.style.top = `${newY}px`;
};

/**
 * Handle drag end
 */
CrimsonManager.prototype.handleDragEnd = function() {
  this.isDragging = false;
  this.popup.classList.remove('dragging');
  this.saveMainWindowState();

  document.removeEventListener('mousemove', this.handleDragMove);
  document.removeEventListener('mouseup', this.handleDragEnd);
};

// ==================== MAIN POPUP RESIZE ====================

/**
 * Handle resize start
 * @param {MouseEvent} e - Mouse event
 * @param {string} corner - Which corner: 'br', 'bl', 'tr', 'tl'
 */
CrimsonManager.prototype.handleResizeStart = function(e, corner = 'br') {
  this.isResizing = true;
  this.resizeCorner = corner;
  this.popup.classList.add('resizing');

  this.resizeStartX = e.clientX;
  this.resizeStartY = e.clientY;

  const rect = this.popup.getBoundingClientRect();
  this.popupStartWidth = rect.width;
  this.popupStartHeight = rect.height;
  this.popupStartLeft = rect.left;
  this.popupStartTop = rect.top;

  document.addEventListener('mousemove', this.handleResizeMove);
  document.addEventListener('mouseup', this.handleResizeEnd);

  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle resize move
 */
CrimsonManager.prototype.handleResizeMove = function(e) {
  if (!this.isResizing) return;

  const deltaX = e.clientX - this.resizeStartX;
  const deltaY = e.clientY - this.resizeStartY;

  let newWidth = this.popupStartWidth;
  let newHeight = this.popupStartHeight;
  let newLeft = this.popupStartLeft;
  let newTop = this.popupStartTop;

  switch (this.resizeCorner) {
    case 'br':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight + deltaY));
      break;
    case 'bl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight + deltaY));
      if (newWidth !== this.popupStartWidth - deltaX) {
        const actualDeltaX = this.popupStartWidth - newWidth;
        newLeft = this.popupStartLeft + actualDeltaX;
      } else {
        newLeft = this.popupStartLeft + deltaX;
      }
      break;
    case 'tr':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight - deltaY));
      if (newHeight !== this.popupStartHeight - deltaY) {
        const actualDeltaY = this.popupStartHeight - newHeight;
        newTop = this.popupStartTop + actualDeltaY;
      } else {
        newTop = this.popupStartTop + deltaY;
      }
      break;
    case 'tl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight - deltaY));
      if (newWidth !== this.popupStartWidth - deltaX) {
        const actualDeltaX = this.popupStartWidth - newWidth;
        newLeft = this.popupStartLeft + actualDeltaX;
      } else {
        newLeft = this.popupStartLeft + deltaX;
      }
      if (newHeight !== this.popupStartHeight - deltaY) {
        const actualDeltaY = this.popupStartHeight - newHeight;
        newTop = this.popupStartTop + actualDeltaY;
      } else {
        newTop = this.popupStartTop + deltaY;
      }
      break;
  }

  this.popup.style.width = `${newWidth}px`;
  this.popup.style.height = `${newHeight}px`;

  if (this.resizeCorner === 'bl' || this.resizeCorner === 'tl') {
    this.popup.style.left = `${newLeft}px`;
  }
  if (this.resizeCorner === 'tr' || this.resizeCorner === 'tl') {
    this.popup.style.top = `${newTop}px`;
  }
};

/**
 * Handle resize end
 */
CrimsonManager.prototype.handleResizeEnd = function() {
  this.isResizing = false;
  this.popup.classList.remove('resizing');
  this.saveMainWindowState();

  document.removeEventListener('mousemove', this.handleResizeMove);
  document.removeEventListener('mouseup', this.handleResizeEnd);
};

// ==================== CITATION POPUP DRAG & RESIZE ====================

/**
 * Handle citation popup drag start
 */
CrimsonManager.prototype.handleCitationDragStart = function(e) {
  if (e.target.closest('.crimson-citation-popup-close')) return;

  this.isCitationDragging = true;
  this.activeCitationPopup.classList.add('dragging');

  this.citationDragStartX = e.clientX;
  this.citationDragStartY = e.clientY;

  const rect = this.activeCitationPopup.getBoundingClientRect();
  this.citationPopupStartX = rect.left;
  this.citationPopupStartY = rect.top;

  document.addEventListener('mousemove', this.handleCitationDragMove);
  document.addEventListener('mouseup', this.handleCitationDragEnd);

  e.preventDefault();
};

/**
 * Handle citation popup drag move
 */
CrimsonManager.prototype.handleCitationDragMove = function(e) {
  if (!this.isCitationDragging || !this.activeCitationPopup) return;

  const deltaX = e.clientX - this.citationDragStartX;
  const deltaY = e.clientY - this.citationDragStartY;

  let newX = this.citationPopupStartX + deltaX;
  let newY = this.citationPopupStartY + deltaY;

  const rect = this.activeCitationPopup.getBoundingClientRect();
  const minVisible = 50;

  newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
  newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

  this.activeCitationPopup.style.left = `${newX}px`;
  this.activeCitationPopup.style.top = `${newY}px`;
};

/**
 * Handle citation popup drag end
 */
CrimsonManager.prototype.handleCitationDragEnd = function() {
  this.isCitationDragging = false;
  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.remove('dragging');
    this.saveCitationPopupState(this.activeCitationPopup);
  }

  document.removeEventListener('mousemove', this.handleCitationDragMove);
  document.removeEventListener('mouseup', this.handleCitationDragEnd);
};

/**
 * Handle citation popup resize start
 * @param {MouseEvent} e - Mouse event
 * @param {string} corner - Which corner: 'br', 'bl', 'tr', 'tl'
 */
CrimsonManager.prototype.handleCitationResizeStart = function(e, corner = 'br') {
  this.isCitationResizing = true;
  this.citationResizeCorner = corner;
  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.add('resizing');
  }

  this.citationResizeStartX = e.clientX;
  this.citationResizeStartY = e.clientY;

  const rect = this.activeCitationPopup.getBoundingClientRect();
  this.citationPopupStartWidth = rect.width;
  this.citationPopupStartHeight = rect.height;
  this.citationPopupStartLeft = rect.left;
  this.citationPopupStartTop = rect.top;

  document.addEventListener('mousemove', this.handleCitationResizeMove);
  document.addEventListener('mouseup', this.handleCitationResizeEnd);

  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle citation popup resize move
 */
CrimsonManager.prototype.handleCitationResizeMove = function(e) {
  if (!this.isCitationResizing || !this.activeCitationPopup) return;

  const deltaX = e.clientX - this.citationResizeStartX;
  const deltaY = e.clientY - this.citationResizeStartY;

  let newWidth = this.citationPopupStartWidth;
  let newHeight = this.citationPopupStartHeight;
  let newLeft = this.citationPopupStartLeft;
  let newTop = this.citationPopupStartTop;

  switch (this.citationResizeCorner) {
    case 'br':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight + deltaY));
      break;
    case 'bl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight + deltaY));
      if (newWidth !== this.citationPopupStartWidth - deltaX) {
        const actualDeltaX = this.citationPopupStartWidth - newWidth;
        newLeft = this.citationPopupStartLeft + actualDeltaX;
      } else {
        newLeft = this.citationPopupStartLeft + deltaX;
      }
      break;
    case 'tr':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight - deltaY));
      if (newHeight !== this.citationPopupStartHeight - deltaY) {
        const actualDeltaY = this.citationPopupStartHeight - newHeight;
        newTop = this.citationPopupStartTop + actualDeltaY;
      } else {
        newTop = this.citationPopupStartTop + deltaY;
      }
      break;
    case 'tl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight - deltaY));
      if (newWidth !== this.citationPopupStartWidth - deltaX) {
        const actualDeltaX = this.citationPopupStartWidth - newWidth;
        newLeft = this.citationPopupStartLeft + actualDeltaX;
      } else {
        newLeft = this.citationPopupStartLeft + deltaX;
      }
      if (newHeight !== this.citationPopupStartHeight - deltaY) {
        const actualDeltaY = this.citationPopupStartHeight - newHeight;
        newTop = this.citationPopupStartTop + actualDeltaY;
      } else {
        newTop = this.citationPopupStartTop + deltaY;
      }
      break;
  }

  this.activeCitationPopup.style.width = `${newWidth}px`;
  this.activeCitationPopup.style.height = `${newHeight}px`;

  if (this.citationResizeCorner === 'bl' || this.citationResizeCorner === 'tl') {
    this.activeCitationPopup.style.left = `${newLeft}px`;
  }
  if (this.citationResizeCorner === 'tr' || this.citationResizeCorner === 'tl') {
    this.activeCitationPopup.style.top = `${newTop}px`;
  }
};

/**
 * Handle citation popup resize end
 */
CrimsonManager.prototype.handleCitationResizeEnd = function() {
  this.isCitationResizing = false;
  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.remove('resizing');
    this.saveCitationPopupState(this.activeCitationPopup);
  }

  document.removeEventListener('mousemove', this.handleCitationResizeMove);
  document.removeEventListener('mouseup', this.handleCitationResizeEnd);
};

// ==================== DEBUG PANEL RESIZE ====================

/**
 * Handle debug panel resize start
 */
CrimsonManager.prototype.handleDebugResizeStart = function(e) {
  this.isResizingDebug = true;
  this.debugResizeStartY = e.clientY;
  this.debugStartHeight = this.debugPanel?.offsetHeight || 150;

  this.debugPanel?.classList.add('no-transition');

  document.addEventListener('mousemove', this.handleDebugResizeMove);
  document.addEventListener('mouseup', this.handleDebugResizeEnd);
  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle debug panel resize move
 */
CrimsonManager.prototype.handleDebugResizeMove = function(e) {
  if (!this.isResizingDebug || !this.debugPanel) return;

  const deltaY = e.clientY - this.debugResizeStartY;
  const newHeight = Math.max(50, Math.min(400, this.debugStartHeight - deltaY));
  this.debugPanel.style.flex = `0 0 ${newHeight}px`;
};

/**
 * Handle debug panel resize end
 */
CrimsonManager.prototype.handleDebugResizeEnd = function() {
  this.isResizingDebug = false;
  this.debugPanel?.classList.remove('no-transition');
  document.removeEventListener('mousemove', this.handleDebugResizeMove);
  document.removeEventListener('mouseup', this.handleDebugResizeEnd);
};

// ==================== REASONING PANEL RESIZE ====================

/**
 * Handle reasoning panel resize start
 */
CrimsonManager.prototype.handleReasoningResizeStart = function(e) {
  this.isResizingReasoning = true;
  this.reasoningResizeStartY = e.clientY;
  this.reasoningStartHeight = this.reasoningPanel?.offsetHeight || 150;

  this.reasoningPanel?.classList.add('no-transition');

  document.addEventListener('mousemove', this.handleReasoningResizeMove);
  document.addEventListener('mouseup', this.handleReasoningResizeEnd);
  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle reasoning panel resize move
 */
CrimsonManager.prototype.handleReasoningResizeMove = function(e) {
  if (!this.isResizingReasoning || !this.reasoningPanel) return;

  const deltaY = e.clientY - this.reasoningResizeStartY;
  const newHeight = Math.max(50, Math.min(400, this.reasoningStartHeight + deltaY));
  this.reasoningPanel.style.flex = `0 0 ${newHeight}px`;
};

/**
 * Handle reasoning panel resize end
 */
CrimsonManager.prototype.handleReasoningResizeEnd = function() {
  this.isResizingReasoning = false;
  this.reasoningPanel?.classList.remove('no-transition');
  document.removeEventListener('mousemove', this.handleReasoningResizeMove);
  document.removeEventListener('mouseup', this.handleReasoningResizeEnd);
};

// ==================== SEND & INPUT HANDLING ====================

/**
 * Handle send button click
 */
CrimsonManager.prototype.handleSend = function() {
  const message = this.input.value.trim();
  if (!message || this.isStreaming) return;

  this.addMessage('user', message);
  this.addInputToHistory(message);

  this.input.value = '';
  this.autoResizeInput();

  this.sendChatMessage(message);
};

/**
 * Handle input keydown
 */
CrimsonManager.prototype.handleInputKeydown = function(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    this.handleSend();
    return;
  }

  if (e.key === 'ArrowUp' && (this.input.selectionStart === 0 || this.input.value === '')) {
    e.preventDefault();
    this.navigateInputHistory(-1);
    return;
  }

  if (e.key === 'ArrowDown' && (this.input.selectionStart === this.input.value.length || this.input.value === '')) {
    e.preventDefault();
    this.navigateInputHistory(1);
    return;
  }
};

/**
 * Auto-resize input based on content
 */
CrimsonManager.prototype.autoResizeInput = function() {
  this.input.style.height = 'auto';
  const newHeight = Math.min(150, Math.max(35, this.input.scrollHeight));
  this.input.style.height = `${newHeight}px`;
};
