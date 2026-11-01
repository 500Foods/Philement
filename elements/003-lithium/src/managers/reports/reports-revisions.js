/**
 * Revisions page for Manager 24.
 * QueryRef 171 lists the saved revisions of one report.
 * QueryRef 172 deletes revisions newer than a chosen rev_id.
 * Copy loads that revision through 167 and inserts it as a new
 * report through 168. Both actions wait for edit mode.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { authQuery } from '../../shared/conduit.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import { openReportText } from './reports-file.js';
import {
  applyStoredReport,
  reportKey,
  reportSaveError,
  reportSaveParams,
  reportUserId,
  REPORT_GET_QUERY,
  REPORT_INSERT_QUERY,
  revisionFrom,
} from './reports-save.js';

export const REPORT_REVISIONS_QUERY = 171;
export const REPORT_REVERT_QUERY = 172;

/**
 * @param {unknown} row
 * @param {string} name
 * @returns {string}
 */
function textField(row, name) {
  if (!row || typeof row !== 'object') return '';
  const value = row[name] ?? row[name.toUpperCase()];
  if (typeof value === 'string') return value.trim();
  if (value == null) return '';
  return String(value).trim();
}

/**
 * @param {Array|null|undefined} rows
 * @returns {Array<{ report_id: number, rev_id: number, created_at: string, created_id: number, account_name: string }>}
 */
