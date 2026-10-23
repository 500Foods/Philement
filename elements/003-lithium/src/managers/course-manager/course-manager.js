/**
 * Course Manager — Manager ID 34
 *
 * Staff catalog for the 500 Courses operator surface.
 * Authz is CATCHUP L3: QueryRef 155 names, then staff or admin.
 * The list uses the Query Manager edit path. Learners and inbox
 * land in later Band D phases.
 */

import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { initToolbars } from '../../core/manager-ui.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { restorePanelState, togglePanelCollapse } from '../../core/panel-collapse.js';
import { authQuery, invokeScript } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import LithiumTable from '../../tables/lithium-table-main.js';
import { performLocalSearch } from '../../tables/search/search-utils.js';
import { resolveCourseManagerAccess } from './course-access.js';
import { CourseEditor } from './course-edit.js';
import {
  COURSE_DETAIL_TABS,
  COURSE_LIST_QUERY_REF,
  COURSE_PRICE_CURRENCIES,
  COURSE_PRICES_QUERY_REF,
  COURSE_SEARCH_FIELDS,
  COURSE_TABLE_KEY_IDX,
  COURSE_TABLE_PATH,
  ENROLMENT_HISTORY_QUERY_REF,
  MANAGEMENT_HISTORY_QUERY_REF,
  NOT_ENABLED_TIP,
  SCRIPT_DETAIL_QUERY_REF,
  actionButtonState,
  actionOutcome,
  courseFormFromRow,
  historyParams,
  historyText,
  priceListView,
  retireInvokeOptions,
  retireLabel,
  scriptDetailParams,
  scriptIsInvokable,
  syncInvokeOptions,
  SYNC_SCRIPT,
  RETIRE_SCRIPT,
} from './course-catalog.js';
import './course-manager.css';

