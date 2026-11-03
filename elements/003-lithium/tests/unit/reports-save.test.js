import { beforeEach, describe, expect, it, vi } from 'vitest';
import { authQuery } from '../../src/shared/conduit.js';
import { toast } from '../../src/shared/toast.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { createHistory } from '../../src/managers/reports/model/history.js';
import {
  deleteListedReport,
  openListedReport,
  REPORT_DELETE_QUERY,
  REPORT_GET_QUERY,
  REPORT_INSERT_QUERY,
  reportKey,
  reportSaveParams,
  revisionFrom,
  saveListedReport,
} from '../../src/managers/reports/reports-save.js';

vi.mock('../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
}));

function managerWith(doc) {
  return {
    doc,
    app: { api: { name: 'api' } },
    _closed: false,
    _page: 'editor',
    _metaLock: false,
    _openReportId: 0,
    _openRevision: null,
    _reportOpenToken: 0,
    imported: '',
    summary: undefined,
    _importText(text) {
      this.imported = text;
      this.doc = JSON.parse(text);
    },
    _paintSummary(value) {
      this.summary = value;
    },
    _failImport(message) {
      this.failed = message;
    },
    _commitMetaFields() {},
    commitJson() {
      return true;
    },
  };
}

describe('report save parameters', () => {
  it('allocates a new report and keeps a history trail', () => {
    const history = createHistory(createBlank());
    const doc = history.commit({ ...createBlank(), name: 'Orders' });
    const params = reportSaveParams(doc, { summary: 'Notes' }, null);
    expect(params.INTEGER.REPORT_ID).toBe(0);
    expect(params.STRING.NAME).toBe('Orders');
    expect(params.STRING.SUMMARY).toBe('Notes');
    expect(params.STRING.THUMBNAIL).toBe('');
    expect(params.STRING.DESIGN).toBe('');
    expect(params.STRING.SCRIPTS).toBe('');
    expect(params.STRING.PARAMETERS).toBe('');
    expect(params.STRING.CATEGORY).toBe('');
    const stored = JSON.parse(params.STRING.REPORT);
    expect(stored.name).toBe('Orders');
    expect(stored.history.undo).toHaveLength(1);
  });

  it('omits history after clear and appends an existing report', () => {
    const history = createHistory(createBlank());
    history.commit({ ...createBlank(), name: 'Orders' });
    const cleared = history.clear();
    const params = reportSaveParams(cleared, {
      report_id: '4',
      design: 'layout',
      summary: '',
    }, {
      report_id: 4,
      thumbnail: 'thumb',
      design: '',
      scripts: 'script',
      parameters: '',
    });
    expect(params.INTEGER.REPORT_ID).toBe(4);
    expect(params.STRING.THUMBNAIL).toBe('thumb');
    expect(params.STRING.DESIGN).toBe('layout');
    expect(params.STRING.SCRIPTS).toBe('script');
    expect(Object.hasOwn(JSON.parse(params.STRING.REPORT), 'history')).toBe(false);
    expect(reportKey(0)).toBe(0);
    expect(reportKey('')).toBe(0);
    expect(revisionFrom([{ report_id: '9', rev_id: 2 }])).toEqual({ report_id: 9, rev_id: 2 });
    expect(revisionFrom([])).toBeNull();
  });
});

