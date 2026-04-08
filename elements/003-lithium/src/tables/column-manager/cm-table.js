/**
 * Column Manager - Table Initialization Module
 *
 * Inner LithiumTable setup and configuration.
 *
 * @module tables/column-manager/cm-table
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { LithiumTable } from '../lithium-table-main.js';
import { loadColtypes } from '../lithium-table.js';
import { buildColumnManagerDefinition } from './cm-columns.js';
import { handleCellEdit, handleRowReorder } from './cm-actions.js';
import { isDepth2ColumnManager } from './cm-ui.js';
import { restoreSetting } from './cm-state.js';

/**
 * Initialize the column manager as a LithiumTable
 * @param {Object} cm - ColumnManager instance
 */
export async function initColumnTable(cm) {
  const tableContainer = cm.popup.querySelector(`#${cm.cssPrefix}-table-container-${cm.managerId}`);
  const navigatorContainer = cm.popup.querySelector(`#${cm.cssPrefix}-navigator-${cm.managerId}`);

  if (!tableContainer || !navigatorContainer) {
    log(Subsystems.TABLE, Status.ERROR, '[ColumnManager] Container elements not found');
    return;
  }

  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Initializing table with ${cm.columnData.length} rows`);

  // Create column definition for the column manager table
  const columnDef = buildColumnManagerDefinition(cm);

  // Check if this is a depth-2 column manager
  const isDepth2 = isDepth2ColumnManager(cm);

  const userEditModeChange = () => {
    requestAnimationFrame(() => cm.updateButtonState());
  };
  const userDirtyChange = () => {
    requestAnimationFrame(() => cm.updateButtonState());
  };

  cm.columnTable = new LithiumTable({
    container: tableContainer,
    navigatorContainer: navigatorContainer,
    app: cm.app,
    tablePath: getManagerTablePath(cm),
    tableDef: columnDef,
    coltypes: await getColtypesForManager(cm),
    cssPrefix: 'lithium',
    storageKey: getManagerTableStorageKey(cm),
    readonly: false,
    alwaysEditable: false,
    useColumnManager: !isDepth2,
    onRowSelected: (rowData) => onRowSelected(cm, rowData),
    onRowDeselected: () => onRowDeselected(cm),
    onExecuteSave: () => cm.handleColumnTableExecuteSave(),
    onSetTableWidth: (mode) => cm.setTableWidth(mode),
  });

  await cm.columnTable.init();

  const previousEditModeChange = cm.columnTable.onEditModeChange;
  cm.columnTable.onEditModeChange = (...args) => {
    previousEditModeChange?.(...args);
    userEditModeChange(...args);
  };

  const previousDirtyChange = cm.columnTable.onDirtyChange;
  cm.columnTable.onDirtyChange = (...args) => {
    previousDirtyChange?.(...args);
    userDirtyChange(...args);
  };

  // Set the data using replaceData
  if (cm.columnData.length > 0 && cm.columnTable.table) {
    await cm.columnTable.table.replaceData(cm.columnData);
    log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Data set successfully');
  } else {
    log(Subsystems.TABLE, Status.WARN, '[ColumnManager] No data to set');
  }

  // Wire row reorder events
  wireRowReorderEvents(cm);
  configureNavigatorButtons(cm);

  // In manual mode, ensure no sorts are active
  if (cm.orderingMode === 'manual' && cm.columnTable?.table) {
    cm.columnTable.table.clearSort();
  }

  // Restore selected row
  restoreSelectedRow(cm);
  cm.syncDirtyState();
}

/**
 * Get storage key for manager table
 * @param {Object} cm - ColumnManager instance
 * @returns {string} Storage key
 */
function getManagerTableStorageKey(cm) {
  return isDepth2ColumnManager(cm)
    ? 'lithium_column_manager_manager'
    : 'lithium_column_manager';
}

/**
 * Get table path for manager table
 * @param {Object} cm - ColumnManager instance
 * @returns {string} Table path
 */
function getManagerTablePath(cm) {
  return isDepth2ColumnManager(cm)
    ? 'column-manager/column-manager-manager'
    : 'column-manager/column-manager';
}

/**
 * Get coltypes for the column manager table
 * @param {Object} cm - ColumnManager instance
 * @returns {Object} Coltypes
 */
export async function getColtypesForManager(cm) {
  try {
    return await loadColtypes();
  } catch (err) {
    log(Subsystems.TABLE, Status.WARN, `[ColumnManager] Failed to load coltypes: ${err.message}`);
    return getDefaultColtypes();
  }
}

/**
 * Get default coltypes when loading fails
 * @returns {Object} Default coltypes
 */
function getDefaultColtypes() {
  return {
    string: { description: 'Text data', align: 'left', editor: 'input', sorter: 'string' },
    integer: { description: 'Whole numbers', align: 'right', editor: 'number', sorter: 'number' },
    boolean: {
      description: 'True/false values',
      align: 'center',
      formatter: 'tickCross',
      formatterParams: { allowEmpty: false, allowTruthy: true },
      editor: 'tickCross',
      sorter: 'boolean',
    },
    lookupFixed: {
      description: 'Fixed dropdown list',
      align: 'left',
      editor: 'select',
      sorter: 'string',
      formatter: 'lookup',
    },
    stringList: {
      description: 'Editable text lookup from column values',
      align: 'left',
      editor: 'list',
      editorParams: {
        autocomplete: true,
        freetext: true,
        listOnEmpty: true,
        valuesLookup: 'column',
      },
      sorter: 'string',
      formatter: 'plaintext',
    },
  };
}

/**
 * Configure navigator buttons for column manager
 * @param {Object} cm - ColumnManager instance
 */
export function configureNavigatorButtons(cm) {
  if (!cm.columnTable?.navigatorContainer) return;

  const nav = cm.columnTable.navigatorContainer;
  const disableButton = (action, title) => {
    const btn = nav.querySelector(`#${cm.columnTable.cssPrefix}-nav-${action}`);
    if (!btn) return;
    btn.disabled = true;
    if (title) btn.title = title;
  };

  disableButton('add', 'Adding columns is not supported here');
  disableButton('duplicate', 'Duplicating columns is not supported here');
  disableButton('flag', 'Flag is not supported here');
  disableButton('annotate', 'Annotations are not supported here');
  disableButton('delete', 'Deleting columns is not supported here');

  cm.columnTable.updateDuplicateButtonState = () => {
    disableButton('duplicate', 'Duplicating columns is not supported here');
  };
  cm.columnTable.handleAdd = () => {};
  cm.columnTable.handleDuplicate = () => {};
  cm.columnTable.handleDelete = () => {};

  ['menu', 'width', 'layout', 'template'].forEach((action) => {
    const btn = nav.querySelector(`#${cm.columnTable.cssPrefix}-nav-${action}`);
    btn?.addEventListener('mousedown', (event) => {
      event?.stopPropagation?.();
    }, true);
  });
}

