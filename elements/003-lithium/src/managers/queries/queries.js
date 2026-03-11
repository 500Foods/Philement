/**
 * Queries Manager
 *
 * Query builder and execution interface.
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import 'tabulator-tables/dist/css/tabulator.min.css';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import './queries.css';

// Font size constants
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const FONT_SIZE_STEP = 2;

// History field for CodeMirror undo/redo state
let historyField;

export default class QueriesManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.isResizing = false;
    this.table = null;
    this._arrowRotation = 0;
    
    // CodeMirror editor instance
    this.sqlEditor = null;
    this.currentQuery = null;
    
    // Font size state
    this.fontSize = DEFAULT_FONT_SIZE;
    
    // SQL dialect (could be made configurable)
    this.sqlDialect = 'sql';
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupSplitter();
    await this.initTable();
  }

  async render() {
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
      collapseIcon: this.container.querySelector('#queries-collapse-icon'),
      sqlEditorContainer: this.container.querySelector('#queries-sql-editor'),
      // Toolbar buttons
      undoBtn: this.container.querySelector('#queries-undo-btn'),
      redoBtn: this.container.querySelector('#queries-redo-btn'),
      fontDecreaseBtn: this.container.querySelector('#queries-font-decrease-btn'),
      fontIncreaseBtn: this.container.querySelector('#queries-font-increase-btn'),
      prettifyBtn: this.container.querySelector('#queries-prettify-btn'),
    };
  }

  setupEventListeners() {
    // Tab switching
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => {
        const tabId = btn.dataset.tab;
        this.switchTab(tabId);
      });
    });

    // Collapse/Expand
    this.elements.collapseBtn?.addEventListener('click', () => {
      this.toggleCollapse();
    });

    // Navigator Search
    this.elements.navigatorContainer.innerHTML = `
      <div class="queries-search-box">
        <input type="text" id="queries-search-input" placeholder="Search queries...">
        <button id="queries-search-btn"><fa fa-search></fa></button>
      </div>
    `;

    const searchInput = this.elements.navigatorContainer.querySelector('#queries-search-input');
    const searchBtn = this.elements.navigatorContainer.querySelector('#queries-search-btn');

    const performSearch = () => {
      this.loadQueries(searchInput.value);
    };

    searchBtn.addEventListener('click', performSearch);
    searchInput.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        performSearch();
      }
    });
    
    // Toolbar buttons
    this.elements.undoBtn?.addEventListener('click', () => this.handleUndo());
    this.elements.redoBtn?.addEventListener('click', () => this.handleRedo());
    this.elements.fontDecreaseBtn?.addEventListener('click', () => this.handleFontDecrease());
    this.elements.fontIncreaseBtn?.addEventListener('click', () => this.handleFontIncrease());
    this.elements.prettifyBtn?.addEventListener('click', () => this.handlePrettify());
  }

  toggleCollapse() {
    const isCollapsed = this.elements.leftPanel.classList.toggle('collapsed');
    this.elements.splitter.classList.toggle('collapsed', isCollapsed);

    // Rotate the collapse arrow icon.
    // Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
    // Always use a fresh DOM query (same pattern as main.js _getArrowEl).
    const iconEl = this._getCollapseIconEl();
    if (iconEl) {
      this._arrowRotation += 180;
      iconEl.style.transform = `rotate(${this._arrowRotation}deg)`;
    }
  }

  /**
   * Get the collapse arrow icon element via a fresh DOM query.
   * Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
   * @returns {HTMLElement|null}
   */
  _getCollapseIconEl() {
    return document.querySelector('#queries-collapse-icon') ||
           document.querySelector('#queries-collapse-btn')?.firstElementChild;
  }

  switchTab(tabId) {
    // Update buttons
    this.elements.tabBtns.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.tab === tabId);
    });

    // Update panes
    this.elements.tabPanes.forEach(pane => {
      pane.classList.toggle('active', pane.id === `queries-tab-${tabId}`);
    });
    
    // Enable/disable toolbar buttons based on tab
    const isSqlTab = tabId === 'sql';
    const buttons = [this.elements.undoBtn, this.elements.redoBtn, this.elements.fontDecreaseBtn, this.elements.fontIncreaseBtn, this.elements.prettifyBtn];
    buttons.forEach(btn => {
      if (btn) {
        // Only disable undo/redo based on actual state, keep font buttons enabled if on SQL tab
        if (!isSqlTab) {
          btn.disabled = true;
        } else if (btn === this.elements.undoBtn || btn === this.elements.redoBtn) {
          // Keep undo/redo enabled - state will be updated by updateToolbarState
          btn.disabled = false;
        }
      }
    });
    
    // Initialize SQL editor when switching to SQL tab
    if (tabId === 'sql' && !this.sqlEditor) {
      this.initSqlEditor(this.currentQuery?.query_text || this.currentQuery?.sql || '');
    }
  }

  async initTable() {
    // Initialize Tabulator
    this.table = new Tabulator(this.elements.tableContainer, {
      layout: "fitColumns",
      responsiveLayout: "collapse",
      selectableRows: 1,
      columns: [
        { title: "Ref", field: "query_ref", width: 80 },
        { title: "Name", field: "name" },
      ],
    });

    this.table.on("rowSelectionChanged", (data, rows) => {
      if (data.length > 0) {
        this.loadQueryDetails(data[0]);
      } else {
        this.clearQueryDetails();
      }
    });

    // Load initial data
    await this.loadQueries();
  }

  async loadQueries(searchTerm = '') {
    try {
      let rows;
      if (searchTerm) {
        // QueryRef 32: Get Queries List + Search
        rows = await authQuery(this.app.api, 32, {
          STRING: { SEARCH: searchTerm.toUpperCase() },
        });
      } else {
        // QueryRef 25: Get Queries List
        rows = await authQuery(this.app.api, 25);
      }

      this.table.setData(rows);
    } catch (error) {
      console.error('[QueriesManager] Failed to load queries:', error);
      // Show detailed error message from server if available
      const errorMessage = error.serverError?.message || error.message || 'Failed to load queries. Please try again.';
      toast.error(errorMessage);
    }
  }

  loadQueryDetails(queryData) {
    // Store current query reference
    this.currentQuery = queryData;
    
    // Fetch full query details using QueryRef 27
    this.fetchQueryDetails(queryData.query_ref);
  }

  /**
   * Fetch full query details from the API
   */
  async fetchQueryDetails(queryRef) {
    try {
      // QueryRef 27: Get System Query - returns full query details
      const queryDetails = await authQuery(this.app.api, 27, {
        INTEGER: { QUERY_REF: queryRef },
      });
      
      if (queryDetails && queryDetails.length > 0) {
        const fullQuery = queryDetails[0];
        
        // Update current query with full details
        this.currentQuery = fullQuery;
        
        // Get SQL content from query
        const sqlContent = fullQuery.query_text || fullQuery.sql || '';
        
        // Initialize or update the SQL editor with the query content
        this.initSqlEditor(sqlContent);
        
        console.log('Loaded query details:', fullQuery);
      }
    } catch (error) {
      console.error('[QueriesManager] Failed to fetch query details:', error);
      toast.error('Failed to load query details. Please try again.');
    }
  }

  clearQueryDetails() {
    // Placeholder for clearing tabs
  }

  setupSplitter() {
    if (!this.elements.splitter) return;

    this.handleSplitterMouseDown = this.handleSplitterMouseDown.bind(this);
    this.handleSplitterMouseMove = this.handleSplitterMouseMove.bind(this);
    this.handleSplitterMouseUp = this.handleSplitterMouseUp.bind(this);

    this.elements.splitter.addEventListener('mousedown', this.handleSplitterMouseDown);
  }

  handleSplitterMouseDown(event) {
    event.preventDefault();
    this.isResizing = true;
    this.elements.splitter.classList.add('resizing');
    document.body.style.cursor = 'col-resize';
    
    document.addEventListener('mousemove', this.handleSplitterMouseMove);
    document.addEventListener('mouseup', this.handleSplitterMouseUp);
  }

  handleSplitterMouseMove(event) {
    if (!this.isResizing) return;
    
    const containerRect = this.elements.container.getBoundingClientRect();
    const newWidth = event.clientX - containerRect.left;
    
    // Constrain width
    const minWidth = 200;
    const maxWidth = containerRect.width - 300; // Ensure right panel has space
    const constrainedWidth = Math.max(minWidth, Math.min(maxWidth, newWidth));
    
    this.elements.leftPanel.style.width = `${constrainedWidth}px`;
  }

  handleSplitterMouseUp() {
    this.isResizing = false;
    this.elements.splitter.classList.remove('resizing');
    document.body.style.cursor = '';
    
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
  }

  /**
   * Initialize CodeMirror SQL editor
   */
  async initSqlEditor(initialContent = '') {
    if (!this.elements.sqlEditorContainer) return;
    
    // If editor already exists, just update content
    if (this.sqlEditor) {
      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: initialContent },
      });
      return;
    }

    try {
      // Dynamic import CodeMirror packages
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap, highlightActiveLine: highlightActiveLineView },
        { defaultKeymap, history, historyKeymap },
        { sql },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-sql'),
        import('@codemirror/theme-one-dark'),
      ]);
      
      // Import history field for undo/redo state checking
      const historyModule = await import('@codemirror/commands');
      historyField = historyModule.historyField;

      // Create editor state with SQL support
      const startState = EditorState.create({
        doc: initialContent,
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          sql(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%', fontSize: `${this.fontSize}px` },
            '.cm-scroller': { overflow: 'auto' },
            '.cm-content': { fontFamily: 'var(--font-mono, monospace)' },
          }),
          EditorView.updateListener.of((update) => {
            if (update.docChanged) {
              this.updateToolbarState();
            }
          }),
        ],
      });

      this.sqlEditor = new EditorView({
        state: startState,
        parent: this.elements.sqlEditorContainer,
      });
      
      // Update toolbar state after initialization
      this.updateToolbarState();
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize CodeMirror:', error);
      // Fallback to textarea
      this.elements.sqlEditorContainer.innerHTML = `
        <textarea id="queries-sql-editor-fallback" 
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:${this.fontSize}px;">
          ${initialContent}
        </textarea>`;
    }
  }

  /**
   * Update toolbar button states based on editor state
   */
  updateToolbarState() {
    if (!this.sqlEditor) return;
    
    // For CodeMirror 6, we use a simpler approach - check if there are any undo/redo changes available
    // The history field can tell us but it's complex to access. Instead, we'll track this via the update listener
    // For now, we'll enable both buttons and let the user try - CodeMirror will handle the actual state
    
    // Update font size buttons
    if (this.elements.fontDecreaseBtn) {
      this.elements.fontDecreaseBtn.disabled = this.fontSize <= MIN_FONT_SIZE;
    }
    if (this.elements.fontIncreaseBtn) {
      this.elements.fontIncreaseBtn.disabled = this.fontSize >= MAX_FONT_SIZE;
    }
  }

  /**
   * Handle undo button click
   */
  async handleUndo() {
    if (!this.sqlEditor) return;
    
    try {
      const { undo } = await import('@codemirror/commands');
      undo(this.sqlEditor);
    } catch (error) {
      console.error('[QueriesManager] Failed to undo:', error);
    }
  }

  /**
   * Handle redo button click
   */
  async handleRedo() {
    if (!this.sqlEditor) return;
    
    try {
      const { redo } = await import('@codemirror/commands');
      redo(this.sqlEditor);
    } catch (error) {
      console.error('[QueriesManager] Failed to redo:', error);
    }
  }

  /**
   * Handle font decrease button click
   */
  handleFontDecrease() {
    if (this.fontSize <= MIN_FONT_SIZE) return;
    
    this.fontSize = Math.max(MIN_FONT_SIZE, this.fontSize - FONT_SIZE_STEP);
    this.updateEditorFontSize();
  }

  /**
   * Handle font increase button click
   */
  handleFontIncrease() {
    if (this.fontSize >= MAX_FONT_SIZE) return;
    
    this.fontSize = Math.min(MAX_FONT_SIZE, this.fontSize + FONT_SIZE_STEP);
    this.updateEditorFontSize();
  }

  /**
   * Update the editor's font size
   */
  updateEditorFontSize() {
    if (!this.sqlEditor) return;
    
    // Apply font size to the editor's DOM element
    const editorElement = this.sqlEditor.dom;
    if (editorElement) {
      editorElement.style.fontSize = `${this.fontSize}px`;
    }
    
    this.updateToolbarState();
  }

  /**
   * Handle prettify button click
   */
  async handlePrettify() {
    if (!this.sqlEditor) return;
    
    const currentContent = this.sqlEditor.state.doc.toString();
    if (!currentContent.trim()) return;
    
    try {
      // Dynamic import sql-formatter
      const { default: format } = await import('sql-formatter');
      
      const formatted = format(currentContent, {
        language: this.sqlDialect,
        tabWidth: 2,
        keywordCase: 'upper',
      });
      
      // Replace content in editor
      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: formatted },
      });
    } catch (error) {
      console.error('[QueriesManager] Failed to prettify SQL:', error);
      toast.error('Failed to format SQL. Please try again.');
    }
  }

  teardown() {
    // Cleanup event listeners
    if (this.elements.splitter) {
      this.elements.splitter.removeEventListener('mousedown', this.handleSplitterMouseDown);
    }
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
    
    // Destroy CodeMirror editor
    if (this.sqlEditor) {
      this.sqlEditor.destroy();
      this.sqlEditor = null;
    }
  }
}
