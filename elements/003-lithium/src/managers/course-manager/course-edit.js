/**
 * Course Manager record editing.
 *
 * Same path as Query Manager: LithiumTable navigator buttons, and
 * ManagerEditHelper for footer save and cancel. The grid stays a
 * viewer. The form is the editor. Delete does not remove a course.
 */

import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { setupManagerFooterIcons } from '../../core/manager-ui.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import {
  COURSE_INSERT_QUERY_REF,
  COURSE_UPDATE_QUERY_REF,
  buildCourseInsertParams,
  buildCourseUpdateParams,
  courseDraftSignature,
  courseDuplicateRow,
  courseFormFromRow,
  courseUserId,
} from './course-catalog.js';

const DELETE_MESSAGE = 'Courses stay in the catalog. Set Retired under Options.';

export class CourseEditor {
  /**
   * @param {object} manager
   */
  constructor(manager) {
    this.manager = manager;
    this._footer = null;
    this.editHelper = new ManagerEditHelper({
      name: 'Course',
      onAfterEditModeChange: () => this.manager._paintActions?.(),
    });
  }

  bindForm() {
    const form = this.manager.elements.form;
    form.addEventListener('submit', (event) => event.preventDefault());
    form.addEventListener('input', () => {
      this.manager._paintPreview();
      this.manager._paintActions();
      this.editHelper.checkDirtyState();
    });
    form.addEventListener('change', () => {
      this.manager._paintActions();
      this.editHelper.checkDirtyState();
    });
    this.setEditable(false);
  }

  /**
   * @param {object} table
   */
  attach(table) {
    this.editHelper.registerTable(table);
    this.editHelper.registerEditor('course', {
      getContent: () => courseDraftSignature(this.readFields()),
      setContent: (content) => this.restore(content),
      setEditable: (editable) => this.setEditable(editable),
      boundTable: table,
    });
    table.handleDelete = () => {
      toast.info('Not deleted', { description: DELETE_MESSAGE, duration: 4000 });
    };
    if (typeof table.handleCancel === 'function') {
      const inner = table.handleCancel.bind(table);
      table.handleCancel = async () => {
        const row = table.getEditingRow?.();
        const id = Number(row?.getData?.()?.course_id);
        const isNew = !Number.isInteger(id) || id <= 0;
        await inner();
        if (isNew) {
          row?.delete?.();
          this.showEmpty();
        }
      };
    }
  }

  wireFooter() {
    const slot = this.manager.container.closest('.manager-slot');
    const group = slot?.querySelector('.manager-slot-footer .subpanel-header-group');
    if (!group) return;
    const placeholder = group.querySelector('.slot-footer-placeholder');
    const footer = setupManagerFooterIcons(group, {
      onPrint: () => this._print(),
      onEmail: () => this._email(),
      onDownload: () => this._download(),
      onExport: (value) => this._download(value),
      reportOptions: [{ value: 'courses-view', label: 'Courses' }],
      anchor: placeholder,
      showSaveCancel: true,
      exportOptions: [
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
      ],
    });
    this._footer = footer;
    this.editHelper.wireFooterButtons(footer.saveBtn, footer.cancelBtn, footer.dummyBtn);
  }

  readFields() {
    const el = this.manager.elements;
    return {
      courseId: this.manager._mode === 'edit' ? this.manager._selectedId : undefined,
      code: el.code.value,
      slug: el.slug.value,
      title: el.title.value,
      imagePath: el.image.value,
      canvasCourseId: el.canvas.value,
      pricingType: el.pricing.value,
      featured: el.featured.checked,
      retired: el.retired.checked,
      sortOrder: el.sort.value,
      tags: el.tags.value,
    };
  }

  showEmpty() {
    this.applyForm(courseFormFromRow(null));
  }

  /**
   * @param {object|null|undefined} row
   */
  applyRow(row) {
    this.applyForm(courseFormFromRow(row));
  }

