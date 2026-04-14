/**
 * LithiumTable UI Module (Refactored)
 *
 * Navigator, popups, column chooser, and visual updates.
 * Delegates to feature-specific modules.
 *
 * @module tables/lithium-table-ui
 */

import { log, Subsystems, Status } from '../core/log.js';
import { LithiumColumnManager } from './lithium-column-manager.js';
import { toast } from '../shared/toast.js';
import {
  buildColumnDefinitionsFromTemplateState,
  extractTemplateColumnFromColumn,
  createTemplateColumnFromTableDef,
} from './lithium-table-template.js';

// Import modular UI components
import {
  buildNavigator,
  updateEditButtonState,
  updateDuplicateButtonState,
  updateMoveButtonState,
} from './navigator/navigator-builder.js';

import {
  toggleNavPopup,
  closeNavPopup,
  closeTransientPopups,
  createPopupSeparator,
} from './popups/popup-manager.js';

import {
  buildTemplatePopup,
  getSavedTemplates,
  saveStoredTemplates,
  getTemplateName,
} from './popups/template-popup.js';

import {
  setTableWidth,
  restoreTableWidth,
  setTableLayout,
  restoreLayoutMode,
} from './settings/table-settings.js';

import { createFilterEditorFunction } from './filters/filter-editor.js';

import {
  updateSelectorCell,
  updateVisibleColumnClasses,
  getVisibleColumnBoundaries,
  applyVisibleColumnClassesToRow,
  applyVisibleColumnClassesToRowElement,
} from './visual/visual-updates.js';

import {
  showLoading,
  hideLoading,
} from './visual/loading-indicator.js';

import {
  saveSelectedRowId,
  restoreSelectedRowId,
  clearSavedRowSelection,
  saveFiltersVisible,
  restoreFiltersVisible,
} from './persistence/persistence.js';

import { refreshTabulatorSchemas } from '../shared/lookups.js';

// ── Mixin for UI Operations ─────────────────────────────────────────────────

