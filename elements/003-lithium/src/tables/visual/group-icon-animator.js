/**
 * Group Icon Animator Module
 *
 * Handles expand/collapse arrow animation for Tabulator group headers.
 * Uses prior-state pinning pattern because Tabulator regenerates group
 * header DOM on every toggle, so CSS-only transitions don't work.
 *
 * @module tables/visual/group-icon-animator
 */

import { processIcons } from '../../core/icons.js';

/**
 * GroupIconAnimator class for managing group toggle icon animations.
 * Instantiated per LithiumTable instance.
 */
export class GroupIconAnimator {
  /**
   * Create a GroupIconAnimator
   * @param {Object} table - The LithiumTable instance
   */
  constructor(table) {
    this.table = table;
    // Map<string, boolean> — group path key → last-known visible state
    this._groupVisibilityState = new Map();
    // Set<HTMLElement> — group rows currently running animation
    this._groupAnimating = new Set();
  }

  /**
   * Build a stable key for a Tabulator group component based on its parent
   * chain. Nested groups are separated with "||" so sibling keys can't
   * collide with compound parent keys.
   */
  _groupPathKey(groupComponent) {
    const parts = [];
    let g = groupComponent;
    while (g && typeof g.getKey === 'function') {
      parts.unshift(String(g.getKey()));
      g = typeof g.getParentGroup === 'function' ? g.getParentGroup() : null;
    }
    return parts.join('||');
  }

  /**
   * Recursively collect all group components (including nested sub-groups).
   */
  _collectAllGroups(groupComponents, out = []) {
    for (const gc of groupComponents || []) {
      out.push(gc);
      if (typeof gc.getSubGroups === 'function') {
        const subs = gc.getSubGroups();
        if (subs && subs.length) this._collectAllGroups(subs, out);
      }
    }
    return out;
  }

  /**
   * Synchronise group toggle icons with their current visibility state and
   * animate any groups whose state changed since the last sync.
   *
   * Safe to call repeatedly — unchanged groups are no-ops.
   */
  updateGroupIcons() {
    if (!this.table.table || !this.table.container) return;

    // Defer one frame so Tabulator has finished regenerating the group DOM.
    requestAnimationFrame(() => this._syncGroupIconsNow());
  }

  _syncGroupIconsNow() {
    if (!this.table.table || !this.table.container) return;

    let groupComponents;
    try {
      groupComponents = this.table.table.getGroups?.();
    } catch {
      groupComponents = null;
    }
    if (!groupComponents) return;

    const allGroups = this._collectAllGroups(groupComponents);
    const priorState = this._groupVisibilityState;
    const nextState = new Map();
    const toAnimate = []; // { rowEl, fromVisible }

    for (const gc of allGroups) {
      const key = this._groupPathKey(gc);
      if (!key) continue;

      const rowEl = typeof gc.getElement === 'function' ? gc.getElement() : null;
      if (!rowEl || !this.table.container.contains(rowEl)) continue;

      // Ensure any freshly-inserted <fa> tags in this group header have
      // been converted to <i>. Safe to call repeatedly.
      processIcons(rowEl);

      const isVisible = rowEl.classList.contains('tabulator-group-visible');
      nextState.set(key, isVisible);

      // CRITICAL: if this row is mid-animation, leave it completely alone.
      // Tabulator fires several events for one toggle, and each triggers a sync.
      if (this._groupAnimating.has(rowEl)) continue;

      const hadPrior = priorState.has(key);
      const wasVisible = priorState.get(key);

      // Clean up any leftover anim classes on non-animating rows
      rowEl.classList.remove(
        'li-group-anim-from-collapsed',
        'li-group-anim-from-expanded',
      );

      if (!hadPrior || wasVisible === isVisible) continue;

      toAnimate.push({ rowEl, fromVisible: wasVisible });
    }

    // Commit new state immediately.
    this._groupVisibilityState = nextState;

    if (toAnimate.length === 0) return;

    // Step 1: apply "starting position" class and mark each row as animating
    for (const { rowEl, fromVisible } of toAnimate) {
      this._groupAnimating.add(rowEl);
      rowEl.classList.add(
        fromVisible
          ? 'li-group-anim-from-expanded'
          : 'li-group-anim-from-collapsed',
      );
    }

    // Force a paint of the starting position
    void this.table.container.offsetHeight;

    // Step 2: on the next frame, remove the class for transition to new state
    requestAnimationFrame(() => {
      for (const { rowEl } of toAnimate) {
        rowEl.classList.remove(
          'li-group-anim-from-collapsed',
          'li-group-anim-from-expanded',
        );
        this._groupAnimating.delete(rowEl);
      }
    });
  }

  /**
   * Reset the animation state. Called when table data changes completely.
   */
  reset() {
    this._groupVisibilityState.clear();
    this._groupAnimating.clear();
  }
}

export default GroupIconAnimator;
