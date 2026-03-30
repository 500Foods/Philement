/**
 * Version History Manager — Manager ID 6
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
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
import '../../styles/vendor-tabulator.css';
import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import '../../core/manager-panels.css';
import './version-history.css';

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

    // Version History Manager ID = 6
    this.managerId = 6;

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
  }

  async init() {
    await this.loadDependencies();
    await this.render();
    this.setupEventListeners();
    await this.initVersionsTable();
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
      app: this.app,
      readonly: true,
      onRowSelected: (rowData) => this.handleVersionSelected(rowData),
      onRowDeselected: () => this.handleVersionDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[VersionHistory] Loaded ${rows.length} versions`);
      },
      onSetTableWidth: (mode) => this.setTableWidth(mode),
      onRefresh: () => this.loadVersionList(),
    });

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

  setTableWidth(mode) {
    const panel = this.elements.leftPanel;
    if (!panel) return;

    // mode === null means LithiumTable has no saved width mode.
    // Apply the PanelStateManager pixel width as fallback.
    if (mode === null) {
      panel.style.width = `${this.leftPanelWidth}px`;
      return;
    }

    // Match Query Manager width setpoints
    const widths = { narrow: 160, compact: 314, normal: 468, wide: 622 };

    this._tableWidthMode = mode;

    if (mode === 'auto') {
      panel.style.width = '';
      const currentWidth = panel.offsetWidth;
      this.leftPanelWidth = currentWidth;
      this.leftPanelState.saveWidth(currentWidth);
      return;
    }

    // Named mode: set the panel width but do NOT overwrite this.leftPanelWidth
    // (which holds the pixel width for collapse/expand).
    const widthPx = widths[mode] || 314;
    panel.style.width = `${widthPx}px`;
    log(Subsystems.MANAGER, Status.INFO, `[VersionHistory] Table width set to: ${mode}`);
  }

  // ── Data Loading ──────────────────────────────────────────────────────────

  async loadVersionList() {
    if (!this.app?.api || !this.versionsTable?.table) return;

    try {
      this.versionsTable.showLoading();

      // QueryRef 26 - Get Lookup with collection field
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

    log(Subsystems.MANAGER, Status.INFO, `[VersionSelection] Row selected: key_idx=${keyIdx}, clientserver=${rowData.clientserver}`);

    this.selectedVersionId = keyIdx;
    this.selectedVersionData = rowData;

    // Persist selection
    try { localStorage.setItem(SELECTED_VERSION_KEY, String(keyIdx)); } catch (_e) { /* ignore */ }

    // Load full document details including Summary from QueryRef 45
    await this.loadVersionDetails(keyIdx, rowData.clientserver);
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

  updatePreview() {
    if (!this.elements.previewContent) return;

    if (!this.selectedVersionData) {
      this.elements.previewContent.innerHTML = '<p class="version-preview-placeholder">Select a version to preview its summary</p>';
      return;
    }

    try {
      // Get summary from QueryRef 45 response (or fallback to code field)
      const summary = this.selectedVersionData.summary
        || this.selectedVersionData.code
        || '';

      // Get released and focus fields using exact JSON keys
      const released = this.selectedVersionData.Released;
      const focus = this.selectedVersionData.Focus;
      const status = this.selectedVersionData.value_txt || 'Unknown';

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

    // Save collapsed state
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  restorePanelState() {
    // Re-read collapsed state from localStorage (handles edge cases)
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

    // Restore collapsed state using shared utility
    restorePanelState({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      isCollapsed: this.isLeftPanelCollapsed,
    });

    // Panel width is handled by LithiumTable's setupPersistence():
    // - If a width mode was saved, it calls onSetTableWidth(mode)
    // - If no mode was saved, it calls onSetTableWidth(null), and setTableWidth
    //   applies the PanelStateManager pixel width as fallback
  }

  // ── Cleanup ────────────────────────────────────────────────────────────────

  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[VersionHistory] Cleaning up...');

    this.splitter?.destroy();
    this.splitter = null;

    this.versionsTable?.destroy();
    this.versionsTable = null;
  }
}
