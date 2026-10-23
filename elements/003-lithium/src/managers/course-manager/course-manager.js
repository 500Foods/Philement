/**
 * Course Manager — Manager ID 34
 *
 * Staff catalog for the 500 Courses operator surface.
 * Authz is CATCHUP L3: QueryRef 155 names, then staff or admin.
 * Commerce, learners, and inbox land in later Band D phases.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { authQuery, invokeScript } from '../../shared/conduit.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import LithiumTable from '../../tables/lithium-table-main.js';
import { resolveCourseManagerAccess } from './course-access.js';
import {
  COURSE_INSERT_QUERY_REF,
  COURSE_LIST_QUERY_REF,
  COURSE_TABLE_KEY_IDX,
  COURSE_TABLE_PATH,
  COURSE_UPDATE_QUERY_REF,
  ENROLMENT_HISTORY_QUERY_REF,
  MANAGEMENT_HISTORY_QUERY_REF,
  NOT_ENABLED_TIP,
  SCRIPT_DETAIL_QUERY_REF,
  actionButtonState,
  actionOutcome,
  buildCourseInsertParams,
  buildCourseUpdateParams,
  courseDraftSignature,
  courseFormFromRow,
  courseUserId,
  courseVisible,
  historyParams,
  historyText,
  retireInvokeOptions,
  retireLabel,
  scriptDetailParams,
  scriptIsInvokable,
  syncInvokeOptions,
  SYNC_SCRIPT,
  RETIRE_SCRIPT,
} from './course-catalog.js';
import './course-manager.css';

const SHELL_TEXT = 'Manage the 500 Courses catalog, enrollments, commerce, and learner accounts.';

const EMPTY_FORM = {
  code: '',
  slug: '',
  title: '',
  imagePath: '',
  canvasCourseId: '',
  pricingType: 'free',
  featured: false,
  sortOrder: '',
  tags: '{}',
};

export default class CourseManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this._allowed = false;
    this.courseTable = null;
    this._mode = 'create';
    this._selectedId = null;
    this._retired = false;
    this._dirty = false;
    this._signature = courseDraftSignature(EMPTY_FORM);
    this._switching = false;
    this._filtering = false;
    this._busy = false;
    this._syncInvokable = false;
    this._retireInvokable = false;
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

  _renderStaff() {
    this.container.innerHTML = `
      <div class="course-manager-container course-manager-staff">
        <div class="course-manager-header">
          <fa fa-book-open></fa>
          <h2>Course Manager</h2>
        </div>
        <p class="course-manager-intro">${SHELL_TEXT}</p>
        <div class="course-manager-split">
          <section class="course-manager-list">
            <div class="course-manager-list-tools">
              <input id="course-search" class="input" type="search"
                placeholder="Search code, slug, title, or Canvas id"
                aria-label="Search courses">
              <label class="course-retired-label" for="course-retired-chip">
                <input id="course-retired-chip" type="checkbox">
                Retired
              </label>
              <button id="course-refresh-btn" class="btn btn-sm btn-secondary" type="button">Refresh</button>
              <button id="course-new-btn" class="btn btn-sm btn-primary" type="button">New</button>
            </div>
            <div id="course-table" class="course-table-host"></div>
          </section>
          <section class="course-manager-detail">
            <form id="course-form" class="course-form">
              <div class="course-field">
                <label for="course-code">Code</label>
                <input id="course-code" class="input" type="text" autocomplete="off">
              </div>
              <div class="course-field">
                <label for="course-slug">Slug</label>
                <input id="course-slug" class="input" type="text" autocomplete="off">
              </div>
              <div class="course-field">
                <label for="course-title">Title</label>
                <input id="course-title" class="input" type="text" autocomplete="off">
                <p id="course-title-hint" class="course-field-hint"></p>
              </div>
              <div class="course-field">
                <label for="course-image">Image path</label>
                <input id="course-image" class="input" type="text" autocomplete="off">
                <img id="course-image-preview" class="course-image-preview" alt="">
              </div>
              <div class="course-field">
                <label for="course-canvas-id">Canvas course id</label>
                <input id="course-canvas-id" class="input" type="text" inputmode="numeric" autocomplete="off">
              </div>
              <div class="course-field">
                <label for="course-pricing">Pricing</label>
                <select id="course-pricing" class="select">
                  <option value="free">free</option>
                  <option value="paid">paid</option>
                </select>
              </div>
              <div class="course-field course-field-check">
                <label for="course-featured">
                  <input id="course-featured" type="checkbox">
                  Featured
                </label>
              </div>
              <div class="course-field">
                <label for="course-sort">Sort order</label>
                <input id="course-sort" class="input" type="text" inputmode="numeric" autocomplete="off">
              </div>
              <div class="course-field">
                <label for="course-tags">Tags</label>
                <textarea id="course-tags" class="textarea" rows="4" spellcheck="false"></textarea>
              </div>
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
                <span class="course-field-label">Stripe product</span>
                <div id="course-stripe" class="course-readonly-line"></div>
              </div>
              <div class="course-field">
                <span class="course-field-label">Last sync</span>
                <div id="course-synced" class="course-readonly-line"></div>
              </div>
              <div class="course-actions">
                <button id="course-save-btn" class="btn btn-sm btn-primary" type="button">Save</button>
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
                <button id="course-enrol-history-btn" class="btn btn-sm btn-secondary" type="button">Enrolment history</button>
                <button id="course-manage-history-btn" class="btn btn-sm btn-secondary" type="button">Management history</button>
              </div>
              <pre id="course-history" class="course-history"></pre>
            </form>
          </section>
        </div>
      </div>
    `;
    processIcons(this.container);
    this.elements = {
      search: this.container.querySelector('#course-search'),
      retired: this.container.querySelector('#course-retired-chip'),
      refresh: this.container.querySelector('#course-refresh-btn'),
      newer: this.container.querySelector('#course-new-btn'),
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
      featured: this.container.querySelector('#course-featured'),
      sort: this.container.querySelector('#course-sort'),
      tags: this.container.querySelector('#course-tags'),
      summary: this.container.querySelector('#course-summary'),
      description: this.container.querySelector('#course-description'),
      published: this.container.querySelector('#course-published'),
      stripe: this.container.querySelector('#course-stripe'),
      synced: this.container.querySelector('#course-synced'),
      save: this.container.querySelector('#course-save-btn'),
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
    this._showCreate();
    this._bindStaff();
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
    this.elements.form.addEventListener('submit', (event) => event.preventDefault());
    this.elements.form.addEventListener('input', () => {
      this._syncDirty();
      this._paintPreview();
      this._paintActions();
    });
    this.elements.form.addEventListener('change', () => this._syncDirty());
    this.elements.search.addEventListener('input', () => this._applyFilter());
    this.elements.retired.addEventListener('change', () => this._applyFilter());
    this.elements.refresh.addEventListener('click', () => this._refresh());
    this.elements.newer.addEventListener('click', () => this._startNew());
    this.elements.save.addEventListener('click', () => this._save());
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

  async _mountTable() {
    this.courseTable = new LithiumTable({
      container: this.elements.table,
      tablePath: COURSE_TABLE_PATH,
      lookupKeyIdx: COURSE_TABLE_KEY_IDX,
      primaryKeyField: ['course_id'],
      queryRef: COURSE_LIST_QUERY_REF,
      cssPrefix: 'course',
      storageKey: 'course_manager_table',
      app: this.app,
      readonly: true,
      tableWidthMode: 'wide',
      onRowSelected: (row) => this._onRowSelected(row),
      onRowDeselected: () => this._onRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `Course Manager loaded ${rows.length} courses`);
        this._applyFilter();
      },
    });
    await this.courseTable.init();
    await this.courseTable.loadData();
  }

  _readFields() {
    return {
      courseId: this._mode === 'edit' ? this._selectedId : undefined,
      code: this.elements.code.value,
      slug: this.elements.slug.value,
      title: this.elements.title.value,
      imagePath: this.elements.image.value,
      canvasCourseId: this.elements.canvas.value,
      pricingType: this.elements.pricing.value,
      featured: this.elements.featured.checked,
      sortOrder: this.elements.sort.value,
      tags: this.elements.tags.value,
    };
  }

  _syncDirty() {
    this._dirty = courseDraftSignature(this._readFields()) !== this._signature;
  }

  _showCreate() {
    this._mode = 'create';
    this._selectedId = null;
    this._retired = false;
    this._fillForm({
      ...EMPTY_FORM,
      summary: '',
      description: '',
      published: '',
      stripe: '',
      synced: '',
      imageUrl: '',
      canvasUrl: '',
    });
    this.elements.title.readOnly = false;
    this.elements.titleHint.textContent = 'Required. A later sync can replace it.';
    this.elements.history.textContent = '';
    this._signature = courseDraftSignature(this._readFields());
    this._dirty = false;
    this._paintActions();
  }

  _fillForm(form) {
    this.elements.code.value = form.code || '';
    this.elements.slug.value = form.slug || '';
    this.elements.title.value = form.title || '';
    this.elements.image.value = form.imagePath || '';
    this.elements.canvas.value = form.canvasCourseId || '';
    const pricing = form.pricingType === 'paid' ? 'paid' : 'free';
    this.elements.pricing.value = pricing;
    this.elements.featured.checked = Boolean(form.featured);
    this.elements.sort.value = form.sortOrder == null ? '' : String(form.sortOrder);
    this.elements.tags.value = form.tags || '{}';
    this.elements.summary.textContent = form.summary || '';
    this.elements.description.textContent = form.description || '';
    this.elements.published.textContent = form.published || '';
    this.elements.stripe.textContent = form.stripe || '';
    this.elements.synced.textContent = form.synced || '';
    this._paintPreview(form.imageUrl);
  }

  _applyRow(row) {
    const form = courseFormFromRow(row);
    this._mode = 'edit';
    this._selectedId = form.courseId || null;
    this._retired = form.retired;
    this._fillForm(form);
    this.elements.title.readOnly = true;
    this.elements.titleHint.textContent = 'Updated from Canvas.';
    this.elements.history.textContent = '';
    this._signature = courseDraftSignature(this._readFields());
    this._dirty = false;
    this._paintActions();
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
    this.elements.retire.textContent = retireLabel(this._retired ? 1 : 0);
    const canvasUrl = courseFormFromRow({
      canvas_course_id: this.elements.canvas.value,
    }).canvasUrl;
    this.elements.canvasBtn.disabled = !canvasUrl;
  }

  _applyButton(button, tip, state) {
    button.disabled = state.disabled || this._busy;
    tip.title = state.title;
  }

  _applyFilter() {
    const table = this.courseTable && this.courseTable.table;
    if (!table || typeof table.setFilter !== 'function') return;
    const term = this.elements.search.value;
    const retiredOnly = this.elements.retired.checked;
    this._filtering = true;
    try {
      if (!String(term || '').trim() && !retiredOnly) {
        table.clearFilter();
      } else {
        table.setFilter((row) => courseVisible(row, term, retiredOnly));
      }
    } finally {
      this._filtering = false;
    }
  }

  async _onRowSelected(row) {
    if (this._switching || this._filtering || !row) return;
    const next = courseFormFromRow(row);
    if (this._mode === 'edit' && next.courseId === this._selectedId) return;
    if (this._dirty) {
      const proceed = await this._confirmSave();
      if (!proceed) {
        this._restoreSelection();
        return;
      }
    }
    this._applyRow(row);
  }

  _onRowDeselected() {
    if (this._switching || this._filtering) return;
  }

  async _confirmSave() {
    const save = await confirmDialog({
      title: 'Unsaved changes',
      message: 'Save this course before leaving it?',
      confirmLabel: 'Save',
      cancelLabel: 'Stay',
    });
    if (!save) return false;
    return this._save();
  }

  _restoreSelection() {
    this._switching = true;
    try {
      if (this._selectedId && this.courseTable && this.courseTable.autoSelectRow) {
        this.courseTable.autoSelectRow(this._selectedId);
      } else if (this.courseTable && this.courseTable.table && this.courseTable.table.deselectRow) {
        this.courseTable.table.deselectRow();
      }
    } finally {
      this._switching = false;
    }
  }

  async _startNew() {
    if (this._dirty) {
      const proceed = await this._confirmSave();
      if (!proceed) return;
    }
    this._switching = true;
    try {
      if (this.courseTable && this.courseTable.table && this.courseTable.table.deselectRow) {
        this.courseTable.table.deselectRow();
      }
      this._showCreate();
    } finally {
      this._switching = false;
    }
  }

  async _refresh() {
    if (this._dirty) {
      const proceed = await this._confirmSave();
      if (!proceed) return;
    }
    if (this.courseTable && this.courseTable.loadData) {
      await this.courseTable.loadData();
    }
  }

  async _save() {
    const fields = this._readFields();
    const built = this._mode === 'edit'
      ? buildCourseUpdateParams(fields, courseUserId(this.app))
      : buildCourseInsertParams(fields, courseUserId(this.app));
    if (!built.ok) {
      toast.error(built.message);
      return false;
    }
    const api = this.app && this.app.api;
    if (!api) {
      toast.error('Save failed', { description: 'No server connection.' });
      return false;
    }
    const queryRef = this._mode === 'edit' ? COURSE_UPDATE_QUERY_REF : COURSE_INSERT_QUERY_REF;
    try {
      await authQuery(api, queryRef, built.params);
    } catch (error) {
      toast.error('Save failed', {
        description: error.message,
        serverError: error.serverError,
      });
      return false;
    }
    this._signature = courseDraftSignature(fields);
    this._dirty = false;
    toast.success('Course saved');
    if (queryRef === COURSE_INSERT_QUERY_REF) {
      this._showCreate();
    }
    if (this.courseTable && this.courseTable.loadData) {
      this._switching = true;
      try {
        await this.courseTable.loadData();
      } finally {
        this._switching = false;
      }
    }
    return true;
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
    const options = retireInvokeOptions(this._selectedId, this._retired ? 1 : 0);
    const label = this._retired ? 'Course unretired' : 'Course retired';
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
