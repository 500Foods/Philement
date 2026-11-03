/**
 * Report list for Manager 24.
 * The left pane is a LithiumTable of the reports table.
 * List is QueryRef 165, search is 166, open is 167, and save is 168.
 * Save appends a revision and sends the row category.
 * Duplicate has no query. Delete is QueryRef 170 and removes
 * every revision of the report. Import and export still write
 * the definition file.
 */

import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { setupManagerFooterIcons } from '../../core/manager-ui.js';
import { AuditInfoFooter } from '../../core/audit-info-footer.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import { LithiumTable } from '../../tables/lithium-table-main.js';
import { definitionMeta, withDefinitionMeta } from './reports-meta.js';
import { reportEditorContent } from './reports-json.js';
import {
  deleteListedReport,
  openListedReport,
  reportKey,
  REPORT_DELETE_QUERY,
  REPORT_GET_QUERY,
  REPORT_INSERT_QUERY,
  REPORT_LIST_QUERY,
  REPORT_SEARCH_QUERY,
  saveListedReport,
} from './reports-save.js';

const LIST_KEY = 'lithium_reports_left';
const LIST_WIDTH = 360;

const COLTYPES = {
  default: {},
  string: { sorter: 'string', editor: 'input', editorParams: { search: false } },
  index: { sorter: 'number', hozAlign: 'right' },
  text: { sorter: 'string', editor: 'textarea' },
};

const ACTION_LABEL = {
  duplicate: 'Duplicate',
  delete: 'Delete',
};

const ACTION_DETAIL = {
  duplicate: 'Duplicate is not available. There is no duplicate query for reports.',
  delete: 'Delete is not available. A saved report is kept as revisions.',
};

/**
 * @param {string} field
 * @param {string} title
 * @param {string} coltype
 * @param {object} [extra]
 */
function column(field, title, coltype, extra = {}) {
  return {
    title,
    field,
    coltype,
    visible: false,
    sort: true,
    filter: false,
    editable: false,
    calculated: false,
    primaryKey: false,
    ...extra,
  };
}

/** Coltypes passed with the in-memory table. String and text include editors. */
export function reportListColtypes() {
  return {
    default: { ...COLTYPES.default },
    string: { ...COLTYPES.string, editorParams: { ...COLTYPES.string.editorParams } },
    index: { ...COLTYPES.index },
    text: { ...COLTYPES.text },
  };
}

/** Column list for the reports table. No QueryRefs. */
export function reportListTableDef() {
  return {
    title: 'Reports',
    table: 'reports',
    readonly: false,
    layout: 'fitColumns',
    selectableRows: 1,
    columns: {
      report_id: column('report_id', 'Report', 'index', { primaryKey: true }),
      rev_id: column('rev_id', 'Rev', 'index', { primaryKey: true, visible: true }),
      name: column('name', 'Name', 'string', { visible: true, editable: true }),
      category: column('category', 'Category', 'string', { visible: true, editable: true }),
      summary: column('summary', 'Summary', 'text', { visible: true, editable: true }),
      report: column('report', 'Definition', 'text'),
      thumbnail: column('thumbnail', 'Thumbnail', 'text'),
      design: column('design', 'Design', 'text'),
      scripts: column('scripts', 'Scripts', 'text'),
      parameters: column('parameters', 'Parameters', 'text'),
    },
  };
}

/**
 * Visible failure for a list action that has no QueryRef yet.
 * @param {'save'|'duplicate'|'delete'} action
 * @returns {Error}
 */
export function reportListUnavailable(action) {
  const label = ACTION_LABEL[action] || 'Save';
  const error = new Error(`No queryRef configured for ${action}`);
  error.serverError = {
    error: `${label} is not available`,
    message: ACTION_DETAIL[action] || `${label} is not available.`,
  };
  return error;
}

/**
 * @param {object} manager
 */
function wireFooter(manager) {
  const slot = manager.container.closest('.manager-slot');
  const group = slot?.querySelector('.manager-slot-footer .subpanel-header-group');
  if (!group) return;
  const placeholder = group.querySelector('.slot-footer-placeholder');
  const footer = setupManagerFooterIcons(group, {
    anchor: placeholder,
    showSaveCancel: true,
    fillerTitle: 'Reports',
  });
  manager.editHelper.wireFooterButtons(footer.saveBtn, footer.cancelBtn, footer.dummyBtn);
  wireAuditFooter(manager);
}

