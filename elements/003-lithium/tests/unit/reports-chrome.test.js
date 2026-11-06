import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { foldedRanges } from '@codemirror/language';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { authQuery, invokeScript } from '../../src/shared/conduit.js';
import { toast } from '../../src/shared/toast.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { definitionText } from '../../src/managers/reports/reports-file.js';
import { reportEditorContent, reportJsonEdit, reportJsonText } from '../../src/managers/reports/reports-json.js';
import { reportListColtypes, reportListTableDef, reportListUnavailable } from '../../src/managers/reports/reports-table.js';
import { composeParams, composeResult, normalizeIr } from '../../src/managers/reports/reports-compose.js';
import { openListedReport } from '../../src/managers/reports/reports-save.js';
import ReportsManager from '../../src/managers/reports/reports.js';
import { resolveColumn } from '../../src/tables/lithium-table.js';

vi.mock('../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
  invokeScript: vi.fn(),
}));

vi.mock('../../src/tables/lithium-table-main.js', () => ({
  LithiumTable: class {
    constructor(options) {
      this.options = options;
      this.queryRefs = null;
      this.currentData = [{ report_id: 1, rev_id: 1, name: 'Kept' }];
      this.table = { redraw() {} };
      this.handleAdd = async () => {};
      this.handleDelete = async () => {
        this.currentData = [];
      };
      this.setSplitter = () => {};
      this.destroy = () => {};
    }

    async init() {}

    async loadData() {}
  },
}));

const html = readFileSync(
  resolve(process.cwd(), 'src/managers/reports/reports.html'),
  'utf8',
);

function foldCount(view) {
  let count = 0;
  foldedRanges(view.state).between(0, view.state.doc.length, () => {
    count += 1;
  });
  return count;
}

describe('report JSON text', () => {
  it('drops a pasted history trail and rejects a buffer that is not an object', () => {
    const edited = reportJsonEdit(JSON.stringify({
      format: 1,
      name: 'A',
      history: { undo: [], redo: [] },
    }));
    expect(edited.error).toBeUndefined();
    expect(edited.doc.name).toBe('A');
    expect(Object.hasOwn(edited.doc, 'history')).toBe(false);
    expect(reportJsonText(edited.doc)).not.toMatch(/"history"/);

    expect(reportJsonEdit('{').error).toBe('Report JSON could not be read.');
    expect(reportJsonEdit('[]').error).toBe('Report JSON must be an object.');
    expect(reportJsonEdit('null').error).toBe('Report JSON must be an object.');
  });
});

describe('report list columns', () => {
  it('names the reports columns and refuses duplicate and delete', () => {
    const def = reportListTableDef();
    expect(def.table).toBe('reports');
    expect(def.columns.collection).toBeUndefined();
    expect(def.columns.rev_id).toMatchObject({ visible: true, editable: false });
    expect(def.columns.name).toMatchObject({ visible: true, editable: true });
    expect(def.columns.category).toMatchObject({ visible: true, editable: true });
    expect(def.columns.summary).toMatchObject({ visible: true, editable: true });
    expect(Object.keys(def.columns).slice(0, 5)).toEqual([
      'report_id', 'rev_id', 'name', 'category', 'summary',
    ]);
    expect(def.columns.report.visible).toBe(false);
    const coltypes = reportListColtypes();
    expect(resolveColumn('name', def.columns.name, coltypes).editor).toBe('input');
    expect(resolveColumn('category', def.columns.category, coltypes).editor).toBe('input');
    expect(resolveColumn('summary', def.columns.summary, coltypes).editor).toBe('textarea');
    expect(resolveColumn('report_id', def.columns.report_id, coltypes).editor).toBeUndefined();
    expect(JSON.stringify(def)).not.toMatch(/queryRef|query_ref/i);

    const missing = reportListUnavailable('delete');
    expect(missing.message).toBe('No queryRef configured for delete');
    expect(missing.serverError.error).toBe('Delete is not available');
    expect(missing.serverError.message).toBe('Delete is not available. A saved report is kept as revisions.');
    expect(reportListUnavailable('duplicate').message).toMatch(/duplicate/);
    expect(reportListUnavailable('duplicate').serverError.message).toMatch(/no duplicate query/);
  });
});

