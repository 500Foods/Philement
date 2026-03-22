/**
 * Queries Manager - UI Module
 *
 * Handles UI helpers, popups, footer, and font settings.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons } from '../../core/manager-ui.js';

// Constants
const PANEL_WIDTH_KEY = 'lithium_queries_panel_width';
const LAYOUT_MODE_KEY = 'lithium_queries_layout_mode';
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const FONT_SIZE_STEP = 2;
const DEFAULT_FONT_FAMILY = '"Vanadium Mono", var(--font-mono, monospace)';

/**
 * UIManager - Manages UI components and interactions
 */
export class UIManager {
  constructor(manager) {
    this.manager = manager;
    this._columnChooserPopup = null;
    this._columnChooserCloseHandler = null;
    this._activeNavPopup = null;
    this._activeNavPopupId = null;
    this._navPopupCloseHandler = null;
    this._footerExportPopup = null;
    this._footerExportPopupBound = null;
    this.fontPopup = null;
    this._fontPopupCloseHandler = null;
    this._fontTarget = 'sql';
    this._arrowRotation = 0;
    this._filtersVisible = false;
    this._tableWidthMode = 'compact';
    this._tableLayoutMode = 'fitColumns';
    this.isResizing = false;
  }

  /**
   * Toggle collapse of left panel
   */
  toggleCollapse() {
    const isCollapsed = this.manager.elements.leftPanel.classList.toggle('collapsed');
    this.manager.elements.splitter.classList.toggle('collapsed', isCollapsed);
    const iconEl = document.querySelector('#queries-collapse-icon') || document.querySelector('#queries-collapse-btn')?.firstElementChild;
    if (iconEl) {
      this._arrowRotation += 180;
      iconEl.style.transform = `rotate(${this._arrowRotation}deg)`;
    }
  }

  /**
   * Create filter editor for header
   */
  createFilterEditor(cell, onRendered, success, cancel, editorParams) {
    const input = document.createElement('input');
    input.type = 'text';
    input.placeholder = editorParams?.placeholder || 'filter...';
    input.className = 'queries-header-filter-input';

    let clearBtn = null;
    const updateClearBtn = () => { if (clearBtn) clearBtn.style.display = input.value ? 'flex' : 'none'; };

    input.addEventListener('input', () => { updateClearBtn(); success(input.value); });
    input.addEventListener('keydown', (e) => { if (e.key === 'Escape') { input.value = ''; updateClearBtn(); success(''); e.stopPropagation(); } });

    onRendered(() => {
      const parent = input.parentElement;
      if (!parent) return;
      parent.style.position = 'relative';
      clearBtn = document.createElement('span');
      clearBtn.className = 'queries-header-filter-clear';
      clearBtn.innerHTML = '&times;';
      clearBtn.title = 'Clear filter';
      clearBtn.style.display = 'none';
      clearBtn.addEventListener('click', (e) => { e.stopPropagation(); input.value = ''; clearBtn.style.display = 'none'; success(''); });
      parent.appendChild(clearBtn);
    });

    return input;
  }

  // ============ FOOTER ============

