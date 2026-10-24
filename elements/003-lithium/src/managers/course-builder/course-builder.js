/**
 * Course Builder — Manager ID 35
 *
 * Staff suggestion list. The left side is a LithiumTable. The right
 * side opens on Submission. The toolbar sits above the Working Title.
 * Authz is the same L3 check as Course Manager.
 */

import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { initToolbars } from '../../core/manager-ui.js';
import { setupManagerFooterIcons } from '../../core/manager-export.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { restorePanelState, togglePanelCollapse } from '../../core/panel-collapse.js';
import { sanitizeHtml } from '../../core/utils.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import LithiumTable from '../../tables/lithium-table-main.js';
import { resolveCourseManagerAccess } from '../course-manager/course-access.js';
import {
  ANNOTATION_MAX,
  BUILDER_TABLE_KEY_IDX,
  BUILDER_TABLE_PATH,
  SUBMISSION_FIELDS,
  SUGGESTION_LIST_QUERY_REF,
  SUGGESTION_SEARCH_FIELDS,
  WORKING_TITLE_MAX,
  suggestionListCells,
  suggestionRowData,
  workingTitleForEdit,
} from './course-suggestions.js';
import {
  builderDirty,
  cancelBuilder,
  installBuilderActions,
  onStageChange,
  paintBuilderChrome,
  restoreSubmission,
  saveBuilder,
} from './course-builder-actions.js';
import './course-builder.css';

