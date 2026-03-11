/**
 * Queries Manager
 *
 * Query builder and execution interface.
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import 'tabulator-tables/dist/css/tabulator_midnight.min.css';
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
    this.summaryEditor = null;
    this.collectionEditor = null;
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
      summaryEditorContainer: this.container.querySelector('#queries-tab-summary'),
      collectionEditorContainer: this.container.querySelector('#queries-tab-collection'),
      previewContainer: this.container.querySelector('#queries-tab-preview'),
      testContainer: this.container.querySelector('#queries-tab-test'),
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
    const buttons = [this.elements.undoBtn, this.elements.redoBtn, this.elements.fontDecreaseBtn, this.elements.fontIncreaseBtn];
    buttons.forEach(btn => {
      if (btn) {
        btn.disabled = !isSqlTab;
      }
    });
    
    // Prettify button should be enabled when SQL tab is visible
    if (this.elements.prettifyBtn) {
      this.elements.prettifyBtn.disabled = !isSqlTab;
    }
    
    // Initialize editors when switching to their tabs
    if (tabId === 'sql') {
      const sqlContent = this._pendingSqlContent || this.currentQuery?.code || this.currentQuery?.query_text || this.currentQuery?.sql || '';
      if (!this.sqlEditor) {
        this.initSqlEditor(sqlContent);
      }
    }
    
    // Initialize summary editor if needed
    if (tabId === 'summary') {
      const summaryContent = this._pendingSummaryContent || this.currentQuery?.summary || this.currentQuery?.markdown || '';
      if (!this.summaryEditor) {
        this.initSummaryEditor(summaryContent);
      }
    }
    
    // Initialize collection editor if needed
    if (tabId === 'collection') {
      let collectionContent = this._pendingCollectionContent || this.currentQuery?.collection || this.currentQuery?.json || {};
      if (!this.collectionEditor) {
        this.initCollectionEditor(collectionContent);
      }
    }
    
    // Render preview if needed
    if (tabId === 'preview') {
      this.renderMarkdownPreview();
    }
  }

  async initTable() {
    // Initialize Tabulator
    this.table = new Tabulator(this.elements.tableContainer, {
      layout: "fitColumns",
      responsiveLayout: "collapse",
      selectableRows: 1,
      resizableColumns: false,
      // Custom dual-arrow sort indicator (▲/▼ with active direction highlighted)
      headerSortElement: '<span class="queries-sort-icons"><span class="queries-sort-asc">▲</span><span class="queries-sort-desc">▼</span></span>',
      columns: [
        { title: "Ref", field: "query_ref", width: 80, headerSort: true },
        { title: "Name", field: "name", headerSort: true },
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
      // Show detailed error toast with subsystem badge
      // (toast.error auto-logs server error details via _logServerError)
      toast.error('Query Load Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
    }
  }

  loadQueryDetails(queryData) {
    // Store current query reference
    this.currentQuery = queryData;
    
    // Fetch full query details using QueryRef 27
    this.fetchQueryDetails(queryData.query_id);
  }

  /**
   * Fetch full query details from the API
   */
  async fetchQueryDetails(queryId) {
    try {
      // QueryRef 27: Get System Query - returns full query details
      const queryDetails = await authQuery(this.app.api, 27, {
        INTEGER: { QUERYID: queryId },
      });
      
      if (queryDetails && queryDetails.length > 0) {
        const fullQuery = queryDetails[0];
        
        // Update current query with full details
        this.currentQuery = fullQuery;
        
        // Get content from API response - using correct field names from QueryRef 27
        const sqlContent = fullQuery.code || fullQuery.query_text || fullQuery.sql || '';
        const summaryContent = fullQuery.summary || fullQuery.markdown || '';
        const collectionContent = fullQuery.collection || fullQuery.json || {};
        
        // Initialize or update the SQL editor with the query content
        // Only initialize if we're on the SQL tab or if editor doesn't exist
        if (this.sqlEditor) {
          this.sqlEditor.dispatch({
            changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: sqlContent },
          });
        }
        
        // Update summary content if editor exists
        if (this.summaryEditor) {
          this.summaryEditor.dispatch({
            changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: summaryContent },
          });
        }
        
        // Update collection content if editor exists
        if (this.collectionEditor) {
          const collectionStr = typeof collectionContent === 'object' 
            ? JSON.stringify(collectionContent, null, 2) 
            : collectionContent;
          this.collectionEditor.dispatch({
            changes: { from: 0, to: this.collectionEditor.state.doc.length, insert: collectionStr },
          });
        }
        
        // Store data for lazy initialization
        this._pendingSqlContent = sqlContent;
        this._pendingSummaryContent = summaryContent;
        this._pendingCollectionContent = collectionContent;
        
        // If SQL tab is currently active, initialize the editor immediately
        const sqlTabBtn = document.querySelector('.queries-tab-btn[data-tab="sql"]');
        const isSqlTabActive = sqlTabBtn?.classList.contains('active');
        
        if (isSqlTabActive && !this.sqlEditor) {
          this.initSqlEditor(sqlContent);
        } else if (this.sqlEditor && sqlContent) {
          // Editor exists, update content
          this.sqlEditor.dispatch({
            changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: sqlContent },
          });
        }
        
