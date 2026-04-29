/**
 * State management for MainManager
 * Handles saving/loading sidebar state and user info
 */

import { getClaims } from '../../core/jwt.js';
import { log, Subsystems, Status } from '../../core/log.js';

// localStorage keys for sidebar state
const SIDEBAR_WIDTH_KEY = 'lithium_sidebar_width';
const SIDEBAR_COLLAPSED_KEY = 'lithium_sidebar_collapsed';
const LAST_MANAGER_KEY = 'lithium_last_manager';

/**
 * Methods to be mixed into MainManager prototype
 */
export const stateMethods = {
  /**
   * Save sidebar state to localStorage
   */
  saveSidebarState() {
    try {
      localStorage.setItem(SIDEBAR_COLLAPSED_KEY, JSON.stringify(this.isSidebarCollapsed));
      
      if (!this.isSidebarCollapsed && this.elements.sidebar) {
        const currentWidth = this.elements.sidebar.offsetWidth;
        localStorage.setItem(SIDEBAR_WIDTH_KEY, String(currentWidth));
      }
    } catch (error) {
      console.warn('[MainManager] Failed to save sidebar state:', error);
    }
  },

  /**
   * Load sidebar state from localStorage.
   * Applies the final collapsed state (including .vertical) without animation.
   */
  loadSidebarState() {
    try {
      // Load collapsed state
      const collapsedStored = localStorage.getItem(SIDEBAR_COLLAPSED_KEY);
      if (collapsedStored) {
        this.isSidebarCollapsed = JSON.parse(collapsedStored);
      }

      // Load width
      const widthStored = localStorage.getItem(SIDEBAR_WIDTH_KEY);
      if (widthStored) {
        this.expandedWidth = parseInt(widthStored, 10);
      }

      // Apply state (skip animation on initial load)
      if (this.elements.sidebar) {
        if (this.isSidebarCollapsed) {
          this.applyWithoutTransition(() => {
            this.elements.sidebar.classList.add('collapsed');
            this.elements.sidebarIconGroup?.classList.add('collapsed');
            this.elements.sidebarIconGroup?.classList.add('vertical');
            // Arrow at final collapsed position (pointing right = 180°)
            this._arrowRotation = 180;
            const arrowEl = this._getArrowEl();
            if (arrowEl) {
              arrowEl.style.transform = 'rotate(180deg)';
            }
          });
          // Collapse all groups in the menu
          this._collapseAllGroups();
        } else if (this.expandedWidth) {
          this.setSidebarWidth(this.expandedWidth);
        }
      }
    } catch (error) {
      console.warn('[MainManager] Failed to load sidebar state:', error);
    }
  },

  /**
   * Load user info from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (claims) {
      this.user = {
        id: claims.user_id,
        username: claims.username || 'User',
        email: claims.email,
        roles: claims.roles || [],
      };
    }
  },
};
