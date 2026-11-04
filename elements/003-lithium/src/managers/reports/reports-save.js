/**
 * Report list queries for Manager 24.
 * List is 165, search is 166, get is 167, and insert is 168.
 * Insert with report id 0 allocates a report and stores revision 1.
 * Any other id appends the next revision. INTEGER USERID is the
 * signed-in account and is stored as created_id once that rewrite
 * is applied. The stored definition keeps a non-empty history
 * trail. Delete is QueryRef 170 and removes every revision.
 * Duplicate has no query. Category is a row column. A blank
 * category is an empty string.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { definitionText, openReportText } from './reports-file.js';
import { definitionMeta, rowSummary } from './reports-meta.js';

export const REPORT_LIST_QUERY = 165;
export const REPORT_SEARCH_QUERY = 166;
export const REPORT_GET_QUERY = 167;
export const REPORT_INSERT_QUERY = 168;
export const REPORT_DELETE_QUERY = 170;

/**
 * @param {string} title
 * @param {string} message
 * @returns {Error}
 */
export function reportSaveError(title, message) {
  const error = new Error(message);
  error.serverError = { error: title, message };
  return error;
}

/**
 * @param {unknown} value
 * @returns {number}
 */
export function reportKey(value) {
  const number = typeof value === 'number' ? value : Number(value);
  if (!Number.isInteger(number) || number <= 0) return 0;
  return number;
}

/**
 * Signed-in account id for a new revision. A missing id is 0.
 * @param {object|null|undefined} app
 * @returns {number}
 */
export function reportUserId(app) {
  const raw = app && app.auth && app.auth.user ? app.auth.user.id : 0;
  const id = typeof raw === 'number' ? raw : Number(raw);
  return Number.isInteger(id) && id > 0 ? id : 0;
}

/**
 * @param {unknown} value
 * @returns {string}
 */
function storedText(value) {
  return typeof value === 'string' ? value : '';
}

/**
 * @param {object|null|undefined} row
 * @returns {object}
 */
function rowData(row) {
  if (row && typeof row.getData === 'function') return row.getData() || {};
  if (row && typeof row === 'object') return row;
  return {};
}

/**
 * @param {Array|null|undefined} rows
 * @returns {{ report_id: number, rev_id: number }|null}
 */
export function revisionFrom(rows) {
  const row = Array.isArray(rows) ? rows[0] : null;
  if (!row || typeof row !== 'object') return null;
  const reportId = reportKey(row.report_id ?? row.REPORT_ID);
  const revId = reportKey(row.rev_id ?? row.REV_ID);
  if (!reportId || !revId) return null;
  return { report_id: reportId, rev_id: revId };
}

/**
 * Parameters for QueryRef 168. A blank large column is an empty string.
 * @param {object} doc
 * @param {object|null|undefined} row
 * @param {object|null|undefined} kept
 * @param {number} [userId]
 * @returns {{ INTEGER: { REPORT_ID: number, USERID: number }, STRING: object }}
 */
export function reportSaveParams(doc, row, kept, userId) {
  const data = row || {};
  const reportId = reportKey(data.report_id);
  const user = Number.isInteger(userId) && userId > 0 ? userId : 0;
  const same = Boolean(kept && kept.report_id === reportId && reportId > 0);
  const carried = (field) => {
    const onRow = storedText(data[field]);
    if (onRow !== '') return onRow;
    return same ? storedText(kept[field]) : '';
  };
  const category = data && Object.hasOwn(data, 'category')
    ? storedText(data.category)
    : (same ? storedText(kept && kept.category) : '');
  return {
    INTEGER: { REPORT_ID: reportId, USERID: user },
    STRING: {
      NAME: definitionMeta(doc).name,
      REPORT: definitionText(doc),
      THUMBNAIL: carried('thumbnail'),
      DESIGN: carried('design'),
      SCRIPTS: carried('scripts'),
      PARAMETERS: carried('parameters'),
      SUMMARY: storedText(data.summary),
      CATEGORY: category,
    },
  };
}

/**
 * @param {object} manager
 * @param {object} data
 */
function rememberOpen(manager, data) {
  const reportId = reportKey(data && data.report_id);
  if (manager._generationReportId != null && manager._generationReportId !== reportId
    && typeof manager.clearGeneration === 'function') {
    manager.clearGeneration();
  }
  manager._openReportId = reportId;
  manager._openRevision = reportId ? {
    report_id: reportId,
    rev_id: reportKey(data.rev_id),
    thumbnail: storedText(data.thumbnail),
    design: storedText(data.design),
    scripts: storedText(data.scripts),
    parameters: storedText(data.parameters),
    category: storedText(data.category),
  } : null;
}

/**
 * @param {object} manager
 * @param {string} text
 * @param {object} data
 */
export function applyStoredReport(manager, text, data) {
  const opened = openReportText(manager.doc, text);
  if (opened.error) {
    if (typeof manager._failImport === 'function') manager._failImport(opened.error);
    return false;
  }
  if (typeof manager._importText === 'function') manager._importText(text);
  rememberOpen(manager, data);
  if (typeof manager._paintSummary === 'function') manager._paintSummary(rowSummary(data));
  return true;
}

/**
 * Open a listed revision. A row that already carries the definition
 * uses that text. A saved id loads QueryRef 167. A new row does not.
 * @param {object} manager
 * @param {object} row
 */
