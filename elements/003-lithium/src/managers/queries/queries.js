/**
 * Queries Manager
 *
 * Query builder and execution interface.
 * Refactored to use modular architecture with JsonTree.js for JSON editing.
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import '../../styles/vendor-tabulator.css';
import 'json-tree.js/dist/jsontree.js.min.css';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import {
  loadColtypes,
  loadTableDef,
  resolveColumns,
  resolveTableOptions,
  getQueryRefs,
  getPrimaryKeyField,
  preloadLookups,
} from '../../core/lithium-table.js';
import { setJsonTreeData } from '../../components/json-tree-component.js';
import './queries.css';

// Import modular components
import { createDirtyStateTracker } from './queries-dirty.js';
import { createEditModeManager } from './queries-edit-mode.js';
import { createTemplateManager } from './queries-templates.js';
import { createNavigationManager } from './queries-navigation.js';
import { createEditorManager } from './queries-editors.js';
import { createUIManager } from './queries-ui.js';

// Constants
const SELECTED_ROW_KEY = 'lithium_queries_selected_id';

/**
 * Queries Manager - Main class
 * Orchestrates the Query Manager functionality using modular architecture
 */
export default class QueriesManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.table = null;

    // Config
    this.tableDef = null;
    this.coltypes = null;
    this.queryRefs = null;
    this.primaryKeyField = 'query_id';

    // Current query data
    this.currentQuery = null;
    this._loadedDetailRowId = null;
    this._pendingSqlContent = null;
    this._pendingSummaryContent = null;
    this._pendingCollectionContent = null;

    // State flags
    this._filtersVisible = false;
    this._tableWidthMode = 'compact';
    this._tableLayoutMode = 'fitColumns';

    // Initialize modular managers
    this.dirtyTracker = createDirtyStateTracker(this);
    this.editModeManager = createEditModeManager(this);
    this.templateManager = createTemplateManager(this);
    this.navManager = createNavigationManager(this);
    this.editorManager = createEditorManager(this);
    this.uiManager = createUIManager(this);

    // Expose editor references for backward compatibility
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.collectionEditor = null;
  }

  // ============ LIFECYCLE ============

  async init() {
    await this._render();
    this._setupEventListeners();
    this.uiManager.setupSplitter();
    this.uiManager.restorePanelWidth();
    await this._initTable();
    this.uiManager.setupFooter();
  }

  async _render() {
    try {
      const response = await fetch('/src/managers/queries/queries.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[QueriesManager] Failed to load template:', error);
      this.container.innerHTML = '<div class="error">Failed to load Queries Manager</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.queries-manager-container'),
      leftPanel: this.container.querySelector('.queries-left-panel'),
      tableContainer: this.container.querySelector('#queries-table-container'),
      navigatorContainer: this.container.querySelector('#queries-navigator-container'),
      splitter: this.container.querySelector('.queries-splitter'),
      rightPanel: this.container.querySelector('.queries-right-panel'),
      tabBtns: this.container.querySelectorAll('.queries-tab-btn:not(.queries-collapse-btn)'),
      tabPanes: this.container.querySelectorAll('.queries-tab-pane'),
      collapseBtn: this.container.querySelector('#queries-collapse-btn'),
      sqlEditorContainer: this.container.querySelector('#queries-sql-editor'),
      summaryEditorContainer: this.container.querySelector('#queries-tab-summary'),
      collectionEditorContainer: this.container.querySelector('#queries-tab-collection'),
      previewContainer: this.container.querySelector('#queries-tab-preview'),
      undoBtn: this.container.querySelector('#queries-undo-btn'),
      redoBtn: this.container.querySelector('#queries-redo-btn'),
      fontBtn: this.container.querySelector('#queries-font-btn'),
      prettifyBtn: this.container.querySelector('#queries-prettify-btn'),
      tabsHeader: this.container.querySelector('.queries-tabs-header'),
    };
  }

  teardown() {
    this.uiManager.closeColumnChooser();
    this.uiManager.closeNavPopup();
    this.uiManager.closeFooterExportPopup();
    this.uiManager.hideFontPopup();

    this.editorManager.destroy();
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.collectionEditor = null;

    if (this.table) {
      this.table.destroy();
      this.table = null;
    }
  }

  // ============ TABLE INITIALIZATION ============

  async _initTable() {
    [this.coltypes, this.tableDef] = await Promise.all([
      loadColtypes(),
      loadTableDef('queries/query-manager'),
    ]);

    if (this.tableDef) {
      this.queryRefs = getQueryRefs(this.tableDef);
      this.primaryKeyField = getPrimaryKeyField(this.tableDef) || 'query_id';

      const lookupRefs = Object.values(this.tableDef.columns || {})
        .map(col => col.lookupRef).filter(Boolean);
      if (lookupRefs.length > 0 && this.app?.api) {
        await preloadLookups([...new Set(lookupRefs)], authQuery, this.app.api);
      }
    }

    const dataColumns = this.tableDef && this.coltypes
      ? resolveColumns(this.tableDef, this.coltypes, { filterEditor: this.uiManager.createFilterEditor.bind(this.uiManager) })
      : this._getFallbackColumns();

    this.editModeManager.applyEditModeGate(dataColumns);

    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: 'collapse' };

    const persistedLayout = this.uiManager.restoreLayoutMode();
    if (persistedLayout) tableOptions.layout = persistedLayout;
    this._tableLayoutMode = tableOptions.layout || 'fitColumns';

    const selectorColumn = this._buildSelectorColumn();

    this.table = new Tabulator(this.elements.tableContainer, {
      ...tableOptions,
      selectableRows: 1,
      scrollToRowPosition: "center",
      scrollToRowIfVisible: false,
      headerSortElement: '<span class="queries-sort-icons"><span class="queries-sort-asc">▲</span><span class="queries-sort-desc">▼</span></span>',
      columns: [selectorColumn, ...dataColumns],
    });

    this._wireTableEvents();
    this.templateManager.applyDefaultTemplate();
    this.elements.tableContainer.setAttribute('tabindex', '0');
    this.elements.tableContainer.addEventListener('keydown', (e) => this._handleTableKeydown(e));

    this.navManager.loadQueries();
  }

  _buildSelectorColumn() {
    return {
      title: "",
      field: "_selector",
      frozen: true,
      width: 16,
      minWidth: 16,
      maxWidth: 16,
      resizable: false,
      headerSort: false,
      hozAlign: "center",
      vertAlign: "middle",
      cssClass: "queries-selector-col",
      titleFormatter: () => {
        const wrapper = document.createElement('span');
        wrapper.className = 'queries-col-chooser-btn';
        wrapper.title = 'Column Chooser';
        wrapper.innerHTML = '<fa fa-ellipsis-stroke-vertical>';
        return wrapper;
      },
      formatter: (cell) => {
        const row = cell.getRow();
        const pkField = this.primaryKeyField;
        if (row.isSelected()) {
          if (this.editModeManager.isEditing() && this.editModeManager.getEditingRowId() === row.getData()[pkField]) {
            return '<fa fa-i-cursor fa-swap-opacity queries-selector-indicator queries-selector-edit>';
          }
          return '<fa fa-caret-right fa-swap-opacity queries-selector-indicator queries-selector-active>';
        }
        return '';
      },
      headerClick: (e, column) => this.uiManager.toggleColumnChooser(e, column),
      cellClick: (e, cell) => {
        e?.stopPropagation?.();
        this.navManager._selectDataRow(cell.getRow());
      },
    };
  }

  _getFallbackColumns() {
    return [
      { title: "Ref", field: "query_ref", width: 80, resizable: true, headerSort: true,
        headerFilter: this.uiManager.createFilterEditor.bind(this.uiManager), headerFilterFunc: "like", bottomCalc: "count" },
      { title: "Name", field: "name", resizable: true, headerSort: true,
        headerFilter: this.uiManager.createFilterEditor.bind(this.uiManager), headerFilterFunc: "like" },
    ];
  }

  // ============ EVENT HANDLERS ============

  _setupEventListeners() {
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => this.switchTab(btn.dataset.tab));
    });

    this.elements.collapseBtn?.addEventListener('click', () => this.uiManager.toggleCollapse());
    this.elements.undoBtn?.addEventListener('click', () => this.editorManager.handleUndo());
    this.elements.redoBtn?.addEventListener('click', () => this.editorManager.handleRedo());
    this.elements.fontBtn?.addEventListener('click', () => this.uiManager.handleFontClick());
    this.elements.prettifyBtn?.addEventListener('click', () => this.editorManager.handlePrettify());

    this.buildNavigator();
  }

  // ============ NAVIGATOR ============

  buildNavigator() {
    const nav = this.elements.navigatorContainer;
    if (!nav) return;

    nav.innerHTML = this._getNavigatorHTML();
    processIcons(nav);
    this._wireNavigatorButtons(nav);
  }

  _getNavigatorHTML() {
    return `
      <div class="queries-nav-wrapper">
        <div class="queries-nav-block queries-nav-block-control">
          <button type="button" class="queries-nav-btn" id="queries-nav-refresh" title="Refresh"><fa fa-arrows-rotate></fa></button>
          <button type="button" class="queries-nav-btn ${this._filtersVisible ? 'queries-nav-btn-active' : ''}" id="queries-nav-filter" title="Toggle Filters"><fa fa-filter></fa></button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-menu" title="Table Options"><fa fa-layer-group></fa></button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-width" title="Table Width"><fa fa-left-right></fa></button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-layout" title="Table Layout"><fa fa-table-columns></fa></button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-template" title="Templates"><fa fa-screwdriver-wrench></fa></button>
        </div>
        <div class="queries-nav-block queries-nav-block-move">
          <button type="button" class="queries-nav-btn" id="queries-nav-first" title="First Record"><fa fa-backward-fast></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-pgup" title="Previous Page"><fa fa-backward-step></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-prev" title="Previous Record"><fa fa-caret-left></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-next" title="Next Record"><fa fa-caret-right></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-pgdn" title="Next Page"><fa fa-forward-step></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-last" title="Last Record"><fa fa-forward-fast></fa></button>
        </div>
        <div class="queries-nav-block queries-nav-block-manage">
          <button type="button" class="queries-nav-btn" id="queries-nav-add" title="Add"><fa fa-plus></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-duplicate" title="Duplicate"><fa fa-copy></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-edit" title="Edit"><fa fa-pen-to-square></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-save" title="Save" disabled><fa fa-floppy-disk></fa></button>
          <button type="button" class="queries-nav-btn" id="queries-nav-cancel" title="Cancel" disabled><fa fa-ban></fa></button>
          <button type="button" class="queries-nav-btn queries-nav-btn-danger" id="queries-nav-delete" title="Delete"><fa fa-trash-can></fa></button>
        </div>
        <div class="queries-nav-block queries-nav-block-search">
          <button type="button" class="queries-nav-btn" id="queries-search-btn" title="Search"><fa fa-magnifying-glass></fa></button>
          <input type="text" class="queries-nav-search-input" id="queries-search-input" placeholder="Search...">
          <button type="button" class="queries-nav-btn" id="queries-search-clear-btn" title="Clear Search"><fa fa-xmark></fa></button>
        </div>
      </div>
    `;
  }

  _wireNavigatorButtons(nav) {
    // Control
    nav.querySelector('#queries-nav-refresh')?.addEventListener('click', () => this.navManager.handleNavRefresh());
    nav.querySelector('#queries-nav-filter')?.addEventListener('click', () => this.handleNavFilter());
    nav.querySelector('#queries-nav-menu')?.addEventListener('click', (e) => this.uiManager.toggleNavPopup(e, 'menu'));
    nav.querySelector('#queries-nav-width')?.addEventListener('click', (e) => this.uiManager.toggleNavPopup(e, 'width'));
    nav.querySelector('#queries-nav-layout')?.addEventListener('click', (e) => this.uiManager.toggleNavPopup(e, 'layout'));
    nav.querySelector('#queries-nav-template')?.addEventListener('click', (e) => this.uiManager.toggleNavPopup(e, 'template'));

    // Move
    nav.querySelector('#queries-nav-first')?.addEventListener('click', () => this.navManager.navigateFirst());
    nav.querySelector('#queries-nav-pgup')?.addEventListener('click', () => this.navManager.navigatePrevPage());
    nav.querySelector('#queries-nav-prev')?.addEventListener('click', () => this.navManager.navigatePrevRec());
    nav.querySelector('#queries-nav-next')?.addEventListener('click', () => this.navManager.navigateNextRec());
    nav.querySelector('#queries-nav-pgdn')?.addEventListener('click', () => this.navManager.navigateNextPage());
    nav.querySelector('#queries-nav-last')?.addEventListener('click', () => this.navManager.navigateLast());

    // Manage
    nav.querySelector('#queries-nav-add')?.addEventListener('click', () => this.navManager.handleNavAdd());
    nav.querySelector('#queries-nav-duplicate')?.addEventListener('click', () => this.navManager.handleNavDuplicate());
    nav.querySelector('#queries-nav-edit')?.addEventListener('click', () => this.navManager.handleNavEdit());
    nav.querySelector('#queries-nav-save')?.addEventListener('click', () => this.navManager.handleNavSave());
    nav.querySelector('#queries-nav-cancel')?.addEventListener('click', () => this.navManager.handleNavCancel());
    nav.querySelector('#queries-nav-delete')?.addEventListener('click', () => this.navManager.handleNavDelete());

    // Search
    const searchInput = nav.querySelector('#queries-search-input');
    nav.querySelector('#queries-search-btn')?.addEventListener('click', () => this.navManager.loadQueries(searchInput.value));
    searchInput?.addEventListener('keypress', (e) => { if (e.key === 'Enter') this.navManager.loadQueries(searchInput.value); });
    nav.querySelector('#queries-search-clear-btn')?.addEventListener('click', () => { searchInput.value = ''; this.navManager.loadQueries(); });
  }

  // ============ TAB SWITCHING ============

  async switchTab(tabId) {
    this.elements.tabBtns.forEach(btn => btn.classList.toggle('active', btn.dataset.tab === tabId));
    this.elements.tabPanes.forEach(pane => pane.classList.toggle('active', pane.id === `queries-tab-${tabId}`));

    const isSqlTab = tabId === 'sql';
    if (this.elements.undoBtn) this.elements.undoBtn.disabled = !isSqlTab;
    if (this.elements.redoBtn) this.elements.redoBtn.disabled = !isSqlTab;
    if (this.elements.fontBtn) this.elements.fontBtn.disabled = !['sql', 'summary', 'preview'].includes(tabId);
    if (this.elements.prettifyBtn) this.elements.prettifyBtn.disabled = !isSqlTab;

    if (tabId === 'sql') {
      const sqlContent = this._pendingSqlContent || this.currentQuery?.code || this.currentQuery?.query_text || this.currentQuery?.sql || '';
      if (!this.sqlEditor) {
        this.editorManager.initSqlEditor(sqlContent).then(() => {
          this.sqlEditor = this.editorManager.sqlEditor;
        });
      }
    }
    if (tabId === 'summary') {
      const summaryContent = this._pendingSummaryContent || this.currentQuery?.summary || this.currentQuery?.markdown || '';
      if (!this.summaryEditor) {
        this.editorManager.initSummaryEditor(summaryContent).then(() => {
          this.summaryEditor = this.editorManager.summaryEditor;
        });
      }
    }
    if (tabId === 'collection') {
      const collectionContent = this._pendingCollectionContent || this.currentQuery?.collection || this.currentQuery?.json || {};
      if (!this.collectionEditor) {
        this.editorManager.initCollectionEditor(collectionContent).then(() => {
          this.collectionEditor = this.editorManager.collectionEditor;
        });
      } else {
        // Update existing editor with new content
        const { setJsonTreeData } = await import('../../components/json-tree-component.js');
        setJsonTreeData(this.elements.collectionEditorContainer, collectionContent);
      }
    }
    if (tabId === 'preview') {
      this.editorManager.renderMarkdownPreview();
    }
  }

  // ============ NAVIGATOR HANDLERS (delegated to navManager) ============

  handleNavFilter() {
    this._filtersVisible = !this._filtersVisible;
    this.uiManager._filtersVisible = this._filtersVisible;
    
    // Use Tabulator's setHeaderFilterVisibility if available, otherwise toggle
    if (this.table?.setHeaderFilterVisibility) {
      this.table.setHeaderFilterVisibility(this._filtersVisible);
    } else if (this.table?.toggleHeaderFilter) {
      this.table.toggleHeaderFilter();
    }
    
    // Force redraw to ensure filter row appears/disappears
    this.table?.redraw?.();
    
    const btn = this.elements.navigatorContainer?.querySelector('#queries-nav-filter');
    if (btn) btn.classList.toggle('queries-nav-btn-active', this._filtersVisible);
  }

  handleMenuExpandAll() {
    this.table?.getRows().forEach(row => row.treeExpand?.());
  }

  handleMenuCollapseAll() {
    this.table?.getRows().forEach(row => row.treeCollapse?.());
  }

  // ============ TABLE SETTINGS ============

  setTableWidth(mode) {
    const panel = this.elements.leftPanel;
    if (!panel) return;

    const widths = { narrow: 160, compact: 314, normal: 468, wide: 622 };

    this._tableWidthMode = mode;

    if (mode === 'auto') {
      this._applyAutoWidth(panel);
      return;
    }

    panel.style.width = `${widths[mode]}px`;
    this.uiManager.savePanelWidth(parseInt(panel.style.width, 10));
    log(Subsystems.MANAGER, Status.INFO, `Table width set to: ${mode} (${panel.style.width})`);
  }

  setTableLayout(mode) {
    if (!this.table) return;

    this._tableLayoutMode = mode;
    this.uiManager.saveLayoutMode(mode);

    const currentData = this.table.getData();
    const selectedId = this.navManager._getSelectedQueryId();

    this.table.destroy();
    this.table = null;

    const dataColumns = this.tableDef && this.coltypes
      ? resolveColumns(this.tableDef, this.coltypes, { filterEditor: this.uiManager.createFilterEditor.bind(this.uiManager) })
      : this._getFallbackColumns();

    this.editModeManager.applyEditModeGate(dataColumns);

    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: 'collapse' };
    tableOptions.layout = mode;

    const selectorColumn = this._buildSelectorColumn();

    this.table = new Tabulator(this.elements.tableContainer, {
      ...tableOptions,
      selectableRows: 1,
      scrollToRowPosition: "center",
      scrollToRowIfVisible: false,
      headerSortElement: '<span class="queries-sort-icons"><span class="queries-sort-asc">▲</span><span class="queries-sort-desc">▼</span></span>',
      columns: [selectorColumn, ...dataColumns],
    });

    this._wireTableEvents();

    this.table.on("tableBuilt", () => {
      this.table.setData(currentData);
      this.navManager._discoverColumns(currentData);
      requestAnimationFrame(() => {
        this.navManager._autoSelectRow(selectedId);
        this.navManager._updateMoveButtonState();
      });

      if (this._tableWidthMode === 'auto') {
        requestAnimationFrame(() => {
          requestAnimationFrame(() => this._applyAutoWidth(this.elements.leftPanel));
        });
      }
    });

    log(Subsystems.MANAGER, Status.INFO, `Table layout set to: ${mode} (table recreated, ${currentData.length} rows preserved)`);
  }

  _applyAutoWidth(panel) {
    if (!this.table) {
      panel.style.width = '314px';
      this.uiManager.savePanelWidth(314);
      return;
    }

    const currentLayout = this._tableLayoutMode || 'fitColumns';
    const needsLayoutSwitch = (currentLayout === 'fitColumns');

    panel.style.transition = 'none';

    if (needsLayoutSwitch) {
      panel.style.width = '2000px';
      this.table.options.layout = 'fitDataTable';
      this.table.redraw(true);
    }

    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        let totalColWidth = 0;
        this.table.getColumns().forEach(col => {
          if (col.isVisible()) totalColWidth += col.getWidth();
        });

        if (totalColWidth === 0) {
          const tableEl = this.elements.tableContainer?.querySelector('.tabulator-table');
          totalColWidth = tableEl?.scrollWidth || tableEl?.offsetWidth || 300;
        }

        const panelWidth = Math.max(160, totalColWidth + 28);
        panel.style.width = `${panelWidth}px`;

        if (needsLayoutSwitch) {
          this.table.options.layout = currentLayout;
          this.table.redraw(true);
        }

        requestAnimationFrame(() => panel.style.transition = '');
        this.uiManager.savePanelWidth(panelWidth);
        log(Subsystems.MANAGER, Status.INFO, `Table width set to: auto (${panelWidth}px, measured ${totalColWidth}px from columns)`);
      });
    });
  }

  // ============ KEYBOARD HANDLING ============

  _handleTableKeydown(e) {
    if (this.editModeManager.isEditing()) return;

    switch (e.key) {
      case 'ArrowUp':
        e.preventDefault();
        this.navManager.navigatePrevRec();
        break;
      case 'ArrowDown':
        e.preventDefault();
        this.navManager.navigateNextRec();
        break;
      case 'PageUp':
        e.preventDefault();
        this.navManager.navigatePrevPage();
        break;
      case 'PageDown':
        e.preventDefault();
        this.navManager.navigateNextPage();
        break;
      case 'Home':
        e.preventDefault();
        this.navManager.navigateFirst();
        break;
      case 'End':
        e.preventDefault();
        this.navManager.navigateLast();
        break;
      case 'Enter':
        e.preventDefault();
        if (this.table) {
          const selected = this.table.getSelectedRows();
          if (selected.length > 0) this.editModeManager.enterEditMode(selected[0]);
        }
        break;
    }
  }

  // ============ TABLE EVENTS ============

  _wireTableEvents() {
    if (!this.table) return;

    this._bindRowClickEvents();
    this._bindCellClickEvents();
    this._bindEditEvents();
    this._bindSelectionEvents();
  }

  _bindRowClickEvents() {
    this.table.on("rowClick", (e, row) => {
      if (this.editModeManager._isCalcRow(row)) return;
      this.navManager._selectDataRow(row);
    });

    this.table.on("cellMouseDown", (e, cell) => {
      if (cell.getField() === '_selector' || this.editModeManager._isCalcCell(cell)) return;
      this.navManager._selectDataRow(cell.getRow());
    });

    this.table.on("rowDblClick", (e, row) => {
      if (this.editModeManager._isCalcRow(row)) return;

      const targetCellEl = e.target?.closest?.('.tabulator-cell');
      let targetCell = null;

      if (targetCellEl) {
        const field = targetCellEl.getAttribute('tabulator-field');
        if (field && field !== '_selector') {
          targetCell = row.getCell(field);
        }
      }

      this.navManager._selectDataRow(row);
      void this.editModeManager.enterEditMode(row, targetCell);
    });
  }

  _bindCellClickEvents() {
    this.table.on("cellClick", (e, cell) => {
      const field = cell.getField();
      if (field === '_selector' || this.editModeManager._isCalcCell(cell)) return;

      e?.stopPropagation?.();
      e?.stopImmediatePropagation?.();

      this.navManager._selectDataRow(cell.getRow());

      if (this.editModeManager.isEditing()) {
        this.editModeManager._commitActiveCellEdit(cell);
        this.editModeManager._queueCellEdit(cell);
      }
    });
  }

  _bindEditEvents() {
    this.table.on("cellEdited", () => {
      this.editModeManager.handleCellEdited();
    });

    this.table.on("cellEditing", (cell) => {
      this.editModeManager.attachInputListener(cell);
    });
  }

  _bindSelectionEvents() {
    this.table.on("rowSelected", async (row) => {
      if (this.editModeManager._isCalcRow(row)) return;
      this.uiManager.closeColumnChooser();
      this.uiManager.closeNavPopup();
      await this._handleRowSelected(row);
    });

    this.table.on("rowDeselected", (row) => {
      if (this.editModeManager._isCalcRow(row)) return;
      const cell = row.getCell('_selector');
      if (cell) {
        const el = cell.getElement();
        if (el) el.innerHTML = '';
      }
    });

    this.table.on("rowSelectionChanged", (data) => {
      if (data.length > 0) {
        const pkField = this.primaryKeyField || 'query_id';
        const selectedId = data[0]?.[pkField];

        if (this.editModeManager.isEditing() && this.editModeManager.getEditingRowId() != null &&
            String(this.editModeManager.getEditingRowId()) === String(selectedId)) {
          return;
        }

        if (selectedId != null && String(this._loadedDetailRowId) !== String(selectedId)) {
          this.navManager.loadQueryDetails(data[0]);
        }
      }

      this.navManager._updateMoveButtonState();
    });
  }

  async _handleRowSelected(row) {
    const pkField = this.primaryKeyField || 'query_id';
    const newRowId = row.getData()?.[pkField];
    const isSameRow = (this.editModeManager.getEditingRowId() != null)
      ? newRowId === this.editModeManager.getEditingRowId()
      : newRowId == null;

    if (this.editModeManager.isEditing() && isSameRow) {
      this.editModeManager._updateEditingIndicator();
      return;
    }

    if (this.editModeManager.isEditing() && this.dirtyTracker.isAnyDirty()) {
      const saveSucceeded = await this.navManager._autoSaveBeforeRowChange();
      if (!saveSucceeded) {
        const editingRow = this.editModeManager.getEditingRow();
        if (editingRow) editingRow.select();
        return;
      }
      await this.editModeManager.exitEditMode('save');
    } else if (this.editModeManager.isEditing()) {
      await this.editModeManager.exitEditMode('row-change');
    }

    this.editModeManager._updateEditingIndicator();

    const pkValue = row.getData()?.[pkField];
    if (pkValue != null) {
      try { localStorage.setItem(SELECTED_ROW_KEY, String(pkValue)); } catch {}
    }
  }

  // ============ UI HELPERS ============

  _updateSaveCancelButtonState() {
    const nav = this.elements.navigatorContainer;
    if (!nav) return;
    const shouldEnable = this.editModeManager.isEditing() && this.dirtyTracker.isAnyDirty();
    const saveBtn = nav.querySelector('#queries-nav-save');
    const cancelBtn = nav.querySelector('#queries-nav-cancel');
    if (saveBtn) saveBtn.disabled = !shouldEnable;
    if (cancelBtn) cancelBtn.disabled = !shouldEnable;
  }

  _setCodeMirrorEditable(editable) {
    this.editorManager._setCodeMirrorEditable(editable);
  }

  // ============ NAVIGATOR HANDLERS (exposed for navManager) ============

  async handleNavSave() {
    return this.navManager.handleNavSave();
  }

  // ============ PROXY METHODS FOR BACKWARD COMPATIBILITY ============

  // Dirty state tracker proxies - use a backing object for test compatibility
  get _originalData() {
    // Return a proxy that allows both reading and setting properties
    const tracker = this.dirtyTracker;
    return {
      get row() { return tracker._originalRowData; },
      set row(val) { tracker._originalRowData = val; },
      get sql() { return tracker._originalSqlContent; },
      set sql(val) { tracker._originalSqlContent = val; },
      get summary() { return tracker._originalSummaryContent; },
      set summary(val) { tracker._originalSummaryContent = val; },
      get collection() { return tracker._originalCollectionContent; },
      set collection(val) { tracker._originalCollectionContent = val; },
    };
  }
  set _originalData(val) {
    // Allow tests to set individual properties on _originalData
    if (val) {
      if (val.row !== undefined) this.dirtyTracker._originalRowData = val.row;
      if (val.sql !== undefined) this.dirtyTracker._originalSqlContent = val.sql;
      if (val.summary !== undefined) this.dirtyTracker._originalSummaryContent = val.summary;
      if (val.collection !== undefined) this.dirtyTracker._originalCollectionContent = val.collection;
    }
  }
  get _isDirty() { return this.dirtyTracker._isDirty; }
  set _isDirty(val) { this.dirtyTracker._isDirty = val; }
  _isAnyDirty() { return this.dirtyTracker.isAnyDirty(); }
  _setDirty(type, isDirty) { return this.dirtyTracker.setDirty(type, isDirty); }
  _markAllClean() { return this.dirtyTracker.markAllClean(); }
  _captureOriginalData(data) { return this.dirtyTracker.captureOriginalData(data); }
  _revertAllChanges() { return this.dirtyTracker.revertAllChanges(); }
  _getCurrentSqlContent() { return this.dirtyTracker.getCurrentSqlContent(); }
  _getCurrentSummaryContent() { return this.dirtyTracker.getCurrentSummaryContent(); }
  _getCurrentCollectionContent() { return this.dirtyTracker.getCurrentCollectionContent(); }
  _checkSqlDirty() { return this.dirtyTracker.checkSqlDirty(); }
  _checkSummaryDirty() { return this.dirtyTracker.checkSummaryDirty(); }
  _checkCollectionDirty() { return this.dirtyTracker.checkCollectionDirty(); }
  _refreshDirtyState() { return this.dirtyTracker.refreshDirtyState(); }

  // Edit mode manager proxies
  get _isEditing() { return this.editModeManager._isEditing; }
  set _isEditing(val) { this.editModeManager._isEditing = val; }
  get _editingRowId() { return this.editModeManager._editingRowId; }
  set _editingRowId(val) { this.editModeManager._editingRowId = val; }
  get _editTransitionPending() { return this.editModeManager._editTransitionPending; }
  set _editTransitionPending(val) { this.editModeManager._editTransitionPending = val; }
  get _pendingCellEditToken() { return this.editModeManager._pendingCellEditToken; }
  set _pendingCellEditToken(val) { this.editModeManager._pendingCellEditToken = val; }
  get _columnEditors() { return this.editModeManager._columnEditors; }
  set _columnEditors(val) { this.editModeManager._columnEditors = val; }
  async _enterEditMode(row, targetCell) { return this.editModeManager.enterEditMode(row, targetCell); }
  async _exitEditMode(reason) { return this.editModeManager.exitEditMode(reason); }
  _setEditTransitionState(pending) { return this.editModeManager._setEditTransitionState(pending); }
  _updateEditingIndicator() { return this.editModeManager._updateEditingIndicator(); }
  _applyEditModeGate(columns) { return this.editModeManager.applyEditModeGate(columns); }
  _cancelActiveCellEdit() { return this.editModeManager._cancelActiveCellEdit(); }
  _commitActiveCellEdit(nextCell) { return this.editModeManager._commitActiveCellEdit(nextCell); }
  _isCalcRow(row) { return this.editModeManager._isCalcRow(row); }
  _isCalcCell(cell) { return this.editModeManager._isCalcCell(cell); }
  _queueCellEdit(cell) { return this.editModeManager._queueCellEdit(cell); }
  _getEditingRow() { return this.editModeManager.getEditingRow(); }

  // Navigation manager proxies
  loadQueries(searchTerm) { return this.navManager.loadQueries(searchTerm); }
  loadQueryDetails(queryData) { return this.navManager.loadQueryDetails(queryData); }
  async fetchQueryDetails(queryId) { return this.navManager.fetchQueryDetails(queryId); }
  _getSelectedQueryId() { return this.navManager._getSelectedQueryId(); }
  _autoSelectRow(previousId) { return this.navManager._autoSelectRow(previousId); }
  _discoverColumns(rows) { return this.navManager._discoverColumns(rows); }
  handleNavRefresh() { return this.navManager.handleNavRefresh(); }
  handleNavAdd() { return this.navManager.handleNavAdd(); }
  handleNavDuplicate() { return this.navManager.handleNavDuplicate(); }
  handleNavEdit() { return this.navManager.handleNavEdit(); }
  handleNavCancel() { return this.navManager.handleNavCancel(); }
  handleNavDelete() { return this.navManager.handleNavDelete(); }
  navigateFirst() { return this.navManager.navigateFirst(); }
  navigateLast() { return this.navManager.navigateLast(); }
  navigatePrevRec() { return this.navManager.navigatePrevRec(); }
  navigateNextRec() { return this.navManager.navigateNextRec(); }
  navigatePrevPage() { return this.navManager.navigatePrevPage(); }
  navigateNextPage() { return this.navManager.navigateNextPage(); }
  async _checkSaveBeforeNav() { return this.navManager._checkSaveBeforeNav(); }
  async _autoSaveBeforeRowChange() { return this.navManager._autoSaveBeforeRowChange(); }
  _getVisibleRowsAndIndex() { return this.navManager._getVisibleRowsAndIndex(); }
  _selectRowByIndex(index) { return this.navManager._selectRowByIndex(index); }
  _selectDataRow(row) { return this.navManager._selectDataRow(row); }
  _updateMoveButtonState() { return this.navManager._updateMoveButtonState(); }
  _showTableLoading() { return this.navManager._showTableLoading(); }
  _hideTableLoading() { return this.navManager._hideTableLoading(); }

  // UI manager proxies
  toggleCollapse() { return this.uiManager.toggleCollapse(); }
  _createFilterEditor(cell, onRendered, success, cancel, editorParams) {
    return this.uiManager.createFilterEditor(cell, onRendered, success, cancel, editorParams);
  }
  _setupFooter() { return this.uiManager.setupFooter(); }
  _getFooterDatasource() { return this.uiManager.getFooterDatasource(); }
  _handleFooterPrint() { return this.uiManager.handleFooterPrint(); }
  _handleFooterEmail() { return this.uiManager.handleFooterEmail(); }
  _toggleFooterExportPopup(e) { return this.uiManager.toggleFooterExportPopup(e); }
  _closeFooterExportPopup() { return this.uiManager.closeFooterExportPopup(); }
  _handleFooterExport(format) { return this.uiManager.handleFooterExport(format); }
  toggleColumnChooser(e, column) { return this.uiManager.toggleColumnChooser(e, column); }
  _closeColumnChooser() { return this.uiManager.closeColumnChooser(); }
  toggleNavPopup(e, popupId) { return this.uiManager.toggleNavPopup(e, popupId); }
  _closeNavPopup() { return this.uiManager.closeNavPopup(); }
  handleFontClick() { return this.uiManager.handleFontClick(); }
  showFontPopup() { return this.uiManager.showFontPopup(); }
  hideFontPopup() { return this.uiManager.hideFontPopup(); }
  createFontPopup() { return this.uiManager.createFontPopup(); }
  detectPageFonts() { return this.uiManager.detectPageFonts(); }
  _setupSplitter() { return this.uiManager.setupSplitter(); }
  _savePanelWidth(widthPx) { return this.uiManager.savePanelWidth(widthPx); }
  _restorePanelWidth() { return this.uiManager.restorePanelWidth(); }
  _saveLayoutMode(mode) { return this.uiManager.saveLayoutMode(mode); }
  _restoreLayoutMode() { return this.uiManager.restoreLayoutMode(); }

  // Editor manager proxies
  initSqlEditor(initialContent) {
    return this.editorManager.initSqlEditor(initialContent).then(() => {
      this.sqlEditor = this.editorManager.sqlEditor;
    });
  }
  initSummaryEditor(initialContent) {
    return this.editorManager.initSummaryEditor(initialContent).then(() => {
      this.summaryEditor = this.editorManager.summaryEditor;
    });
  }
  initCollectionEditor(initialContent) {
    return this.editorManager.initCollectionEditor(initialContent).then(() => {
      this.collectionEditor = this.editorManager.collectionEditor;
    });
  }
  renderMarkdownPreview() { return this.editorManager.renderMarkdownPreview(); }
  handleUndo() { return this.editorManager.handleUndo(); }
  handleRedo() { return this.editorManager.handleRedo(); }
  handlePrettify() { return this.editorManager.handlePrettify(); }
  applyFontSettings(settings) { return this.editorManager.applyFontSettings(settings); }

  // Template manager proxies
  _loadTemplates() { return this.templateManager.loadTemplates(); }
  _saveTemplates(templates) { return this.templateManager.saveTemplates(templates); }
  _loadDefaultTemplateName() { return this.templateManager.loadDefaultTemplateName(); }
  _saveDefaultTemplateName(name) { return this.templateManager.saveDefaultTemplateName(name); }
  _deleteTemplate(name, e) { return this.templateManager.deleteTemplate(name, e); }
  _showSaveTemplateDialog() { return this.templateManager.showSaveTemplateDialog(); }
  _saveTemplate(name) { return this.templateManager.saveTemplate(name); }
  _showMakeDefaultDialog() { return this.templateManager.showMakeDefaultDialog(); }
  _applyTemplate(template) { return this.templateManager.applyTemplate(template); }
  _validateTemplate(template) { return this.templateManager._validateTemplate(template); }
  _applyTemplateLayout(template) { return this.templateManager._applyTemplateLayout(template); }
  _applyTemplateColumns(template) { return this.templateManager._applyTemplateColumns(template); }
  _reorderColumns(sortedColumns) { return this.templateManager._reorderColumns(sortedColumns); }
  _applyTemplateFilters(template) { return this.templateManager._applyTemplateFilters(template); }
  _handleTemplateError(err) { return this.templateManager._handleTemplateError(err); }
  _detectWidthMode(px) { return this.templateManager._detectWidthMode(px); }
  _applyDefaultTemplate() { return this.templateManager.applyDefaultTemplate(); }
  _getTemplatePopupItems() { return this.templateManager.getTemplatePopupItems(); }
}