//        console.log('Loaded query details:', fullQuery);
      }
    } catch (error) {
      // Show detailed error toast - title comes from serverError.error, description from serverError.message
      // (toast.error auto-logs server error details via _logServerError)
      toast.error('Query Details Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
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
   * Initialize CodeMirror Summary editor (Markdown)
   */
  async initSummaryEditor(initialContent = '') {
    if (!this.elements.summaryEditorContainer) return;
    
    // Use pending content if available
    if (!initialContent && this._pendingSummaryContent) {
      initialContent = this._pendingSummaryContent;
    }
    
    // If editor already exists, just update content
    if (this.summaryEditor) {
      this.summaryEditor.dispatch({
        changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: initialContent },
      });
      return;
    }

    try {
      // Dynamic import CodeMirror packages
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap },
        { defaultKeymap, history, historyKeymap },
        { markdown },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-markdown'),
        import('@codemirror/theme-one-dark'),
      ]);

      // Create editor state with Markdown support
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
          markdown(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%', fontSize: '14px' },
            '.cm-scroller': { overflow: 'auto' },
            '.cm-content': { fontFamily: 'var(--font-mono, monospace)' },
          }),
        ],
      });

      this.summaryEditor = new EditorView({
        state: startState,
        parent: this.elements.summaryEditorContainer,
      });
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize Summary editor:', error);
      // Fallback to textarea
      this.elements.summaryEditorContainer.innerHTML = `
        <textarea id="queries-summary-editor-fallback" 
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:14px;">
          ${initialContent}
        </textarea>`;
    }
  }

  /**
   * Initialize CodeMirror Collection editor (JSON)
   */
  async initCollectionEditor(initialContent = {}) {
    if (!this.elements.collectionEditorContainer) return;
    
    // Use pending content if available
    let content = initialContent;
    if (!initialContent && this._pendingCollectionContent) {
      content = this._pendingCollectionContent;
    }
    
    // Convert object to JSON string if needed
    const jsonContent = typeof content === 'object' 
      ? JSON.stringify(content, null, 2) 
      : content;
    
    // If editor already exists, just update content
    if (this.collectionEditor) {
      this.collectionEditor.dispatch({
        changes: { from: 0, to: this.collectionEditor.state.doc.length, insert: jsonContent },
      });
      return;
    }

    try {
      // Dynamic import CodeMirror packages
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap },
        { defaultKeymap, history, historyKeymap },
        { json },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-json'),
        import('@codemirror/theme-one-dark'),
      ]);

      // Create editor state with JSON support
      const startState = EditorState.create({
        doc: jsonContent,
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          json(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%', fontSize: '14px' },
            '.cm-scroller': { overflow: 'auto' },
            '.cm-content': { fontFamily: 'var(--font-mono, monospace)' },
          }),
        ],
      });

      this.collectionEditor = new EditorView({
        state: startState,
        parent: this.elements.collectionEditorContainer,
      });
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize Collection editor:', error);
      // Fallback to textarea
      this.elements.collectionEditorContainer.innerHTML = `
        <textarea id="queries-collection-editor-fallback" 
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:14px;">
          ${jsonContent}
        </textarea>`;
    }
  }

  /**
   * Render Markdown as HTML in Preview tab
   */
  async renderMarkdownPreview() {
    if (!this.elements.previewContainer) return;
    
    // Get content from summary editor or pending content
    let markdownContent = '';
    if (this.summaryEditor) {
      markdownContent = this.summaryEditor.state.doc.toString();
    } else if (this._pendingSummaryContent) {
      markdownContent = this._pendingSummaryContent;
    } else if (this.currentQuery?.summary) {
      markdownContent = this.currentQuery.summary;
    }
    
    if (!markdownContent.trim()) {
      this.elements.previewContainer.innerHTML = '<p class="text-muted">No summary content to preview.</p>';
      return;
    }
    
    try {
      // Dynamic import marked for markdown parsing
      const { marked } = await import('marked');
      
      // Parse markdown to HTML
      const htmlContent = await marked.parse(markdownContent);
      
      // Render in preview container with styling
      this.elements.previewContainer.innerHTML = `
        <div class="queries-preview-content">
          ${htmlContent}
        </div>
      `;
    } catch (error) {
      console.error('[QueriesManager] Failed to render markdown preview:', error);
      // Fallback: Show raw content
      const escapedContent = markdownContent
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
      this.elements.previewContainer.innerHTML = `
        <div class="queries-preview-content">
          <pre><code>${escapedContent}</code></pre>
        </div>
      `;
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
      toast.error('SQL Format Failed', {
        serverError: error.serverError,
        subsystem: 'Query',
        duration: 8000,
      });
    }
  }

  teardown() {
    // Cleanup event listeners
    if (this.elements.splitter) {
      this.elements.splitter.removeEventListener('mousedown', this.handleSplitterMouseDown);
    }
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
    
    // Destroy CodeMirror editors
    if (this.sqlEditor) {
      this.sqlEditor.destroy();
      this.sqlEditor = null;
    }
    if (this.summaryEditor) {
      this.summaryEditor.destroy();
      this.summaryEditor = null;
    }
    if (this.collectionEditor) {
      this.collectionEditor.destroy();
      this.collectionEditor = null;
    }
  }
}