describe('compose helpers', () => {
  it('sends each json datasource its own rows, or empty sources when static', () => {
    const blank = createBlank();
    expect(composeParams(blank).sources).toEqual({});
    const doc = {
      ...blank,
      datasources: [
        { name: 'main', kind: 'json', rows: [{ a: 1 }] },
        { name: 'map', kind: 'json', rows: [{ k: 0, v: 'false' }] },
        { name: 'none', kind: 'json' },
        { name: 'later', kind: 'query', queryRef: 9 },
      ],
      bands: [{ id: 'd', typeKey: 15, items: [] }, { id: 'p', typeKey: 13, items: [] }],
    };
    const params = composeParams(doc);
    expect(params.stage).toBe('compose');
    expect(params.sources).toEqual({ main: [{ a: 1 }], map: [{ k: 0, v: 'false' }], none: [] });
    expect(params.definition.history).toBeUndefined();
    expect(params.definition.datasources[0].rows).toBeUndefined();
    expect(params.definition.bands[0].datasource).toBe('main');
    expect(params.definition.bands[1].datasource).toBeUndefined();
    expect(doc.datasources[0].rows).toEqual([{ a: 1 }]);
  });

  it('turns empty objects into lists for log, pages, bands and items', () => {
    const ir = normalizeIr({ log: {}, pages: [{ bands: [{ id: 't', items: {} }] }] });
    expect(ir.log).toEqual([]);
    expect(ir.pages[0].bands[0].items).toEqual([]);
    expect(normalizeIr({ pages: {}, log: ['x'] })).toEqual({ pages: [], log: ['x'] });
  });

  it('reads envelopes, string results, and script errors', () => {
    expect(composeResult({ status: 'completed', result: '{"pages":[],"log":{}}' }).ir).toEqual({ pages: [], log: [] });
    expect(composeResult({ status: 'completed', result: { error: 'bad band', log: {} } }).error).toBe('bad band');
    expect(composeResult({ status: 'failed', error: 'boom' }).error).toBe('boom');
    expect(composeResult({ status: 'completed', result: '{' }).error).toMatch(/unreadable/);
  });
});

