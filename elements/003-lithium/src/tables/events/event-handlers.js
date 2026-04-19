/**
 * Table Event Handlers
 *
 * Navigator button handlers and filter/group operations.
 *
 * @module tables/events/event-handlers
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { refreshTabulatorSchemas } from '../../shared/lookups.js';
import { saveFiltersVisible } from '../persistence/persistence.js';

/**
 * Handle refresh button click.
 * @param {Object} table - LithiumTable instance
 */
export async function handleRefresh(table) {
  const refreshBtn = table.navigatorContainer?.querySelector(`#${table.cssPrefix}-nav-refresh`);
  if (refreshBtn) {
    refreshBtn.classList.add('lithium-nav-refresh-spin');
    setTimeout(() => refreshBtn.classList.remove('lithium-nav-refresh-spin'), 750);
  }

  if (table.isEditing && table.isDirty) {
    await table.handleSave?.();
  } else if (table.isEditing) {
    await table.exitEditMode?.('cancel');
  }

  // If this table uses Lookup 59 (indicated by lookupKeyIdx), refresh the schemas first
  // This ensures any changes to Lookup 59 are picked up without requiring a page reload
  if (table.lookupKeyIdx != null) {
    try {
      await refreshTabulatorSchemas();
      // After refreshing Lookup 59, reload the table configuration (schema + template)
      await table.reloadConfiguration?.();
      return;
    } catch (err) {
      log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Schema refresh failed: ${err.message}`);
    }
  }

  // Fallback: just refresh data if no Lookup 59
  if (typeof table.onRefresh === 'function') {
    table.onRefresh();
  } else {
    table.loadData?.();
  }
}

/**
 * Handle filter button click - toggle header filters visibility.
 * @param {Object} table - LithiumTable instance
 */
export function handleFilter(table) {
  table.filtersVisible = !table.filtersVisible;

  const filterBtn = table.navigatorContainer?.querySelector(`#${table.cssPrefix}-nav-filter`);
  if (filterBtn) {
    filterBtn.classList.toggle('lithium-nav-btn-active', table.filtersVisible);
  }

  saveFiltersVisible(table, table.filtersVisible);

  const transitionMs = parseInt(getComputedStyle(document.documentElement)
    .getPropertyValue('--transition-duration').trim()) || 330;

  if (table.filtersVisible) {
    toggleHeaderFilters(table, true);

    const filterInputs = table.container.querySelectorAll('.tabulator-header-filter');
    filterInputs.forEach((el) => {
      el.style.height = '0';
      el.style.minHeight = '0';
      el.style.maxHeight = '0';
      el.style.opacity = '0';
    });

    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        filterInputs.forEach((el) => {
          el.style.removeProperty('height');
          el.style.removeProperty('min-height');
          el.style.removeProperty('max-height');
          el.style.removeProperty('opacity');
        });
        table.container.classList.add('lithium-filters-visible');
      });
    });
  } else {
    table.container.classList.remove('lithium-filters-visible');
    setTimeout(() => {
      toggleHeaderFilters(table, false);
      table.table?.clearHeaderFilter();
    }, transitionMs);
  }
}

/**
 * Toggle header filters on/off by updating column definitions.
 * @param {Object} table - LithiumTable instance
 * @param {boolean} visible - Whether header filters should be visible
 */
export function toggleHeaderFilters(table, visible) {
  if (!table.table) return;

  const columns = table.table.getColumns();
  const filterEditorFn = table.createFilterEditorFunction();

  const updatedColumns = columns.map((column) => {
    const definition = column.getDefinition();

    if (definition.field === '_selector') {
      return definition;
    }

    if (visible) {
      return {
        ...definition,
        headerFilter: filterEditorFn,
        headerFilterFunc: 'like',
      };
    } else {
      // eslint-disable-next-line no-unused-vars
      const { headerFilter: _, headerFilterFunc: __, headerFilterParams: ___, ...rest } = definition;
      return rest;
    }
  });

  table.table.setColumns(updatedColumns);

  // Re-enforce selectableRows: 1 after setColumns() - Tabulator may lose this setting
  table.table.modules.select?.setSelectionMode?.(1);

  requestAnimationFrame(() => {
    clearColumnInlineHeights(table);
  });
}

/**
 * Clear inline height styles from column headers.
 * @param {Object} table - LithiumTable instance
 */
export function clearColumnInlineHeights(table) {
  if (!table.container) return;
  const cols = table.container.querySelectorAll('.tabulator-header .tabulator-col');
  cols.forEach((col) => {
    col.style.removeProperty('height');
    col.style.removeProperty('min-height');
  });
}

/**
 * Expand all grouped rows.
 * @param {Object} table - LithiumTable instance
 */
export function expandAll(table) {
  if (!table.table) return;
  // NOTE: keep the prior _groupVisibilityState intact so updateGroupIcons()
  // can diff old→new and animate only the groups whose state actually
  // changed. Clearing it would make all groups "first sight" and skip
  // animation entirely.
  const currentGroupBy = table.table.options.groupBy;
  table.table.setGroupStartOpen(true);
  // Clear and re-apply grouping to trigger the change
  table.table.setGroupBy(false);
  table.table.setGroupBy(currentGroupBy);
  // Update icons - state comparison will animate changed groups
  setTimeout(() => table.updateGroupIcons(), 16);
}

/**
 * Collapse all grouped rows.
 * @param {Object} table - LithiumTable instance
 */
export function collapseAll(table) {
  if (!table.table) return;
  // See note on expandAll() — we intentionally DO NOT clear
  // _groupVisibilityState here.
  const currentGroupBy = table.table.options.groupBy;
  table.table.setGroupStartOpen(false);
  // Clear and re-apply grouping to trigger the change
  table.table.setGroupBy(false);
  table.table.setGroupBy(currentGroupBy);
  // Update icons - state comparison will animate changed groups
  setTimeout(() => table.updateGroupIcons(), 16);
}