describe('report list save and open', () => {
  beforeEach(() => {
    authQuery.mockReset();
    toast.dismissAll();
  });

  it('inserts the open definition and stores the returned revision', async () => {
    const manager = managerWith(createBlank());
    const row = {
      getData: () => ({ report_id: 4, summary: 'Notes' }),
      update: vi.fn(),
    };
    manager._openReportId = 4;
    manager._openRevision = {
      report_id: 4,
      thumbnail: 'thumb',
      design: '',
      scripts: '',
      parameters: '',
    };
    authQuery.mockResolvedValueOnce([{ report_id: 4, rev_id: 3 }]);

    await saveListedReport(manager, row);

    expect(authQuery).toHaveBeenCalledWith(
      manager.app.api,
      REPORT_INSERT_QUERY,
      expect.objectContaining({ INTEGER: { REPORT_ID: 4, USERID: 0 } }),
    );
    expect(authQuery.mock.calls[0][2].STRING.THUMBNAIL).toBe('thumb');
    expect(row.update).toHaveBeenCalledWith({
      report_id: 4,
      rev_id: 3,
      name: 'Report',
      summary: 'Notes',
      category: '',
      created_id: expect.any(Number),
      created_at: expect.any(String),
      updated_id: 0,
      updated_at: null,
    });
    expect(manager._openReportId).toBe(4);
    expect(manager._openRevision.rev_id).toBe(3);
    expect(manager.doc.name).toBe('Report');
  });

  it('keeps the document when the insert fails or the row is still opening', async () => {
    const manager = managerWith(createBlank());
    const row = {
      getData: () => ({ report_id: 9 }),
      update: vi.fn(),
    };
    await expect(saveListedReport(manager, row)).rejects.toMatchObject({
      serverError: { message: 'This report is still opening.' },
    });
    expect(authQuery).not.toHaveBeenCalled();

    manager._openReportId = 9;
    const failure = new Error('missing');
    failure.serverError = { error: 'Query failed', message: 'relation missing' };
    authQuery.mockRejectedValueOnce(failure);
    await expect(saveListedReport(manager, row)).rejects.toBe(failure);
    expect(row.update).not.toHaveBeenCalled();
    expect(manager.doc.name).toBe('Report');
  });

  it('loads one revision and ignores a stale reply', async () => {
    const manager = managerWith(createBlank());
    authQuery.mockResolvedValueOnce([{
      report_id: 2,
      rev_id: 1,
      report: JSON.stringify({ format: 1, name: 'Listed' }),
      summary: 'Sum',
      thumbnail: 'pic',
      category: 'Sales',
    }]);

    await openListedReport(manager, { report_id: 2, rev_id: 1 });

    expect(authQuery).toHaveBeenCalledWith(manager.app.api, REPORT_GET_QUERY, {
      INTEGER: { REPORT_ID: 2, REV_ID: 1 },
    });
    expect(manager.doc.name).toBe('Listed');
    expect(manager.summary).toBe('Sum');
    expect(manager._openRevision.thumbnail).toBe('pic');
    expect(manager._openRevision.category).toBe('Sales');

    let release;
    const pending = new Promise((resolve) => {
      release = resolve;
    });
    authQuery.mockReturnValueOnce(pending);
    const opening = openListedReport(manager, { report_id: 8, rev_id: 1 });
    manager._reportOpenToken += 1;
    release([{
      report_id: 8,
      rev_id: 1,
      report: JSON.stringify({ format: 1, name: 'Late' }),
    }]);
    await opening;
    expect(manager.doc.name).toBe('Listed');
  });

  it('uses a definition already on the row and skips the get', async () => {
    const manager = managerWith(createBlank());
    await openListedReport(manager, { report: { format: 1, name: 'Inline' } });
    expect(authQuery).not.toHaveBeenCalled();
    expect(manager.doc.name).toBe('Inline');
    expect(manager._openReportId).toBe(0);
  });

  it('sends a cleared category and carries one the row does not list', () => {
    const doc = createBlank();
    const cleared = reportSaveParams(doc, { report_id: 4, category: '' }, {
      report_id: 4,
      category: 'Sales',
    });
    expect(cleared.STRING.CATEGORY).toBe('');
    const carried = reportSaveParams(doc, { report_id: 4 }, {
      report_id: 4,
      category: 'Sales',
    });
    expect(carried.STRING.CATEGORY).toBe('Sales');
  });

  it('deletes every revision of a saved report and skips a draft', async () => {
    const manager = managerWith(createBlank());
    await deleteListedReport(manager, { report_id: 0 });
    await deleteListedReport(manager, { name: 'Draft' });
    expect(authQuery).not.toHaveBeenCalled();

    await deleteListedReport(manager, { report_id: 4 });
    expect(authQuery).toHaveBeenCalledWith(manager.app.api, REPORT_DELETE_QUERY, {
      INTEGER: { REPORT_ID: 4 },
    });

    const failure = new Error('missing');
    failure.serverError = { error: 'Query failed', message: 'relation missing' };
    authQuery.mockRejectedValueOnce(failure);
    await expect(deleteListedReport(manager, { report_id: 4 })).rejects.toBe(failure);

    manager.app = {};
    await expect(deleteListedReport(manager, { report_id: 4 })).rejects.toMatchObject({
      serverError: { error: 'Delete Failed', message: 'The report service is not connected.' },
    });
  });
});