export default class CourseBuilderManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this._allowed = false;
    this.suggestionTable = null;
    this._selectedId = null;
    this._row = null;
    this._mode = 'view';
    this._loadedTitle = '';
    this._loadedAnnotation = '';
    this._sanitize = sanitizeHtml;
    this._dirty = false;
    this._busy = false;
    this._switching = false;
    this._saveBtn = null;
    this._cancelBtn = null;
    this.leftPanelState = new PanelStateManager('lithium_builder_left');
    this.leftPanelWidth = this.leftPanelState.loadWidth(622);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);
    this.splitter = null;
  }

  async init() {
    await this.render();
  }

  async render() {
    const access = await resolveCourseManagerAccess(this.app && this.app.api);
    this._allowed = access.allowed;
    if (!access.allowed) {
      this._renderDenied();
      return;
    }
    this._renderStaff();
    await this._mountStaff();
  }

  _renderDenied() {
    this.container.innerHTML = `
      <div class="builder-denied-wrap">
        <div class="builder-denied">
          <fa fa-lock></fa>
          <h2>Course Builder</h2>
          <p>
            You do not have permission to access Course Builder.
            Contact your system administrator if you believe this is an error.
          </p>
        </div>
      </div>
    `;
    processIcons(this.container);
  }

  _submissionFields() {
    return SUBMISSION_FIELDS.map((field) => `
      <div class="builder-field">
        <span class="builder-field-label">${field.label}</span>
        <div class="${field.html ? 'builder-detail' : 'builder-text'}" data-field="${field.key}"></div>
      </div>
    `).join('');
  }

  _renderStaff() {
    this.container.innerHTML = `
      <div class="builder-staff">
        <div class="manager-panels builder-panels">
          <div class="builder-left-panel" data-panel="left" id="builder-left-panel">
            <div id="builder-table" class="builder-table-host lithium-table-container"></div>
            <div id="builder-navigator" class="builder-navigator lithium-navigator-container"></div>
          </div>
          <div class="lithium-splitter" data-splitter="left" id="builder-splitter"></div>
          <div class="builder-right-panel" data-panel="right">
            <div class="lithium-toolbar" id="builder-toolbar">
              <button type="button" class="lithium-toolbar-btn lithium-collapse-btn" id="builder-collapse-btn" data-collapse-target="left" title="Toggle Table">
                <fa fa-angles-left class="lithium-collapse-icon"></fa>
              </button>
              <button type="button" class="lithium-toolbar-tab active" data-tab="submission" data-collapsible-text>
                <fa fa-lightbulb></fa>
                <span>Submission</span>
              </button>
              <div class="lithium-toolbar-placeholder"></div>
            </div>
            <div class="builder-title-bar">
              <label for="builder-working-title">Working Title</label>
              <input id="builder-working-title" class="input" type="text" maxlength="${WORKING_TITLE_MAX}" autocomplete="off" disabled>
              <div class="builder-stage-row">
                <label for="builder-stage">Stage</label>
                <select id="builder-stage" class="input" disabled></select>
                <span id="builder-receipt" class="builder-receipt"></span>
                <span id="builder-flag" class="builder-flag"></span>
              </div>
            </div>
            <div class="builder-tabs">
              <div class="builder-tab-pane active" id="builder-tab-submission">
                <div class="builder-form">
                  ${this._submissionFields()}
                  <div class="builder-field">
                    <label class="builder-field-label" for="builder-annotation">Annotation</label>
                    <textarea id="builder-annotation" class="input builder-area" maxlength="${ANNOTATION_MAX}" disabled></textarea>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    `;
    processIcons(this.container);
    initToolbars();
    this.elements = {
      leftPanel: this.container.querySelector('#builder-left-panel'),
      collapseBtn: this.container.querySelector('#builder-collapse-btn'),
      navigator: this.container.querySelector('#builder-navigator'),
      table: this.container.querySelector('#builder-table'),
      title: this.container.querySelector('#builder-working-title'),
      stage: this.container.querySelector('#builder-stage'),
      receipt: this.container.querySelector('#builder-receipt'),
      flag: this.container.querySelector('#builder-flag'),
      annotation: this.container.querySelector('#builder-annotation'),
      form: this.container.querySelector('.builder-form'),
    };
  }

  async _mountStaff() {
    this._bindStaff();
    this._wireFooter();
    this._applyRow(null);
    try {
      await this._mountTable();
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `Course Builder list failed: ${error.message}`);
      toast.error('Suggestion list failed to load', { description: error.message });
    }
  }

  _bindStaff() {
    this.elements.collapseBtn?.addEventListener('click', () => this._toggleLeftPanel());
    this.elements.title.addEventListener('input', () => this._syncDirty());
    this.elements.annotation?.addEventListener('input', () => this._syncDirty());
    this.elements.form?.addEventListener('input', () => this._syncDirty());
    this.elements.stage?.addEventListener('change', () => onStageChange(this));
  }

  _wireFooter() {
    const slot = this.container.closest('.manager-slot');
    const group = slot?.querySelector('.manager-slot-footer .subpanel-header-group');
    if (!group) return;
    const placeholder = group.querySelector('.slot-footer-placeholder');
    const footer = setupManagerFooterIcons(group, {
      onPrint: () => this._print(),
      onEmail: () => this._email(),
      onDownload: () => this._download(),
      onExport: (value) => this._download(value),
      reportOptions: [{ value: 'suggestions-view', label: 'Suggestions' }],
      anchor: placeholder,
      showSaveCancel: true,
      onSave: () => saveBuilder(this),
      onCancel: () => cancelBuilder(this),
      exportOptions: [
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
      ],
    });
    this._saveBtn = footer.saveBtn;
    this._cancelBtn = footer.cancelBtn;
    this._syncDirty();
  }

  async _mountTable() {
    this.suggestionTable = new LithiumTable({
      container: this.elements.table,
      navigatorContainer: this.elements.navigator,
      tablePath: BUILDER_TABLE_PATH,
      lookupKeyIdx: BUILDER_TABLE_KEY_IDX,
      primaryKeyField: ['suggestion_id'],
      queryRef: SUGGESTION_LIST_QUERY_REF,
      cssPrefix: 'builder',
      storageKey: 'course_builder_table',
      app: this.app,
      readonly: false,
      localSearch: true,
      localSearchFields: SUGGESTION_SEARCH_FIELDS,
      tableWidthMode: 'wide',
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (row) => this._onRowSelected(row),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `Course Builder loaded ${rows.length} suggestions`);
      },
    });
    await this.suggestionTable.init();
    installBuilderActions(this);
    this._labelNavigatorSearch();
    this._setupSplitter();
    await this.suggestionTable.loadData();
  }

  _labelNavigatorSearch() {
    const input = this.container.querySelector('#builder-search-input');
    if (!input) return;
    input.placeholder = 'Search title, name, or email';
    input.setAttribute('aria-label', 'Search suggestions');
  }

  _setupSplitter() {
    const element = this.container.querySelector('#builder-splitter');
    if (!element || !this.elements.leftPanel) return;
    this.splitter = new LithiumSplitter({
      element,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.suggestionTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.suggestionTable?.table?.redraw?.();
      },
    });
    this.suggestionTable?.setSplitter?.(this.splitter);
    restorePanelState({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      isCollapsed: this.isLeftPanelCollapsed,
    });
  }

  _toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.suggestionTable?.table?.redraw?.(),
    });
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  async _onRowSelected(row) {
    if (this._switching) return;
    const data = suggestionRowData(row);
    const nextId = Number(data && data.suggestion_id);
    const same = Number.isInteger(nextId) && nextId === this._selectedId;
    if (this._dirty && !same) {
      const discard = await confirmDialog({
        title: 'Unsaved changes',
        message: 'Discard the unsaved suggestion changes?',
        confirmLabel: 'Discard',
        cancelLabel: 'Stay',
      });
      if (!discard) {
        this._reselect(this._selectedId);
        return;
      }
    }
    this._applyRow(data);
  }

  /**
   * @param {object|null|undefined} row
   */
  _applyRow(row) {
    const data = suggestionRowData(row);
    const id = Number(data && data.suggestion_id);
    const hasRow = Number.isInteger(id) && id > 0;
    this._mode = 'view';
    this._row = hasRow ? data : null;
    this._selectedId = hasRow ? id : null;
    this._loadedTitle = hasRow ? workingTitleForEdit(data).trim() : '';
    this.elements.title.value = this._loadedTitle;
    this.elements.title.disabled = !hasRow;
    restoreSubmission(this);
    paintBuilderChrome(this);
    this._dirty = false;
    this._syncDirty();
    if (hasRow) {
      const cells = suggestionListCells(data);
      log(Subsystems.MANAGER, Status.INFO,
        `Course Builder selected ${id} state=${cells.state}`);
    }
  }

  _syncDirty() {
    this._dirty = builderDirty(this);
    if (this._saveBtn) this._saveBtn.disabled = !this._dirty || this._busy;
    if (this._cancelBtn) this._cancelBtn.disabled = !this._dirty || this._busy;
  }

  /**
   * @param {number|null} id
   */
  async _reload(id) {
    if (!this.suggestionTable?.loadData) return;
    this._switching = true;
    try {
      await this.suggestionTable.loadData();
    } finally {
      this._switching = false;
    }
    this._reselect(id);
  }

  /**
   * @param {number|null} id
   */
  _reselect(id) {
    const table = this.suggestionTable?.table;
    if (!table || typeof table.getRows !== 'function' || !Number.isInteger(id)) return;
    const match = table.getRows().find((row) => {
      const data = row.getData?.();
      return Number(data && data.suggestion_id) === id;
    });
    if (!match || typeof table.selectRow !== 'function') return;
    this._switching = true;
    try {
      table.deselectRow?.();
      table.selectRow(match);
    } finally {
      this._switching = false;
    }
  }

  _table() {
    return this.suggestionTable && this.suggestionTable.table;
  }

  _print() {
    const table = this._table();
    if (!table || typeof table.print !== 'function') {
      toast.info('No Data', { description: 'No table available to print', duration: 3000 });
      return;
    }
    table.print();
  }

  /**
   * @param {string} [format]
   */
  _download(format) {
    const table = this._table();
    if (!table || typeof table.download !== 'function') {
      toast.info('No Data', { description: 'No table available to download', duration: 3000 });
      return;
    }
    const kind = format === 'json' ? 'json' : 'csv';
    const filename = `suggestions-${new Date().toISOString().slice(0, 10)}`;
    table.download(kind, `${filename}.${kind}`);
  }

  _email() {
    const table = this._table();
    const rows = table && typeof table.getRows === 'function' ? table.getRows('active') : [];
    if (!rows || rows.length === 0) {
      toast.info('No Data', { description: 'No rows to include in the email', duration: 3000 });
      return;
    }
    const lines = rows.slice(0, 50).map((row) => {
      const data = row.getData ? row.getData() : row;
      return suggestionListCells(data).workingTitle;
    });
    const subject = encodeURIComponent(`Suggestions — ${rows.length} rows`);
    const body = encodeURIComponent(lines.join('\n'));
    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  cleanup() {
    this.splitter?._detachListeners?.();
    this.splitter = null;
    if (this.suggestionTable && typeof this.suggestionTable.destroy === 'function') {
      this.suggestionTable.destroy();
    }
    this.suggestionTable = null;
    if (this.container) this.container.innerHTML = '';
    this.elements = {};
    this._saveBtn = null;
    this._cancelBtn = null;
  }
}