  /**
   * Setup footer controls using common function
   */
  setupFooter() {
    const slot = this.manager.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    // Get the placeholder - manager buttons go before it, fixed buttons go after
    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this.handleFooterPrint(),
      onEmail: () => this.handleFooterEmail(),
      onExport: (e) => this.toggleFooterExportPopup(e),
      reportOptions: [
        { value: 'view', label: 'Query List View' },
        { value: 'data', label: 'Query List Data' },
      ],
      fillerTitle: 'Reports',
      anchor: placeholder, // Insert manager buttons before placeholder
    });

    this._footerDatasource = footerElements.reportSelect;
  }

  /**
   * Get footer datasource value
   */
  getFooterDatasource() {
    return this._footerDatasource?.value || 'view';
  }

  /**
   * Handle footer print button
   */
  handleFooterPrint() {
    const mode = this.getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `Footer: Print (${mode})`);
    if (!this.manager.table) return;

    if (mode === 'view') {
      this.manager.table.print();
    } else {
      this.manager.table.print('all', true);
    }
  }

  /**
   * Handle footer email button
   */
  handleFooterEmail() {
    const mode = this.getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `Footer: Email (${mode})`);
    if (!this.manager.table) return;

    const rows = mode === 'view' ? this.manager.table.getRows('active') : this.manager.table.getRows();

    if (rows.length === 0) {
      toast.info('No Data', { description: 'No rows to include in the email', duration: 3000 });
      return;
    }

    const visibleCols = this.manager.table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector');
    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const separator = headers.map(h => '-'.repeat(h.length)).join('  ');

    const dataLines = rows.slice(0, 50).map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val) : '';
      }).join('\t');
    });

    const totalRows = rows.length;
    const truncated = totalRows > 50 ? `\n... (${totalRows - 50} more rows not shown)` : '';
    const modeLabel = mode === 'view' ? 'Filtered View' : 'Full Data';

    const body = [
      `Query Export - ${modeLabel}`,
      `Generated: ${new Date().toLocaleString()}`,
      `Total Rows: ${totalRows}`,
      '',
      headers.join('\t'),
      separator,
      ...dataLines,
      truncated,
    ].join('\n');

    window.location.href = `mailto:?subject=Query Export&body=${encodeURIComponent(body)}`;
  }

  /**
   * Toggle footer export popup
   */
  toggleFooterExportPopup(e) {
    e.stopPropagation();
    this.closeFooterExportPopup();

    const popup = document.createElement('div');
    popup.className = 'queries-popup queries-export-popup';
    popup.innerHTML = `
      <div class="queries-popup-item" data-format="pdf"><fa fa-file-pdf></fa> PDF</div>
      <div class="queries-popup-item" data-format="csv"><fa fa-file-csv></fa> CSV</div>
      <div class="queries-popup-item" data-format="xlsx"><fa fa-file-excel></fa> Excel</div>
      <div class="queries-popup-item" data-format="txt"><fa fa-file-lines></fa> Text</div>
    `;

    const btn = e.currentTarget;
    const rect = btn.getBoundingClientRect();
    
    document.body.appendChild(popup);
    
    // Position popup above the button (bottom-left of popup aligns with top-left of button)
    requestAnimationFrame(() => {
      const popupRect = popup.getBoundingClientRect();
      popup.style.position = 'fixed';
      popup.style.top = `${rect.top - popupRect.height - 8}px`;
      popup.style.left = `${rect.left}px`;
    });
    
    processIcons(popup);

    popup.querySelectorAll('.queries-popup-item').forEach(item => {
      item.addEventListener('click', () => {
        this.handleFooterExport(item.dataset.format);
        this.closeFooterExportPopup();
      });
    });

    this._footerExportPopup = popup;

    // Animate in after a small delay
    setTimeout(() => {
      popup.classList.add('visible');
    }, 10);

    setTimeout(() => {
      document.addEventListener('click', this._footerExportPopupBound = () => {
        this.closeFooterExportPopup();
      }, { once: true });
    }, 0);
  }

  /**
   * Close footer export popup
   */
  closeFooterExportPopup() {
    if (this._footerExportPopup) {
      this._footerExportPopup.remove();
      this._footerExportPopup = null;
    }
  }

  /**
   * Handle footer export
   */
  handleFooterExport(format) {
    const mode = this.getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `Footer: Export ${format} (${mode})`);
    if (!this.manager.table) return;

    const filename = `queries-export-${new Date().toISOString().slice(0, 10)}`;

    switch (format) {
      case 'pdf':
        if (mode === 'view') {
          this.manager.table.download('pdf', `${filename}.pdf`, { orientation: 'landscape' });
        } else {
          this.manager.table.download('pdf', `${filename}.pdf`, { orientation: 'landscape' }, 'all');
        }
        break;
      case 'csv':
        this.manager.table.download('csv', `${filename}.csv`, {}, mode === 'view' ? 'active' : 'all');
        break;
      case 'xlsx':
        this.manager.table.download('xlsx', `${filename}.xlsx`, {}, mode === 'view' ? 'active' : 'all');
        break;
      case 'txt':
        this.manager.table.download('txt', `${filename}.txt`, { delimiter: '\t' }, mode === 'view' ? 'active' : 'all');
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `Unknown export format: ${format}`);
    }
  }

  // ============ COLUMN CHOOSER ============

  /**
   * Toggle column chooser popup
   */
  toggleColumnChooser(e, column) {
    e?.stopPropagation?.();
    if (this._columnChooserPopup) {
      this.closeColumnChooser();
      return;
    }

    const popup = document.createElement('div');
    popup.className = 'queries-col-chooser-popup';

    const title = document.createElement('div');
    title.className = 'queries-col-chooser-title';
    title.textContent = 'Columns';
    popup.appendChild(title);

    const columns = this.manager.table?.getColumns().filter(col => col.getField() !== '_selector') || [];

    const list = document.createElement('div');
    list.className = 'queries-col-chooser-list';
    if (columns.length > 10) list.style.maxHeight = `${10 * 30}px`;

    columns.forEach(col => {
      const colTitle = col.getDefinition().title || col.getField();
      const isVisible = col.isVisible();

      const item = document.createElement('label');
      item.className = 'queries-col-chooser-item';

      const checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      checkbox.checked = isVisible;
      checkbox.className = 'queries-col-chooser-checkbox';
      checkbox.addEventListener('change', () => {
        checkbox.checked ? col.show() : col.hide();
        if (this.manager.table) this.manager.table.redraw(true);
      });

      const labelText = document.createElement('span');
      labelText.className = 'queries-col-chooser-label';
      labelText.textContent = colTitle;

      item.appendChild(checkbox);
      item.appendChild(labelText);
      list.appendChild(item);
    });

    popup.appendChild(list);

    const headerEl = column.getElement();
    const tableRect = this.manager.elements.tableContainer.getBoundingClientRect();
    const headerRect = headerEl.getBoundingClientRect();

    popup.style.left = `${headerRect.left - tableRect.left}px`;
    popup.style.top = `${headerRect.bottom - tableRect.top}px`;

    this.manager.elements.tableContainer.appendChild(popup);

    requestAnimationFrame(() => popup.classList.add('visible'));

    this._columnChooserPopup = popup;

    this._columnChooserCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !headerEl.contains(evt.target)) {
        this.closeColumnChooser();
      }
    };
    setTimeout(() => document.addEventListener('click', this._columnChooserCloseHandler), 0);
  }

  /**
   * Close column chooser popup
   */
  closeColumnChooser() {
    if (this._columnChooserPopup) {
      this._columnChooserPopup.remove();
      this._columnChooserPopup = null;
    }
    if (this._columnChooserCloseHandler) {
      document.removeEventListener('click', this._columnChooserCloseHandler);
      this._columnChooserCloseHandler = null;
    }
  }

  // ============ NAV POPUPS ============

  /**
   * Toggle navigation popup
   */
  toggleNavPopup(e, popupId) {
    e.stopPropagation();
    if (this._activeNavPopup && this._activeNavPopupId === popupId) {
      this.closeNavPopup();
      return;
    }
    this.closeNavPopup();

    const btn = e.currentTarget;
    const items = this._getPopupItems(popupId);
    if (!items.length) return;

    const popup = document.createElement('div');
    popup.className = 'queries-nav-popup';

    items.forEach(item => {
      if (item.isSeparator) {
        const separator = document.createElement('div');
        separator.className = 'queries-nav-popup-separator';
        popup.appendChild(separator);
        return;
      }

      if (item.isTemplate) {
        const row = document.createElement('div');
        row.className = 'queries-nav-popup-template-item';

        const labelBtn = document.createElement('button');
        labelBtn.type = 'button';
        labelBtn.className = 'queries-nav-popup-template-label';
        labelBtn.innerHTML = `
          <span class="queries-nav-popup-template-check">${item.isDefault ? '✓' : ''}</span>
          <span class="queries-nav-popup-template-name">${item.label}</span>
          ${item.isDefault ? '<span class="queries-nav-popup-template-default">(default)</span>' : ''}
        `;
        labelBtn.addEventListener('click', () => {
          this.closeNavPopup();
          item.action();
        });
        row.appendChild(labelBtn);

        const deleteBtn = document.createElement('button');
        deleteBtn.type = 'button';
        deleteBtn.className = 'queries-nav-popup-template-delete';
        deleteBtn.title = 'Delete template';
        deleteBtn.innerHTML = '<fa fa-trash-can></fa>';
        deleteBtn.addEventListener('click', (evt) => this.manager.templateManager.deleteTemplate(item.template.name, evt));
        row.appendChild(deleteBtn);

        popup.appendChild(row);
        return;
      }

      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'queries-nav-popup-item';

      if (typeof item.checked === 'boolean') {
        row.innerHTML = `
          <span class="queries-nav-popup-check">${item.checked ? '✓' : ''}</span>
          <span class="queries-nav-popup-label">${item.label}</span>
        `;
      } else if (item.icon) {
        row.innerHTML = `<fa ${item.icon}></fa> ${item.label}`;
      } else {
        row.textContent = item.label;
      }

      row.addEventListener('click', () => {
        this.closeNavPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    processIcons(popup);

    const navRect = this.manager.elements.navigatorContainer.getBoundingClientRect();
    const btnRect = btn.getBoundingClientRect();
    popup.style.left = `${btnRect.left - navRect.left}px`;

    this.manager.elements.navigatorContainer.style.position = 'relative';
    this.manager.elements.navigatorContainer.appendChild(popup);

    popup.getBoundingClientRect();
    popup.classList.add('visible');

    this._activeNavPopup = popup;
    this._activeNavPopupId = popupId;

    this._navPopupCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this.closeNavPopup();
      }
    };
    document.addEventListener('click', this._navPopupCloseHandler);
  }

  /**
   * Get popup items for a specific popup type
   */
  _getPopupItems(popupId) {
    switch (popupId) {
      case 'menu':
        return [
          { label: 'Expand All', action: () => this.manager.handleMenuExpandAll(), icon: 'fa-expand' },
          { label: 'Collapse All', action: () => this.manager.handleMenuCollapseAll(), icon: 'fa-compress' },
        ];
      case 'template':
        return this.manager.templateManager.getTemplatePopupItems();
      case 'width':
        return [
          { label: 'Narrow', checked: this._tableWidthMode === 'narrow', action: () => this.manager.setTableWidth('narrow') },
          { label: 'Compact', checked: this._tableWidthMode === 'compact', action: () => this.manager.setTableWidth('compact') },
          { label: 'Normal', checked: this._tableWidthMode === 'normal', action: () => this.manager.setTableWidth('normal') },
          { label: 'Wide', checked: this._tableWidthMode === 'wide', action: () => this.manager.setTableWidth('wide') },
          { label: 'Auto', checked: this._tableWidthMode === 'auto', action: () => this.manager.setTableWidth('auto') },
        ];
      case 'layout':
        return [
          { label: 'Fit Columns', checked: this._tableLayoutMode === 'fitColumns', action: () => this.manager.setTableLayout('fitColumns') },
          { label: 'Fit Data', checked: this._tableLayoutMode === 'fitData', action: () => this.manager.setTableLayout('fitData') },
          { label: 'Fit Fill', checked: this._tableLayoutMode === 'fitDataFill', action: () => this.manager.setTableLayout('fitDataFill') },
          { label: 'Fit Stretch', checked: this._tableLayoutMode === 'fitDataStretch', action: () => this.manager.setTableLayout('fitDataStretch') },
          { label: 'Fit Table', checked: this._tableLayoutMode === 'fitDataTable', action: () => this.manager.setTableLayout('fitDataTable') },
        ];
      default:
        return [];
    }
  }

  /**
   * Close navigation popup
   */
  closeNavPopup() {
    if (this._activeNavPopup) {
      this._activeNavPopup.remove();
      this._activeNavPopup = null;
      this._activeNavPopupId = null;
    }
    if (this._navPopupCloseHandler) {
      document.removeEventListener('click', this._navPopupCloseHandler);
      this._navPopupCloseHandler = null;
    }
  }

  // ============ FONT POPUP ============

  /**
   * Handle font button click
   */
  handleFontClick() {
    if (this.fontPopup?.classList.contains('visible')) {
      this.hideFontPopup();
    } else {
      this.showFontPopup();
    }
  }

  /**
   * Show font popup
   */
  showFontPopup() {
    if (!this.fontPopup) this.createFontPopup();
    this.fontPopup.classList.add('visible');

    this._fontPopupCloseHandler = (e) => {
      if (!this.fontPopup.contains(e.target) && !this.manager.elements.fontBtn.contains(e.target)) {
        this.hideFontPopup();
      }
    };
    document.addEventListener('click', this._fontPopupCloseHandler);
  }

  /**
   * Hide font popup
   */
  hideFontPopup() {
    if (this.fontPopup) this.fontPopup.classList.remove('visible');
    if (this._fontPopupCloseHandler) {
      document.removeEventListener('click', this._fontPopupCloseHandler);
      this._fontPopupCloseHandler = null;
    }
  }

  /**
   * Create font popup
   */
  createFontPopup() {
    const detectedFonts = this.detectPageFonts();

    this.fontPopup = document.createElement('div');
    this.fontPopup.className = 'queries-font-popup';
    this.fontPopup.innerHTML = `
      <div class="queries-font-popup-row">
        <span class="queries-font-popup-label">Font</span>
        <select class="queries-font-popup-select" id="font-family-select">
          ${detectedFonts.map(f => `
            <option value="${f.value}" ${f.value === this.manager.editorManager.fontSettings.family ? 'selected' : ''}>${f.name}</option>
          `).join('')}
        </select>
      </div>
      <div class="queries-font-popup-row">
        <span class="queries-font-popup-label">Size</span>
        <input type="number" class="queries-font-popup-input" id="font-size-input"
               min="${MIN_FONT_SIZE}" max="${MAX_FONT_SIZE}" value="${this.manager.editorManager.fontSettings.size}">
      </div>
      <div class="queries-font-popup-row">
        <span class="queries-font-popup-label">Style</span>
        <div class="queries-font-popup-styles">
          <button type="button" class="queries-font-popup-style-btn ${this.manager.editorManager.fontSettings.bold ? 'active' : ''}" id="font-bold-btn" title="Bold">B</button>
          <button type="button" class="queries-font-popup-style-btn ${this.manager.editorManager.fontSettings.italic ? 'active' : ''}" id="font-italic-btn" title="Italic">I</button>
        </div>
      </div>
    `;

    this.manager.elements.tabsHeader.style.position = 'relative';
    this.manager.elements.tabsHeader.appendChild(this.fontPopup);

    const fontSelect = this.fontPopup.querySelector('#font-family-select');
    const sizeInput = this.fontPopup.querySelector('#font-size-input');
    const boldBtn = this.fontPopup.querySelector('#font-bold-btn');
    const italicBtn = this.fontPopup.querySelector('#font-italic-btn');

    fontSelect?.addEventListener('change', (e) => {
      this.manager.editorManager.fontSettings.family = e.target.value;
      this.manager.editorManager.applyFontSettings();
    });

    sizeInput?.addEventListener('change', (e) => {
      let size = parseInt(e.target.value, 10);
      size = Math.max(MIN_FONT_SIZE, Math.min(MAX_FONT_SIZE, size));
      this.manager.editorManager.fontSettings.size = size;
      e.target.value = size;
      this.manager.editorManager.applyFontSettings();
    });

    boldBtn?.addEventListener('click', () => {
      this.manager.editorManager.fontSettings.bold = !this.manager.editorManager.fontSettings.bold;
      boldBtn.classList.toggle('active', this.manager.editorManager.fontSettings.bold);
      this.manager.editorManager.applyFontSettings();
    });

    italicBtn?.addEventListener('click', () => {
      this.manager.editorManager.fontSettings.italic = !this.manager.editorManager.fontSettings.italic;
      italicBtn.classList.toggle('active', this.manager.editorManager.fontSettings.italic);
      this.manager.editorManager.applyFontSettings();
    });
  }

  /**
   * Detect available fonts on the page
   */
  detectPageFonts() {
    const fonts = [];
    fonts.push({ name: 'Vanadium Mono', value: '"Vanadium Mono", var(--font-mono, monospace)' });

    const styles = getComputedStyle(document.documentElement);
    const fontMono = styles.getPropertyValue('--font-mono').trim() || 'monospace';
    const fontSans = styles.getPropertyValue('--font-sans').trim() || 'system-ui, sans-serif';
    const fontSerif = styles.getPropertyValue('--font-serif').trim() || 'Georgia, serif';

    fonts.push({ name: 'Monospace', value: fontMono });
    fonts.push({ name: 'Sans Serif', value: fontSans });
    fonts.push({ name: 'Serif', value: fontSerif });

    const commonFonts = [
      { name: 'System UI', value: 'system-ui, -apple-system, sans-serif' },
      { name: 'Arial', value: 'Arial, sans-serif' },
      { name: 'Courier New', value: '"Courier New", monospace' },
      { name: 'Consolas', value: 'Consolas, monospace' },
      { name: 'Fira Code', value: '"Fira Code", monospace' },
      { name: 'JetBrains Mono', value: '"JetBrains Mono", monospace' },
    ];

    commonFonts.forEach(f => fonts.push(f));
    return fonts;
  }

  // ============ SPLITTER ============

  /**
   * Setup splitter for resizing panels
   */
  setupSplitter() {
    const splitter = this.manager.elements.splitter;
    if (!splitter) return;

    this.handleSplitterMouseDown = this.handleSplitterMouseDown.bind(this);
    this.handleSplitterMouseMove = this.handleSplitterMouseMove.bind(this);
    this.handleSplitterMouseUp = this.handleSplitterMouseUp.bind(this);

    splitter.addEventListener('mousedown', this.handleSplitterMouseDown);
  }

  handleSplitterMouseDown(event) {
    event.preventDefault();
    this.isResizing = true;
    this.manager.elements.splitter.classList.add('resizing');
    document.body.style.cursor = 'col-resize';

    if (this.manager.elements.leftPanel) {
      this.manager.elements.leftPanel.style.transition = 'none';
    }

    document.addEventListener('mousemove', this.handleSplitterMouseMove);
    document.addEventListener('mouseup', this.handleSplitterMouseUp);
  }

  handleSplitterMouseMove(event) {
    if (!this.isResizing) return;

    const containerRect = this.manager.elements.container.getBoundingClientRect();
    const newWidth = event.clientX - containerRect.left;

    const minWidth = 157;
    const maxWidth = containerRect.width - 313;
    const constrainedWidth = Math.max(minWidth, Math.min(maxWidth, newWidth));

    this.manager.elements.leftPanel.style.width = `${constrainedWidth}px`;
  }

  handleSplitterMouseUp() {
    this.isResizing = false;
    this.manager.elements.splitter.classList.remove('resizing');
    document.body.style.cursor = '';

    if (this.manager.elements.leftPanel) {
      this.manager.elements.leftPanel.style.transition = '';
      const finalWidth = this.manager.elements.leftPanel.offsetWidth;
      this.savePanelWidth(finalWidth);
      this._tableWidthMode = this.detectWidthMode(finalWidth);
    }

    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
    this.manager.table?.redraw?.();
  }

  /**
   * Save panel width to localStorage
   */
  savePanelWidth(widthPx) {
    try {
      localStorage.setItem(PANEL_WIDTH_KEY, String(Math.round(widthPx)));
    } catch {}
  }

  /**
   * Restore panel width from localStorage
   */
  restorePanelWidth() {
    try {
      const stored = localStorage.getItem(PANEL_WIDTH_KEY);
      if (stored != null) {
        const px = parseInt(stored, 10);
        if (!isNaN(px) && px >= 100) {
          const panel = this.manager.elements.leftPanel;
          if (panel) panel.style.width = `${px}px`;
          this._tableWidthMode = this.detectWidthMode(px);
        }
      }
    } catch {}
  }

  /**
   * Detect width mode from pixel value
   */
  detectWidthMode(px) {
    const presets = { narrow: 160, compact: 314, normal: 468, wide: 622 };
    const tolerance = 8;
    for (const [mode, target] of Object.entries(presets)) {
      if (Math.abs(px - target) <= tolerance) return mode;
    }
    return 'custom';
  }

  /**
   * Save layout mode to localStorage
   */
  saveLayoutMode(mode) {
    try { localStorage.setItem(LAYOUT_MODE_KEY, mode); } catch {}
  }

  /**
   * Restore layout mode from localStorage
   */
  restoreLayoutMode() {
    try {
      const stored = localStorage.getItem(LAYOUT_MODE_KEY);
      if (stored && ['fitColumns', 'fitData', 'fitDataFill', 'fitDataStretch', 'fitDataTable'].includes(stored)) {
        return stored;
      }
    } catch {}
    return null;
  }
}

/**
 * Create a UI manager for the manager
 */
export function createUIManager(manager) {
  return new UIManager(manager);
}
