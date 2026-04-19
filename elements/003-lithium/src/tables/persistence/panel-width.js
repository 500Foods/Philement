/**
 * Panel Width Persistence Module
 *
 * Panel width and collapsed state management.
 *
 * @module tables/persistence/panel-width
 */

/**
 * Get the width in pixels for a named mode.
 * @param {string} mode - 'narrow', 'compact', 'normal', 'wide', or 'auto'
 * @returns {number|null} Width in pixels, or null for 'auto'
 */
export function getWidthForMode(mode) {
  const widths = { narrow: 160, compact: 314, normal: 468, wide: 622 };
  return widths[mode] || null;
}

/**
 * Apply width to the panel element based on mode.
 * Called during init (restore) and when user picks a width preset.
 * @param {Object} table - LithiumTable instance
 * @param {string|null} mode - Width mode or null for pixel fallback
 */
export function applyPanelWidth(table, mode) {
  if (!table.panel) {
    console.warn('[LithiumTable] applyPanelWidth: no panel!', { storageKey: table.storageKey });
    return;
  }

  if (mode === null) {
    // No saved mode — use PanelStateManager pixel width as fallback
    if (table.panelStateManager) {
      const width = table.panelStateManager.loadWidth(280);
      table.panel.style.width = `${width}px`;
    }
  } else if (mode === 'auto') {
    table.panel.style.width = '';
    const currentWidth = table.panel.offsetWidth;
    if (table.panelStateManager) {
      table.panelStateManager.saveWidth(currentWidth);
    }
  } else {
    // Named mode
    const widthPx = getWidthForMode(mode);
    if (widthPx) {
      table.panel.style.width = `${widthPx}px`;
      if (table.panelStateManager) {
        table.panelStateManager.saveWidth(widthPx);
      }
    }
  }
}

/**
 * Save pixel width from splitter drag.
 * Called by the manager's onResizeEnd callback.
 * @param {Object} table - LithiumTable instance
 * @param {number} width
 */
export function savePanelPixelWidth(table, width) {
  if (table.panelStateManager) {
    table.panelStateManager.saveWidth(width);
  }
}

/**
 * Load collapsed state from persistence.
 * @param {Object} table - LithiumTable instance
 * @param {boolean} defaultValue
 * @returns {boolean}
 */
export function loadCollapsedState(table, defaultValue = false) {
  if (table.panelStateManager) {
    return table.panelStateManager.loadCollapsed(defaultValue);
  }
  return defaultValue;
}

/**
 * Save collapsed state to persistence.
 * @param {Object} table - LithiumTable instance
 * @param {boolean} collapsed
 */
export function saveCollapsedState(table, collapsed) {
  if (table.panelStateManager) {
    table.panelStateManager.saveCollapsed(collapsed);
  }
}

export default {
  getWidthForMode,
  applyPanelWidth,
  savePanelPixelWidth,
  loadCollapsedState,
  saveCollapsedState,
};