describe('Report Manager chrome', () => {
  let container;
  let manager;

  beforeEach(() => {
    localStorage.removeItem('lithium_reports_left_collapsed');
    localStorage.removeItem('lithium_reports_left_width');
    toast.dismissAll();
    authQuery.mockReset();
    invokeScript.mockReset();
    const toastBox = document.getElementById('toast-container');
    if (toastBox) toastBox.innerHTML = '';
    vi.stubGlobal('fetch', vi.fn(async (url) => {
      if (url === '/src/managers/reports/reports.html') {
        return new Response(html, { status: 200, headers: { 'content-type': 'text/html' } });
      }
      return new Response('missing', { status: 404 });
    }));
    container = document.createElement('div');
    container.id = 'manager-slot-24';
    container.className = 'manager-slot slot-visible';
    document.body.appendChild(container);
    const host = document.createElement('div');
    container.appendChild(host);
    manager = new ReportsManager({}, host);
  });

  afterEach(() => {
    manager.destroy();
    container.remove();
    toast.dismissAll();
  });

  async function open() {
    await manager.init();
    manager.setRecordEditing(true);
  }

  function replaceJson(text) {
    const view = manager._json.view;
    view.dispatch({
      changes: { from: 0, to: view.state.doc.length, insert: text },
    });
  }

  function blurJson() {
    manager._json.view.contentDOM.dispatchEvent(new CustomEvent('blur', { bubbles: true }));
  }

  async function waitFolded() {
    for (let step = 0; step < 8; step += 1) {
      manager._json.fold();
      if (foldCount(manager._json.view) > 0) return foldCount(manager._json.view);
      await new Promise((resolve) => {
        setTimeout(resolve, 20);
      });
    }
    return foldCount(manager._json.view);
  }

  it('shows the editor chrome and hides it on the JSON page', async () => {
    await open();
    const json = container.querySelector('#reports-page-json');
    const categories = manager.elements.categories;
    const items = manager.elements.itemsToolbar;
    const view = manager.elements.viewControls;
    expect(json.hidden).toBe(true);
    expect(manager.elements.foldBtn.hidden).toBe(true);
    expect(manager.elements.unfoldBtn.hidden).toBe(true);
    expect(categories.hidden).toBe(false);
    expect(items.hidden).toBe(false);
    expect(view.hidden).toBe(false);
    expect(categories.contains(manager.elements.undoBtn)).toBe(true);
    expect(categories.contains(manager.elements.redoBtn)).toBe(true);
    expect(categories.contains(manager.elements.clearBtn)).toBe(true);
    expect(items.contains(manager.elements.cloneBtn)).toBe(true);
    expect(items.contains(manager.elements.deleteBtn)).toBe(true);
    const itemButtons = [...items.querySelectorAll('button')].map((button) => button.id);
    expect(itemButtons.indexOf('reports-grid-btn')).toBeLessThan(itemButtons.indexOf('reports-snap-btn'));
    expect(itemButtons.indexOf('reports-snap-btn')).toBeLessThan(itemButtons.indexOf('reports-clone-btn'));
    expect(itemButtons.indexOf('reports-clone-btn')).toBeLessThan(itemButtons.indexOf('reports-delete-btn'));
    expect(manager.elements.snapBtn.getAttribute('aria-pressed')).toBe('true');
    expect(manager.elements.snapBtn.classList.contains('lithium-nav-btn-active')).toBe(true);
    expect(manager.elements.snapBtn.disabled).toBe(false);
    expect(manager.elements.gridBtn.getAttribute('aria-pressed')).toBe('true');
    expect(manager.elements.gridBtn.classList.contains('lithium-nav-btn-active')).toBe(true);
    expect(items.querySelector('.reports-toolbar-gap')).not.toBeNull();
    expect(manager.elements.snapBtn.nextElementSibling).toBe(manager.elements.rulerBtn);
    expect(manager.elements.rulerBtn.getAttribute('aria-pressed')).toBe('true');
    expect(manager.elements.crosshairBtn.nextElementSibling.classList.contains('reports-toolbar-gap')).toBe(true);
    expect(manager.elements.editorTab.classList.contains('active')).toBe(true);
    expect(container.querySelector('#reports-page-report').hidden).toBe(true);
    expect(container.querySelector('#reports-new-btn')).toBeNull();
    expect(container.querySelector('#reports-page-size')).toBeNull();
    expect(container.querySelector('.reports-properties')).toBeNull();
    const categoryIds = [...categories.querySelectorAll('.reports-category-btn')].map((button) => button.id);
    expect(categoryIds).toEqual([
      'reports-static-btn',
      'reports-field-btn',
      'reports-aggregate-btn',
      'reports-bands-btn',
      'reports-system-btn',
    ]);
    expect(container.querySelector('#reports-text-btn')).not.toBeNull();

    manager.elements.jsonTab.click();
    expect(json.hidden).toBe(false);
    expect(manager.elements.foldBtn.hidden).toBe(false);
    expect(manager.elements.unfoldBtn.hidden).toBe(false);
    expect(categories.hidden).toBe(true);
    expect(items.hidden).toBe(true);
    expect(view.hidden).toBe(true);
    expect(manager.elements.jsonTab.classList.contains('active')).toBe(true);
    expect(container.querySelector('.reports-manager').dataset.page).toBe('json');
    expect(await waitFolded()).toBeGreaterThan(0);

    manager.elements.editorTab.click();
    expect(json.hidden).toBe(true);
    expect(view.hidden).toBe(false);
    expect(manager.elements.foldBtn.hidden).toBe(true);

    manager.elements.reportTab.click();
    expect(container.querySelector('#reports-page-report').hidden).toBe(false);
    expect(categories.hidden).toBe(true);
    expect(items.hidden).toBe(true);
    expect(view.hidden).toBe(true);
    expect(manager.elements.reportTab.classList.contains('active')).toBe(true);
    expect(container.querySelector('.reports-manager').dataset.page).toBe('report');
  });

  it('edits the report name and title and leaves summary off the definition', async () => {
    await open();
    manager.elements.reportTab.click();
    expect(manager.elements.metaName.value).toBe('Report');
    expect(manager.elements.metaTitle.value).toBe('Report');
    expect(container.querySelector('#reports-meta-group')).toBeNull();
    manager.elements.metaName.value = 'Orders';
    manager.elements.metaName.dispatchEvent(new CustomEvent('change'));
    expect(manager.doc.name).toBe('Orders');
    expect(manager.elements.metaName.value).toBe('Orders');
    expect(manager.doc.history.undo).toHaveLength(1);
    manager.elements.metaTitle.value = 'Order list';
    manager.elements.metaTitle.dispatchEvent(new CustomEvent('change'));
    expect(manager.doc.title).toBe('Order list');
    expect(manager.doc.history.undo).toHaveLength(2);
    manager.elements.metaSummary.value = 'Staff notes';
    manager.elements.metaSummary.dispatchEvent(new CustomEvent('change'));
    expect(manager.doc.summary).toBeUndefined();
    expect(manager.doc.history.undo).toHaveLength(2);
    manager.elements.undoBtn.click();
    expect(manager.doc.title).toBe('Report');
    expect(manager.elements.metaTitle.value).toBe('Report');
  });

  it('leaves the definition alone when Static is clicked', async () => {
    await open();
    const before = definitionText(manager.doc);
    manager.elements.staticBtn.click();
    expect(definitionText(manager.doc)).toBe(before);
    expect(manager.elements.staticBtn.getAttribute('aria-pressed')).toBe('true');
    expect(manager.doc).toEqual(createBlank());
  });

  it('clones the selected item onto the same band and selects the copy', async () => {
    await open();
    expect(manager.elements.cloneBtn.disabled).toBe(true);
    manager.elements.textBtn.click();
    const original = manager.doc.bands.find((band) => band.id === 'd1').items[0];
    expect(manager.elements.cloneBtn.disabled).toBe(false);
    manager.elements.cloneBtn.click();
    const items = manager.doc.bands.find((band) => band.id === 'd1').items;
    expect(items.map((entry) => entry.id)).toEqual(['text_1', 'text_2']);
    expect(items[1].box).toEqual(original.box);
    expect(items[0].box).toEqual(original.box);
    const selected = manager.elements.stage.querySelector('.reports-item-selected');
    expect(selected.dataset.itemId).toBe('text_2');

    manager.elements.stage.querySelector('[data-band-id="ph1"] .reports-band-strip')
      .dispatchEvent(new CustomEvent('pointerdown', { bubbles: true }));
    expect(manager.elements.cloneBtn.disabled).toBe(true);
    const before = definitionText(manager.doc);
    manager.elements.cloneBtn.click();
    expect(definitionText(manager.doc)).toBe(before);
  });

  it('omits history on the JSON page and commits one step on blur', async () => {
    await open();
    manager.elements.textBtn.click();
    expect(manager.doc.history.undo).toHaveLength(1);
    const stored = manager.editHelper.editors.get('definition').getContent();
    expect(stored).toMatch(/"history"/);

    manager.elements.jsonTab.click();
    const text = manager._json.text();
    expect(text).not.toMatch(/"history"/);
    expect(text).toContain('"name": "Report"');
    replaceJson(text.replace('"name": "Report"', '"name": "Renamed"'));
    blurJson();
    expect(manager.doc.name).toBe('Renamed');
    expect(manager.doc.history.undo).toHaveLength(2);
    expect(manager._json.text()).toContain('"name": "Renamed"');
    expect(manager._json.text()).not.toMatch(/"history"/);
    expect(reportJsonText(manager.doc)).toBe(manager._json.text());
  });

  it('keeps JSON read-only until the row is edited, then dirties and reloads the editor', async () => {
    await manager.init();
    const before = definitionText(manager.doc);
    manager.elements.jsonTab.click();
    expect(manager._json.view.state.readOnly).toBe(true);
    replaceJson(manager._json.text().replace('"name": "Report"', '"name": "Locked"'));
    manager.elements.editorTab.click();
    expect(definitionText(manager.doc)).toBe(before);
    expect(manager.elements.stage.textContent).not.toContain('Locked');

    manager.setRecordEditing(true);
    manager.editHelper.activeEditingTable = manager.reportTable;
    manager.editHelper._takeSnapshot();
    manager.elements.jsonTab.click();
    expect(manager._json.view.state.readOnly).toBe(false);
    expect(manager.editHelper.isAnythingDirty()).toBe(false);
    const edited = manager._json.text().replace('"name": "Report"', '"name": "From JSON"');
    replaceJson(edited);
    expect(reportEditorContent(manager.doc, 'json', edited)).not.toBe(before);
    expect(manager.editHelper.isAnythingDirty()).toBe(true);
    manager.elements.editorTab.click();
    expect(manager.doc.name).toBe('From JSON');
    expect(manager.doc.history.undo).toHaveLength(1);
    expect(manager.container.querySelector('#reports-outline').textContent).toContain('From JSON');
    manager.elements.undoBtn.click();
    expect(manager.doc.name).toBe('Report');
    expect(manager.container.querySelector('#reports-outline').textContent).not.toContain('From JSON');
  });

  it('toasts a bad JSON buffer once and leaves the definition', async () => {
    await open();
    manager.elements.textBtn.click();
    const before = definitionText(manager.doc);
    manager.elements.jsonTab.click();
    replaceJson('{');
    manager.elements.editorTab.click();
    expect(definitionText(manager.doc)).toBe(before);
    expect(document.body.textContent).toContain('Report JSON could not be read.');
    const shown = document.querySelectorAll('.toast-error').length;
    expect(shown).toBe(1);

    manager.elements.jsonTab.click();
    expect(manager._json.text()).toBe('{');
    manager.elements.editorTab.click();
    expect(document.querySelectorAll('.toast-error').length).toBe(shown);
    expect(definitionText(manager.doc)).toBe(before);
  });

  it('adds a local draft and refuses to save, copy, or delete a row', async () => {
    await open();
    const table = manager.reportTable;
    expect(table.options.tableDef.table).toBe('reports');
    expect(table.options.queryRef).toBe(165);
    expect(table.options.searchQueryRef).toBe(166);
    expect(table.options.detailQueryRef).toBe(167);
    expect(table.options.insertQueryRef).toBe(168);
    expect(table.options.deleteQueryRef).toBe(170);
    expect(table.options.tablePath).toBe('reports/report-manager');
    const kept = table.currentData.map((row) => row.report_id);
    await table.handleDelete();
    expect(table.currentData.map((row) => row.report_id)).toEqual(kept);
    expect(document.body.textContent).toContain('No Row Selected');
    expect(document.body.textContent).not.toContain('Row Deleted');
    expect(authQuery).not.toHaveBeenCalled();

    await expect(table.options.onExecuteSave()).rejects.toMatchObject({
      serverError: { error: 'Save is not available' },
    });
    await expect(table.options.onDuplicate()).rejects.toThrow('No queryRef configured for duplicate');

    manager._importText(JSON.stringify({ format: 1, name: 'Orders' }));
    await table.handleAdd();
    expect(manager.doc.name).toBe('Report');
    expect(manager.doc).toEqual(createBlank());

    manager._importText(JSON.stringify({ format: 1, name: 'Orders' }));
    table.options.onRowSelected({ report: '' });
    expect(manager.doc.name).toBe('Orders');
    table.options.onRowSelected({ report: { format: 1, name: 'Listed' } });
    expect(manager.doc.name).toBe('Listed');
  });

  it('deletes the selected report after confirmation and keeps it when the call fails', async () => {
    await open();
    const table = manager.reportTable;
    const kept = {
      getData: () => ({ report_id: 4, rev_id: 2, name: 'Kept' }),
      delete: vi.fn(),
    };
    const next = {
      getData: () => ({ report_id: 1, name: 'Other' }),
      select: vi.fn(),
    };
    table.table = {
      redraw() {},
      getSelectedRows: () => [kept],
      getRows: () => [kept, next],
    };
    table.exitEditMode = vi.fn(async () => {});
    manager._openReportId = 4;
    manager.app = { api: { name: 'api' } };

    const cancelled = table.handleDelete();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm')).not.toBeNull();
    });
    expect(document.body.textContent).toContain('Every revision');
    document.querySelector('.lithium-confirm .btn-secondary').click();
    await cancelled;
    expect(authQuery).not.toHaveBeenCalled();
    expect(kept.delete).not.toHaveBeenCalled();

    const failure = new Error('missing');
    failure.serverError = { error: 'Query failed', message: 'relation missing' };
    authQuery.mockRejectedValueOnce(failure);
    const failed = table.handleDelete();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm .btn-danger')).not.toBeNull();
    });
    document.querySelector('.lithium-confirm .btn-danger').click();
    await failed;
    expect(kept.delete).not.toHaveBeenCalled();
    expect(document.body.textContent).toContain('relation missing');

    authQuery.mockResolvedValueOnce([]);
    const removed = table.handleDelete();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm .btn-danger')).not.toBeNull();
    });
    document.querySelector('.lithium-confirm .btn-danger').click();
    await removed;
    expect(authQuery).toHaveBeenLastCalledWith(manager.app.api, 170, {
      INTEGER: { REPORT_ID: 4 },
    });
    expect(kept.delete).toHaveBeenCalledTimes(1);
    expect(next.select).toHaveBeenCalledTimes(1);
    expect(document.body.textContent).toContain('Row Deleted');

    const draft = {
      getData: () => ({ report_id: 0, name: 'Draft' }),
      delete: vi.fn(),
    };
    table.table = {
      redraw() {},
      getSelectedRows: () => [draft],
      getRows: () => [draft],
    };
    manager._openReportId = 0;
    manager.doc = { ...manager.doc, name: 'Draft body' };
    authQuery.mockClear();
    const local = table.handleDelete();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm .btn-danger')).not.toBeNull();
    });
    document.querySelector('.lithium-confirm .btn-danger').click();
    await local;
    expect(authQuery).not.toHaveBeenCalled();
    expect(draft.delete).toHaveBeenCalledTimes(1);
    expect(manager.doc.name).toBe('Report');
    expect(manager._openReportId).toBe(0);
  });

  it('orders the tabs and labels the definition tab', async () => {
    await open();
    const ids = [...container.querySelectorAll('.lithium-toolbar-tab')].map((tab) => tab.id);
    expect(ids).toEqual([
      'reports-report-tab', 'reports-editor-tab', 'reports-json-tab', 'reports-revisions-tab',
      'reports-compose-tab', 'reports-log-tab', 'reports-html-tab', 'reports-pdf-tab',
    ]);
    expect(manager.elements.jsonTab.textContent.trim()).toBe('Definition');
    expect(manager.elements.jsonTab.getAttribute('aria-label')).toBe('Definition');
    manager.elements.htmlTab.click();
    expect(container.querySelector('#reports-page-html').hidden).toBe(false);
    const frame = container.querySelector('#reports-preview-frame');
    expect(frame).not.toBeNull();
    expect(frame.closest('.reports-stage-body')).not.toBeNull();
    expect(frame.getAttribute('sandbox')).toBe('');
    expect(frame.style.getPropertyValue('--reports-preview-width')).toBe('8.5in');
    expect(frame.style.getPropertyValue('--reports-preview-height')).toBe('11in');
    expect(manager.elements.previewBtn.hidden).toBe(false);
    expect(manager.elements.fontBtn.hidden).toBe(true);
    manager.elements.pdfTab.click();
    expect(manager.elements.pdfTab.classList.contains('active')).toBe(true);
    expect(manager.elements.composeBtn.hidden).toBe(true);
    expect(manager.elements.previewBtn.hidden).toBe(true);
  });

  it('runs compose, shows the result, and keeps it after an error', async () => {
    await open();
    manager.app.api = {};
    manager.elements.composeTab.click();
    expect(manager.elements.composeBtn.hidden).toBe(false);
    expect(manager._compose.text()).toContain('"pages": []');
    invokeScript.mockResolvedValueOnce({ status: 'completed', result: { pages: [{ bands: [{ id: 't1', items: {} }] }], log: {} } });
    await manager.runCompose();
    expect(invokeScript.mock.calls[0][1].script).toBe('Reports.Render');
    expect(invokeScript.mock.calls[0][1].params.stage).toBe('compose');
    expect(invokeScript.mock.calls[0][1].params.sources).toEqual({});
    expect(manager._compose.text()).toContain('"t1"');
    expect(manager._compose.text()).toContain('"items": []');
    invokeScript.mockResolvedValueOnce({ status: 'failed', error: 'no script' });
    await manager.runCompose();
    expect(manager._compose.text()).toContain('"t1"');
    expect(document.body.textContent.match(/Compose failed/g)).toHaveLength(1);
  });

  it('renders preview HTML into a sandboxed iframe and keeps it after an error', async () => {
    await open();
    manager.app.api = {};
    manager.elements.htmlTab.click();
    const frame = container.querySelector('#reports-preview-frame');
    const pane = container.querySelector('#reports-page-html');
    const assignHtml = vi.spyOn(pane, 'innerHTML', 'set');
    invokeScript.mockResolvedValueOnce({
      status: 'completed',
      result: {
        html: '<section style="width:8.5in"><div style="position:absolute">North</div></section>',
        log: [{ level: 'info', message: 'ok' }],
      },
    });
    manager.elements.previewBtn.click();
    await vi.waitFor(() => {
      expect(frame.srcdoc).toContain('North');
    });
    expect(invokeScript.mock.calls[0][1].script).toBe('Reports.Render');
    expect(invokeScript.mock.calls[0][1].params.stage).toBe('html');
    expect(frame.srcdoc).toContain('position:absolute');
    expect(frame.style.getPropertyValue('--reports-preview-width')).toBe('8.5in');
    expect(frame.getAttribute('sandbox')).toBe('');
    expect(manager._previewLog).toEqual([{ level: 'info', message: 'ok' }]);
    expect(manager._generationLog).toEqual([{ level: 'info', message: 'ok' }]);
    expect(manager.elements.logTab.dataset.generationFlag).toBe('info');
    expect(assignHtml).not.toHaveBeenCalled();
    const kept = frame.srcdoc;
    invokeScript.mockResolvedValueOnce({ status: 'failed', error: 'stage html is not available yet' });
    manager.elements.previewBtn.click();
    await vi.waitFor(() => {
      expect(document.body.textContent.match(/Preview failed/g)).toHaveLength(1);
    });
    expect(frame.srcdoc).toBe(kept);
    expect(assignHtml).not.toHaveBeenCalled();
  });

  it('swaps import and export for fold and font tools on Definition and Compose', async () => {
    localStorage.removeItem('lithium_reports_font_compose');
    await open();
    const shown = (id) => !container.querySelector(id).hidden;
    expect(shown('#reports-import-btn')).toBe(true);
    expect(shown('#reports-font-btn')).toBe(false);
    for (const tab of [manager.elements.jsonTab, manager.elements.composeTab]) {
      tab.click();
      expect(shown('#reports-import-btn')).toBe(false);
      expect(shown('#reports-export-btn')).toBe(false);
      expect(shown('#reports-fold-btn')).toBe(true);
      expect(shown('#reports-unfold-btn')).toBe(true);
      expect(shown('#reports-font-btn')).toBe(true);
    }
    const right = [...container.querySelectorAll('.lithium-toolbar-placeholder ~ button')].map((b) => b.id);
    expect(right[0]).toBe('reports-compose-btn');
    manager.elements.revisionsTab.click();
    expect(shown('#reports-export-btn')).toBe(true);
    localStorage.setItem('lithium_reports_font_compose', JSON.stringify({ fontSize: '18px', fontFamily: 'serif', fontWeight: 'bold' }));
    manager.elements.composeTab.click();
    expect(manager._compose.view.dom.style.fontSize).toBe('18px');
    expect(manager._compose.view.dom.style.fontWeight).toBe('bold');
    manager.elements.fontBtn.click();
    expect(document.querySelector('.manager-font-popup.visible')).not.toBeNull();
    localStorage.removeItem('lithium_reports_font_compose');
  });

  it('fills the Log tab from the last run and clears it for another report', async () => {
    await open();
    manager.app.api = {};
    manager._openReportId = 1;
    invokeScript.mockResolvedValueOnce({
      status: 'completed',
      result: {
        pages: [],
        log: [{ level: 'warn', message: 'datasource orders has no array in sources; no rows' }],
      },
    });
    await manager.runCompose();
    manager.elements.logTab.click();
    const text = manager._log.view.state.doc.toString();
    expect(text).toContain('   0     1      0  warn');
    expect(text).toContain('datasource orders has no array in sources; no rows');
    expect(manager.elements.logTab.dataset.generationFlag).toBe('warn');
    expect(manager.elements.fontBtn.hidden).toBe(false);
    expect(manager.elements.foldBtn.hidden).toBe(true);
    expect(manager.elements.importBtn.hidden).toBe(true);
    const scroller = manager._log.view.dom.querySelector('.cm-scroller');
    expect(getComputedStyle(scroller).fontFamily).toContain('Vanadium Mono');
    await openListedReport(manager, {
      report_id: 2,
      rev_id: 1,
      name: 'Other',
      report: definitionText(manager.doc),
    });
    expect(manager.elements.logTab.dataset.generationFlag).toBeUndefined();
    expect(manager._log.view.state.doc.toString()).not.toContain('datasource orders');
  });
});