function auditSet(id, at) {
  return id !== undefined && id !== null && id !== '' && Number(id) !== 0
    && at !== undefined && at !== null && at !== '';
}

/**
 * Audit view of a report row. Each save appends a revision, so
 * updated_id stays 0 and the saver is created_id. Use updated_* when
 * set, otherwise created_*. Null when neither pair is set.
 * @param {object|null|undefined} data
 * @returns {object|null}
 */
export function reportAuditRow(data) {
  if (!data || typeof data !== 'object') return null;
  const useUpdated = auditSet(data.updated_id, data.updated_at);
  if (!useUpdated && !auditSet(data.created_id, data.created_at)) return null;
  return {
    ...data,
    created_by: data.created_by ?? data.created_id,
    updated_id: useUpdated ? data.updated_id : data.created_id,
    updated_at: useUpdated ? data.updated_at : data.created_at,
  };
}

/**
 * Audit button after save/cancel, as in Query Manager. The table
 * gets a thin adapter so row selection shows the fallback view.
 * @param {object} manager
 */
function wireAuditFooter(manager) {
  if (manager.auditFooter) manager.auditFooter.destroy();
  const footer = new AuditInfoFooter(manager);
  footer.init();
  if (!footer.auditButton) return;
  manager.auditFooter = footer;
  manager.updateAuditFooter = (data) => footer.update(reportAuditRow(data));
  if (manager.reportTable) {
    manager.reportTable.auditFooter = { update: (data) => manager.updateAuditFooter(data) };
  }
}

/**
 * @param {object} manager
 */
export async function mountReportList(manager) {
  const { elements } = manager;
  if (!elements.tableContainer || !elements.navigatorContainer || !elements.leftPanel) return;

  const panelState = new PanelStateManager(LIST_KEY);
  manager._panelState = panelState;
  manager._panelWidth = panelState.loadWidth(LIST_WIDTH);
  manager._panelCollapsed = panelState.loadCollapsed(false);

  const table = new LithiumTable({
    container: elements.tableContainer,
    navigatorContainer: elements.navigatorContainer,
    tablePath: 'reports/report-manager',
    tableDef: reportListTableDef(),
    coltypes: COLTYPES,
    primaryKeyField: ['report_id', 'rev_id'],
    cssPrefix: 'reports',
    storageKey: 'reports_table',
    app: manager.app,
    readonly: false,
    tableWidthMode: 'compact',
    panel: elements.leftPanel,
    panelStateManager: panelState,
    queryRef: REPORT_LIST_QUERY,
    searchQueryRef: REPORT_SEARCH_QUERY,
    detailQueryRef: REPORT_GET_QUERY,
    insertQueryRef: REPORT_INSERT_QUERY,
    deleteQueryRef: REPORT_DELETE_QUERY,
    onExecuteSave: (row) => saveListedReport(manager, row),
    onDuplicate: async () => {
      throw reportListUnavailable('duplicate');
    },
    onRowSelected: (row) => {
      openListedReport(manager, row);
    },
  });
  manager.reportTable = table;
  manager.editHelper.registerTable(table);
  manager.editHelper.registerEditor('definition', {
    getContent: () => reportEditorContent(
      manager.doc,
      manager._page,
      manager._recordEditing && manager._json ? manager._json.text() : '',
    ),
    setContent: (content) => manager._importText(content),
    setEditable: (editable) => manager.setRecordEditing(editable),
    boundTable: table,
  });

  try {
    await table.init();
    await table.loadData();
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Report list could not be loaded.';
    log(Subsystems.TABLE, Status.ERROR, `[Reports] ${message}`);
    toast.error('Report list is not available', {
      description: message,
      subsystem: 'Manager',
    });
  }

  const grid = table.table;
  if (grid && typeof grid.on === 'function') {
    grid.on('cellEdited', (cell) => applyListedCell(manager, cell));
  }

  if (typeof table.handleAdd === 'function') {
    const addRow = table.handleAdd.bind(table);
    table.handleAdd = async () => {
      await addRow();
      manager.newReport();
    };
  }
  table.handleDelete = () => deleteReportRow(manager);

  manager.splitter = new LithiumSplitter({
    element: elements.splitter,
    leftPanel: elements.leftPanel,
    minWidth: 157,
    maxWidth: 1000,
    tables: table,
    onResize: (width) => {
      manager._panelWidth = width;
    },
    onResizeEnd: (width) => {
      panelState.saveWidth(width);
      table.table?.redraw?.();
    },
  });
  table.setSplitter?.(manager.splitter);

  manager._onCollapse = () => {
    manager._panelCollapsed = togglePanelCollapse({
      panel: elements.leftPanel,
      splitter: manager.splitter,
      collapseBtn: elements.collapseBtn,
      panelWidth: manager._panelWidth,
      isCollapsed: manager._panelCollapsed,
      onAfterToggle: () => table.table?.redraw?.(),
    });
    panelState.saveCollapsed(manager._panelCollapsed);
  };
  elements.collapseBtn?.addEventListener('click', manager._onCollapse);
  restorePanelState({
    panel: elements.leftPanel,
    splitter: manager.splitter,
    collapseBtn: elements.collapseBtn,
    isCollapsed: manager._panelCollapsed,
  });
  wireFooter(manager);
}