/**
 * Wire row reorder events
 * @param {Object} cm - ColumnManager instance
 */
export function wireRowReorderEvents(cm) {
  if (!cm.columnTable?.table) return;

  if (cm.orderingMode === 'manual') {
    const tableEl = cm.columnTable.container;
    if (tableEl) {
      tableEl.addEventListener('mousedown', (e) => {
        const cell = e.target.closest('.tabulator-cell[data-tabulator-field="drag_handle"]');
        if (!cell) return;

        if (!cm.columnTable?.isEditing) {
          e.preventDefault();
          e.stopPropagation();
          return;
        }

        if (cm.columnTable?.table) {
          cm.columnTable.table.clearSort();
        }
      }, true);
    }

    cm.columnTable.table.on('rowMoved', () => {
      handleRowReorder(cm);
    });
  }

  cm.columnTable.table.on('cellEdited', (cell) => {
    handleCellEdit(cm, cell);
  });

  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Events wired (${cm.orderingMode} mode)`);
}

/**
 * Handle row selection
 * @param {Object} cm - ColumnManager instance
 * @param {Object} rowData - Row data
 */
function onRowSelected(cm, rowData) {
  restoreSetting(cm, 'selectedRow', rowData.field_name);
  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Row selected: ${rowData.field_name}`);
}

/**
 * Handle row deselection
 * @param {Object} cm - ColumnManager instance
 */
function onRowDeselected(cm) {
  restoreSetting(cm, 'selectedRow', null);
}

/**
 * Restore selected row from storage
 * @param {Object} cm - ColumnManager instance
 */
function restoreSelectedRow(cm) {
  const savedField = restoreSetting(cm, 'selectedRow', null);
  if (savedField && cm.columnTable?.table) {
    const rows = cm.columnTable.table.getRows();
    const targetRow = rows.find((row) => row.getData().field_name === savedField);
    if (targetRow) {
      targetRow.select();
    }
  }
}

/**
 * Switch between Manual and Auto ordering modes
 * @param {Object} cm - ColumnManager instance
 * @param {'manual'|'auto'} mode - The ordering mode to switch to
 */
export async function setOrderingMode(cm, mode) {
  if (mode === cm.orderingMode) return;

  cm.orderingMode = mode;

  // Update mode toggle button states
  const modeBtns = cm.popup?.querySelectorAll(`.${cm.cssPrefix}-mode-btn`);
  modeBtns?.forEach((btn) => {
    btn.classList.toggle('active', btn.dataset.mode === mode);
  });

  // Save current data
  let currentData = cm.columnData;
  if (cm.columnTable?.table) {
    const rows = cm.columnTable.table.getRows();
    if (rows.length > 0) {
      currentData = rows.map((row) => row.getData());
    }
  }

  // Destroy existing table
  if (cm.columnTable) {
    cm.columnTable.destroy();
    cm.columnTable = null;
  }

  cm.columnData = currentData;

  const tableContainer = cm.popup.querySelector(`#${cm.cssPrefix}-table-container-${cm.managerId}`);
  const navigatorContainer = cm.popup.querySelector(`#${cm.cssPrefix}-navigator-${cm.managerId}`);

  if (!tableContainer || !navigatorContainer) return;

  tableContainer.innerHTML = '';
  navigatorContainer.innerHTML = '';

  await initColumnTable(cm);

  // Persist mode preference
  restoreSetting(cm, 'orderingMode', mode);

  log(Subsystems.TABLE, Status.INFO, `[ColumnManager] Switched to ${mode} ordering mode`);
}
