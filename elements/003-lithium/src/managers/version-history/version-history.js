/**
 * Version History Manager — Manager ID 11
 *
 * A two-panel interface for viewing document version history:
 * - Left panel: List of versions from Lookup 44 (Server) and 45 (Client)
 * - Right panel: Markdown preview of the selected version's summary
 *
 * Uses:
 * - LithiumTable for the version list (combined Lookup 44 + 45)
 * - QueryRef 26 with LOOKUPID for loading lookup data
 * - Markdown rendering for the summary field
 *
 * @module managers/version-history
 */

import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse, restorePanelState as restoreCollapsedPanelState } from '../../core/panel-collapse.js';
import '../../styles/vendor-tabulator.css';
import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { expandMacros } from '../../core/macro-expansion.js';
import { getMacros } from '../../shared/lookups.js';
import '../../core/manager-panels.css';
import './version-history.css';
import { setupManagerFooterIcons, closeExportPopup, initToolbars } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';

// Dynamic imports
let marked;

// Constants
const SELECTED_VERSION_KEY = 'lithium_version_history_selected_id';

// ── VersionHistoryManager Class ────────────────────────────────────────────

export default class VersionHistoryManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // Version History Manager ID = 11
    this.managerId = 11;

    // Main table (version list)
    this.versionsTable = null;

    // Currently selected version
    this.selectedVersionId = null;
    this.selectedVersionData = null;

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_version_left');

    // Splitter (loaded from persistence)
    this.splitter = null;
    this.leftPanelWidth = this.leftPanelState.loadWidth(350);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    // Table state
    this._tableWidthMode = 'compact';
    this._filtersVisible = false;

    // Prevent duplicate detail loading calls
    this.loadingDetails = false;

    // Edit helper — consolidates edit mode, dirty tracking, and save/cancel buttons
    this.editHelper = new ManagerEditHelper({ name: 'VersionHistory' });
  }

  async init() {
    await this.loadDependencies();
    await this.render();
    this.setupEventListeners();
    await this.initVersionsTable();
    this.setupFooter();
    this.restorePanelState();
  }

  /**
   * Load marked.js for Markdown rendering
   */
  async loadDependencies() {
    try {
      const markedModule = await import('marked');
      marked = markedModule.marked;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[VersionHistory] Failed to load dependencies: ${error.message}`);
    }
  }

  async render() {
    try {
      const response = await fetch('/src/managers/version-history/version-history.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[VersionHistory] Failed to load template: ${error.message}`);
      this.container.innerHTML = '<div class="error">Failed to load Version History</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.version-manager-container'),
      leftPanel: this.container.querySelector('#version-left-panel'),
      rightPanel: this.container.querySelector('#version-right-panel'),
      versionsTableContainer: this.container.querySelector('#version-table-container'),
      versionsNavigator: this.container.querySelector('#version-navigator'),
      splitter: this.container.querySelector('#version-splitter'),
      collapseBtn: this.container.querySelector('#version-collapse-btn'),
      collapseIcon: this.container.querySelector('#version-collapse-icon'),
      previewContent: this.container.querySelector('#version-preview-content'),
    };

    processIcons(this.container);
    initToolbars();
  }

  setupEventListeners() {
    // Collapse button
    this.elements.collapseBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });
  }

  // ── Versions Table ────────────────────────────────────────────────────────

  async initVersionsTable() {
    if (!this.elements.versionsTableContainer || !this.elements.versionsNavigator) return;

    this.versionsTable = new LithiumTable({
      container: this.elements.versionsTableContainer,
      navigatorContainer: this.elements.versionsNavigator,
      tablePath: 'version-manager/version-history',
      cssPrefix: 'version-history',
      storageKey: 'version_history_table',
      app: this.app,
      readonly: true,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this.handleVersionSelected(rowData),
      onRowDeselected: () => this.handleVersionDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[VersionHistory] Loaded ${rows.length} versions`);
      },
      onRefresh: () => this.loadVersionList(),
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.versionsTable);

    await this.versionsTable.init();

    // Initialize splitter after table is ready
    this.setupSplitter();

    // Load data
    await this.loadVersionList();
  }

  /**
   * Wire up navigator buttons for custom behavior
   */
  wireNavigatorButtons() {
    const nav = this.elements.versionsNavigator;
    if (!nav) return;

    // Refresh button - reload the version list
    nav.querySelector('#lithium-nav-refresh')?.addEventListener('click', () => {
      this.loadVersionList();
    });

    // Filter button - toggle filter row visibility
    nav.querySelector('#lithium-nav-filter')?.addEventListener('click', () => {
      this.toggleFilters();
    });
  }

  /**
   * Toggle header filter row visibility
   */
  toggleFilters() {
    this._filtersVisible = !this._filtersVisible;
    const container = this.elements.versionsTableContainer;
    if (!container) return;

    if (this._filtersVisible) {
      container.classList.add('lithium-filters-visible');
    } else {
      container.classList.remove('lithium-filters-visible');
    }

    // Update button state
    const filterBtn = this.elements.versionsNavigator?.querySelector('#lithium-nav-filter');
    if (filterBtn) {
      filterBtn.classList.toggle('lithium-nav-btn-active', this._filtersVisible);
    }

    // Redraw table to adjust
    this.versionsTable?.table?.redraw?.();
  }

  // ── Table Width Control ────────────────────────────────────────────────────
  // Width persistence is now handled centrally by LithiumTable.
  // The Width popup in the Navigator calls LithiumTable.setTableWidth() directly,
  // which saves the mode to localStorage and applies the width to the panel.
  // Splitter drag clears the width mode via LithiumSplitter._clearWidthModes().
  // Panel pixel width is saved by the onResizeEnd callback below.

  // ── Data Loading ──────────────────────────────────────────────────────────

  async loadVersionList() {
    if (!this.app?.api || !this.versionsTable?.table) return;

    try {
      this.versionsTable.showLoading();

      // Check if macros are already loaded from shared lookups (must have entries)
      const sharedMacros = getMacros();
      if (sharedMacros && Object.keys(sharedMacros).length > 0) {
        log(Subsystems.MANAGER, Status.INFO, `[VersionHistory] Using ${Object.keys(sharedMacros).length} shared macros from lookups`);
        this._macrosLookup = null; // Signal to use shared macros
      } else {
        // Fallback: load macros directly via QueryRef 26
        log(Subsystems.MANAGER, Status.INFO, `[VersionHistory] Loading macros directly from Lookup 46`);
        const macrosRows = await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 46 } });
        this._macrosLookup = macrosRows || [];
        log(Subsystems.MANAGER, Status.INFO, `[VersionHistory] Loaded ${this._macrosLookup?.length || 0} macros from Lookup 46`);
      }

      // QueryRef 26 - Get Lookup with collection field (44=Server, 45=Client)
      const [serverRows, clientRows] = await Promise.all([
        authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 44 } }),
        authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 45 } }),
      ]);

      // Tag each row with its source and extract collection fields
      const serverVersions = serverRows.map(row => this.processRow(row, 44, 'Server'));
      const clientVersions = clientRows.map(row => this.processRow(row, 45, 'Client'));

      const combinedRows = [...serverVersions, ...clientVersions];

      if (!this.versionsTable.table) return;

      // Store current data for LithiumTable methods
      this.versionsTable.currentData = combinedRows;

      this.versionsTable.table.blockRedraw?.();
      try {
        this.versionsTable.table.setData(combinedRows);
        this.versionsTable.discoverColumns(combinedRows);
      } finally {
        this.versionsTable.table.restoreRedraw?.();
      }

      // Auto-select first row and update button state
      requestAnimationFrame(() => {
        const activeRows = this.versionsTable.table.getRows('active');
        if (activeRows.length > 0) {
          activeRows[0].select();
        }
        this.versionsTable.updateMoveButtonState();
      });

      log(Subsystems.TABLE, Status.INFO,
        `[VersionHistory] Loaded ${serverRows.length} server + ${clientRows.length} client versions`);
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR, `[VersionHistory] Failed to load version list: ${error.message}`);
    } finally {
      this.versionsTable.hideLoading();
    }
  }

  /**
   * Process a row to extract collection fields and add source tags
   */
  processRow(row, clientserver, sourceLabel) {
    let collection = row.collection;

    // Parse collection if it's a string
    if (typeof collection === 'string') {
      try {
        collection = JSON.parse(collection);
      } catch (_e) {
        collection = {};
      }
    }

    // Handle null/undefined collection
    if (!collection || typeof collection !== 'object') {
      collection = {};
    }

    // Use exact JSON keys from the collection object
    const focus = collection.Focus || '';
    const releaseDate = collection.Released || '';

    // Create a simple string summary of the collection for table display
    const collectionKeys = Object.keys(collection);
    const collectionSummary = collectionKeys.length > 0
      ? `{${collectionKeys.join(', ')}}`
      : '{}';

    return {
      ...row,
      clientserver,
      source_label: sourceLabel,
      Focus: focus,
      Released: releaseDate,
      collection: collectionSummary,
      _collection: collection,
    };
  }

  // ── Version Selection ─────────────────────────────────────────────────────

  async handleVersionSelected(rowData) {
    if (!rowData) {
      log(Subsystems.MANAGER, Status.WARN, '[VersionSelection] No rowData received');
      return;
    }

    const keyIdx = rowData.key_idx;
    if (keyIdx == null) {
      log(Subsystems.MANAGER, Status.WARN, '[VersionSelection] No key_idx in rowData');
      return;
    }

    // Skip if this is the same row that's already selected
    // This prevents duplicate API calls when clicking cells in the same row
    // (cellMouseDown + cellClick + rowClick all fire on a single click)
    if (this.selectedVersionId === keyIdx && this.selectedVersionData) {
      log(Subsystems.MANAGER, Status.DEBUG, '[VersionSelection] Same row already selected, skipping');
      return;
    }

    // Prevent duplicate calls if already loading details for a different row
    if (this.loadingDetails) {
      log(Subsystems.MANAGER, Status.INFO, '[VersionSelection] Details already loading, skipping duplicate call');
      return;
    }

    log(Subsystems.MANAGER, Status.INFO, `[VersionSelection] Row selected: key_idx=${keyIdx}, clientserver=${rowData.clientserver}`);

    this.selectedVersionId = keyIdx;
    this.selectedVersionData = rowData;

    // Persist selection
    try { localStorage.setItem(SELECTED_VERSION_KEY, String(keyIdx)); } catch (_e) { /* ignore */ }

    // Load full document details including Summary from QueryRef 45
    this.loadingDetails = true;
    try {
      await this.loadVersionDetails(keyIdx, rowData.clientserver);
    } finally {
      this.loadingDetails = false;
    }
  }

  async loadVersionDetails(keyIdx, clientserver) {
    if (!this.app?.api) {
      log(Subsystems.MANAGER, Status.WARN, '[VersionDetails] No API available');
      return;
    }

    log(Subsystems.CONDUIT, Status.INFO, `[VersionDetails] Calling QueryRef 45 with CLIENTSERVER=${clientserver || 45}, KEYIDX=${keyIdx}`);

    try {
      // QueryRef 45: Get document by CLIENTSERVER and KEYIDX
      // Returns: summary, code, collection fields
      const detail = await authQuery(this.app.api, 45, {
        INTEGER: { CLIENTSERVER: clientserver || 45, KEYIDX: keyIdx },
      });

      log(Subsystems.CONDUIT, Status.INFO, `[VersionDetails] QueryRef 45 returned ${detail?.length || 0} rows`);

      if (detail && detail.length > 0) {
        const detailData = detail[0];

        // Preserve the processed fields from processRow() that were already extracted
        // QueryRef 45 may return its own collection which would overwrite processed data
        const preservedFields = {
          Focus: this.selectedVersionData?.Focus,
          Released: this.selectedVersionData?.Released,
          _collection: this.selectedVersionData?._collection,
          collection: this.selectedVersionData?.collection,
        };

        this.selectedVersionData = { ...this.selectedVersionData, ...detailData };

        // Restore preserved fields if they exist, or extract from new detail collection
        if (!this.selectedVersionData.Focus && detailData.collection) {
          let col = detailData.collection;
          if (typeof col === 'string') {
            try { col = JSON.parse(col); } catch (_e) { col = {}; }
          }
          if (col && typeof col === 'object') {
            this.selectedVersionData.Focus = col.Focus || preservedFields.Focus || '';
            this.selectedVersionData.Released = col.Released || preservedFields.Released || '';
          }
        } else {
          // Use preserved values
          if (preservedFields.Focus) this.selectedVersionData.Focus = preservedFields.Focus;
          if (preservedFields.Released) this.selectedVersionData.Released = preservedFields.Released;
        }

        // Always preserve the processed collection string
        if (preservedFields.collection) {
          this.selectedVersionData.collection = preservedFields.collection;
        }

        log(Subsystems.CONDUIT, Status.INFO, `[VersionDetails] Summary field present: ${!!detailData.summary}`);
      }
    } catch (error) {
      log(Subsystems.CONDUIT, Status.ERROR, `[VersionDetails] Failed to load details: ${error.message}`);
    }

    this.updatePreview();
  }

  handleVersionDeselected() {
    this.selectedVersionId = null;
    this.selectedVersionData = null;
    this.clearDetailView();
  }

  clearDetailView() {
    if (this.elements.previewContent) {
      this.elements.previewContent.innerHTML = '<p class="version-preview-placeholder">Select a version to preview its summary</p>';
    }
  }

  // ── Preview ────────────────────────────────────────────────────────────────

  /**
   * Detect if content is HTML or Markdown
   */
  detectContentType(content) {
    if (!content) return 'markdown';

    const trimmed = content.trim();

    // Check for HTML patterns
    const htmlTagPattern = /^\s*<(?:!DOCTYPE|html|head|body|div|p|span|h[1-6]|ul|ol|li|table|tr|td|th|pre|code|a |img|br|hr|strong|em|b|i|blockquote)/i;
    const htmlTagAnywherePattern = /<(?:div|p|span|h[1-6]|table|tr|td|th|pre|code|a\s|img|blockquote)\s*[^>]*>/i;

    if (htmlTagPattern.test(trimmed) || htmlTagAnywherePattern.test(trimmed)) {
      return 'html';
    }

    return 'markdown';
  }

  /**
   * Expand macros in content using shared macro expansion system
   * Falls back to legacy lookup-based expansion if shared macros not available
   * Macros appear as {MACRO_NAME} and are replaced with their values
   */
  expandContentMacros(content) {
    if (!content || typeof content !== 'string') {
      return content;
    }

    // Check if we have shared macros available
    const sharedMacros = getMacros();
    if (sharedMacros && Object.keys(sharedMacros).length > 0) {
      // Use shared macro expansion system
      return expandMacros(content, { keepUnknown: true });
    }

    // Fallback: use legacy lookup-based expansion
    return this.expandMacrosLegacy(content);
  }

  /**
   * Legacy macro expansion using direct lookup data
   * Used when shared macros are not available
   */
  expandMacrosLegacy(content) {
    if (!content || typeof content !== 'string') {
      return content;
    }

    // Find all {VAR} patterns in content
    const macroMatches = content.match(/\{[A-Za-z0-9_.]+\}/g);
    if (!macroMatches || macroMatches.length === 0) {
      return content;
    }

    // Get unique macros found in content
    const uniqueMacros = [...new Set(macroMatches)];
    log(Subsystems.MANAGER, Status.DEBUG, `[VersionHistory] Found macros (legacy): ${uniqueMacros.join(', ')}`);

    // Get macros from pre-loaded lookup (from loadVersionList)
    const macrosLookup = this._macrosLookup;

    if (!macrosLookup || macrosLookup.length === 0) {
      log(Subsystems.MANAGER, Status.WARN, '[VersionHistory] Macros lookup not loaded');
      return content;
    }

    // Get current locale for variant lookup
    const currentLocale = navigator.language || 'en-US';

    // Build replacement map
    const replacements = {};
    for (const macro of uniqueMacros) {
      // Find matching entry in lookup (case-insensitive match on value_txt)
      const macroEntry = macrosLookup.find(m =>
        m?.value_txt?.toUpperCase() === macro.toUpperCase()
      );

      if (!macroEntry) {
        log(Subsystems.MANAGER, Status.DEBUG, `[VersionHistory] Macro not found in lookup: ${macro}`);
        continue;
      }

      // Parse collection for locale variants
      let collection = macroEntry.collection;
      if (typeof collection === 'string') {
        try {
          collection = JSON.parse(collection);
        } catch (_e) {
          collection = {};
        }
      }

      if (!collection || typeof collection !== 'object') {
        continue;
      }

      // Try to find the best locale match
      // Priority: exact match > language match (en-US > en) > Default
      let replacement = null;

      // Exact locale match
      if (collection[currentLocale]) {
        replacement = collection[currentLocale];
      }
      // Language-only match (e.g., "en" matches "en-US")
      else if (currentLocale.includes('-')) {
        const langOnly = currentLocale.split('-')[0];
        for (const key of Object.keys(collection)) {
          if (key.toLowerCase().startsWith(langOnly.toLowerCase())) {
            replacement = collection[key];
            break;
          }
        }
      }
      // Fall back to Default
      if (!replacement && collection.Default) {
        replacement = collection.Default;
      }

      if (replacement) {
        replacements[macro] = replacement;
        log(Subsystems.MANAGER, Status.DEBUG, `[VersionHistory] Expanding ${macro} -> ${replacement}`);
      }
    }

    // Apply all replacements
    let result = content;
    for (const [macro, replacement] of Object.entries(replacements)) {
      // Replace all occurrences (global regex)
      const macroRegex = new RegExp(macro.replace(/[{}]/g, m => `\\${m}`), 'g');
      result = result.replace(macroRegex, replacement);
    }

    return result;
  }

  updatePreview() {
    if (!this.elements.previewContent) return;

    if (!this.selectedVersionData) {
      this.elements.previewContent.innerHTML = '<p class="version-preview-placeholder">Select a version to preview its summary</p>';
      return;
    }

    try {
      // Get summary from QueryRef 45 response (or fallback to code field)
      let summary = this.selectedVersionData.summary
        || this.selectedVersionData.code
        || '';

      // Expand macros in the content
      summary = this.expandContentMacros(summary);

      // Get released and focus fields using exact JSON keys
      const released = this.selectedVersionData.Released;
      const focus = this.selectedVersionData.Focus;
      // Expand macros in status as well
      let status = this.expandContentMacros(this.selectedVersionData.value_txt) || 'Unknown';

      let htmlContent = '';

      if (summary) {
        const contentType = this.detectContentType(summary);

        if (contentType === 'html') {
          htmlContent = summary;
        } else if (marked) {
          htmlContent = marked.parse(summary);
        } else {
          htmlContent = `<pre style="white-space: pre-wrap;">${this.escapeHtml(summary)}</pre>`;
        }
      } else {
        htmlContent = '<p class="version-preview-placeholder">No summary available for this version</p>';
      }

      // Build header with status and metadata
      const versionHeader = `
        <div class="version-preview-header">
          <h2>${this.escapeHtml(status)}</h2>
          <div class="version-meta">
            ${released ? `<span class="version-meta-item">Released: ${released}</span>` : ''}
            ${focus ? `<span class="version-meta-item">Focus: ${focus}</span>` : ''}
          </div>
        </div>
      `;

      this.elements.previewContent.innerHTML = `<div class="version-preview-rendered">${versionHeader}${htmlContent}</div>`;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[VersionHistory] Failed to update preview: ${error.message}`);
      this.elements.previewContent.innerHTML = '<p class="version-preview-placeholder">Error rendering preview</p>';
    }
  }

  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML
      .replace(/</g, '<')
      .replace(/>/g, '>')
      .replace(/\n/g, '<br>');
  }

  // ── Splitter Logic ─────────────────────────────────────────────────────────

  setupSplitter() {
    this.splitter = new LithiumSplitter({
      element: this.elements.splitter,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.versionsTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.versionsTable?.table?.redraw?.();
      },
    });

    // Bind splitter to table for centralized width mode clearing
    this.versionsTable?.setSplitter(this.splitter);
  }

  toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.versionsTable?.table?.redraw?.(),
    });

    // Save collapsed state via PanelStateManager
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  restorePanelState() {
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

    restoreCollapsedPanelState({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      isCollapsed: this.isLeftPanelCollapsed,
    });
  }

  // ── Footer Setup ───────────────────────────────────────────────────────────

  setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this.handleFooterPrint(),
      onEmail: () => this.handleFooterEmail(),
      onDownload: () => this.handleFooterDownload(),
      onExport: (value, label) => this.handleFooterExport(value, label),
      onClipboard: (value, label) => this.handleFooterClipboard(value, label),
      reportOptions: [
        { value: 'version-view', label: 'Versions View' },
        { value: 'version-data', label: 'Versions Data' },
        { value: 'version-summary', label: 'Version Summary' },
      ],
      fillerTitle: 'Versions',
      anchor: placeholder,
      showSaveCancel: true,
      showClipboard: true,
      exportOptions: [
        { value: 'pdf', label: 'PDF', icon: 'fa-file-pdf', enabled: true },
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'xls', label: 'XLS', icon: 'fa-file-excel', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
        { value: 'html', label: 'HTML', icon: 'fa-file-html', enabled: true },
        { value: 'markdown', label: 'Markdown', icon: 'fa-file-code', enabled: true },
        { value: 'txt', label: 'Text', icon: 'fa-file-lines', enabled: true },
      ],
    });

    this._footerDatasource = footerElements.reportSelect;
    this._footerElements = footerElements;

    // Wire save/cancel buttons to the editHelper (handles all state management)
    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );

    log(Subsystems.MANAGER, Status.INFO, '[VersionHistory] Footer controls initialized');
  }

  _getFooterDatasource() {
    return this._footerDatasource?.value || 'version-view';
  }

  handleFooterPrint() {
    const mode = this._getFooterDatasource();

    // Handle Version Summary print
    if (mode === 'version-summary') {
      this.handleVersionSummaryExport();
      return;
    }

    const table = this.versionsTable?.table;

    if (!table) {
      return;
    }

    if (mode.endsWith('-view')) {
      table.print();
    } else {
      table.print('all', true);
    }
  }

  handleFooterEmail() {
    const mode = this._getFooterDatasource();

    // Handle Version Summary email
    if (mode === 'version-summary') {
      this.handleVersionSummaryEmail();
      return;
    }

    const table = this.versionsTable?.table;

    if (!table) {
      return;
    }

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) {
      return;
    }

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

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
    const modeLabel = isViewMode ? 'Filtered View' : 'Full Data';

    const subject = encodeURIComponent(`Versions — ${totalRows} rows (${modeLabel})`);
    const body = encodeURIComponent(
      `Versions Export (${modeLabel})\n` +
      `${totalRows} row(s)\n\n` +
      `${headers.join('\t')}\n${separator}\n` +
      `${dataLines.join('\n')}${truncated}\n`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  toggleFooterExportPopup(e) {
    e.stopPropagation();

    if (this._footerExportPopup) {
      this._closeFooterExportPopup();
      return;
    }

    const btn = e.currentTarget;
    const mode = this._getFooterDatasource();
    const formats = [
      { label: 'PDF', action: () => this.handleFooterExport('pdf', mode) },
      { label: 'CSV', action: () => this.handleFooterExport('csv', mode) },
      { label: 'TXT', action: () => this.handleFooterExport('txt', mode) },
      { label: 'XLS', action: () => this.handleFooterExport('xls', mode) },
    ];

    const popup = document.createElement('div');
    popup.className = 'version-footer-export-popup';
    formats.forEach(item => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'version-footer-export-popup-item';
      row.textContent = item.label;
      row.addEventListener('click', () => {
        this._closeFooterExportPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    const btnRect = btn.getBoundingClientRect();
    document.body.appendChild(popup);

    requestAnimationFrame(() => {
      const popupRect = popup.getBoundingClientRect();
      popup.style.position = 'fixed';
      popup.style.top = `${btnRect.top - popupRect.height - 8}px`;
      popup.style.left = `${btnRect.left}px`;
    });

    setTimeout(() => {
      popup.classList.add('visible');
    }, 10);

    this._footerExportPopup = popup;

    this._footerExportCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this._closeFooterExportPopup();
      }
    };
    document.addEventListener('click', this._footerExportCloseHandler);
  }

  _closeFooterExportPopup() {
    if (this._footerExportPopup) {
      this._footerExportPopup.remove();
      this._footerExportPopup = null;
    }
    if (this._footerExportCloseHandler) {
      document.removeEventListener('click', this._footerExportCloseHandler);
      this._footerExportCloseHandler = null;
    }
  }

  handleFooterExport(value, label) {
    const mode = this._getFooterDatasource();

    // Handle Version Summary export - export as markdown regardless of format
    if (mode === 'version-summary') {
      this.handleVersionSummaryExport();
      return;
    }

    const table = this.versionsTable?.table;

    if (!table) {
      return;
    }

    const filename = `versions-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    this._doDownload(table, value, filename, downloadOpts);
  }

  handleFooterDownload() {
    const mode = this._getFooterDatasource();

    // Handle Version Summary download
    if (mode === 'version-summary') {
      this.handleVersionSummaryExport();
      return;
    }

    const table = this.versionsTable?.table;

    if (!table) {
      return;
    }

    const filename = `versions-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    const exportFormat = this._footerElements?.exportFormat || 'pdf';
    this._doDownload(table, exportFormat, filename, downloadOpts);
  }

  _doDownload(table, format, filename, downloadOpts) {
    switch (format) {
      case 'pdf':
        table.download('pdf', `${filename}.pdf`, { orientation: 'landscape', ...downloadOpts });
        break;
      case 'csv':
        table.download('csv', `${filename}.csv`, downloadOpts);
        break;
      case 'txt':
        table.download('csv', `${filename}.txt`, downloadOpts);
        break;
      case 'xls':
        table.download('xlsx', `${filename}.xlsx`, downloadOpts);
        break;
      case 'json':
        table.download('json', `${filename}.json`, downloadOpts);
        break;
      case 'html':
        table.download('html', `${filename}.html`, downloadOpts);
        break;
      case 'markdown':
        this.handleMarkdownExport(this._getFooterDatasource(), filename);
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `[VersionHistory] Unknown export format: ${format}`);
    }
  }

  handleFooterClipboard(value, label) {
    const mode = this._getFooterDatasource();

    // Handle Version Summary clipboard copy
    if (mode === 'version-summary') {
      this.handleVersionSummaryClipboard();
      return;
    }

    const table = this.versionsTable?.table;

    if (!table) {
      return;
    }

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) {
      return;
    }

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const dataLines = rows.map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val) : '';
      }).join('\t');
    });

    const text = `${headers.join('\t')}\n${dataLines.join('\n')}`;

    navigator.clipboard.writeText(text).then(() => {
      toast.success('Copied', { description: 'Table data copied to clipboard', duration: 2000 });
    }).catch(err => {
      toast.error('Copy Failed', { description: 'Failed to copy to clipboard', duration: 3000 });
    });
  }

  handleMarkdownExport(mode, filename) {
    const table = this.versionsTable?.table;
    if (!table) return;

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) return;

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const headerRow = `| ${headers.join(' | ')} |`;
    const separatorRow = `| ${headers.map(() => '---').join(' | ')} |`;
    const dataRows = rows.map(row => {
      const data = row.getData();
      const rowData = visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val).replace(/\|/g, '\\|') : '';
      });
      return `| ${rowData.join(' | ')} |`;
    });

    const markdown = `${headerRow}\n${separatorRow}\n${dataRows.join('\n')}`;

    const blob = new Blob([markdown], { type: 'text/markdown' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${filename}.md`;
    a.click();
    URL.revokeObjectURL(url);
  }

  handleVersionSummaryExport() {
    if (!this.selectedVersionData) {
      toast.info('No Version Selected', { description: 'Please select a version to export', duration: 3000 });
      return;
    }

    const v = this.selectedVersionData;
    const version = v.value_txt || v.key_idx || 'Unknown';
    const released = v.Released || 'N/A';
    const focus = v.Focus || 'N/A';
    const summary = v.summary || v.code || '';

    const markdown = `# Version ${version}

**Released:** ${released}  
**Focus:** ${focus}

---

## Summary

${summary}
`;

    const blob = new Blob([markdown], { type: 'text/markdown' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `version-${version}-summary.md`;
    a.click();
    URL.revokeObjectURL(url);
  }

  handleVersionSummaryClipboard() {
    if (!this.selectedVersionData) {
      toast.info('No Version Selected', { description: 'Please select a version to copy', duration: 3000 });
      return;
    }

    const v = this.selectedVersionData;
    const version = v.value_txt || v.key_idx || 'Unknown';
    const released = v.Released || 'N/A';
    const focus = v.Focus || 'N/A';
    const summary = v.summary || v.code || '';

    const text = `Version: ${version}
Released: ${released}
Focus: ${focus}

Summary:
${summary}`;

    navigator.clipboard.writeText(text).then(() => {
      toast.success('Copied', { description: 'Version summary copied to clipboard', duration: 2000 });
    }).catch(err => {
      toast.error('Copy Failed', { description: 'Failed to copy to clipboard', duration: 3000 });
    });
  }

  handleVersionSummaryEmail() {
    if (!this.selectedVersionData) {
      toast.info('No Version Selected', { description: 'Please select a version to email', duration: 3000 });
      return;
    }

    const v = this.selectedVersionData;
    const version = v.value_txt || v.key_idx || 'Unknown';
    const released = v.Released || 'N/A';
    const focus = v.Focus || 'N/A';
    const summary = v.summary || v.code || '';

    const subject = encodeURIComponent(`Version ${version} Summary`);
    const body = encodeURIComponent(
      `Version: ${version}\n` +
      `Released: ${released}\n` +
      `Focus: ${focus}\n\n` +
      `Summary:\n${summary}`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  // Edit mode, dirty tracking, and save/cancel button management are now
  // handled by this.editHelper (ManagerEditHelper).

// ── Cleanup ────────────────────────────────────────────────────────────────

  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[VersionHistory] Cleaning up...');

    // Clean up edit helper
    this.editHelper?.destroy();

    this.splitter?.destroy();
    this.splitter = null;

    this.versionsTable?.destroy();
    this.versionsTable = null;
  }
}
