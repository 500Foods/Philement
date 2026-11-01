/**
 * Report list for Manager 24.
 * The left pane is a LithiumTable of the reports table. QueryRefs
 * for that list are Phase 17 and Phase 18. Until they exist, save,
 * duplicate, and delete of a row fail visibly. Import and export
 * remain the way a definition leaves the browser.
 */

import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { setupManagerFooterIcons } from '../../core/manager-ui.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';
import { LithiumTable } from '../../tables/lithium-table-main.js';
import { definitionMeta, rowSummary, withDefinitionMeta } from './reports-meta.js';
import { definitionText } from './reports-file.js';

const LIST_KEY = 'lithium_reports_left';
const LIST_WIDTH = 360;

const COLTYPES = {
  default: {},
  string: { sorter: 'string', editor: 'input', editorParams: { search: false } },
  index: { sorter: 'number', hozAlign: 'right' },
  text: { sorter: 'string', editor: 'textarea' },
};

const ACTION_LABEL = {
  save: 'Save',
  duplicate: 'Duplicate',
  delete: 'Delete',
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
      rev_id: column('rev_id', 'Rev', 'index', { primaryKey: true }),
      name: column('name', 'Name', 'string', { visible: true, editable: true }),
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
    message: `${label} is not available until the reports table query is applied.`,
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
    onExecuteSave: async () => {
      throw reportListUnavailable('save');
    },
    onDuplicate: async () => {
      throw reportListUnavailable('duplicate');
    },
    onRowSelected: (row) => openListedReport(manager, row),
  });
  manager.reportTable = table;
  manager.editHelper.registerTable(table);
  manager.editHelper.registerEditor('definition', {
    getContent: () => (manager.doc ? definitionText(manager.doc) : ''),
    setContent: (content) => manager._importText(content),
    setEditable: () => {},
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
  table.handleDelete = async () => {
    const error = reportListUnavailable('delete');
    toast.error(error.serverError.error, {
      description: error.serverError.message,
      subsystem: 'Manager',
    });
  };

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
 * Open a listed definition. A row with no definition leaves the
 * designer on the document it already has.
 * @param {object} manager
 * @param {object} row
 */
function openListedReport(manager, row) {
  const stored = row && row.report;
  if (stored != null && stored !== '') {
    const text = typeof stored === 'string' ? stored : JSON.stringify(stored);
    manager._importText(text);
  }
  manager._paintSummary(rowSummary(row));
}

/**
 * A name edit on the list is the same definition name.
 * Summary stays on the row.
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
