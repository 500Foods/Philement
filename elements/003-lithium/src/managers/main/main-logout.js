/**
 * Logout panel for MainManager
 * Handles showing/hiding the logout panel with keyboard navigation
 */

import { eventBus } from '../../core/event-bus.js';
import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Methods to be mixed into MainManager prototype
 */
export const logoutMethods = {
  /**
   * Show the logout panel with overlay
   */
  showLogoutPanel() {
    if (this.isLogoutPanelVisible) return;

    this.isLogoutPanelVisible = true;
    this.selectedLogoutIndex = 0;

    // Show overlay and panel by adding visible class
    // CSS handles the opacity/visibility transition
    this.elements.logoutOverlay?.classList.add('visible');
    this.elements.logoutPanel?.classList.add('visible');

    // Add keyboard listener
    document.addEventListener('keydown', this.handleKeyDown);

    // Focus and select the first logout option for accessibility after transition
    setTimeout(() => {
      this._updateLogoutSelection();
    }, 300);
  },

  /**
   * Hide the logout panel
   */
  hideLogoutPanel() {
    if (!this.isLogoutPanelVisible) return;

    this.isLogoutPanelVisible = false;

    // Hide overlay and panel by removing visible class
    // CSS handles the opacity/visibility transition
    this.elements.logoutOverlay?.classList.remove('visible');
    this.elements.logoutPanel?.classList.remove('visible');

    // Remove keyboard listener
    document.removeEventListener('keydown', this.handleKeyDown);

    // Return focus to logout button after transition completes
    setTimeout(() => {
      this.elements.sidebarLogoutBtn?.focus();
    }, 300);
  },
};