export function revisionRows(rows) {
  if (!Array.isArray(rows)) return [];
  const list = [];
  for (const row of rows) {
    const revId = reportKey(row && (row.rev_id ?? row.REV_ID));
    if (!revId) continue;
    list.push({
      report_id: reportKey(row.report_id ?? row.REPORT_ID),
      rev_id: revId,
      created_at: textField(row, 'created_at'),
      created_id: reportKey(row.created_id ?? row.CREATED_ID),
      account_name: textField(row, 'account_name'),
    });
  }
  return list;
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function revisionLabel(row) {
  const rev = reportKey(row && row.rev_id);
  const when = row && row.created_at ? row.created_at : 'Unknown time';
  const name = row && typeof row.account_name === 'string' ? row.account_name.trim() : '';
  const who = name || (reportKey(row && row.created_id) ? `account ${reportKey(row.created_id)}` : 'Unknown');
  return `Revision ${rev} · ${when} · ${who}`;
}

/**
 * Copy needs a saved report, edit mode, and a listed revision.
 * Revert also needs a revision older than the newest one.
 * @param {Array|null|undefined} rows
 * @param {unknown} selectedRev
 * @param {boolean} editing
 * @param {unknown} reportId
 * @returns {{ copy: boolean, revert: boolean }}
 */
export function revisionActions(rows, selectedRev, editing, reportId) {
  const list = Array.isArray(rows) ? rows : [];
  const selected = reportKey(selectedRev);
  const known = list.some((row) => row.rev_id === selected);
  const newest = list.reduce((max, row) => Math.max(max, row.rev_id || 0), 0);
  const ready = Boolean(editing && reportKey(reportId) && known);
  return {
    copy: ready,
    revert: ready && selected < newest,
  };
}

/**
 * @param {object} manager
 * @param {string} status
 * @param {Array} rows
 */
function paintRevisions(manager, status, rows) {
  manager._revisionRows = rows;
  const elements = manager.elements || {};
  if (elements.revisionStatus) elements.revisionStatus.textContent = status;
  const select = elements.revisionSelect;
  if (select) {
    const preferred = reportKey(manager._openRevision && manager._openRevision.rev_id);
    select.replaceChildren();
    for (const row of rows) {
      const option = document.createElement('option');
      option.value = String(row.rev_id);
      option.textContent = revisionLabel(row);
      select.append(option);
    }
    const ids = rows.map((row) => row.rev_id);
    const pick = ids.includes(preferred) ? preferred : (ids[0] || 0);
    if (pick) select.value = String(pick);
    select.disabled = rows.length === 0;
  }
  if (typeof manager.syncRevisionActions === 'function') manager.syncRevisionActions();
}

/**
 * @param {object} manager
 * @returns {object|null}
 */
function selectedRevision(manager) {
  const select = manager.elements && manager.elements.revisionSelect;
  const revId = reportKey(select && select.value);
  const rows = Array.isArray(manager._revisionRows) ? manager._revisionRows : [];
  return rows.find((row) => row.rev_id === revId) || null;
}

/**
 * @param {object} manager
 * @param {number} reportId
 * @param {number} revId
 */
async function loadRevisionBody(manager, reportId, revId) {
  const rows = await authQuery(manager.app.api, REPORT_GET_QUERY, {
    INTEGER: { REPORT_ID: reportId, REV_ID: revId },
  });
  const record = Array.isArray(rows) ? rows[0] : null;
  const body = record && record.report;
  if (body == null || body === '') {
    throw reportSaveError('Revision could not be read', 'The revision has no definition.');
  }
  const text = typeof body === 'string' ? body : JSON.stringify(body);
  const opened = openReportText(manager.doc, text);
  if (opened.error || !opened.doc) {
    throw reportSaveError('Revision could not be read', opened.error || 'The revision has no definition.');
  }
  return {
    text,
    doc: opened.doc,
    record: { ...record, report_id: reportId, rev_id: revId },
  };
}

/**
 * @param {object} manager
 * @param {unknown} error
 * @param {string} title
 */
function toastRevisionError(manager, error, title) {
  if (manager._closed) return;
  const message = error instanceof Error && error.message ? error.message : title;
  log(Subsystems.CONDUIT, Status.ERROR, `[Reports] ${message}`);
  toast.error(error.serverError?.error || title, {
    description: error.serverError?.message || message,
    serverError: error.serverError,
    subsystem: 'Conduit',
  });
}

/**
 * @param {object} manager
 */
export function bindRevisionControls(manager) {
  const {
    revisionsTab, revisionSelect, revisionCopy, revisionRevert, revisionForm,
  } = manager.elements;
  manager._onRevisions = () => {
    manager.showPage('revisions');
  };
  manager._onRevisionPick = () => manager.syncRevisionActions();
  manager._onRevisionCopy = () => {
    manager.copyRevision();
  };
  manager._onRevisionRevert = () => {
    manager.revertRevision();
  };
  manager._onRevisionSubmit = (event) => event.preventDefault();
  revisionsTab.addEventListener('click', manager._onRevisions);
  revisionSelect.addEventListener('change', manager._onRevisionPick);
  revisionCopy.addEventListener('click', manager._onRevisionCopy);
  revisionRevert.addEventListener('click', manager._onRevisionRevert);
  revisionForm.addEventListener('submit', manager._onRevisionSubmit);
}

/**
 * @param {object} manager
 */
export function unbindRevisionControls(manager) {
  const elements = manager.elements || {};
  if (elements.revisionsTab && manager._onRevisions) {
    elements.revisionsTab.removeEventListener('click', manager._onRevisions);
  }
  if (elements.revisionSelect && manager._onRevisionPick) {
    elements.revisionSelect.removeEventListener('change', manager._onRevisionPick);
  }
  if (elements.revisionCopy && manager._onRevisionCopy) {
    elements.revisionCopy.removeEventListener('click', manager._onRevisionCopy);
  }
  if (elements.revisionRevert && manager._onRevisionRevert) {
    elements.revisionRevert.removeEventListener('click', manager._onRevisionRevert);
  }
  if (elements.revisionForm && manager._onRevisionSubmit) {
    elements.revisionForm.removeEventListener('submit', manager._onRevisionSubmit);
  }
}

export const reportRevisionMethods = {
  syncRevisionActions() {
    const elements = this.elements || {};
    const actions = revisionActions(
      this._revisionRows,
      elements.revisionSelect && elements.revisionSelect.value,
      this._recordEditing,
      this._openReportId,
    );
    if (elements.revisionCopy) elements.revisionCopy.disabled = !actions.copy;
    if (elements.revisionRevert) elements.revisionRevert.disabled = !actions.revert;
  },

  refreshRevisions() {
    if (this._closed || this._page !== 'revisions') return undefined;
    return this.openRevisions();
  },

  /**
   * Fill the revisions list for the open saved report.
   * A draft has no server revisions.
   */
  async openRevisions() {
    if (this._closed) return;
    const token = (this._revisionsToken = (this._revisionsToken || 0) + 1);
    const reportId = reportKey(this._openReportId);
    if (!reportId) {
      paintRevisions(this, 'Save the report before its revisions can be listed.', []);
      return;
    }
    if (!this.app?.api) {
      paintRevisions(this, 'Revisions could not be loaded.', []);
      toast.error('Revisions could not be loaded', {
        description: 'The report service is not connected.',
        subsystem: 'Manager',
      });
      return;
    }
    paintRevisions(this, 'Loading revisions.', []);
    try {
      const rows = await authQuery(this.app.api, REPORT_REVISIONS_QUERY, {
        INTEGER: { REPORT_ID: reportId },
      });
      if (this._closed || this._revisionsToken !== token) return;
      const list = revisionRows(rows);
      const status = list.length === 1 ? '1 revision.' : `${list.length} revisions.`;
      paintRevisions(this, list.length ? status : 'This report has no saved revisions.', list);
    } catch (error) {
      if (this._closed || this._revisionsToken !== token) return;
      paintRevisions(this, 'Revisions could not be loaded.', []);
      toastRevisionError(this, error, 'Revisions could not be loaded');
    }
  },

  /** Copy the selected revision into a new report at revision 1. */
  async copyRevision() {
    if (this._revisionsBusy || this._closed || !this._recordEditing) return;
    const reportId = reportKey(this._openReportId);
    const picked = selectedRevision(this);
    const actions = revisionActions(this._revisionRows, picked && picked.rev_id, true, reportId);
    if (!actions.copy || !picked) return;
    this._revisionsBusy = true;
    try {
      const accepted = await confirmDialog({
        title: 'Copy revision',
        message: `Copy revision ${picked.rev_id} to a new report. The new report starts at revision 1.`,
        confirmLabel: 'Copy',
      });
      if (!accepted || this._closed) return;
      if (!this.app?.api) {
        throw reportSaveError('Copy Failed', 'The report service is not connected.');
      }
      const loaded = await loadRevisionBody(this, reportId, picked.rev_id);
      if (this._closed) return;
      const source = {
        report_id: 0,
        summary: loaded.record.summary,
        thumbnail: loaded.record.thumbnail,
        design: loaded.record.design,
        scripts: loaded.record.scripts,
        parameters: loaded.record.parameters,
      };
      if (typeof loaded.record.category === 'string') source.category = loaded.record.category;
      const params = reportSaveParams(loaded.doc, source, null, reportUserId(this.app));
      const rows = await authQuery(this.app.api, REPORT_INSERT_QUERY, params);
      const saved = revisionFrom(rows);
      if (!saved) throw reportSaveError('Copy Failed', 'The insert did not return a report id.');
      const table = this.reportTable && this.reportTable.table;
      if (table && typeof table.addRow === 'function') {
        await table.addRow({
          report_id: saved.report_id,
          rev_id: saved.rev_id,
          name: params.STRING.NAME,
          summary: params.STRING.SUMMARY,
          category: params.STRING.CATEGORY,
        }, true);
      }
      log(Subsystems.CONDUIT, Status.INFO,
        `[Reports] Copied report ${reportId} revision ${picked.rev_id} as ${saved.report_id}`);
      toast.success('Copied to a new report', {
        description: `Report ${saved.report_id} is revision ${saved.rev_id}.`,
        subsystem: 'Manager',
      });
    } catch (error) {
      toastRevisionError(this, error, 'Copy Failed');
    } finally {
      this._revisionsBusy = false;
      this.syncRevisionActions();
    }
  },

  /**
   * Delete revisions newer than the selection, then reload that revision.
   * QueryRef 170 is not used. That query deletes every revision.
   */
  async revertRevision() {
    if (this._revisionsBusy || this._closed || !this._recordEditing) return;
    const reportId = reportKey(this._openReportId);
    const picked = selectedRevision(this);
    const actions = revisionActions(this._revisionRows, picked && picked.rev_id, true, reportId);
    if (!actions.revert || !picked) return;
    this._revisionsBusy = true;
    try {
      const accepted = await confirmDialog({
        title: 'Revert report',
        message: `Revert to revision ${picked.rev_id}. Every newer revision is deleted, and this report reloads as revision ${picked.rev_id}.`,
        confirmLabel: 'Revert',
        danger: true,
      });
      if (!accepted || this._closed) return;
      if (!this.app?.api) {
        throw reportSaveError('Revert Failed', 'The report service is not connected.');
      }
      const loaded = await loadRevisionBody(this, reportId, picked.rev_id);
      if (this._closed) return;
      await authQuery(this.app.api, REPORT_REVERT_QUERY, {
        INTEGER: { REPORT_ID: reportId, REV_ID: picked.rev_id },
      });
      if (this._closed) return;
      const applied = applyStoredReport(this, loaded.text, loaded.record);
      if (!applied) {
        toast.error('Revert failed', {
          description: 'The newer revisions were deleted. Open the report again.',
          subsystem: 'Manager',
        });
        await this.openRevisions();
        return;
      }
      const selected = this.reportTable?.table?.getSelectedRows?.()?.[0];
      if (selected && typeof selected.update === 'function') {
        const held = this._metaLock;
        this._metaLock = true;
        try {
          selected.update({ rev_id: picked.rev_id });
        } finally {
          this._metaLock = held;
        }
      }
      if (this.editHelper && typeof this.editHelper._takeSnapshot === 'function') {
        this.editHelper._takeSnapshot();
      }
      this.editHelper?.checkDirtyState?.();
      log(Subsystems.CONDUIT, Status.INFO,
        `[Reports] Reverted report ${reportId} to revision ${picked.rev_id}`);
      toast.success('Reverted', {
        description: `This report is revision ${picked.rev_id}.`,
        subsystem: 'Manager',
      });
      await this.openRevisions();
    } catch (error) {
      toastRevisionError(this, error, 'Revert Failed');
    } finally {
      this._revisionsBusy = false;
      this.syncRevisionActions();
    }
  },
};
