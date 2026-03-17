/**
 * Lookups Manager — Manager ID 5
 *
 * A dual-table interface for managing lookup tables:
 * - Left panel (parent): List of all lookups
 * - Right panel (child): Values for the selected lookup
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

    // Splitter state
    this.leftPanelWidth = 350;
    this.isLeftPanelCollapsed = false;
    this.isResizing = false;
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupSplitter();
    await this.initParentTable();
    await this.initChildTable();
    this.setupFooter();
  }

  async render() {
    this.container.innerHTML = `
      <div class="lookups-manager-container">
        <!-- Left Panel: Parent Table (Lookups List) -->
        <div class="lookups-left-panel" id="lookups-left-panel">
          <div class="lookups-table-container" id="lookups-parent-table-container"></div>
          <div class="lookups-navigator-container" id="lookups-parent-navigator"></div>
        </div>

        <!-- Splitter -->
        <div class="lookups-splitter" id="lookups-splitter"></div>

        <!-- Right Panel: Child Table (Lookup Values) -->
        <div class="lookups-right-panel" id="lookups-right-panel">
          <div class="lookups-child-header">
            <button type="button" class="subpanel-header-btn subpanel-header-close lookups-collapse-btn" id="lookups-collapse-btn" title="Toggle left panel">
              <fa fa-chevron-left id="lookups-collapse-icon"></fa>
            </button>
            <span class="lookups-child-title" id="lookups-child-title">Select a lookup</span>
          </div>
          <div class="lookups-child-table-container" id="lookups-child-table-container"></div>
          <div class="lookups-navigator-container" id="lookups-child-navigator"></div>
        </div>
      </div>
    `;

    this.elements = {
      container: this.container.querySelector('.lookups-manager-container'),
      leftPanel: this.container.querySelector('#lookups-left-panel'),
      parentTableContainer: this.container.querySelector('#lookups-parent-table-container'),
      parentNavigator: this.container.querySelector('#lookups-parent-navigator'),
      splitter: this.container.querySelector('#lookups-splitter'),
      rightPanel: this.container.querySelector('#lookups-right-panel'),
      childTableContainer: this.container.querySelector('#lookups-child-table-container'),
      childNavigator: this.container.querySelector('#lookups-child-navigator'),
      childTitle: this.container.querySelector('#lookups-child-title'),
      collapseBtn: this.container.querySelector('#lookups-collapse-btn'),
      collapseIcon: this.container.querySelector('#lookups-collapse-icon'),
    };

    // Process icons
    processIcons(this.container);
  }

  setupEventListeners() {
    // Collapse/expand button
    this.elements.collapseBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });
  }

  // ── Parent Table Initialization ────────────────────────────────────────────

  async initParentTable() {
    if (!this.elements.parentTableContainer || !this.elements.parentNavigator) return;

    this.parentTable = new LithiumTable({
      container: this.elements.parentTableContainer,
      navigatorContainer: this.elements.parentNavigator,
      tablePath: 'lookups/lookups-list',
      queryRef: 30, // QueryRef 30 - Lookup Names
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
      queryRef: 34, // QueryRef 034 - Get Lookup List (requires LOOKUPID param)
      cssPrefix: 'lookups-child',
      storageKey: 'lookups_child_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => {
        log(Subsystems.TABLE, Status.DEBUG, `[Lookups] Child row selected: ${rowData?.LOOKUPVALUEID}`);
      },
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookup values`);
      },
    });

    await this.childTable.init();
  }

  // ── Parent/Child Relationship ──────────────────────────────────────────────

  async handleParentRowSelected(rowData) {
    if (!rowData) return;

    const lookupId = rowData.LOOKUPID;
    const lookupName = rowData.LOOKUPNAME;

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
      this.elements.childTitle.textContent = 'Select a lookup';
    }

    // Clear child table data
    if (this.childTable?.table) {
      this.childTable.table.setData([]);
    }
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

  // ── Splitter Logic ─────────────────────────────────────────────────────────

  setupSplitter() {
    const splitter = this.elements.splitter;
    const leftPanel = this.elements.leftPanel;

    if (!splitter || !leftPanel) return;

    // Mouse down on splitter
    splitter.addEventListener('mousedown', (e) => {
      if (this.isLeftPanelCollapsed) return;

      this.isResizing = true;
      splitter.classList.add('resizing');
      document.body.style.cursor = 'col-resize';
      document.body.style.userSelect = 'none';

      const startX = e.clientX;
      const startWidth = leftPanel.offsetWidth;

      const onMouseMove = (e) => {
        if (!this.isResizing) return;

        const delta = e.clientX - startX;
        const newWidth = Math.max(200, Math.min(600, startWidth + delta));

        leftPanel.style.width = `${newWidth}px`;
        this.leftPanelWidth = newWidth;
      };

      const onMouseUp = () => {
        this.isResizing = false;
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

      this.isResizing = true;
      splitter.classList.add('resizing');

      const touch = e.touches[0];
      const startX = touch.clientX;
      const startWidth = leftPanel.offsetWidth;

      const onTouchMove = (e) => {
        if (!this.isResizing) return;

        const touch = e.touches[0];
        const delta = touch.clientX - startX;
        const newWidth = Math.max(200, Math.min(600, startWidth + delta));

        leftPanel.style.width = `${newWidth}px`;
        this.leftPanelWidth = newWidth;
      };

      const onTouchEnd = () => {
        this.isResizing = false;
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
    const splitter = this.elements.splitter;
    const collapseIcon = this.elements.collapseIcon;

    if (!leftPanel || !splitter) return;

    this.isLeftPanelCollapsed = !this.isLeftPanelCollapsed;

    if (this.isLeftPanelCollapsed) {
      leftPanel.classList.add('collapsed');
      splitter.classList.add('collapsed');
      if (collapseIcon) collapseIcon.style.transform = 'rotate(180deg)';
    } else {
      leftPanel.classList.remove('collapsed');
      leftPanel.style.width = `${this.leftPanelWidth}px`;
      splitter.classList.remove('collapsed');
      if (collapseIcon) collapseIcon.style.transform = 'rotate(0deg)';
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
    parentSelect.className = 'queries-footer-datasource';
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
    childSelect.className = 'queries-footer-datasource';
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
    fillerBtn.className = 'subpanel-header-btn queries-footer-filler';
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

    // Clean up tables
    this.parentTable?.destroy();
    this.childTable?.destroy();

    this.parentTable = null;
    this.childTable = null;
  }
}
