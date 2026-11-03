/* globals Event */
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { authQuery } from '../../src/shared/conduit.js';
import { toast } from '../../src/shared/toast.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { definitionText } from '../../src/managers/reports/reports-file.js';
import {
  revisionActions,
  revisionLabel,
  revisionRows,
  REPORT_REVERT_QUERY,
  REPORT_REVISIONS_QUERY,
} from '../../src/managers/reports/reports-revisions.js';
import { REPORT_GET_QUERY, REPORT_INSERT_QUERY, reportSaveParams, reportUserId } from '../../src/managers/reports/reports-save.js';
import ReportsManager from '../../src/managers/reports/reports.js';

vi.mock('../../src/managers/reports/report-lookups.js', () => ({
  loadPanelLookups: async () => ({}),
}));

vi.mock('../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
}));

vi.mock('../../src/tables/lithium-table-main.js', () => ({
  LithiumTable: class {
    constructor() {
      this.table = { redraw() {} };
      this.handleAdd = async () => {};
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

describe('revision labels', () => {
  it('names the revision, the time, and the account', () => {
    const rows = revisionRows([
      { rev_id: '3', created_at: '2026-10-31 18:04', created_id: 9, account_name: 'Andrew' },
      { REV_ID: 1, CREATED_ID: 0 },
      { rev_id: 0 },
    ]);
    expect(rows).toEqual([
      { report_id: 0, rev_id: 3, created_at: '2026-10-31 18:04', created_id: 9, account_name: 'Andrew' },
      { report_id: 0, rev_id: 1, created_at: '', created_id: 0, account_name: '' },
    ]);
    expect(revisionLabel(rows[0])).toBe('Revision 3 · 2026-10-31 18:04 · Andrew');
    expect(revisionLabel(rows[1])).toBe('Revision 1 · Unknown time · Unknown');
    expect(revisionLabel({ rev_id: 2, created_id: 4, created_at: '', account_name: '' }))
      .toBe('Revision 2 · Unknown time · account 4');
    expect(revisionActions(rows, 1, true, 4)).toEqual({ copy: true, revert: true });
    expect(revisionActions(rows, 3, true, 4)).toEqual({ copy: true, revert: false });
    expect(revisionActions(rows, 1, false, 4)).toEqual({ copy: false, revert: false });
    expect(revisionActions(rows, 1, true, 0)).toEqual({ copy: false, revert: false });
    expect(reportUserId({ auth: { user: { id: '9' } } })).toBe(9);
    expect(reportUserId({})).toBe(0);
    expect(reportSaveParams(createBlank(), { report_id: 4 }, null, 9).INTEGER).toEqual({
      REPORT_ID: 4,
      USERID: 9,
    });
  });
});

describe('Revisions page', () => {
  let container;
  let manager;

  beforeEach(() => {
    localStorage.removeItem('lithium_reports_left_collapsed');
    localStorage.removeItem('lithium_reports_left_width');
    toast.dismissAll();
    authQuery.mockReset();
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
    manager = new ReportsManager({ api: { name: 'api' }, auth: { user: { id: 9 } } }, host);
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

  it('lists saved revisions and keeps copy and revert behind edit mode', async () => {
    await open();
    expect(container.querySelector('#reports-page-revisions').hidden).toBe(true);
    await manager.showPage('revisions');
    expect(container.querySelector('#reports-page-revisions').hidden).toBe(false);
    expect(container.querySelector('#reports-page-editor').hidden).toBe(true);
    expect(manager.elements.revisionsTab.classList.contains('active')).toBe(true);
    expect(manager.elements.editorTab.classList.contains('active')).toBe(false);
    expect(manager.elements.revisionStatus.textContent).toBe('Save the report before its revisions can be listed.');
    expect(authQuery).not.toHaveBeenCalled();

    manager._openReportId = 4;
    manager._openRevision = { report_id: 4, rev_id: 3 };
    authQuery.mockResolvedValueOnce([
      { rev_id: 3, created_at: '2026-10-31 18:04', created_id: 9, account_name: 'Andrew' },
      { rev_id: 1, created_at: '2026-10-31 17:00', created_id: 0, account_name: '' },
    ]);
    await manager.showPage('revisions');
    expect(authQuery).toHaveBeenCalledWith(manager.app.api, REPORT_REVISIONS_QUERY, {
      INTEGER: { REPORT_ID: 4 },
    });
    expect(manager.elements.revisionSelect.options).toHaveLength(2);
    expect(manager.elements.revisionSelect.value).toBe('3');
    expect(manager.elements.revisionSelect.options[0].textContent).toBe('Revision 3 · 2026-10-31 18:04 · Andrew');
    expect(manager.elements.revisionCopy.disabled).toBe(false);
    expect(manager.elements.revisionRevert.disabled).toBe(true);

    manager.setRecordEditing(false);
    expect(manager.elements.revisionCopy.disabled).toBe(true);
    expect(manager.elements.revisionRevert.disabled).toBe(true);
    manager.setRecordEditing(true);
    manager.elements.revisionSelect.value = '1';
    manager.elements.revisionSelect.dispatchEvent(new Event('change'));
    expect(manager.elements.revisionRevert.disabled).toBe(false);
  });

  it('copies the selected revision to a new report and reverts by deleting newer rows', async () => {
    await open();
    manager._openReportId = 4;
    manager._openRevision = { report_id: 4, rev_id: 3, category: 'Sales' };
    manager.doc = { ...manager.doc, name: 'Current' };
    const added = vi.fn();
    const updated = vi.fn();
    manager.reportTable.table = {
      redraw() {},
      addRow: added,
      getSelectedRows: () => [{ update: updated }],
    };
    authQuery.mockResolvedValueOnce([
      { rev_id: 3, created_at: 'later', created_id: 9, account_name: 'Andrew' },
      { rev_id: 1, created_at: 'earlier', created_id: 9, account_name: 'Andrew' },
    ]);
    await manager.showPage('revisions');
    manager.elements.revisionSelect.value = '1';
    manager.syncRevisionActions();

    const older = definitionText({ ...createBlank(), name: 'Older' });
    authQuery.mockResolvedValueOnce([{
      report_id: 4,
      rev_id: 1,
      report: older,
      summary: 'Old notes',
      category: 'Sales',
      thumbnail: 'pic',
      design: '',
      scripts: '',
      parameters: '',
    }]);
    authQuery.mockResolvedValueOnce([{ report_id: 20, rev_id: 1 }]);
    const copying = manager.copyRevision();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm .btn-primary')).not.toBeNull();
    });
    document.querySelector('.lithium-confirm .btn-secondary').click();
    await copying;
    expect(authQuery).toHaveBeenCalledTimes(1);

    const copied = manager.copyRevision();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm .btn-primary')).not.toBeNull();
    });
    document.querySelector('.lithium-confirm .btn-primary').click();
    await copied;
    expect(authQuery).toHaveBeenNthCalledWith(2, manager.app.api, REPORT_GET_QUERY, {
      INTEGER: { REPORT_ID: 4, REV_ID: 1 },
    });
    const insert = authQuery.mock.calls[2];
    expect(insert[1]).toBe(REPORT_INSERT_QUERY);
    expect(insert[2].INTEGER).toEqual({ REPORT_ID: 0, USERID: 9 });
    expect(insert[2].STRING.NAME).toBe('Older');
    expect(insert[2].STRING.CATEGORY).toBe('Sales');
    expect(insert[2].STRING.THUMBNAIL).toBe('pic');
    expect(JSON.parse(insert[2].STRING.REPORT).name).toBe('Older');
    expect(added).toHaveBeenCalledWith({
      report_id: 20,
      rev_id: 1,
      name: 'Older',
      summary: 'Old notes',
      category: 'Sales',
    }, true);
    expect(manager.doc.name).toBe('Current');
    expect(document.body.textContent).toContain('Copied to a new report');

    authQuery.mockResolvedValueOnce([{
      report_id: 4,
      rev_id: 1,
      report: older,
      summary: 'Old notes',
      category: 'Sales',
    }]);
    authQuery.mockResolvedValueOnce([]);
    authQuery.mockResolvedValueOnce([
      { rev_id: 1, created_at: 'earlier', created_id: 9, account_name: 'Andrew' },
    ]);
    const reverting = manager.revertRevision();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm .btn-danger')).not.toBeNull();
    });
    document.querySelector('.lithium-confirm .btn-danger').click();
    await reverting;
    expect(authQuery).toHaveBeenCalledWith(manager.app.api, REPORT_REVERT_QUERY, {
      INTEGER: { REPORT_ID: 4, REV_ID: 1 },
    });
    expect(manager.doc.name).toBe('Older');
    expect(manager._openRevision.rev_id).toBe(1);
    expect(updated).toHaveBeenCalledWith({ rev_id: 1 });
    expect(manager.editHelper.isAnythingDirty()).toBe(false);
    expect(document.body.textContent).toContain('Reverted');
    expect(manager.elements.revisionSelect.options).toHaveLength(1);
  });
});
