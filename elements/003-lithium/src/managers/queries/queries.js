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

export default class QueriesManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.isResizing = false;
    this.table = null;
    this._arrowRotation = 0;
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
      toast.error('Failed to load queries. Please try again.');
    }
  }

  loadQueryDetails(queryData) {
    // Placeholder for loading details into tabs
    console.log('Selected query:', queryData);
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

  teardown() {
    // Cleanup event listeners
    if (this.elements.splitter) {
      this.elements.splitter.removeEventListener('mousedown', this.handleSplitterMouseDown);
    }
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
  }
}