export const LithiumTableUIMixin = {
  // ── Navigator (delegated to navigator-builder.js) ─────────────────────────

  buildNavigator() {
    buildNavigator(this);
  },

  updateEditButtonState() {
    if (this._inSelectionTransition) return;
    updateEditButtonState(this);
  },

  updateDuplicateButtonState() {
    if (this._inSelectionTransition) {
      return;
    }
    updateDuplicateButtonState(this);
  },

  updateMoveButtonState() {
    if (this._inSelectionTransition) {
      return;
    }
    updateMoveButtonState(this);
  },

  // ── Popup Menus (delegated to popups/) ────────────────────────────────────

  toggleNavPopup(e, popupId) {
    if (popupId === 'template') {
      e.stopPropagation();

      if (this.activeNavPopup && this.activeNavPopupId === 'template') {
        closeNavPopup(this);
        return;
      }

      closeNavPopup(this);

      const btn = e.currentTarget;
      const popup = buildTemplatePopup(this);

      if (!popup) return;

      // Use popup-manager's show logic inline for template popup
      popup.style.position = 'fixed';
      popup.style.top = '0px';
      popup.style.left = '0px';
      popup.style.bottom = 'auto';
      popup.classList.add('visible');

      const hostPopup = btn?.closest?.('.col-manager-popup');
      if (hostPopup) {
        const hostZIndex = parseInt(window.getComputedStyle(hostPopup).zIndex, 10);
        if (Number.isFinite(hostZIndex)) {
          popup.style.zIndex = String(hostZIndex + 10);
        }
      }

      document.body.appendChild(popup);

      const viewportPadding = 8;
      const gap = 4;
      const btnRect = btn.getBoundingClientRect();
      const popupRect = popup.getBoundingClientRect();
      const popupWidth = popupRect.width || popup.offsetWidth || 0;
      const popupHeight = popupRect.height || popup.offsetHeight || 0;
      const availableAbove = btnRect.top - viewportPadding;
      const availableBelow = window.innerHeight - btnRect.bottom - viewportPadding;

      let top;
      if (popupHeight <= availableAbove || availableAbove >= availableBelow) {
        top = btnRect.top - popupHeight - gap;
      } else {
        top = btnRect.bottom + gap;
      }

      let left = btnRect.left;
      if (left + popupWidth > window.innerWidth - viewportPadding) {
        left = window.innerWidth - popupWidth - viewportPadding;
      }
      if (left < viewportPadding) {
        left = viewportPadding;
      }

      const maxTop = Math.max(viewportPadding, window.innerHeight - popupHeight - viewportPadding);
      popup.style.top = `${Math.min(Math.max(top, viewportPadding), maxTop)}px`;
      popup.style.left = `${left}px`;
      popup.style.right = 'auto';

      requestAnimationFrame(() => {
        popup.getBoundingClientRect();
        const newTop = Math.min(Math.max(parseFloat(popup.style.top), viewportPadding), maxTop);
        popup.style.top = `${newTop}px`;
      });

      this.activeNavPopup = popup;
      this.activeNavPopupId = 'template';
      this.activeNavPopupButton = btn;

      this.navPopupCloseHandler = (evt) => {
        if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
          closeNavPopup(this);
        }
      };
      document.addEventListener('click', this.navPopupCloseHandler);

      return;
    }

    toggleNavPopup(this, e, popupId);
  },

  buildStandardNavPopup(_items) {
    // Delegated to popup-manager
    return null;
  },

  showNavPopup(_btn, _popup, _popupId) {
    // Delegated to popup-manager via toggleNavPopup
  },

  positionNavPopup(_btn, _popup) {
    // Delegated to popup-manager
  },

  buildTemplatePopup() {
    return buildTemplatePopup(this);
  },

  createTemplateMenuAction(_label, _action, _options = {}) {
    // Delegated to template-popup
  },

  createPopupSeparator() {
    return createPopupSeparator();
  },

  refreshTemplatePopup() {
    // Handled by template-popup module
  },

  getTemplateStorageKey() {
    return `${this.storageKey}_templates`;
  },

  getSavedTemplates() {
    return getSavedTemplates(this);
  },

  saveStoredTemplates(templates) {
    saveStoredTemplates(this, templates);
  },

  getTemplateName(template) {
    return getTemplateName(this, template);
  },

  getSelectedTemplateName(templates = []) {
    if (this.templateMenuSelectedName && templates.some(
      (template) => getTemplateName(this, template) === this.templateMenuSelectedName,
    )) {
      return this.templateMenuSelectedName;
    }

    if (this.activeTemplateName && templates.some(
      (template) => getTemplateName(this, template) === this.activeTemplateName,
    )) {
      return this.activeTemplateName;
    }

    return null;
  },

  logTemplateMenuSelection(action, detail = '') {
    const suffix = detail ? `: ${detail}` : '';
    log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template menu ${action}${suffix}`);
  },

  closeNavPopup() {
    closeNavPopup(this);
  },

  closeTransientPopups() {
    closeTransientPopups(this);
  },

  getPopupItems(_popupId) {
    // Delegated to popup-manager
    return [];
  },

  // ── Table Settings (delegated to settings/table-settings.js) ──────────────

  setTableWidth(mode) {
    setTableWidth(this, mode);
  },

  saveTableWidth(_mode) {
    // Delegated
  },

  restoreTableWidth() {
    return restoreTableWidth(this);
  },

  applyPanelWidth(mode) {
    if (!this.panel) return;

    if (mode === 'auto') {
      this.panel.style.width = 'auto';
      this.panel.style.flex = '1';
      return;
    }

    // Get width from CSS variable
    const widthVar = `--table-width-${mode}`;
    const computedStyle = getComputedStyle(document.documentElement);
    const width = computedStyle.getPropertyValue(widthVar).trim();

    if (width) {
      this.panel.style.width = width;
      this.panel.style.flex = '0 0 auto';
    } else {
      // Fallback values if CSS vars not available
      const fallbacks = {
        narrow: '160px',
        compact: '314px',
        normal: '468px',
        wide: '622px',
      };
      this.panel.style.width = fallbacks[mode] || fallbacks.compact;
      this.panel.style.flex = '0 0 auto';
    }
  },

  async setTableLayout(mode) {
    await setTableLayout(this, mode);
  },

  saveLayoutMode(_mode) {
    // Delegated
  },

  restoreLayoutMode() {
    return restoreLayoutMode(this);
  },

  // ── Column Manager ────────────────────────────────────────────────────────

  async toggleColumnManager(e, column) {
    e.stopPropagation();

    if (this.columnManager) {
      this.closeColumnManager();
      return;
    }

    const parentColumnManager = this.container.closest('.col-manager-popup');
    this.closeAllColumnManagers(parentColumnManager);

    const managerId = this.getManagerId();

    if (!this.columnManager) {
      this.columnManager = new LithiumColumnManager({
        parentContainer: this.container,
        anchorElement: column.getElement(),
        parentTable: this,
        app: this.app,
        managerId: managerId,
        cssPrefix: 'col-manager',
        onColumnChange: (field, property, value) => {
          this.onColumnManagerChange(field, property, value);
        },
        onClose: () => {
          this.columnManager.hide();
        },
      });
    }

    await this.columnManager.init();
    this.columnManager.show();
  },

  closeAllColumnManagers(parentColumnManager = null) {
    const popups = document.querySelectorAll('.col-manager-popup');

    popups.forEach((popup) => {
      if (popup === parentColumnManager) {
        return;
      }

      if (popup._columnManagerInstance) {
        popup._columnManagerInstance.cleanup();
      } else {
        popup.remove();
      }
    });
  },

  getManagerId() {
    if (this.storageKey) {
      return this.storageKey;
    }

    if (this.tablePath) {
      return this.tablePath.replace(/\//g, '_');
    }

    if (this.container?.id) {
      return this.container.id;
    }

    return `table_${Date.now()}`;
  },

  closeColumnManager() {
    if (this.columnManager) {
      this.columnManager.cleanup();
      this.columnManager = null;
    }
  },

  onColumnManagerChange(field, property, value) {
    log(Subsystems.TABLE, Status.DEBUG,
      `[LithiumTable] Column manager change: ${field}.${property} = ${value}`);
  },

  // ── Legacy Column Chooser (fallback) ──────────────────────────────────────

  toggleColumnChooser(e, column) {
    e.stopPropagation();

    if (this.useColumnManager !== false) {
      this.toggleColumnManager(e, column);
      return;
    }

    if (this.columnChooserPopup) {
      this.closeColumnChooser();
      return;
    }

    const popup = document.createElement('div');
    popup.className = `${this.cssPrefix}-col-chooser-popup`;

    const title = document.createElement('div');
    title.className = `${this.cssPrefix}-col-chooser-title`;
    title.textContent = 'Columns';
    popup.appendChild(title);

    const columns = this.table.getColumns().filter(
      (col) => col.getField() !== '_selector'
    );

    const list = document.createElement('div');
    list.className = `${this.cssPrefix}-col-chooser-list`;
    if (columns.length > 10) {
      list.style.maxHeight = `${10 * 30}px`;
    }

    columns.forEach((col) => {
      const colTitle = col.getDefinition().title || col.getField();
      const isVisible = col.isVisible();

      const item = document.createElement('label');
      item.className = `${this.cssPrefix}-col-chooser-item`;

      const checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      checkbox.checked = isVisible;
      checkbox.className = `${this.cssPrefix}-col-chooser-checkbox`;
      checkbox.addEventListener('change', () => {
        if (checkbox.checked) col.show();
        else col.hide();
        this.table.redraw(true);
      });

      const labelText = document.createElement('span');
      labelText.className = `${this.cssPrefix}-col-chooser-label`;
      labelText.textContent = colTitle;

      item.appendChild(checkbox);
      item.appendChild(labelText);
      list.appendChild(item);
    });

    popup.appendChild(list);

    const headerEl = column.getElement();
    const tableRect = this.container.getBoundingClientRect();
    const headerRect = headerEl.getBoundingClientRect();

    popup.style.left = `${headerRect.left - tableRect.left}px`;
    popup.style.top = `${headerRect.bottom - tableRect.top}px`;

    this.container.appendChild(popup);

    requestAnimationFrame(() => {
      popup.classList.add('visible');
    });

    this.columnChooserPopup = popup;

    this.columnChooserCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !headerEl.contains(evt.target)) {
        this.closeColumnChooser();
      }
    };
    setTimeout(() => {
      document.addEventListener('click', this.columnChooserCloseHandler);
    }, 0);
  },

  closeColumnChooser() {
    if (this.columnChooserPopup) {
      this.columnChooserPopup.remove();
      this.columnChooserPopup = null;
    }
    if (this.columnChooserCloseHandler) {
      document.removeEventListener('click', this.columnChooserCloseHandler);
      this.columnChooserCloseHandler = null;
    }
    this.closeColumnManager();
  },

  // ── Filter Editor (delegated to filters/filter-editor.js) ─────────────────

  createFilterEditorFunction() {
    return createFilterEditorFunction(this.cssPrefix);
  },

  createFilterEditor(cell, onRendered, success, cancel, editorParams) {
    return this.createFilterEditorFunction()(cell, onRendered, success, cancel, editorParams);
  },

  // ── Visual Updates (delegated to visual/visual-updates.js) ────────────────

  updateSelectorCell(row, isSelected) {
    updateSelectorCell(this, row, isSelected);
  },

  updateVisibleColumnClasses() {
    updateVisibleColumnClasses(this);
  },

  getVisibleColumnBoundaries() {
    return getVisibleColumnBoundaries(this);
  },

  applyVisibleColumnClassesToRow(row, firstField, lastField) {
    applyVisibleColumnClassesToRow(this, row, firstField, lastField);
  },

  applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField) {
    applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField);
  },

  // ── Loading Indicator (delegated to visual/loading-indicator.js) ──────────

  showLoading() {
    showLoading(this);
  },

  hideLoading() {
    hideLoading(this);
  },

  // ── Event Handlers ────────────────────────────────────────────────────────

  async handleRefresh() {
    const refreshBtn = this.navigatorContainer?.querySelector(`#${this.cssPrefix}-nav-refresh`);
    if (refreshBtn) {
      refreshBtn.classList.add('lithium-nav-refresh-spin');
      setTimeout(() => refreshBtn.classList.remove('lithium-nav-refresh-spin'), 750);
    }

    if (this.isEditing && this.isDirty) {
      await this.handleSave?.();
    } else if (this.isEditing) {
      await this.exitEditMode?.('cancel');
    }

    // If this table uses Lookup 59 (indicated by lookupKeyIdx), refresh the schemas first
    // This ensures any changes to Lookup 59 are picked up without requiring a page reload
    if (this.lookupKeyIdx != null) {
      try {
        await refreshTabulatorSchemas();
        // After refreshing Lookup 59, reload the table configuration (schema + template)
        await this.reloadConfiguration?.();
        return;
      } catch (err) {
        log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Schema refresh failed: ${err.message}`);
      }
    }

    // Fallback: just refresh data if no Lookup 59
    if (typeof this.onRefresh === 'function') {
      this.onRefresh();
    } else {
      this.loadData?.();
    }
  },

  handleFilter() {
    this.filtersVisible = !this.filtersVisible;

    const filterBtn = this.navigatorContainer?.querySelector(`#${this.cssPrefix}-nav-filter`);
    if (filterBtn) {
      filterBtn.classList.toggle('lithium-nav-btn-active', this.filtersVisible);
    }

    saveFiltersVisible(this, this.filtersVisible);

    const transitionMs = parseInt(getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration').trim()) || 330;

    if (this.filtersVisible) {
      this.toggleHeaderFilters(true);

      const filterInputs = this.container.querySelectorAll('.tabulator-header-filter');
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
          this.container.classList.add('lithium-filters-visible');
        });
      });
    } else {
      this.container.classList.remove('lithium-filters-visible');
      setTimeout(() => {
        this.toggleHeaderFilters(false);
        this.table?.clearHeaderFilter();
      }, transitionMs);
    }
  },

  /**
   * Toggle header filters on/off by updating column definitions.
   * @param {boolean} visible - Whether header filters should be visible
   */
  toggleHeaderFilters(visible) {
    if (!this.table) return;

    const columns = this.table.getColumns();
    const filterEditorFn = this.createFilterEditorFunction();

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

    this.table.setColumns(updatedColumns);

    // Re-enforce selectableRows: 1 after setColumns() - Tabulator may lose this setting
    this.table.modules.select?.setSelectionMode?.(1);

    requestAnimationFrame(() => {
      this.clearColumnInlineHeights();
    });
  },

  clearColumnInlineHeights() {
    if (!this.container) return;
    const cols = this.container.querySelectorAll('.tabulator-header .tabulator-col');
    cols.forEach((col) => {
      col.style.removeProperty('height');
      col.style.removeProperty('min-height');
    });
  },

  expandAll() {
    if (!this.table) return;
    const groups = this.table.getGroups();
    if (groups.length > 0) {
      groups.forEach((group) => group.show());
    }
  },

  collapseAll() {
    if (!this.table) return;
    const groups = this.table.getGroups();
    if (groups.length > 0) {
      groups.forEach((group) => group.hide());
    }
  },

  // ── Persistence (delegated to persistence/persistence.js) ─────────────────

  saveSelectedRowId(rowId) {
    saveSelectedRowId(this, rowId);
  },

  restoreSelectedRowId() {
    return restoreSelectedRowId(this);
  },

  clearSavedRowSelection() {
    clearSavedRowSelection(this);
  },

  saveFiltersVisible(visible) {
    saveFiltersVisible(this, visible);
  },

  restoreFiltersVisible() {
    return restoreFiltersVisible(this);
  },

  async setupPersistence() {
    const restoredWidth = restoreTableWidth(this);

    if (restoredWidth) {
      this.tableWidthMode = restoredWidth;
      if (this.panel) {
        this.applyPanelWidth(restoredWidth);
      } else if (typeof this.onSetTableWidth === 'function') {
        this.onSetTableWidth(restoredWidth);
      }
    } else {
      if (this.panel) {
        this.applyPanelWidth(null);
      } else if (typeof this.onSetTableWidth === 'function') {
        this.onSetTableWidth(null);
      }
    }

    const savedFiltersVisible = restoreFiltersVisible(this);
    if (savedFiltersVisible && this.table) {
      this.filtersVisible = true;
      this.container.classList.add('lithium-filters-visible');
      const filterBtn = this.navigatorContainer?.querySelector(`#${this.cssPrefix}-nav-filter`);
      if (filterBtn) {
        filterBtn.classList.add('lithium-nav-btn-active');
      }
      this.toggleHeaderFilters(true);
    }

    const templates = getSavedTemplates(this);
    const defaultTemplate = templates.find((template) => getTemplateName(this, template) === 'Default');
    if (defaultTemplate) {
      await this.loadTemplate?.(defaultTemplate);
    }
  },

  // ── Template System ───────────────────────────────────────────────────────

  generateTemplateJSON() {
    if (!this.table) return null;

    const columns = this.table.getColumns();
    const columnDefs = {};

    if (this.columnManager?.columnTable?.table) {
      const pendingColumns = this.columnManager.buildPendingTemplateColumns?.();
      if (pendingColumns && Object.keys(pendingColumns).length > 0) {
        const sorters = this.table.getSorters?.() || [];
        const initialSort = sorters.map((s) => ({
          column: s.field,
          dir: s.dir,
        }));

        const tableDef = {
          $schema: '../tabledef-schema.json',
          $version: '1.0.0',
          $description: `Table definition for ${this.tableDef?.title || this.tablePath}`,
          table: this.tablePath?.split('/').pop() || 'table',
          title: this.tableDef?.title || 'Table',
          queryRef: this.queryRefs?.queryRef || null,
          searchQueryRef: this.queryRefs?.searchQueryRef || null,
          detailQueryRef: this.queryRefs?.detailQueryRef || null,
          readonly: this.readonly || false,
          selectableRows: 1,
          layout: this.tableLayoutMode || 'fitColumns',
          resizableColumns: true,
          columns: pendingColumns,
        };

        if (initialSort.length > 0) {
          tableDef.initialSort = initialSort;
        }

        return {
          ...tableDef,
          _templateMeta: {
            name: this.tableDef?.title || 'Custom Template',
            tablePath: this.tablePath,
            managerId: this.getManagerId?.() || this.storageKey,
            createdAt: new Date().toISOString(),
            widthMode: this.tableWidthMode,
          },
        };
      }
    }

    // Start with ALL properties from the current tableDef to ensure exhaustiveness
    const tableDef = {
      ...this.tableDef,
      columns: {}, // Will be rebuilt below
    };

    // Override runtime properties that may differ from the loaded tableDef
    tableDef.layout = this.tableLayoutMode || tableDef.layout || 'fitColumns';
    tableDef.selectableRows = 1; // Always single-select for templates

    // First, include ALL columns from the base tableDef (from Lookup 59 or auto-discovered)
    // This ensures new columns are included in the saved template
    const baseColumns = this.tableDef?.columns || {};
    for (const [fieldName, colDef] of Object.entries(baseColumns)) {
      if (!fieldName || fieldName === '_selector') continue;
      columnDefs[fieldName] = createTemplateColumnFromTableDef(fieldName, colDef);
    }

    // Then merge with current visible columns (may have user changes like width, visible, etc.)
    // This preserves base properties while adding runtime changes
    columns.forEach((col) => {
      const field = col.getField();

      if (field === '_selector') return;

      const baseCol = columnDefs[field] || {};
      const runtimeCol = extractTemplateColumnFromColumn(col, this.primaryKeyField);

      // Merge: base properties first, then runtime overrides
      columnDefs[field] = { ...baseCol, ...runtimeCol };
    });

    const sorters = this.table.getSorters?.() || [];
    const initialSort = sorters.map((s) => ({
      column: s.field,
      dir: s.dir,
    }));

    tableDef.columns = columnDefs;

    if (initialSort.length > 0) {
      tableDef.initialSort = initialSort;
    }

    return {
      ...tableDef,
      _templateMeta: {
        name: this.tableDef?.title || 'Custom Template',
        tablePath: this.tablePath,
        managerId: this.getManagerId?.() || this.storageKey,
        createdAt: new Date().toISOString(),
        widthMode: this.tableWidthMode,
      },
    };
  },

  async copyTemplateToClipboard(templateOverride = null) {
    try {
      const template = templateOverride || this.generateTemplateJSON();
      if (!template) {
        toast.error('Failed to generate template', { duration: 3000 });
        return;
      }

      const jsonString = JSON.stringify(template, null, 2);
      await navigator.clipboard.writeText(jsonString);

      const templateName = getTemplateName(this, template);

      toast.success('Template copied to clipboard', {
        description: templateOverride ? templateName : `${Object.keys(template.columns || {}).length} columns exported`,
        duration: 3000,
      });

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Template copied to clipboard for ${this.storageKey}`);
    } catch (error) {
      toast.error('Failed to copy template', {
        description: error.message,
        duration: 5000,
      });
      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Failed to copy template: ${error.message}`);
    }
  },

  saveTemplate() {
    const template = this.generateTemplateJSON();
    if (!template) return;

    const defaultName = `${template._templateMeta?.name || 'Template'} - ${new Date().toLocaleDateString()}`;
    const name = window.prompt('Enter template name:', defaultName);
    if (!name) return;

    template._templateMeta = template._templateMeta || {};
    template._templateMeta.name = name;

    const templates = getSavedTemplates(this);

    const existingIndex = templates.findIndex((item) => getTemplateName(this, item) === name);

    if (existingIndex >= 0) {
      templates[existingIndex] = template;
    } else {
      templates.push(template);
    }

    try {
      saveStoredTemplates(this, templates);
      this.templateMenuSelectedName = name;
      toast.success('Template saved', { description: name, duration: 3000 });
      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Template saved: ${name}`);
      return template;
    } catch (_e) {
      toast.error('Failed to save template', {
        description: 'Storage quota may be exceeded',
        duration: 5000,
      });
    }
  },

  deleteTemplate(index) {
    const templates = getSavedTemplates(this);

    if (index >= 0 && index < templates.length) {
      const name = getTemplateName(this, templates[index]);
      templates.splice(index, 1);
      try {
        saveStoredTemplates(this, templates);
        if (this.templateMenuSelectedName === name) {
          this.templateMenuSelectedName = null;
        }
        if (this.activeTemplateName === name) {
          this.activeTemplateName = null;
        }
        toast.success('Template deleted', { description: name, duration: 3000 });
        return true;
      } catch (_e) {
        console.error(`[LithiumTable] FAILED to delete template: ${_e?.message}`);
      }
    }
    return false;
  },

  async clearTemplate() {
    if (!this.table) return;

    const confirmed = window.confirm(
      'Reload the default template from the database?\n\nThis will reset the current table view, but will not delete any saved templates.'
    );
    if (!confirmed) return;

    try {
      localStorage.removeItem(`${this.storageKey}_width_mode`);
      localStorage.removeItem(`${this.storageKey}_layout_mode`);
      this.activeTemplateName = null;
      this.templateMenuSelectedName = null;

      const currentData = this.table.getData();
      const selectedId = this.getSelectedRowId?.();

      await this.loadConfiguration?.();

      this.table.destroy();
      this.table = null;

      await this.initTable?.();
      this.table.setData(currentData);
      this.table.modules.select?.setSelectionMode?.(1);

      await new Promise((resolve) => {
        requestAnimationFrame(() => {
          this.autoSelectRow?.(selectedId);
          // Button state updates are handled internally by autoSelectRow
          resolve();
        });
      });

      log(Subsystems.TABLE, Status.INFO, '[LithiumTable] Default template reloaded from database');
    } catch (error) {
      toast.error('Failed to load default template', {
        description: error.message,
        duration: 5000,
      });
      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Failed to reload default template: ${error.message}`);
    }
  },

  async loadTemplate(template) {
    if (!this.table || !template) return;

    try {
      const templateName = template._templateMeta?.name || template.name || 'Template';

      // Ensure we have a base tableDef (from Lookup 59 or auto-discovered)
      // before applying the template. This ensures all columns are available
      // in the Column Manager, including new columns that may have been
      // added since the template was saved.
      if (!this.tableDef && this.tablePath) {
        log(Subsystems.TABLE, Status.INFO, `[${this.cssPrefix}] Reloading tableDef before applying template...`);
        await this.loadConfiguration?.();
      }

      const widthMode = template._templateMeta?.widthMode || template.layout?.widthMode;
      if (widthMode && this.setTableWidth) {
        try {
          this.setTableWidth(widthMode);
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply width: ${e.message}`);
        }
      }

      if (template.layout && this.setTableLayout) {
        try {
          await this.setTableLayout(template.layout);
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply layout: ${e.message}`);
        }
      }

      if (template.columns && typeof template.columns === 'object') {
        this.applyTemplateColumns(template.columns);
      }

      if (template.initialSort && Array.isArray(template.initialSort) && typeof this.table.setSort === 'function') {
        try {
          this.table.setSort(template.initialSort.map((s) => ({
            column: s.column,
            dir: s.dir,
          })));
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply sort: ${e.message}`);
        }
      }

      // Restore filter visibility state
      if (template._filtersVisible === true && !this.filtersVisible) {
        this.toggleHeaderFilters?.(true);
      } else if (template._filtersVisible === false && this.filtersVisible) {
        this.toggleHeaderFilters?.(false);
      }

      // Restore filter values
      if (template._filterValues && typeof template._filterValues === 'object') {
        try {
          for (const [field, value] of Object.entries(template._filterValues)) {
            if (value != null && value !== '') {
              this.table.setHeaderFilterValue?.(field, value);
            }
          }
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply filter values: ${e.message}`);
        }
      }

      if (typeof this.table.redraw === 'function') {
        this.table.redraw(true);
      }

      this.activeTemplateName = templateName;
      this.templateMenuSelectedName = templateName;

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Template loaded: ${templateName}`);
      return true;
    } catch (error) {
      toast.error('Failed to load template', {
        description: error.message,
        duration: 5000,
      });
      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Failed to load template: ${error.message}`);
      return false;
    }
  },

  applyTemplateColumns(templateColumns) {
    if (!this.table || !this.coltypes) return;

    const columns = buildColumnDefinitionsFromTemplateState({
      table: this.table,
      templateColumns,
      tableDefColumns: this.tableDef?.columns || {},
      coltypes: this.coltypes,
      filterEditor: this.createFilterEditorFunction(),
      filtersVisible: this.filtersVisible,
      primaryKeyField: this.primaryKeyField,
      readonly: this.readonly,
      selectorColumn: this.buildSelectorColumn?.(),
    });

    this.applyResolvedColumnDefinitions(columns);
  },

  applyResolvedColumnDefinitions(columns) {
    if (!this.table || !Array.isArray(columns) || columns.length === 0) return;

    const currentData = this.table.getData();
    const selectedId = this.getSelectedRowId?.();
    const currentSorters = this.table.getSorters?.() || [];
    const currentHeaderFilters = this.table.getHeaderFilters?.() || [];

    this.table.setColumns(columns);
    this.table.setData(currentData);

    // Re-enforce selectableRows: 1 after setColumns() - Tabulator may lose this setting
    this.table.modules.select?.setSelectionMode?.(1);

    requestAnimationFrame(() => {
      if (currentSorters.length > 0) {
        this.table.setSort(currentSorters.map((sorter) => ({
          column: sorter.field || sorter.column,
          dir: sorter.dir,
        })));
      }

      if (this.filtersVisible && currentHeaderFilters.length > 0) {
        currentHeaderFilters.forEach((filter) => {
          if (filter?.field != null) {
            this.table.setHeaderFilterValue(filter.field, filter.value);
          }
        });
      }

      this.autoSelectRow?.(selectedId);
      this.updateMoveButtonState?.();
      this.updateDuplicateButtonState?.();
      this.updateVisibleColumnClasses();
      this.clearColumnInlineHeights();
      this.table.redraw(true);
    });
  },

  resetTemplate() {
    return this.clearTemplate();
  },
};

export default LithiumTableUIMixin;