export default class CourseManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this._allowed = false;
    this.courseTable = null;
    this.editor = new CourseEditor(this);
    this._mode = 'create';
    this._selectedId = null;
    this._switching = false;
    this._busy = false;
    this._priceToken = 0;
    this._syncInvokable = false;
    this._retireInvokable = false;
    this.leftPanelState = new PanelStateManager('lithium_course_left');
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
      <div class="course-manager-container">
        <div class="course-manager-denied">
          <fa fa-lock></fa>
          <h2>Course Manager</h2>
          <p class="denied-message">
            You do not have permission to access Course Manager.
            Contact your system administrator if you believe this is an error.
          </p>
        </div>
      </div>
    `;
    processIcons(this.container);
  }

  _tabButtons() {
    return COURSE_DETAIL_TABS.map((tab, index) => `
      <button type="button" class="lithium-toolbar-tab${index === 0 ? ' active' : ''}" data-tab="${tab.id}" data-collapsible-text>
        <fa fa-${tab.icon}></fa>
        <span>${tab.label}</span>
      </button>
    `).join('');
  }

  _priceRows() {
    return COURSE_PRICE_CURRENCIES.map((currency) => `
      <div class="course-price-row" data-currency="${currency}">
        <span class="course-price-code">${currency.toUpperCase()}</span>
        <span class="course-price-amount"></span>
        <span class="course-price-stripe"></span>
        <span class="course-price-state"></span>
      </div>
    `).join('');
  }

  _renderStaff() {
    this.container.innerHTML = `
      <div class="course-manager-staff">
        <div class="manager-panels course-panels">
          <div class="course-left-panel" data-panel="left" id="course-left-panel">
            <div id="course-table" class="course-table-host lithium-table-container"></div>
            <div id="course-navigator" class="course-navigator lithium-navigator-container"></div>
          </div>
          <div class="lithium-splitter" data-splitter="left" id="course-splitter"></div>
          <div class="course-right-panel" data-panel="right">
            <div class="lithium-toolbar" id="course-toolbar">
              <button type="button" class="lithium-toolbar-btn lithium-collapse-btn" id="course-collapse-btn" data-collapse-target="left" title="Toggle Table">
                <fa fa-angles-left class="lithium-collapse-icon"></fa>
              </button>
              ${this._tabButtons()}
              <div class="lithium-toolbar-placeholder"></div>
            </div>
            <div class="course-tabs-content">
              <div class="course-tab-pane active" id="course-tab-course">
                <form id="course-form" class="course-form">
                  <div class="course-field">
                    <label for="course-code">Code</label>
                    <input id="course-code" class="input" type="text" autocomplete="off" disabled>
                  </div>
                  <div class="course-field">
                    <label for="course-slug">Slug</label>
                    <input id="course-slug" class="input" type="text" autocomplete="off" disabled>
                  </div>
                  <div class="course-field">
                    <label for="course-title">Title</label>
                    <input id="course-title" class="input" type="text" autocomplete="off" disabled>
                    <p id="course-title-hint" class="course-field-hint"></p>
                  </div>
                  <div class="course-field">
                    <label for="course-image">Image path</label>
                    <input id="course-image" class="input" type="text" autocomplete="off" disabled>
                  </div>
                  <div class="course-field">
                    <label for="course-canvas-id">Canvas course id</label>
                    <input id="course-canvas-id" class="input" type="text" inputmode="numeric" autocomplete="off" disabled>
                  </div>
                  <div class="course-field">
                    <label for="course-pricing">Pricing</label>
                    <select id="course-pricing" class="select" disabled>
                      <option value="free">free</option>
                      <option value="paid">paid</option>
                    </select>
                  </div>
                  <div class="course-field">
                    <label for="course-sort">Sort order</label>
                    <input id="course-sort" class="input" type="text" inputmode="numeric" autocomplete="off" disabled>
                  </div>
                  <div class="course-field">
                    <label for="course-tags">Tags</label>
                    <textarea id="course-tags" class="textarea" rows="4" spellcheck="false" disabled></textarea>
                  </div>
                  <section class="course-section" id="course-options">
                    <span class="course-section-label">Options</span>
                    <label class="course-field-check" for="course-option-retired">
                      <input id="course-option-retired" type="checkbox" disabled>
                      Retired
                    </label>
                    <label class="course-field-check" for="course-option-featured">
                      <input id="course-option-featured" type="checkbox" disabled>
                      Featured
                    </label>
                  </section>
                </form>
              </div>
              <div class="course-tab-pane" id="course-tab-canvas">
                <div class="course-form">
                  <img id="course-image-preview" class="course-image-preview" alt="">
                  <div class="course-field">
                    <span class="course-field-label">Summary</span>
                    <div id="course-summary" class="course-readonly-block"></div>
                  </div>
                  <div class="course-field">
                    <span class="course-field-label">Description</span>
                    <div id="course-description" class="course-readonly-block"></div>
                  </div>
                  <div class="course-field">
                    <span class="course-field-label">Published</span>
                    <div id="course-published" class="course-readonly-line"></div>
                  </div>
                  <div class="course-field">
                    <span class="course-field-label">Last sync</span>
                    <div id="course-synced" class="course-readonly-line"></div>
                  </div>
                  <div class="course-actions">
                    <span class="course-action-tip" id="course-sync-tip" title="${NOT_ENABLED_TIP}">
                      <button id="course-sync-btn" class="btn btn-sm btn-secondary" type="button" disabled>Sync from Canvas</button>
                    </span>
                    <span class="course-action-tip" id="course-sync-all-tip" title="${NOT_ENABLED_TIP}">
                      <button id="course-sync-all-btn" class="btn btn-sm btn-secondary" type="button" disabled>Sync all linked</button>
                    </span>
                    <span class="course-action-tip" id="course-retire-tip" title="${NOT_ENABLED_TIP}">
                      <button id="course-retire-btn" class="btn btn-sm btn-secondary" type="button" disabled>Retire</button>
                    </span>
                    <button id="course-canvas-btn" class="btn btn-sm btn-secondary" type="button" disabled>Open Canvas</button>
                  </div>
                </div>
              </div>
              <div class="course-tab-pane" id="course-tab-commerce">
                <div class="course-form">
                  <section class="course-section is-empty" id="course-prices">
                    <span class="course-section-label">Prices</span>
                    <p id="course-prices-status" class="course-field-hint"></p>
                    ${this._priceRows()}
                  </section>
                  <div class="course-field">
                    <span class="course-field-label">Stripe product</span>
                    <div id="course-stripe" class="course-readonly-line"></div>
                  </div>
                </div>
              </div>
              <div class="course-tab-pane" id="course-tab-history">
                <div class="course-form">
                  <div class="course-actions">
                    <button id="course-enrol-history-btn" class="btn btn-sm btn-secondary" type="button">Enrolment history</button>
                    <button id="course-manage-history-btn" class="btn btn-sm btn-secondary" type="button">Management history</button>
                  </div>
                  <pre id="course-history" class="course-history"></pre>
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
      leftPanel: this.container.querySelector('#course-left-panel'),
      collapseBtn: this.container.querySelector('#course-collapse-btn'),
      navigator: this.container.querySelector('#course-navigator'),
      table: this.container.querySelector('#course-table'),
      form: this.container.querySelector('#course-form'),
      code: this.container.querySelector('#course-code'),
      slug: this.container.querySelector('#course-slug'),
      title: this.container.querySelector('#course-title'),
      titleHint: this.container.querySelector('#course-title-hint'),
      image: this.container.querySelector('#course-image'),
      preview: this.container.querySelector('#course-image-preview'),
      canvas: this.container.querySelector('#course-canvas-id'),
      pricing: this.container.querySelector('#course-pricing'),
      featured: this.container.querySelector('#course-option-featured'),
      retired: this.container.querySelector('#course-option-retired'),
      sort: this.container.querySelector('#course-sort'),
      tags: this.container.querySelector('#course-tags'),
      prices: this.container.querySelector('#course-prices'),
      priceStatus: this.container.querySelector('#course-prices-status'),
      summary: this.container.querySelector('#course-summary'),
      description: this.container.querySelector('#course-description'),
      published: this.container.querySelector('#course-published'),
      stripe: this.container.querySelector('#course-stripe'),
      synced: this.container.querySelector('#course-synced'),
      syncTip: this.container.querySelector('#course-sync-tip'),
      sync: this.container.querySelector('#course-sync-btn'),
      syncAllTip: this.container.querySelector('#course-sync-all-tip'),
      syncAll: this.container.querySelector('#course-sync-all-btn'),
      retireTip: this.container.querySelector('#course-retire-tip'),
      retire: this.container.querySelector('#course-retire-btn'),
      canvasBtn: this.container.querySelector('#course-canvas-btn'),
      enrolHistory: this.container.querySelector('#course-enrol-history-btn'),
      manageHistory: this.container.querySelector('#course-manage-history-btn'),
      history: this.container.querySelector('#course-history'),
    };
  }

  async _mountStaff() {
    this.editor.showEmpty();
    this.editor.bindForm();
    this._bindStaff();
    this.editor.wireFooter();
    await this._loadScriptFlags();
    this._paintActions();
    try {
      await this._mountTable();
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `Course Manager list failed: ${error.message}`);
      toast.error('Course list failed to load', { description: error.message });
    }
  }

  _bindStaff() {
    this.elements.collapseBtn?.addEventListener('click', () => this._toggleLeftPanel());
    this.container.querySelectorAll('[data-tab]').forEach((button) => {
      button.addEventListener('click', () => this._switchTab(button.dataset.tab));
    });
    this.elements.sync.addEventListener('click', () => this._syncOne());
    this.elements.syncAll.addEventListener('click', () => this._syncAll());
    this.elements.retire.addEventListener('click', () => this._retire());
    this.elements.canvasBtn.addEventListener('click', () => this._openCanvas());
    this.elements.enrolHistory.addEventListener('click', () => {
      this._loadHistory(ENROLMENT_HISTORY_QUERY_REF);
    });
    this.elements.manageHistory.addEventListener('click', () => {
      this._loadHistory(MANAGEMENT_HISTORY_QUERY_REF);
    });
  }

  async _loadScriptFlags() {
    const api = this.app && this.app.api;
    if (!api) return;
    const [syncRows, retireRows] = await Promise.all([
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(SYNC_SCRIPT)).catch(() => null),
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(RETIRE_SCRIPT)).catch(() => null),
    ]);
    this._syncInvokable = scriptIsInvokable(syncRows);
    this._retireInvokable = scriptIsInvokable(retireRows);
  }

  _switchTab(tabId) {
    this.container.querySelectorAll('[data-tab]').forEach((button) => {
      button.classList.toggle('active', button.dataset.tab === tabId);
    });
    this.container.querySelectorAll('.course-tab-pane').forEach((pane) => {
      pane.classList.toggle('active', pane.id === `course-tab-${tabId}`);
    });
  }

  async _mountTable() {
    this.courseTable = new LithiumTable({
      container: this.elements.table,
      navigatorContainer: this.elements.navigator,
      tablePath: COURSE_TABLE_PATH,
      lookupKeyIdx: COURSE_TABLE_KEY_IDX,
      primaryKeyField: ['course_id'],
      queryRef: COURSE_LIST_QUERY_REF,
      cssPrefix: 'course',
      storageKey: 'course_manager_table',
      app: this.app,
      readonly: false,
      onExecuteSave: (row) => this.editor.executeSave(row),
      onDuplicate: (rowData) => this.editor.duplicateRow(rowData),
      localSearch: true,
      localSearchFields: COURSE_SEARCH_FIELDS,
      tableWidthMode: 'wide',
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (row) => this._onRowSelected(row),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `Course Manager loaded ${rows.length} courses`);
        this._reapplyListFilter();
      },
    });
    await this.courseTable.init();
    this.editor.attach(this.courseTable);
    this._labelNavigatorSearch();
    this._setupSplitter();
    await this.courseTable.loadData();
  }

  _labelNavigatorSearch() {
    const input = this.container.querySelector('#course-search-input');
    if (!input) return;
    input.placeholder = 'Search code, slug, title, or Canvas id';
    input.setAttribute('aria-label', 'Search courses');
  }

  _reapplyListFilter() {
    if (!this.courseTable || !this.courseTable.table) return;
    const term = this.courseTable._localSearchTerm || '';
    performLocalSearch(this.courseTable, term);
  }

  _setupSplitter() {
    const element = this.container.querySelector('#course-splitter');
    if (!element || !this.elements.leftPanel) return;
    this.splitter = new LithiumSplitter({
      element,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.courseTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.courseTable?.table?.redraw?.();
      },
    });
    this.courseTable?.setSplitter?.(this.splitter);
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
      onAfterToggle: () => this.courseTable?.table?.redraw?.(),
    });
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  _paintPreview(knownUrl) {
    const url = knownUrl == null
      ? courseFormFromRow({ image_path: this.elements.image.value }).imageUrl
      : knownUrl;
    const preview = this.elements.preview;
    if (!url) {
      preview.removeAttribute('src');
      preview.classList.remove('is-visible');
      return;
    }
    preview.src = url;
    preview.classList.add('is-visible');
  }

  _hasRow() {
    return this._mode === 'edit' && Number.isInteger(this._selectedId) && this._selectedId > 0;
  }

  _paintActions() {
    const hasRow = this._hasRow();
    const sync = actionButtonState({
      invokable: this._syncInvokable,
      needsRow: true,
      hasRow,
    });
    const syncAll = actionButtonState({
      invokable: this._syncInvokable,
      needsRow: false,
      hasRow,
    });
    const retire = actionButtonState({
      invokable: this._retireInvokable,
      needsRow: true,
      hasRow,
    });
    this._applyButton(this.elements.sync, this.elements.syncTip, sync);
    this._applyButton(this.elements.syncAll, this.elements.syncAllTip, syncAll);
    this._applyButton(this.elements.retire, this.elements.retireTip, retire);
    const retired = this.elements.retired && this.elements.retired.checked;
    this.elements.retire.textContent = retireLabel(retired ? 1 : 0);
    const canvasUrl = courseFormFromRow({
      canvas_course_id: this.elements.canvas.value,
    }).canvasUrl;
    this.elements.canvasBtn.disabled = !canvasUrl;
  }

  _applyButton(button, tip, state) {
    button.disabled = state.disabled || this._busy;
    tip.title = state.title;
  }

  _onRowSelected(row) {
    if (this._switching || !row) return;
    this.editor.applyRow(row);
  }

  _paintPrices(view, status) {
    const section = this.elements.prices;
    if (!section) return;
    this.elements.priceStatus.textContent = status || '';
    section.classList.toggle('is-empty', Boolean(status));
    const rows = view || [];
    section.querySelectorAll('.course-price-row').forEach((row) => {
      const item = rows.find((entry) => entry.currency === row.dataset.currency);
      row.querySelector('.course-price-amount').textContent = item ? item.amount : '';
      row.querySelector('.course-price-stripe').textContent = item ? item.stripe : '';
      row.querySelector('.course-price-state').textContent = item ? item.active : '';
    });
  }

  async _loadPrices(courseId) {
    const token = this._priceToken + 1;
    this._priceToken = token;
    const id = Number(courseId);
    if (!Number.isInteger(id) || id <= 0) {
      this._paintPrices([], 'Select a course.');
      return;
    }
    const api = this.app && this.app.api;
    if (!api) {
      this._paintPrices([], 'Prices are unavailable.');
      return;
    }
    try {
      const rows = await authQuery(api, COURSE_PRICES_QUERY_REF, { INTEGER: { COURSEID: id } });
      if (this._priceToken !== token) return;
      this._paintPrices(priceListView(rows), '');
    } catch (error) {
      if (this._priceToken !== token) return;
      log(Subsystems.MANAGER, Status.ERROR, `Course prices failed: ${error.message}`);
      this._paintPrices([], 'Prices are unavailable.');
    }
  }

  async _refresh() {
    if (this.courseTable && typeof this.courseTable.handleRefresh === 'function') {
      await this.courseTable.handleRefresh();
      return;
    }
    if (this.courseTable && this.courseTable.loadData) {
      await this.courseTable.loadData();
    }
  }

  async _syncOne() {
    if (!this._syncInvokable || !this._hasRow()) return;
    const options = syncInvokeOptions(this._selectedId, 'one');
    await this._runAction(options, 'Sync finished');
  }

  async _syncAll() {
    if (!this._syncInvokable) return;
    const options = syncInvokeOptions(null, 'all');
    await this._runAction(options, 'Sync finished');
  }

  async _retire() {
    if (!this._retireInvokable || !this._hasRow()) return;
    const retired = this.elements.retired && this.elements.retired.checked;
    const options = retireInvokeOptions(this._selectedId, retired ? 1 : 0);
    const label = retired ? 'Course unretired' : 'Course retired';
    await this._runAction(options, label);
  }

  async _runAction(options, successLabel) {
    if (!options || this._busy) return;
    const api = this.app && this.app.api;
    if (!api) {
      toast.error('Action failed', { description: 'No server connection.' });
      return;
    }
    this._busy = true;
    this._paintActions();
    try {
      const envelope = await invokeScript(api, options);
      const outcome = actionOutcome(envelope);
      if (!outcome.ok) {
        toast.error('Action failed', { description: outcome.detail });
        return;
      }
      toast.success(successLabel);
      if (this.courseTable && this.courseTable.loadData) {
        await this.courseTable.loadData();
      }
    } catch (error) {
      toast.error('Action failed', { description: error.message });
    } finally {
      this._busy = false;
      this._paintActions();
    }
  }

  _openCanvas() {
    const url = courseFormFromRow({ canvas_course_id: this.elements.canvas.value }).canvasUrl;
    if (!url) return;
    window.open(url, '_blank', 'noopener,noreferrer');
  }

  async _loadHistory(queryRef) {
    if (!this._hasRow()) {
      toast.error('Select a course');
      return;
    }
    const params = historyParams(this._selectedId);
    const api = this.app && this.app.api;
    if (!params || !api) {
      this.elements.history.textContent = 'No history.';
      return;
    }
    try {
      const rows = await authQuery(api, queryRef, params);
      const text = historyText(rows);
      this.elements.history.textContent = text || 'No history.';
    } catch (error) {
      this.elements.history.textContent = 'No history.';
      toast.error('History failed', {
        description: error.message,
        serverError: error.serverError,
      });
    }
  }

  cleanup() {
    this.editor?.destroy();
    this.splitter?._detachListeners?.();
    this.splitter = null;
    if (this.courseTable && typeof this.courseTable.destroy === 'function') {
      this.courseTable.destroy();
    }
    this.courseTable = null;
    if (this.container) {
      this.container.innerHTML = '';
    }
    this.elements = {};
  }
}
