/**
 * Splitter functionality for MainManager
 * Handles resizable sidebar with mouse and touch events
 */

import { log, Subsystems, Status } from '../../core/log.js';

// Sidebar constraints
const MIN_SIDEBAR_WIDTH = 220;
const MAX_SIDEBAR_WIDTH = 400;

/**
 * Methods to be mixed into MainManager prototype
 */
export const splitterMethods = {
  /**
   * Set up the resizable splitter
   */
  setupSplitter() {
    if (!this.elements.sidebarSplitter) return;

    // Mouse events
    this.elements.sidebarSplitter.addEventListener('mousedown', this.handleSplitterMouseDown);
    
    // Touch events for mobile
    this.elements.sidebarSplitter.addEventListener('touchstart', this.handleSplitterTouchStart, { passive: false });
  },

  /**
   * Handle splitter mouse down
   */
  handleSplitterMouseDown(event) {
    if (this.isSidebarCollapsed) return;
    
    event.preventDefault();
    this.isResizing = true;
    this.elements.sidebarSplitter.classList.add('resizing');
    document.body.classList.add('resizing');
    
    // Remove width transition so resizing follows mouse immediately
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = 'none';
    }
    
    document.addEventListener('mousemove', this.handleSplitterMouseMove);
    document.addEventListener('mouseup', this.handleSplitterMouseUp);
  },

  /**
   * Handle splitter mouse move
   */
  handleSplitterMouseMove(event) {
    if (!this.isResizing) return;
    
    const newWidth = event.clientX;
    this.setSidebarWidth(newWidth);
  },

  /**
   * Handle splitter mouse up
   */
  handleSplitterMouseUp() {
    this.isResizing = false;
    this.elements.sidebarSplitter?.classList.remove('resizing');
    document.body.classList.remove('resizing');
    
    // Restore width transition for expand/collapse button
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = '';
    }
    
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
    
    // Save the current width
    this.saveSidebarState();
  },

  /**
   * Handle splitter touch start
   */
  handleSplitterTouchStart(event) {
    if (this.isSidebarCollapsed) return;
    
    event.preventDefault();
    this.isResizing = true;
    this.elements.sidebarSplitter.classList.add('resizing');
    document.body.classList.add('resizing');
    
    // Remove width transition so resizing follows touch immediately
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = 'none';
    }
    
    document.addEventListener('touchmove', this.handleSplitterTouchMove, { passive: false });
    document.addEventListener('touchend', this.handleSplitterTouchEnd);
    document.addEventListener('touchcancel', this.handleSplitterTouchEnd);
  },

  /**
   * Handle splitter touch move
   */
  handleSplitterTouchMove(event) {
    if (!this.isResizing) return;
    
    event.preventDefault();
    const touch = event.touches[0];
    const newWidth = touch.clientX;
    this.setSidebarWidth(newWidth);
  },

  /**
   * Handle splitter touch end
   */
  handleSplitterTouchEnd() {
    this.isResizing = false;
    this.elements.sidebarSplitter?.classList.remove('resizing');
    document.body.classList.remove('resizing');
    
    // Restore width transition for expand/collapse button
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = '';
    }
    
    document.removeEventListener('touchmove', this.handleSplitterTouchMove);
    document.removeEventListener('touchend', this.handleSplitterTouchEnd);
    document.removeEventListener('touchcancel', this.handleSplitterTouchEnd);
    
    // Save the current width
    this.saveSidebarState();
  },

  /**
   * Set sidebar width with constraints
   */
  setSidebarWidth(width) {
    // Constrain width to valid range
    const constrainedWidth = Math.max(MIN_SIDEBAR_WIDTH, Math.min(MAX_SIDEBAR_WIDTH, width));
    
    if (this.elements.sidebar) {
      this.elements.sidebar.style.width = `${constrainedWidth}px`;
    }
  },
};
