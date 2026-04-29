/**
 * Sidebar collapse/expand animations for MainManager
 * Handles the two-stage animation for collapsing and expanding the sidebar
 */

import { getTip } from '../../core/tooltip-api.js';
import { initTooltips } from '../../core/tooltip-api.js';
import { log, Subsystems, Status } from '../../core/log.js';

// Icon group sizing (matches CSS --icon-w, --icon-h, --icon-gap)
const ICON_W = 40;
const ICON_H = 35;
const ICON_GAP = 2;
const STEP_X = ICON_W + ICON_GAP;   // 42px — horizontal step
const STEP_Y = ICON_H + ICON_GAP;   // 37px — vertical step

/**
 * Methods to be mixed into MainManager prototype
 */
export const collapseMethods = {
  /**
   * Toggle sidebar collapse/expand with two-stage animation.
   *
   * Collapsing
   *   Stage 1 — Icons stack horizontally behind arrow, sidebar narrows,
   *             arrow rotates left → up.
   *   Stage 2 — flex-direction snaps to column, icons fan out vertically,
   *             arrow rotates up → right.
   *
   * Expanding (reverse)
   *   Stage 1 — Icons collapse vertically back to a stack,
   *             arrow rotates right → down.
   *   Stage 2 — flex-direction snaps to row, sidebar widens,
   *             icons fan out horizontally, arrow rotates down → left.
   */
  toggleSidebarCollapse() {
    if (this._isAnimating) return;

    this.isSidebarCollapsed = !this.isSidebarCollapsed;

    const iconGroup = this.elements.sidebarIconGroup;
    const sidebar   = this.elements.sidebar;
    const arrow     = this._getArrowEl();
    if (!iconGroup || !sidebar) return;

    // Update collapse button tooltip text
    const collapseBtn = this.elements.sidebarCollapseBtn;
    if (collapseBtn) {
      const tipInstance = getTip(collapseBtn);
      if (tipInstance) {
        tipInstance.updateContent(this.isSidebarCollapsed ? 'Expand Sidebar' : 'Collapse Sidebar');
      }
    }

    const wrappers    = iconGroup.querySelectorAll('.sidebar-icon-wrapper');
    const halfDur     = this.getTransitionDuration() / 2;
    this._isAnimating = true;

    clearTimeout(this._stageTimer);

    if (this.isSidebarCollapsed) {
      /* ---- COLLAPSING ---- */
      this.expandedWidth = sidebar.offsetWidth;

      // Stage 1: sidebar narrows, icons stack horizontally
      iconGroup.style.transition = 'none';
      this._setWrapperTransitions(wrappers, 'none');
      wrappers.forEach(w => { w.style.flex = `0 0 ${ICON_W}px`; });
      iconGroup.style.width =
        (wrappers.length * ICON_W + (wrappers.length - 1) * ICON_GAP) + 'px';
      void iconGroup.offsetHeight;                       // flush snap

      // Re-enable transitions and animate to collapsed targets
      iconGroup.style.transition = '';
      this._setWrapperTransitions(wrappers, '');

      sidebar.classList.add('collapsed');
      iconGroup.classList.add('collapsed');

      // Clear inline overrides — CSS .collapsed provides identical values
      wrappers.forEach(w => { w.style.flex = ''; });
      iconGroup.style.width = '';

      wrappers.forEach((w, i) => {
        if (i > 0) w.style.transform = `translateX(${-i * STEP_X}px)`;
      });

      // Arrow: +90° (left → up)
      this._arrowRotation += 90;
      if (arrow) arrow.style.transform = `rotate(${this._arrowRotation}deg)`;

      // Stage 2: after stage 1 completes, fan out vertically
      this._stageTimer = setTimeout(() => {
        // Snap: disable transitions, switch to column, set stacked-Y transforms
        this._setWrapperTransitions(wrappers, 'none');
        iconGroup.classList.add('vertical');
        iconGroup.style.height = `${ICON_H}px`;            // lock height
        wrappers.forEach((w, i) => {
          w.style.transform = i > 0 ? `translateY(${-i * STEP_Y}px)` : '';
        });
        void iconGroup.offsetHeight;                         // flush

        // Animate: re-enable transitions, release height, fan out
        this._setWrapperTransitions(wrappers, '');
        iconGroup.style.height = '';                         // CSS → full vertical height
        wrappers.forEach(w => { w.style.transform = ''; }); // natural column positions

        // Arrow: +90° (up → right)
        this._arrowRotation += 90;
        const arrow2 = this._getArrowEl();
        if (arrow2) arrow2.style.transform = `rotate(${this._arrowRotation}deg)`;

        // Mark animation complete after stage 2 finishes
        this._stageTimer = setTimeout(() => {
          this._isAnimating = false;
          // Collapse all groups when sidebar is collapsed
          this._collapseAllGroups();
        }, halfDur);
      }, halfDur);

    } else {
      /* ---- EXPANDING ---- */
      
      // Expand groups containing open managers when sidebar expands
      this._expandGroupsWithOpenManagers();

      // Stage 1: collapse vertically back to stack, full rounding
      iconGroup.classList.add('stacking');
      iconGroup.style.height = `${ICON_H}px`;              // shrink container
      wrappers.forEach((w, i) => {
        if (i > 0) w.style.transform = `translateY(${-i * STEP_Y}px)`;
      });

      // Arrow: +90° (right → down)
      this._arrowRotation += 90;
      if (arrow) arrow.style.transform = `rotate(${this._arrowRotation}deg)`;

      // Stage 2: after vertical collapse, fan out horizontally
      this._stageTimer = setTimeout(() => {
        // Snap: disable transitions, switch to row, set stacked-X transforms
        this._setWrapperTransitions(wrappers, 'none');
        iconGroup.classList.remove('vertical');
        iconGroup.classList.remove('stacking');
        iconGroup.style.height = '';                         // back to CSS row height
        wrappers.forEach((w, i) => {
          w.style.transform = i > 0 ? `translateX(${-i * STEP_X}px)` : '';
        });
        void iconGroup.offsetHeight;                         // flush

        // Animate: re-enable transitions, expand sidebar + icons
        this._setWrapperTransitions(wrappers, '');
        sidebar.classList.remove('collapsed');
        iconGroup.classList.remove('collapsed');
        wrappers.forEach(w => { w.style.transform = ''; }); // natural row positions

        if (this.expandedWidth) {
          this.setSidebarWidth(this.expandedWidth);
        }

        // Arrow: +90° (down → left)
        this._arrowRotation += 90;
        const arrow2 = this._getArrowEl();
        if (arrow2) arrow2.style.transform = `rotate(${this._arrowRotation}deg)`;

        // Mark animation complete after stage 2 finishes
        this._stageTimer = setTimeout(() => {
          this._isAnimating = false;
          // Scroll the active manager into view after expansion completes
          this._scrollActiveManagerIntoView();
        }, halfDur);
      }, halfDur);
    }

    this.saveSidebarState();
  },

  /**
   * Get transition duration from CSS variable
   */
  getTransitionDuration() {
    const duration = getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration')
      .trim();
    // Parse the value - could be in ms or s
    if (duration.includes('ms')) {
      return parseInt(duration, 10);
    } else if (duration.includes('s')) {
      return parseFloat(duration) * 1000;
    }
    // Default fallback
    return 2500;
  },

  /**
   * Apply changes without CSS transitions (for instant state setup on load).
   * @param {Function} fn - Function to execute with transitions disabled
   */
  applyWithoutTransition(fn) {
    const sidebar   = this.elements.sidebar;
    const iconGroup = this.elements.sidebarIconGroup;

    const els = [
      sidebar,
      iconGroup,
      ...(iconGroup?.querySelectorAll('.sidebar-icon-wrapper, .sidebar-icon-btn') || [])
    ].filter(Boolean);

    // Also include the collapse icon for arrow rotation
    const arrowIcon = this._getArrowEl();
    if (arrowIcon) els.push(arrowIcon);

    els.forEach(el => { el.style.transition = 'none'; });
    fn();
    void sidebar?.offsetHeight;
    requestAnimationFrame(() => {
      els.forEach(el => { el.style.transition = ''; });
    });
  },

  /**
   * Set the transition property on all icon wrappers.
   * @param {NodeList} wrappers - The wrapper elements
   * @param {string} value - CSS transition value ('' to restore, 'none' to disable)
   */
  _setWrapperTransitions(wrappers, value) {
    wrappers.forEach(w => { w.style.transition = value; });
  },

  /**
   * Get the collapse arrow icon element via a fresh DOM query.
   * Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
   * @returns {Element|null}
   */
  _getArrowEl() {
    return document.querySelector('#collapse-icon') ||
           document.querySelector('#sidebar-collapse-btn')?.firstElementChild;
  },

  /**
   * Handle window resize — reset icon group state on mobile/desktop transitions
   */
  handleWindowResize() {
    if (window.innerWidth <= 768) {
      // On mobile, reset to expanded state for the slide-out overlay
      if (this.elements.sidebar) {
        this.elements.sidebar.classList.remove('collapsed');
      }
      if (this.elements.sidebarIconGroup) {
        this.elements.sidebarIconGroup.classList.remove('collapsed');
        this.elements.sidebarIconGroup.classList.remove('vertical');
        this.elements.sidebarIconGroup.style.height = '';
        this.elements.sidebarIconGroup.querySelectorAll('.sidebar-icon-wrapper').forEach(w => {
          w.style.transform = '';
        });
      }
      // Reset arrow rotation
      const arrowMobile = this._getArrowEl();
      if (arrowMobile) {
        arrowMobile.style.transform = '';
      }
    } else {
      // On desktop, restore collapsed state if needed
      if (this.isSidebarCollapsed) {
        if (this.elements.sidebar) {
          this.elements.sidebar.classList.add('collapsed');
        }
        if (this.elements.sidebarIconGroup) {
          this.elements.sidebarIconGroup.classList.add('collapsed');
          this.elements.sidebarIconGroup.classList.add('vertical');
        }
        this._arrowRotation = 180;
        const arrowDesktop = this._getArrowEl();
        if (arrowDesktop) {
          arrowDesktop.style.transform = 'rotate(180deg)';
        }
      }
    }
  },
};
