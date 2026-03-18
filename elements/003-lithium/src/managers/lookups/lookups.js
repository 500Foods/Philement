/**
 * Lookups Manager — Manager ID 5
 *
 * A three-panel interface for managing lookup tables:
 * - Left panel: List of all lookups (parent table)
 * - Middle panel: Values for the selected lookup (child table)
 * - Right panel: Tabbed view with JSON, Summary, and Preview
 *
 * Uses the reusable LithiumTable component for both tables.
 *
 * @module managers/lookups
 */

import { LithiumTable } from '../../core/lithium-table-main.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import SunEditor from 'suneditor';
import 'suneditor/css/editor';
import './lookups.css';

// ── Footer Select Options ───────────────────────────────────────────────────

const FOOTER_SELECTS = [
  {
    id: 'lookups-parent-select',
    options: [
      { value: 'view', label: 'Lookup List View' },
      { value: 'data', label: 'Lookup List Data' },
    ],
    value: 'view',
  },
  {
    id: 'lookups-child-select',
    options: [
      { value: 'view', label: 'Lookup Values View' },
      { value: 'data', label: 'Lookup Values Data' },
    ],
    value: 'view',
  },
];

// ── LookupsManager Class ────────────────────────────────────────────────────

export default class LookupsManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // Parent table (lookups list)
    this.parentTable = null;

    // Child table (lookup values)
    this.childTable = null;

    // Currently selected lookup
    this.selectedLookupId = null;
    this.selectedLookupName = null;

    // Currently selected lookup value
    this.selectedLookupValue = null;

    // Splitter state
    this.leftPanelWidth = 280;
    this.middlePanelWidth = 350;
    this.isLeftPanelCollapsed = false;
    this.isMiddlePanelCollapsed = false;
    this.isResizingLeft = false;
    this.isResizingRight = false;

    // Editor instances
    this.sunEditor = null;
    this.currentDetailData = null;

    // Active tab
    this.activeTab = 'json';

    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-sans)';
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupSplitters();
    await this.initParentTable();
    await this.initChildTable();
    this.setupFooter();
    this.setupTabs();
  }

  async render() {
    try {
      const response = await fetch('/src/managers/lookups/lookups.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[LookupsManager] Failed to load template:', error);
      this.container.innerHTML = '<div class="error">Failed to load Lookups Manager</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.lookups-manager-container'),
      leftPanel: this.container.querySelector('#lookups-left-panel'),
      middlePanel: this.container.querySelector('#lookups-middle-panel'),
      rightPanel: this.container.querySelector('#lookups-right-panel'),
      parentTableContainer: this.container.querySelector('#lookups-parent-table-container'),
      parentNavigator: this.container.querySelector('#lookups-parent-navigator'),
      childTableContainer: this.container.querySelector('#lookups-child-table-container'),
      childNavigator: this.container.querySelector('#lookups-child-navigator'),
      childTitle: this.container.querySelector('#lookups-child-title'),
      splitterLeft: this.container.querySelector('#lookups-splitter-left'),
      splitterRight: this.container.querySelector('#lookups-splitter-right'),
      collapseLeftBtn: this.container.querySelector('#lookups-collapse-left-btn'),
      collapseMiddleBtn: this.container.querySelector('#lookups-collapse-middle-btn'),
      collapseLeftIcon: this.container.querySelector('#lookups-collapse-left-icon'),
      collapseMiddleIcon: this.container.querySelector('#lookups-collapse-middle-icon'),
      fontBtn: this.container.querySelector('#lookups-font-btn'),
      tabBtns: this.container.querySelectorAll('.lookups-tab-btn'),
      tabPanes: this.container.querySelectorAll('.lookups-tab-pane'),
      jsonEditor: this.container.querySelector('#lookups-json-editor'),
      summaryEditor: this.container.querySelector('#lookups-summary-editor'),
      previewContent: this.container.querySelector('#lookups-preview-content'),
    };

    // Process icons
    processIcons(this.container);
  }

  setupEventListeners() {
    // Collapse/expand left panel button
    this.elements.collapseLeftBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });

    // Collapse/expand middle panel button
    this.elements.collapseMiddleBtn?.addEventListener('click', () => {
      this.toggleMiddlePanel();
    });

    // Font button
    this.elements.fontBtn?.addEventListener('click', (e) => {
      this.toggleFontPopup(e);
    });

    // Initialize font popup
    this.initFontPopup();
  }

  // ── Font Popup ─────────────────────────────────────────────────────────────

  initFontPopup() {
    // Create font popup element
    this.fontPopup = document.createElement('div');
    this.fontPopup.className = 'lookups-font-popup';
    const fontSizeValue = parseInt(this.editorFontSize, 10) || 14;
    this.fontPopup.innerHTML = `
      <div class="lookups-font-popup-row">
        <label class="lookups-font-popup-label">Size</label>
        <input type="number" class="lookups-font-popup-input" id="lookups-font-size" value="${fontSizeValue}" min="8" max="32">
      </div>
      <div class="lookups-font-popup-row">
        <label class="lookups-font-popup-label">Family</label>
        <select class="lookups-font-popup-select" id="lookups-font-family">
          <option value="var(--font-sans)" ${this.editorFontFamily === 'var(--font-sans)' ? 'selected' : ''}>Sans Serif</option>
          <option value="var(--font-mono)" ${this.editorFontFamily === 'var(--font-mono)' ? 'selected' : ''}>Monospace</option>
          <option value="serif" ${this.editorFontFamily === 'serif' ? 'selected' : ''}>Serif</option>
        </select>
      </div>
      <div class="lookups-font-popup-row">
        <label class="lookups-font-popup-label">Style</label>
        <div class="lookups-font-popup-styles">
          <button type="button" class="lookups-font-popup-style-btn" data-style="normal">Normal</button>
          <button type="button" class="lookups-font-popup-style-btn" data-style="bold">Bold</button>
        </div>
      </div>
    `;

    // Append to font button's parent (the toolbar)
    if (this.elements.fontBtn) {
      this.elements.fontBtn.parentElement.appendChild(this.fontPopup);
    }

    // Setup font popup event listeners
    const sizeInput = this.fontPopup.querySelector('#lookups-font-size');
    const familySelect = this.fontPopup.querySelector('#lookups-font-family');
    const styleBtns = this.fontPopup.querySelectorAll('.lookups-font-popup-style-btn');

    sizeInput?.addEventListener('change', (e) => {
      this.editorFontSize = `${e.target.value}px`;
      this.applyFontSettings();
    });

    familySelect?.addEventListener('change', (e) => {
      this.editorFontFamily = e.target.value;
      this.applyFontSettings();
    });

    styleBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        styleBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        this.applyFontSettings();
      });
    });

    // Close popup when clicking outside
    document.addEventListener('click', (e) => {
      if (this.fontPopup?.classList.contains('visible') &&
          !this.fontPopup.contains(e.target) &&
          !this.elements.fontBtn?.contains(e.target)) {
        this.fontPopup.classList.remove('visible');
      }
    });
  }

  toggleFontPopup(e) {
    e.stopPropagation();
    if (this.fontPopup) {
      this.fontPopup.classList.toggle('visible');
    }
  }

  applyFontSettings() {
    // Apply font settings to JSON editor
    if (this.elements.jsonEditor) {
      this.elements.jsonEditor.style.fontSize = this.editorFontSize;
      this.elements.jsonEditor.style.fontFamily = this.editorFontFamily;
    }

    // Apply font settings to SunEditor if initialized
    if (this.sunEditor) {
      const editorBody = this.sunEditor.getContext()?.element?.wysiwyg;
      if (editorBody) {
        editorBody.style.fontSize = this.editorFontSize;
        editorBody.style.fontFamily = this.editorFontFamily;
      }
    }

    // Apply font settings to preview content
    if (this.elements.previewContent) {
      this.elements.previewContent.style.fontSize = this.editorFontSize;
      this.elements.previewContent.style.fontFamily = this.editorFontFamily;
    }
  }

  setupTabs() {
    // Tab switching
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => {
        const tabId = btn.dataset.tab;
        this.switchTab(tabId);
      });
    });
  }

  switchTab(tabId) {
    this.activeTab = tabId;

    // Update buttons
    this.elements.tabBtns.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.tab === tabId);
    });

    // Update panes
    this.elements.tabPanes.forEach(pane => {
      pane.classList.toggle('active', pane.id === `lookups-tab-${tabId}`);
    });

    // Initialize editors on first view
    if (tabId === 'summary' && !this.sunEditor) {
      this.initSummaryEditor();
    }

    // Refresh preview if switching to preview tab
    if (tabId === 'preview') {
      this.refreshPreview();
    }
  }

  initSummaryEditor() {
    if (!this.elements.summaryEditor || this.sunEditor) return;

    try {
      this.sunEditor = SunEditor.create(this.elements.summaryEditor, {
        height: '100%',
        width: '100%',
        defaultStyle: 'font-family: var(--font-sans); font-size: 14px;',
        buttonList: [
          ['undo', 'redo'],
          ['font', 'fontSize', 'formatBlock'],
          ['bold', 'underline', 'italic', 'strike', 'subscript', 'superscript'],
          ['removeFormat'],
          '/', // Line break
          ['fontColor', 'hiliteColor'],
          ['outdent', 'indent'],
          ['align', 'horizontalRule', 'list', 'table'],
          ['link', 'image'],
          ['fullScreen', 'showBlocks', 'codeView'],
        ],
        placeholder: 'Enter summary here...',
        darkMode: true,
      });

      // Set initial content if we have detail data
      if (this.currentDetailData?.summary) {
        this.sunEditor.setContents(this.currentDetailData.summary);
      }
    } catch (error) {
      console.error('[LookupsManager] Failed to initialize SunEditor:', error);
    }
  }

  // ── Parent Table Initialization ────────────────────────────────────────────

  async initParentTable() {
    if (!this.elements.parentTableContainer || !this.elements.parentNavigator) return;

    this.parentTable = new LithiumTable({
      container: this.elements.parentTableContainer,
      navigatorContainer: this.elements.parentNavigator,
      tablePath: 'lookups/lookups-list',
      queryRef: 30, // QueryRef 30 - Get Lookups List
      searchQueryRef: 31, // QueryRef 31 - Get Lookups List + Search
      cssPrefix: 'lookups-parent',
      storageKey: 'lookups_parent_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => this.handleParentRowSelected(rowData),
      onRowDeselected: () => this.handleParentRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookups`);
      },
    });

    await this.parentTable.init();

    // Load parent data
    await this.parentTable.loadData();
  }

  // ── Child Table Initialization ─────────────────────────────────────────────

  async initChildTable() {
    if (!this.elements.childTableContainer || !this.elements.childNavigator) return;

    this.childTable = new LithiumTable({
      container: this.elements.childTableContainer,
      navigatorContainer: this.elements.childNavigator,
      tablePath: 'lookups/lookup-values',
      queryRef: 34, // QueryRef 34 - Get Lookup List (requires LOOKUPID param)
      cssPrefix: 'lookups-child',
      storageKey: 'lookups_child_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => this.handleChildRowSelected(rowData),
      onRowDeselected: () => this.handleChildRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookup values`);
      },
    });

    await this.childTable.init();
  }

  // ── Parent/Child Relationship ──────────────────────────────────────────────

  async handleParentRowSelected(rowData) {
    if (!rowData) return;

    const lookupId = rowData.key_idx; // key_idx is the lookup_id for lookup_id=0 entries
    const lookupName = rowData.value_txt;

    if (!lookupId) return;

    this.selectedLookupId = lookupId;
    this.selectedLookupName = lookupName;

    // Update child table title
    if (this.elements.childTitle) {
      this.elements.childTitle.textContent = lookupName
        ? `${lookupName} (${lookupId})`
        : `Lookup ${lookupId}`;
    }

    // Load child data for this lookup
    await this.loadChildData(lookupId);
  }

  handleParentRowDeselected() {
    this.selectedLookupId = null;
    this.selectedLookupName = null;

    // Clear child table title
    if (this.elements.childTitle) {
      this.elements.childTitle.textContent = 'Select a Lookup';
    }

    // Clear child table data
    if (this.childTable?.table) {
      this.childTable.table.setData([]);
    }

    // Clear detail view
    this.clearDetailView();
  }

  async loadChildData(lookupId) {
    if (!this.childTable || !this.app?.api) return;

    try {
      this.childTable.showLoading();

      const rows = await authQuery(this.app.api, 34, {
        INTEGER: { LOOKUPID: lookupId },
      });

      if (!this.childTable.table) return;

      this.childTable.table.blockRedraw?.();
      try {
        this.childTable.table.setData(rows);
        this.childTable.discoverColumns(rows);
      } finally {
        this.childTable.table.restoreRedraw?.();
      }

      // Auto-select first row if any
      requestAnimationFrame(() => {
        const activeRows = this.childTable.table.getRows('active');
        if (activeRows.length > 0) {
          activeRows[0].select();
        }
        this.childTable.updateMoveButtonState();
      });

      log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookup values for lookup ${lookupId}`);
    } catch (error) {
      toast.error('Failed to load lookup values', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 6000,
      });
    } finally {
      this.childTable.hideLoading();
    }
  }

  // ── Child/Detail Relationship ──────────────────────────────────────────────

  async handleChildRowSelected(rowData) {
    if (!rowData) return;

    this.selectedLookupValue = rowData;

    // Fetch full detail using QueryRef 35
    await this.loadDetailData(rowData);
  }

  handleChildRowDeselected() {
    this.selectedLookupValue = null;
    this.clearDetailView();
  }

  async loadDetailData(rowData) {
    if (!this.app?.api || !this.selectedLookupId) return;

    try {
      const detail = await authQuery(this.app.api, 35, {
        INTEGER: {
          LOOKUPID: this.selectedLookupId,
          KEYIDX: rowData.key_idx,
        },
      });

      if (detail && detail.length > 0) {
        this.currentDetailData = detail[0];
        this.updateDetailView();
      }
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR, `[Lookups] Failed to load detail: ${error.message}`);
      // Don't show toast - detail view is optional
    }
  }

  updateDetailView() {
    if (!this.currentDetailData) return;

    // Update JSON tab
    if (this.elements.jsonEditor) {
      this.elements.jsonEditor.textContent = JSON.stringify(this.currentDetailData, null, 2);
    }

    // Update Summary editor if initialized
    if (this.sunEditor) {
      this.sunEditor.setContents(this.currentDetailData.summary || '');
    }

    // Refresh preview if active
    if (this.activeTab === 'preview') {
      this.refreshPreview();
    }
  }

  clearDetailView() {
    this.currentDetailData = null;

    if (this.elements.jsonEditor) {
      this.elements.jsonEditor.textContent = 'Select a lookup entry to view details';
    }

    if (this.sunEditor) {
      this.sunEditor.setContents('');
    }

    if (this.elements.previewContent) {
      this.elements.previewContent.innerHTML = '<p class="lookups-preview-placeholder">Select a lookup entry to preview</p>';
    }
  }

  async refreshPreview() {
    if (!this.elements.previewContent) return;

    if (!this.currentDetailData) {
      this.elements.previewContent.innerHTML = '<p class="lookups-preview-placeholder">Select a lookup entry to preview</p>';
      return;
    }

    try {
      // Try to use highlight.js or prism.js if available
      let htmlContent = '';

      // Get content from SunEditor or raw summary
      const summaryContent = this.sunEditor
        ? this.sunEditor.getContents()
        : (this.currentDetailData.summary || '');

      if (summaryContent) {
        // Check if highlight.js is available
        if (window.hljs) {
          // Simple markdown-like processing with syntax highlighting
          htmlContent = this.processContentWithHighlighting(summaryContent);
        } else {
          // Basic HTML processing
          htmlContent = this.basicContentProcessing(summaryContent);
        }
      } else {
        htmlContent = '<p class="lookups-preview-placeholder">No summary content available</p>';
      }

      this.elements.previewContent.innerHTML = htmlContent;
    } catch (error) {
      console.error('[LookupsManager] Failed to refresh preview:', error);
      this.elements.previewContent.innerHTML = '<p class="lookups-preview-placeholder">Error rendering preview</p>';
    }
  }

  processContentWithHighlighting(content) {
    // Escape HTML first
    let html = content
      .replace(/&/g, '&')
      .replace(/</g, '<')
      .replace(/>/g, '>');

    // Process code blocks with highlight.js
    html = html.replace(/```(\w+)?\n([\s\S]*?)```/g, (match, lang, code) => {
      if (window.hljs) {
        try {
          const highlighted = lang
            ? window.hljs.highlight(code.trim(), { language: lang }).value
            : window.hljs.highlightAuto(code.trim()).value;
          return `<pre><code class="hljs">${highlighted}</code></pre>`;
        } catch (e) {
          return `<pre><code>${code.trim()}</code></pre>`;
        }
      }
      return `<pre><code>${code.trim()}</code></pre>`;
    });

    // Process inline code
    html = html.replace(/`([^`]+)`/g, '<code>$1</code>');

    // Process basic formatting
    html = html.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    html = html.replace(/\*([^*]+)\*/g, '<em>$1</em>');
    html = html.replace(/^# (.+)$/gm, '<h1>$1</h1>');
    html = html.replace(/^## (.+)$/gm, '<h2>$1</h2>');
    html = html.replace(/^### (.+)$/gm, '<h3>$1</h3>');
    html = html.replace(/\n/g, '<br>');

    return `<div class="lookups-preview-rendered">${html}</div>`;
  }

  basicContentProcessing(content) {
    // Simple HTML escaping and line breaks
    return content
      .replace(/&/g, '&')
      .replace(/</g, '<')
      .replace(/>/g, '>')
      .replace(/\n/g, '<br>');
  }

  // ── Splitter Logic ─────────────────────────────────────────────────────────

  setupSplitters() {
    // Left splitter (between left and middle panels)
    this.setupLeftSplitter();

    // Right splitter (between middle and right panels)
    this.setupRightSplitter();
  }

  setupLeftSplitter() {
    const splitter = this.elements.splitterLeft;
    const leftPanel = this.elements.leftPanel;

    if (!splitter || !leftPanel) return;

    // Mouse down on left splitter
    splitter.addEventListener('mousedown', (e) => {
      if (this.isLeftPanelCollapsed) return;

      this.isResizingLeft = true;
      splitter.classList.add('resizing');
      document.body.style.cursor = 'col-resize';
      document.body.style.userSelect = 'none';

      const startX = e.clientX;
      const startWidth = leftPanel.offsetWidth;

      const onMouseMove = (e) => {
        if (!this.isResizingLeft) return;

        const delta = e.clientX - startX;
        const newWidth = Math.max(150, Math.min(450, startWidth + delta));

        leftPanel.style.width = `${newWidth}px`;
        this.leftPanelWidth = newWidth;
      };

      const onMouseUp = () => {
        this.isResizingLeft = false;
        splitter.classList.remove('resizing');
        document.body.style.cursor = '';
        document.body.style.userSelect = '';

        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);

        // Redraw tables after resize
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    });

    // Touch support for mobile
    splitter.addEventListener('touchstart', (e) => {
      if (this.isLeftPanelCollapsed) return;

      this.isResizingLeft = true;
      splitter.classList.add('resizing');

      const touch = e.touches[0];
      const startX = touch.clientX;
      const startWidth = leftPanel.offsetWidth;

      const onTouchMove = (e) => {
        if (!this.isResizingLeft) return;

        const touch = e.touches[0];
        const delta = touch.clientX - startX;
        const newWidth = Math.max(150, Math.min(450, startWidth + delta));

        leftPanel.style.width = `${newWidth}px`;
        this.leftPanelWidth = newWidth;
      };

      const onTouchEnd = () => {
        this.isResizingLeft = false;
        splitter.classList.remove('resizing');

        document.removeEventListener('touchmove', onTouchMove);
        document.removeEventListener('touchend', onTouchEnd);

        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      };

      document.addEventListener('touchmove', onTouchMove);
      document.addEventListener('touchend', onTouchEnd);
    });
  }

  setupRightSplitter() {
    const splitter = this.elements.splitterRight;
    const middlePanel = this.elements.middlePanel;

    if (!splitter || !middlePanel) return;

    // Mouse down on right splitter
    splitter.addEventListener('mousedown', (e) => {
      if (this.isMiddlePanelCollapsed) return;

      this.isResizingRight = true;
      splitter.classList.add('resizing');
      document.body.style.cursor = 'col-resize';
      document.body.style.userSelect = 'none';

      const startX = e.clientX;
      const startWidth = middlePanel.offsetWidth;

      const onMouseMove = (e) => {
        if (!this.isResizingRight) return;

        const delta = e.clientX - startX;
        const newWidth = Math.max(200, Math.min(550, startWidth + delta));

        middlePanel.style.width = `${newWidth}px`;
        this.middlePanelWidth = newWidth;
      };

      const onMouseUp = () => {
        this.isResizingRight = false;
        splitter.classList.remove('resizing');
        document.body.style.cursor = '';
        document.body.style.userSelect = '';

        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);

        // Redraw tables after resize
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    });

    // Touch support for mobile
    splitter.addEventListener('touchstart', (e) => {
      if (this.isMiddlePanelCollapsed) return;

      this.isResizingRight = true;
      splitter.classList.add('resizing');

      const touch = e.touches[0];
      const startX = touch.clientX;
      const startWidth = middlePanel.offsetWidth;

      const onTouchMove = (e) => {
        if (!this.isResizingRight) return;

        const touch = e.touches[0];
        const delta = touch.clientX - startX;
        const newWidth = Math.max(200, Math.min(550, startWidth + delta));

        middlePanel.style.width = `${newWidth}px`;
        this.middlePanelWidth = newWidth;
      };

      const onTouchEnd = () => {
        this.isResizingRight = false;
        splitter.classList.remove('resizing');

        document.removeEventListener('touchmove', onTouchMove);
        document.removeEventListener('touchend', onTouchEnd);

        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      };

      document.addEventListener('touchmove', onTouchMove);
      document.addEventListener('touchend', onTouchEnd);
    });
  }

  toggleLeftPanel() {
    const leftPanel = this.elements.leftPanel;
    const splitter = this.elements.splitterLeft;

    if (!leftPanel || !splitter) return;

    this.isLeftPanelCollapsed = !this.isLeftPanelCollapsed;

    if (this.isLeftPanelCollapsed) {
      leftPanel.classList.add('collapsed');
      splitter.classList.add('collapsed');
    } else {
      leftPanel.classList.remove('collapsed');
      leftPanel.style.width = `${this.leftPanelWidth}px`;
      splitter.classList.remove('collapsed');
    }

    // Redraw tables after animation
    setTimeout(() => {
      this.parentTable?.table?.redraw?.();
      this.childTable?.table?.redraw?.();
    }, 350);
  }

  toggleMiddlePanel() {
    const middlePanel = this.elements.middlePanel;
    const splitter = this.elements.splitterRight;

    if (!middlePanel || !splitter) return;

    this.isMiddlePanelCollapsed = !this.isMiddlePanelCollapsed;

    if (this.isMiddlePanelCollapsed) {
      middlePanel.classList.add('collapsed');
      splitter.classList.add('collapsed');
    } else {
      middlePanel.classList.remove('collapsed');
      middlePanel.style.width = `${this.middlePanelWidth}px`;
      splitter.classList.remove('collapsed');
    }

    // Redraw tables after animation
    setTimeout(() => {
      this.parentTable?.table?.redraw?.();
      this.childTable?.table?.redraw?.();
    }, 350);
  }

  // ── Footer Setup ───────────────────────────────────────────────────────────

  setupFooter() {
    // Walk up to the .manager-slot from our workspace container
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    // Find the subpanel-header-group
    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    // Remove the generic "Reports Placeholder" button
    const reportsBtn = group.querySelector('.slot-reports-btn');
    if (reportsBtn) reportsBtn.remove();

    // Identify the first right-side fixed button to use as insertion anchor
    const anchor = group.querySelector('.slot-notifications-btn');

    // Create parent table select
    const parentSelect = document.createElement('select');
    parentSelect.id = 'lookups-parent-select';
    parentSelect.className = 'lookups-footer-select';
    parentSelect.title = 'Parent Table Data Source';
    parentSelect.innerHTML = `
      <option value="view">Lookup List View</option>
      <option value="data">Lookup List Data</option>
    `;
    parentSelect.addEventListener('change', (e) => {
      this.handleParentSelectChange(e.target.value);
    });
    group.insertBefore(parentSelect, anchor);

    // Create child table select
    const childSelect = document.createElement('select');
    childSelect.id = 'lookups-child-select';
    childSelect.className = 'lookups-footer-select';
    childSelect.title = 'Child Table Data Source';
    childSelect.innerHTML = `
      <option value="view">Lookup Values View</option>
      <option value="data">Lookup Values Data</option>
    `;
    childSelect.addEventListener('change', (e) => {
      this.handleChildSelectChange(e.target.value);
    });
    group.insertBefore(childSelect, anchor);

    // Filler button
    const fillerBtn = document.createElement('button');
    fillerBtn.type = 'button';
    fillerBtn.className = 'subpanel-header-btn lookups-footer-filler';
    fillerBtn.title = 'Lookups';
    group.insertBefore(fillerBtn, anchor);

    log(Subsystems.MANAGER, Status.INFO, '[LookupsManager] Footer controls initialized');
  }

  handleParentSelectChange(value) {
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Parent select changed to: ${value}`);
    if (value === 'data') {
      toast.info('Data View', { description: 'Raw data view not yet implemented', duration: 3000 });
    }
  }

  handleChildSelectChange(value) {
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Child select changed to: ${value}`);
    if (value === 'data') {
      toast.info('Data View', { description: 'Raw data view not yet implemented', duration: 3000 });
    }
  }

  // ── Lifecycle Methods ──────────────────────────────────────────────────────

  /**
   * Called when the manager is activated
   */
  onActivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Lookups] Activated');

    // Redraw tables to ensure proper layout
    if (this.parentTable?.table) {
      this.parentTable.table.redraw?.();
    }
    if (this.childTable?.table) {
      this.childTable.table.redraw?.();
    }
  }

  /**
   * Called when the manager is deactivated
   */
  onDeactivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Lookups] Deactivated');
  }

  /**
   * Called before the manager is destroyed
   */
  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[Lookups] Cleaning up...');

    // Destroy SunEditor
    if (this.sunEditor) {
      this.sunEditor.destroy();
      this.sunEditor = null;
    }

    // Remove font popup
    if (this.fontPopup) {
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    // Clean up tables
    this.parentTable?.destroy();
    this.childTable?.destroy();

    this.parentTable = null;
    this.childTable = null;
  }
}
