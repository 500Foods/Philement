/**
 * Queries Manager - Templates Module
 *
 * Handles template saving, loading, and application.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';

// Constants
const TEMPLATES_KEY = 'lithium_queries_templates';
const DEFAULT_TEMPLATE_KEY = 'lithium_queries_default_template';

/**
 * TemplateManager - Manages table layout templates
 */
export class TemplateManager {
  constructor(manager) {
    this.manager = manager;
  }

  /**
   * Load templates from localStorage
   */
  loadTemplates() {
    try {
      const stored = localStorage.getItem(TEMPLATES_KEY);
      if (stored) return JSON.parse(stored);
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to load templates: ${err.message}`);
    }
    return [];
  }

  /**
   * Save templates to localStorage
   */
  saveTemplates(templates) {
    try {
      localStorage.setItem(TEMPLATES_KEY, JSON.stringify(templates));
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to save templates: ${err.message}`);
    }
  }

  /**
   * Load default template name
   */
  loadDefaultTemplateName() {
    try {
      return localStorage.getItem(DEFAULT_TEMPLATE_KEY);
    } catch (err) {
      return null;
    }
  }

  /**
   * Save default template name
   */
  saveDefaultTemplateName(name) {
    try {
      if (name) {
        localStorage.setItem(DEFAULT_TEMPLATE_KEY, name);
      } else {
        localStorage.removeItem(DEFAULT_TEMPLATE_KEY);
      }
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to save default template: ${err.message}`);
    }
  }

  /**
   * Delete a template by name
   */
  deleteTemplate(templateName) {
    const confirmed = window.confirm(`Delete template "${templateName}"?\n\nThis action cannot be undone.`);
    if (!confirmed) return;

    const templates = this.loadTemplates();
    const filtered = templates.filter(t => t.name !== templateName);
    this.saveTemplates(filtered);

    const defaultName = this.loadDefaultTemplateName();
    if (defaultName === templateName) {
      this.saveDefaultTemplateName(null);
    }

    toast.success('Template Deleted', { description: `"${templateName}" has been deleted`, duration: 3000 });
  }

  /**
   * Show save template dialog
   */
  showSaveTemplateDialog() {
    if (!this.manager.table) {
      toast.info('No Table', { description: 'Table not initialized', duration: 3000 });
      return;
    }

    const dialog = document.createElement('div');
    dialog.className = 'queries-template-dialog';
    dialog.innerHTML = `
      <div class="queries-template-dialog-content">
        <h3>Save Template</h3>
        <input type="text" class="queries-template-input" placeholder="Template name..." maxlength="50">
        <div class="queries-template-dialog-buttons">
          <button type="button" class="queries-template-btn queries-template-btn-secondary" data-action="cancel">Cancel</button>
          <button type="button" class="queries-template-btn queries-template-btn-primary" data-action="save">Save</button>
        </div>
      </div>
    `;

    document.body.appendChild(dialog);

    const input = dialog.querySelector('.queries-template-input');
    const saveBtn = dialog.querySelector('[data-action="save"]');
    const cancelBtn = dialog.querySelector('[data-action="cancel"]');

    requestAnimationFrame(() => input.focus());

    const closeDialog = () => dialog.remove();

    const handleSave = () => {
      const name = input.value.trim();
      if (!name) {
        input.classList.add('error');
        return;
      }
      this.saveTemplate(name);
      closeDialog();
    };

    saveBtn.addEventListener('click', handleSave);
    cancelBtn.addEventListener('click', closeDialog);

    input.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') handleSave();
      else if (e.key === 'Escape') closeDialog();
    });

    dialog.addEventListener('click', (e) => {
      if (e.target === dialog) closeDialog();
    });
  }

  /**
   * Save current table configuration as a template
   */
  saveTemplate(name) {
    if (!this.manager.table) return;

    const templates = this.loadTemplates();
    const existingIndex = templates.findIndex(t => t.name === name);

    const allColumns = this.manager.table.getColumns();
    const columnConfig = allColumns
      .filter(col => col.getField() && col.getField() !== '_selector')
      .map((col, index) => ({
        field: col.getField(),
        visible: col.isVisible(),
        width: col.getWidth(),
        position: index,
      }));

    const sorters = this.manager.table.getSorters().map(s => ({ field: s.field, dir: s.dir }));

    const filters = [];
    allColumns.forEach(col => {
      const field = col.getField();
      if (!field || field === '_selector') return;
      const filterVal = this.manager.table.getHeaderFilterValue?.(field);
      if (filterVal !== undefined && filterVal !== '' && filterVal !== null) {
        filters.push({ field, value: filterVal });
      }
    });

    const panelWidth = this.manager.elements.leftPanel?.offsetWidth || 314;
    const layoutMode = this.manager._tableLayoutMode || 'fitColumns';
    const filtersVisible = this.manager._filtersVisible || false;

    const template = {
      name,
      columns: columnConfig,
      sorters,
      filters,
      panelWidth,
      layoutMode,
      filtersVisible,
      createdAt: new Date().toISOString(),
    };

    if (existingIndex >= 0) {
      templates[existingIndex] = template;
      toast.success('Template Updated', { description: `"${name}" has been updated`, duration: 3000 });
    } else {
      templates.push(template);
      toast.success('Template Saved', { description: `"${name}" has been saved`, duration: 3000 });
    }

    this.saveTemplates(templates);
    log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Template saved: ${name} (${columnConfig.length} columns)`);
  }

  /**
   * Show make default dialog
   */
  showMakeDefaultDialog() {
    const templates = this.loadTemplates();
    const currentDefault = this.loadDefaultTemplateName();

    if (templates.length === 0) {
      toast.info('No Templates', { description: 'Save a template first', duration: 3000 });
      return;
    }

    const dialog = document.createElement('div');
    dialog.className = 'queries-template-dialog';

    const templateOptions = templates.map(t => {
      const isDefault = t.name === currentDefault;
      return `<option value="${t.name}" ${isDefault ? 'selected' : ''}>${t.name}${isDefault ? ' (current default)' : ''}</option>`;
    }).join('');

    dialog.innerHTML = `
      <div class="queries-template-dialog-content">
        <h3>Set Default Template</h3>
        <select class="queries-template-select">
          <option value="">-- No Default --</option>
          ${templateOptions}
        </select>
        <div class="queries-template-dialog-buttons">
          <button type="button" class="queries-template-btn queries-template-btn-secondary" data-action="cancel">Cancel</button>
          <button type="button" class="queries-template-btn queries-template-btn-primary" data-action="set">Set Default</button>
        </div>
      </div>
    `;

    document.body.appendChild(dialog);

    const select = dialog.querySelector('.queries-template-select');
    const setBtn = dialog.querySelector('[data-action="set"]');
    const cancelBtn = dialog.querySelector('[data-action="cancel"]');

    const closeDialog = () => dialog.remove();

    setBtn.addEventListener('click', () => {
      const selectedName = select.value;
      if (selectedName) {
        this.saveDefaultTemplateName(selectedName);
        toast.success('Default Set', { description: `"${selectedName}" is now the default template`, duration: 3000 });
      } else {
        this.saveDefaultTemplateName(null);
        toast.info('Default Cleared', { description: 'No default template set', duration: 3000 });
      }
      closeDialog();
    });

    cancelBtn.addEventListener('click', closeDialog);
    dialog.addEventListener('click', (e) => {
      if (e.target === dialog) closeDialog();
    });
  }

  /**
   * Apply a template to the table
   */
  applyTemplate(template) {
    if (!this._validateTemplate(template)) return;

    try {
      this._applyTemplateLayout(template);
      this._applyTemplateColumns(template);
      this._applyTemplateFilters(template);

      this.manager.table.redraw(true);
      toast.success('Template Applied', { description: `"${template.name}" has been applied`, duration: 3000 });
      log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Template applied: ${template.name}`);
    } catch (err) {
      this._handleTemplateError(err);
    }
  }

  /**
   * Validate template before applying
   */
  _validateTemplate(template) {
    if (!this.manager.table || !template) {
      toast.error('Template Error', { description: 'Failed to apply template', duration: 3000 });
      return false;
    }
    if (!template.columns || template.columns.length === 0) {
      toast.error('Template Error', { description: 'Template has no column data', duration: 3000 });
      return false;
    }
    return true;
  }

  /**
   * Apply layout settings from template
   */
  _applyTemplateLayout(template) {
    if (template.layoutMode && template.layoutMode !== this.manager._tableLayoutMode) {
      this.manager.setTableLayout(template.layoutMode);
    }

    if (template.panelWidth && this.manager.elements.leftPanel) {
      this.manager.elements.leftPanel.style.width = `${template.panelWidth}px`;
      this.manager._savePanelWidth(template.panelWidth);
      this.manager._tableWidthMode = this._detectWidthMode(template.panelWidth);
    }
  }

  /**
   * Apply column configuration from template
   */
  _applyTemplateColumns(template) {
    if (this.manager.table.blockRedraw) this.manager.table.blockRedraw();

    try {
      const sortedColumns = [...template.columns].sort((a, b) => a.position - b.position);
      this._reorderColumns(sortedColumns);

      if (template.sorters?.length > 0) {
        this.manager.table.setSort(template.sorters);
      }
    } finally {
      if (this.manager.table.restoreRedraw) this.manager.table.restoreRedraw();
    }
  }

  /**
   * Reorder columns according to template
   */
  _reorderColumns(sortedColumns) {
    for (let i = sortedColumns.length - 1; i >= 0; i--) {
      const colConfig = sortedColumns[i];
      const col = this.manager.table.getColumn(colConfig.field);

      if (!col) {
        console.warn(`[Template] Column not found: ${colConfig.field}`);
        continue;
      }

      if (i > 0) {
        const afterCol = this.manager.table.getColumn(sortedColumns[i - 1].field);
        if (afterCol) this.manager.table.moveColumn(col, afterCol, false);
      }

      colConfig.visible ? col.show() : col.hide();

      if (colConfig.width) col.setWidth(colConfig.width);
    }
  }

  /**
   * Apply filters from template
   */
  _applyTemplateFilters(template) {
    if (template.filtersVisible !== undefined && template.filtersVisible !== this.manager._filtersVisible) {
      this.manager.handleNavFilter();
    }

    if (template.filters?.length > 0) {
      template.filters.forEach(filter => {
        this.manager.table.setHeaderFilterValue?.(filter.field, filter.value);
      });
    }
  }

  /**
   * Handle template application error
   */
  _handleTemplateError(err) {
    if (this.manager.table?.restoreRedraw) this.manager.table.restoreRedraw();
    console.error('[Template] Error applying template:', err);
    toast.error('Template Error', { description: err.message, duration: 3000 });
    log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to apply template: ${err.message}`);
  }

  /**
   * Detect width mode from pixel value
   */
  _detectWidthMode(px) {
    const presets = { narrow: 160, compact: 314, normal: 468, wide: 622 };
    const tolerance = 8;
    for (const [mode, target] of Object.entries(presets)) {
      if (Math.abs(px - target) <= tolerance) return mode;
    }
    return 'custom';
  }

  /**
   * Apply default template on load
   */
  applyDefaultTemplate() {
    const defaultName = this.loadDefaultTemplateName();
    if (!defaultName) return;

    const templates = this.loadTemplates();
    const defaultTemplate = templates.find(t => t.name === defaultName);

    if (defaultTemplate) {
      this.manager.table.on('tableBuilt', () => {
        this.applyTemplate(defaultTemplate);
      });
    }
  }

  /**
   * Get popup items for template menu
   */
  getTemplatePopupItems() {
    const items = [];
    const templates = this.loadTemplates();
    const defaultTemplateName = this.loadDefaultTemplateName();

    templates.forEach(template => {
      const isDefault = template.name === defaultTemplateName;
      items.push({
        label: template.name,
        action: () => this.applyTemplate(template),
        isTemplate: true,
        template,
        isDefault,
      });
    });

    if (templates.length > 0) {
      items.push({ label: '', action: () => {}, isSeparator: true });
    }

    items.push({ label: 'Save template...', action: () => this.showSaveTemplateDialog(), icon: 'fa-star' });

    if (templates.length > 0) {
      items.push({ label: 'Make default...', action: () => this.showMakeDefaultDialog(), icon: 'fa-heart' });
    }

    return items;
  }
}

/**
 * Create a template manager for the manager
 */
export function createTemplateManager(manager) {
  return new TemplateManager(manager);
}