  /**
   * @param {object} form
   */
  applyForm(form) {
    const isNew = !(Number(form.courseId) > 0);
    const el = this.manager.elements;
    this.manager._mode = isNew ? 'create' : 'edit';
    this.manager._selectedId = isNew ? null : form.courseId;
    el.code.value = form.code || '';
    el.slug.value = form.slug || '';
    el.title.value = form.title || '';
    el.image.value = form.imagePath || '';
    el.canvas.value = form.canvasCourseId || '';
    el.pricing.value = form.pricingType === 'paid' ? 'paid' : 'free';
    el.featured.checked = Boolean(form.featured);
    el.retired.checked = Boolean(form.retired);
    el.sort.value = form.sortOrder == null ? '' : String(form.sortOrder);
    el.tags.value = form.tags || '{}';
    el.title.readOnly = !isNew;
    el.titleHint.textContent = isNew
      ? 'Required. A later sync can replace it.'
      : 'Updated from Canvas.';
    el.summary.textContent = form.summary || '';
    el.description.textContent = form.description || '';
    el.published.textContent = form.published || '';
    el.stripe.textContent = form.stripe || '';
    el.synced.textContent = form.synced || '';
    el.history.textContent = '';
    this.manager._paintPreview(form.imageUrl || '');
    this.manager._paintActions();
    if (isNew) this.manager._paintPrices([], 'Select a course.');
    else this.manager._loadPrices(form.courseId);
  }

  /**
   * @param {string} content
   */
  restore(content) {
    let fields;
    try {
      fields = JSON.parse(content);
    } catch {
      return;
    }
    const el = this.manager.elements;
    el.code.value = String(fields.code ?? '');
    el.slug.value = String(fields.slug ?? '');
    el.title.value = String(fields.title ?? '');
    el.image.value = String(fields.imagePath ?? '');
    el.canvas.value = String(fields.canvasCourseId ?? '');
    el.pricing.value = fields.pricingType === 'paid' ? 'paid' : 'free';
    el.featured.checked = fields.featured === 1;
    el.retired.checked = fields.retired === 1;
    el.sort.value = String(fields.sortOrder ?? '');
    el.tags.value = String(fields.tags ?? '{}');
    this.manager._paintPreview();
    this.manager._paintActions();
  }

  /**
   * @param {boolean} editable
   */
  setEditable(editable) {
    const el = this.manager.elements;
    [
      el.code, el.slug, el.title, el.image, el.canvas,
      el.pricing, el.featured, el.retired, el.sort, el.tags,
    ].forEach((node) => {
      if (node) node.disabled = !editable;
    });
    const existing = this.manager._mode === 'edit';
    el.title.readOnly = !editable || existing;
  }

  /**
   * @param {object} row
   */
  async executeSave(row) {
    const data = row?.getData?.() || {};
    const courseId = Number(data.course_id);
    const isInsert = !Number.isInteger(courseId) || courseId <= 0;
    const fields = this.readFields();
    if (!isInsert) fields.courseId = courseId;
    const userId = courseUserId(this.manager.app);
    const built = isInsert
      ? buildCourseInsertParams(fields, userId)
      : buildCourseUpdateParams(fields, userId);
    if (!built.ok) {
      throw Object.assign(new Error(built.message), { serverError: built.message });
    }
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      throw Object.assign(new Error('No server connection.'), {
        serverError: 'No server connection.',
      });
    }
    const queryRef = isInsert ? COURSE_INSERT_QUERY_REF : COURSE_UPDATE_QUERY_REF;
    await authQuery(api, queryRef, built.params);
    if (isInsert) this.showEmpty();
    if (this.manager.courseTable && this.manager.courseTable.loadData) {
      this.manager._switching = true;
      try {
        await this.manager.courseTable.loadData();
      } finally {
        this.manager._switching = false;
      }
    }
  }

  /**
   * @param {object} rowData
   * @returns {object}
   */
  duplicateRow(rowData) {
    return courseDuplicateRow(rowData);
  }

  destroy() {
    this.editHelper.destroy();
  }

  _table() {
    return this.manager.courseTable && this.manager.courseTable.table;
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
    const kind = format || (this._footer && this._footer.exportFormat) || 'csv';
    const extension = kind === 'json' ? 'json' : 'csv';
    const filename = `courses-${new Date().toISOString().slice(0, 10)}`;
    table.download(kind, `${filename}.${extension}`);
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
      return [data.code, data.title].filter((part) => part != null && part !== '').join(' — ');
    });
    const subject = encodeURIComponent(`Courses — ${rows.length} rows`);
    const body = encodeURIComponent(lines.join('\n'));
    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }
}