export async function openListedReport(manager, row) {
  const token = (manager._reportOpenToken = (manager._reportOpenToken || 0) + 1);
  const data = row || {};
  const stored = data.report;
  if (stored != null && stored !== '') {
    const text = typeof stored === 'string' ? stored : JSON.stringify(stored);
    applyStoredReport(manager, text, data);
    if (typeof manager.refreshRevisions === 'function') manager.refreshRevisions();
    return;
  }

  const reportId = reportKey(data.report_id);
  const revId = reportKey(data.rev_id);
  if (!reportId || !revId) {
    if (typeof manager._paintSummary === 'function') manager._paintSummary(rowSummary(data));
    return;
  }
  if (!manager.app?.api) {
    toast.error('Report could not be opened', {
      description: 'The report service is not connected.',
      subsystem: 'Manager',
    });
    return;
  }

  try {
    const rows = await authQuery(manager.app.api, REPORT_GET_QUERY, {
      INTEGER: { REPORT_ID: reportId, REV_ID: revId },
    });
    if (manager._closed || manager._reportOpenToken !== token) return;
    const record = Array.isArray(rows) ? rows[0] : null;
    const body = record && record.report;
    if (body == null || body === '') {
      throw reportSaveError('Report could not be opened', 'The revision has no definition.');
    }
    const text = typeof body === 'string' ? body : JSON.stringify(body);
    const opened = applyStoredReport(manager, text, { ...data, ...record, report_id: reportId, rev_id: revId });
    if (opened) {
      log(Subsystems.CONDUIT, Status.INFO, `[Reports] Opened report ${reportId} revision ${revId}`);
      if (typeof manager.refreshRevisions === 'function') manager.refreshRevisions();
    }
  } catch (error) {
    if (manager._closed || manager._reportOpenToken !== token) return;
    const message = error instanceof Error && error.message ? error.message : 'Report could not be opened.';
    log(Subsystems.CONDUIT, Status.ERROR, `[Reports] Open failed: ${message}`);
    toast.error(error.serverError?.error || 'Report could not be opened', {
      description: error.serverError?.message || message,
      serverError: error.serverError,
      subsystem: 'Conduit',
    });
  }
}

/**
 * Append a revision of the open definition. A failed call throws and
 * leaves the document dirty for the table save path.
 * @param {object} manager
 * @param {object} row
 */
export async function saveListedReport(manager, row) {
  if (!manager?.doc) {
    throw reportSaveError('Save is not available', 'There is no open report.');
  }
  if (manager._page === 'json' && typeof manager.commitJson === 'function' && !manager.commitJson({ quiet: true })) {
    throw reportSaveError('Report JSON could not be read.', 'Fix the JSON before saving.');
  }
  if (typeof manager._commitMetaFields === 'function') manager._commitMetaFields();

  const data = rowData(row);
  const reportId = reportKey(data.report_id);
  if (reportId > 0 && manager._openReportId !== reportId) {
    throw reportSaveError('Save is not available', 'This report is still opening.');
  }
  if (!manager.app?.api) {
    throw reportSaveError('Save is not available', 'The report service is not connected.');
  }

  const params = reportSaveParams(
    manager.doc,
    data,
    manager._openRevision,
    reportUserId(manager.app),
  );
  const rows = await authQuery(manager.app.api, REPORT_INSERT_QUERY, params);
  const saved = revisionFrom(rows);
  if (!saved) {
    throw reportSaveError('Save Failed', 'The insert did not return a report id.');
  }

  const held = manager._metaLock;
  manager._metaLock = true;
  try {
    if (row && typeof row.update === 'function') {
      row.update({
        report_id: saved.report_id,
        rev_id: saved.rev_id,
        name: definitionMeta(manager.doc).name,
        summary: storedText(rowData(row).summary),
        category: params.STRING.CATEGORY,
        created_id: saved.created_id ?? reportUserId(manager.app),
        created_at: saved.created_at ?? new Date().toISOString(),
        updated_id: saved.updated_id ?? 0,
        updated_at: saved.updated_at ?? null,
      });
    }
  } finally {
    manager._metaLock = held;
  }

  manager._openReportId = saved.report_id;
  if (manager._generationReportId === 0 || manager._generationReportId === reportId) {
    manager._generationReportId = saved.report_id;
  }
  manager._openRevision = {
    report_id: saved.report_id,
    rev_id: saved.rev_id,
    thumbnail: params.STRING.THUMBNAIL,
    design: params.STRING.DESIGN,
    scripts: params.STRING.SCRIPTS,
    parameters: params.STRING.PARAMETERS,
    category: params.STRING.CATEGORY,
  };
  log(Subsystems.CONDUIT, Status.INFO,
    `[Reports] Saved report ${saved.report_id} revision ${saved.rev_id}`);
  if (typeof manager.updateAuditFooter === 'function') manager.updateAuditFooter(rowData(row));
  if (typeof manager.refreshRevisions === 'function') manager.refreshRevisions();
}

/**
 * Delete every revision of a saved report. A draft id does not call
 * QueryRef 170. A failed call throws and leaves the row in place.
 * @param {object} manager
 * @param {object|null|undefined} data
 */
export async function deleteListedReport(manager, data) {
  const reportId = reportKey(data && data.report_id);
  if (!reportId) return;
  if (!manager?.app?.api) {
    throw reportSaveError('Delete Failed', 'The report service is not connected.');
  }
  await authQuery(manager.app.api, REPORT_DELETE_QUERY, {
    INTEGER: { REPORT_ID: reportId },
  });
  log(Subsystems.CONDUIT, Status.INFO, `[Reports] Deleted report ${reportId}`);
}