/**
 * A name edit on the list is the same definition name.
 * Summary and category stay on the row.
 * @param {object} manager
 * @param {object} cell
 */
function applyListedCell(manager, cell) {
  if (!cell || manager._metaLock) return;
  const field = typeof cell.getField === 'function' ? cell.getField() : '';
  const raw = typeof cell.getValue === 'function' ? cell.getValue() : '';
  const value = raw == null ? '' : String(raw);
  if (field === 'summary') {
    manager._paintSummary(value);
    return;
  }
  if (field !== 'name' || !manager.doc) return;
  if (definitionMeta(manager.doc).name === value) return;
  manager._commitDocument(withDefinitionMeta(manager.doc, { name: value }));
}

/**
 * Remove the selected report. A saved id calls QueryRef 170.
 * A draft is removed locally. Cancel leaves the row.
 * @param {object} manager
 */
async function deleteReportRow(manager) {
  const grid = manager.reportTable && manager.reportTable.table;
  const selected = grid && typeof grid.getSelectedRows === 'function'
    ? grid.getSelectedRows()
    : [];
  if (!selected.length) {
    toast.info('No Row Selected', { description: 'Please select a row to delete', duration: 3000 });
    return;
  }

  const row = selected[0];
  const data = typeof row.getData === 'function' ? row.getData() || {} : {};
  const name = typeof data.name === 'string' && data.name !== '' ? data.name : 'this report';
  const confirmed = await confirmDialog({
    title: 'Delete report',
    message: `Delete "${name}"?\n\nEvery revision of this report is removed.`,
    confirmLabel: 'Delete',
    danger: true,
  });
  if (!confirmed || manager._closed) return;

  const reportId = reportKey(data.report_id);
  try {
    await deleteListedReport(manager, data);
  } catch (error) {
    const message = error instanceof Error && error.message ? error.message : 'Delete failed.';
    log(Subsystems.CONDUIT, Status.ERROR, `[Reports] Delete failed: ${message}`);
    toast.error('Delete Failed', {
      description: error.serverError?.message || message,
      serverError: error.serverError,
      subsystem: 'Conduit',
    });
    return;
  }

  const rows = typeof grid.getRows === 'function' ? grid.getRows('active') : [];
  const index = rows.findIndex((entry) => entry === row);
  const nextRow = rows[index + 1] || rows[index - 1] || null;
  const clearsOpen = (reportId > 0 && manager._openReportId === reportId)
    || (reportId === 0 && !manager._openReportId);
  if (typeof row.delete === 'function') row.delete();
  if (nextRow && typeof nextRow.select === 'function') {
    nextRow.select();
    requestAnimationFrame(() => {
      nextRow.scrollTo?.('center', false);
    });
  }
  if (typeof manager.reportTable.exitEditMode === 'function') {
    await manager.reportTable.exitEditMode('cancel');
  }
  if (!nextRow && clearsOpen && typeof manager.newReport === 'function') manager.newReport();
  toast.success('Row Deleted', { duration: 3000 });
}
