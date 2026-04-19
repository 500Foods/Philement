/**
 * LithiumTable UI Module (Refactored)
 *
 * Navigator, popups, column chooser, and visual updates.
 * Delegates to feature-specific modules.
 *
 * @module tables/lithium-table-ui
 */

import { log, Subsystems, Status } from '../core/log.js';
import { toast } from '../shared/toast.js';
import {
  buildColumnDefinitionsFromTemplateState,
  generateTemplateJSON as generateTemplateJSONFromCapture,
} from './template/capture.js';

import {
  handleRefresh,
  handleFilter,
  toggleHeaderFilters,
  clearColumnInlineHeights,
  expandAll,
  collapseAll,
} from './events/event-handlers.js';

import {
  toggleColumnManager,
  closeAllColumnManagers,
  getManagerId,
  closeColumnManager,
  onColumnManagerChange,
} from './column-manager/cm-integration.js';

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

      // Check for Column Manager popup
      const hostPopup = btn?.closest?.('.col-manager-popup');
      if (hostPopup) {
        const hostZIndex = parseInt(window.getComputedStyle(hostPopup).zIndex, 10);
        if (Number.isFinite(hostZIndex)) {
          popup.style.zIndex = String(hostZIndex + 10);
        }
      } else {
        // Check for Crimson popup
        const crimsonPopup = btn?.closest?.('.crimson-citation-popup');
        if (crimsonPopup) {
          const crimsonZIndex = parseInt(window.getComputedStyle(crimsonPopup).zIndex, 10);
          if (Number.isFinite(crimsonZIndex)) {
            popup.style.zIndex = String(crimsonZIndex + 10);
          }
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

  buildTemplatePopup() {
    return buildTemplatePopup(this);
  },

  createPopupSeparator() {
    return createPopupSeparator();
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

  async setTableLayout(mode) {
    await setTableLayout(this, mode);
  },

  saveLayoutMode(_mode) {
    // Delegated
  },

  restoreLayoutMode() {
    return restoreLayoutMode(this);
  },

  // ── Column Manager (delegated to column-manager/cm-integration.js) ────────

  async toggleColumnManager(e, column) {
    return toggleColumnManager(this, e, column);
  },

  closeAllColumnManagers(parentColumnManager = null) {
    return closeAllColumnManagers(parentColumnManager);
  },

  getManagerId() {
    return getManagerId(this);
  },

  closeColumnManager() {
    return closeColumnManager(this);
  },

  onColumnManagerChange(field, property, value) {
    return onColumnManagerChange(this, field, property, value);
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

  // ── Event Handlers (delegated to events/event-handlers.js) ────────────────

  async handleRefresh() {
    return handleRefresh(this);
  },

  handleFilter() {
    return handleFilter(this);
  },

  toggleHeaderFilters(visible) {
    return toggleHeaderFilters(this, visible);
  },

  clearColumnInlineHeights() {
    return clearColumnInlineHeights(this);
  },

  expandAll() {
    return expandAll(this);
  },

  collapseAll() {
    return collapseAll(this);
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
    return generateTemplateJSONFromCapture(this);
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
