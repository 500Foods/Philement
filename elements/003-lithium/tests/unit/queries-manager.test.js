import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';

vi.mock('tabulator-tables', () => ({
  TabulatorFull: class MockTabulator {},
}));

vi.mock('../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
}));

vi.mock('../../src/shared/toast.js', () => ({
  toast: {
    info: vi.fn(),
    success: vi.fn(),
    error: vi.fn(),
  },
}));

vi.mock('../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: {
    MANAGER: 'MANAGER',
    CONDUIT: 'CONDUIT',
  },
  Status: {
    INFO: 'INFO',
    WARN: 'WARN',
    ERROR: 'ERROR',
  },
}));

vi.mock('../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

vi.mock('../../src/core/lithium-table.js', () => ({
  loadColtypes: vi.fn(),
  loadTableDef: vi.fn(),
  resolveColumns: vi.fn(() => []),
  resolveTableOptions: vi.fn(() => ({})),
  getQueryRefs: vi.fn(() => ({})),
  getPrimaryKeyField: vi.fn(() => 'query_id'),
  preloadLookups: vi.fn(),
}));

import QueriesManager from '../../src/managers/queries/queries.js';

describe('QueriesManager edit-mode table behavior', () => {
  let manager;

  beforeEach(() => {
    manager = new QueriesManager({}, document.createElement('div'));
    manager.elements = {
      tableContainer: document.createElement('div'),
      navigatorContainer: document.createElement('div'),
      sqlEditorContainer: document.createElement('div'),
      summaryEditorContainer: document.createElement('div'),
    };
    manager.primaryKeyField = 'query_id';

    vi.stubGlobal('requestAnimationFrame', (callback) => {
      callback();
      return 1;
    });
  });

  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it('stores editor definitions separately and removes them from base columns', () => {
    const columns = [
      {
        field: 'name',
        editor: 'input',
        editorParams: { search: false },
      },
      {
        field: 'query_ref',
      },
    ];

    manager._applyEditModeGate(columns);

    expect(columns[0].editor).toBe('input');
    expect(columns[0].editorParams).toEqual({ search: false });
    expect(typeof columns[0].editable).toBe('function');
    expect(manager._columnEditors.get('name')).toEqual({
      editor: 'input',
      editorParams: { search: false },
    });
    expect(manager._columnEditors.has('query_ref')).toBe(false);
  });

  it('gates editable columns so only the row in edit mode can be edited', () => {
    const columns = [
      {
        field: 'name',
        editor: 'input',
        editorParams: { search: false },
      },
    ];
    manager._isEditing = true;
    manager._editingRowId = 42;

    manager._applyEditModeGate(columns);

    expect(columns[0].editable({
      getRow: () => ({
        getData: () => ({ query_id: 42 }),
        isSelected: () => true,
      }),
    })).toBe(true);
    expect(columns[0].editable({
      getRow: () => ({
        getData: () => ({ query_id: 7 }),
        isSelected: () => true,
      }),
    })).toBe(false);
  });

  it('disables editable columns outside edit mode without mutating column definitions', () => {
    const columns = [
      {
        field: 'query_timeout',
        editor: 'number',
      },
    ];
    manager._isEditing = false;
    manager._editingRowId = null;

    manager._applyEditModeGate(columns);

    expect(columns[0].editor).toBe('number');
    expect(columns[0].editable({
      getRow: () => ({
        getData: () => ({ query_id: 42 }),
        isSelected: () => true,
      }),
    })).toBe(false);
  });

  it('selects rows consistently on cell interaction and opens editors only in edit mode', () => {
    const handlers = {};
    const deselectRow = vi.fn();
    const selectedRow = {
      isSelected: vi.fn(() => false),
      getData: () => ({ query_id: 99 }),
    };
    manager.table = {
      on: vi.fn((eventName, handler) => {
        handlers[eventName] = handler;
      }),
      deselectRow,
      getSelectedRows: vi.fn(() => [selectedRow]),
    };
    manager._columnEditors = new Map([['name', { editor: 'input' }]]);
    manager._isEditing = true;
    manager._editingRowId = 5;

    manager._wireTableEvents();

    const row = {
      isSelected: vi.fn(() => false),
      select: vi.fn(),
      getData: () => ({ query_id: 5 }),
    };
    const cell = {
      getField: () => 'name',
      getRow: () => row,
      edit: vi.fn(),
    };

    handlers.cellMouseDown({}, cell);
    expect(deselectRow).toHaveBeenCalledTimes(1);
    expect(row.select).toHaveBeenCalledTimes(1);

    const stopPropagation = vi.fn();
    const stopImmediatePropagation = vi.fn();
    handlers.cellClick({ stopPropagation, stopImmediatePropagation }, cell);
    expect(deselectRow).toHaveBeenCalledTimes(2);
    expect(row.select).toHaveBeenCalledTimes(2);
    expect(cell.edit).toHaveBeenCalledTimes(1);
    expect(stopPropagation).toHaveBeenCalledTimes(1);
    expect(stopImmediatePropagation).toHaveBeenCalledTimes(1);
  });

  it('does not open inline editors when edit mode is inactive', () => {
    const handlers = {};
    manager.table = {
      on: vi.fn((eventName, handler) => {
        handlers[eventName] = handler;
      }),
      deselectRow: vi.fn(),
    };
    manager._columnEditors = new Map([['name', { editor: 'input' }]]);
    manager._isEditing = false;
    manager._editingRowId = null;

    manager._wireTableEvents();

    const row = {
      isSelected: vi.fn(() => true),
      select: vi.fn(),
      getData: () => ({ query_id: 5 }),
    };
    const cell = {
      getField: () => 'name',
      getRow: () => row,
      edit: vi.fn(),
    };

    handlers.cellClick({}, cell);

    expect(cell.edit).not.toHaveBeenCalled();
  });

  it('does not refetch details when the selected row is unchanged', () => {
    manager._loadedDetailRowId = 5;
    manager.fetchQueryDetails = vi.fn();

    manager.loadQueryDetails({ query_id: 5, name: 'Same Row' });

    expect(manager.fetchQueryDetails).not.toHaveBeenCalled();
  });

  it('does not churn selection when the same logical row is already selected', () => {
    const deselectRow = vi.fn();
    const selectedRow = {
      isSelected: vi.fn(() => true),
      getData: () => ({ query_id: 5 }),
    };
    const targetRow = {
      isSelected: vi.fn(() => false),
      select: vi.fn(),
      getData: () => ({ query_id: 5 }),
    };

    manager.table = {
      deselectRow,
      getSelectedRows: vi.fn(() => [selectedRow]),
    };

    const changed = manager._selectDataRow(targetRow);

    expect(changed).toBe(false);
    expect(deselectRow).not.toHaveBeenCalled();
    expect(targetRow.select).toHaveBeenCalledTimes(1);
  });

  it('closes transient popups before changing to a different row', () => {
    const deselectRow = vi.fn();
    const selectedRow = {
      isSelected: vi.fn(() => true),
      getData: () => ({ query_id: 5 }),
    };
    const targetRow = {
      isSelected: vi.fn(() => false),
      select: vi.fn(),
      getData: () => ({ query_id: 6 }),
    };

    manager.table = {
      deselectRow,
      getSelectedRows: vi.fn(() => [selectedRow]),
    };
    manager._closeColumnChooser = vi.fn();
    manager._closeNavPopup = vi.fn();
    manager._closeFooterExportPopup = vi.fn();
    manager.hideFontPopup = vi.fn();

    const changed = manager._selectDataRow(targetRow);

    expect(changed).toBe(true);
    expect(manager._closeColumnChooser).toHaveBeenCalledTimes(1);
    expect(manager._closeNavPopup).toHaveBeenCalledTimes(1);
    expect(manager._closeFooterExportPopup).toHaveBeenCalledTimes(1);
    expect(manager.hideFontPopup).toHaveBeenCalledTimes(1);
    expect(deselectRow).toHaveBeenCalledTimes(1);
    expect(targetRow.select).toHaveBeenCalledTimes(1);
  });

  it('ignores footer calc cells for selection and edit activation', () => {
    const handlers = {};
    manager.table = {
      on: vi.fn((eventName, handler) => {
        handlers[eventName] = handler;
      }),
      deselectRow: vi.fn(),
      getSelectedRows: vi.fn(() => []),
    };
    manager._columnEditors = new Map([['query_ref', { editor: 'input' }]]);
    manager._isEditing = true;
    manager._editingRowId = 5;

    manager._wireTableEvents();

    const row = {
      select: vi.fn(),
      getData: () => ({ query_id: 5 }),
      getElement: () => ({
        closest: (selector) => selector.includes('.tabulator-calcs-bottom') ? {} : null,
      }),
    };
    const cell = {
      getField: () => 'query_ref',
      getRow: () => row,
      getElement: () => ({
        closest: (selector) => selector.includes('.tabulator-calcs-bottom') ? {} : null,
      }),
      edit: vi.fn(),
    };

    handlers.cellMouseDown({}, cell);
    handlers.cellClick({ stopPropagation: vi.fn(), stopImmediatePropagation: vi.fn() }, cell);
    handlers.rowClick({}, row);

    expect(manager.table.deselectRow).not.toHaveBeenCalled();
    expect(row.select).not.toHaveBeenCalled();
    expect(cell.edit).not.toHaveBeenCalled();
  });

  it('does not reload details or clear them during same-row edit interactions', () => {
    const handlers = {};
    manager.table = {
      on: vi.fn((eventName, handler) => {
        handlers[eventName] = handler;
      }),
      getSelectedRows: vi.fn(() => []),
    };
    manager._isEditing = true;
    manager._editingRowId = 5;
    manager.loadQueryDetails = vi.fn();
    manager.clearQueryDetails = vi.fn();

    manager._wireTableEvents();

    handlers.rowSelectionChanged([{ query_id: 5, name: 'Row 5' }], []);
    handlers.rowSelectionChanged([], []);

    expect(manager.loadQueryDetails).not.toHaveBeenCalled();
    expect(manager.clearQueryDetails).not.toHaveBeenCalled();
  });

  it('reopens the requested cell editor after same-row edit handoff settles', () => {
    manager._isEditing = true;
    manager._editingRowId = 5;
    manager._columnEditors = new Map([['query_timeout', { editor: 'number' }]]);

    const cell = {
      getField: () => 'query_timeout',
      getRow: () => ({
        isSelected: () => true,
        getData: () => ({ query_id: 5 }),
      }),
      edit: vi.fn(),
    };

    manager._queueCellEdit(cell);

    expect(cell.edit).toHaveBeenCalledTimes(1);
  });

  it('blocks and restores redraw around bulk row updates during loadQueries', async () => {
    const { authQuery } = await import('../../src/shared/conduit.js');

    manager.queryRefs = { queryRef: 25, searchQueryRef: 32 };
    manager._getSelectedQueryId = vi.fn(() => null);
    manager._showTableLoading = vi.fn();
    manager._hideTableLoading = vi.fn();
    manager._discoverColumns = vi.fn();
    manager._autoSelectRow = vi.fn();
    manager.app = { api: {} };

    manager.table = {
      blockRedraw: vi.fn(),
      restoreRedraw: vi.fn(),
      setData: vi.fn(),
    };

    authQuery.mockResolvedValueOnce([{ query_id: 1, name: 'Row 1' }]);

    await manager.loadQueries();

    expect(manager.table.blockRedraw).toHaveBeenCalledTimes(1);
    expect(manager.table.setData).toHaveBeenCalledWith([{ query_id: 1, name: 'Row 1' }]);
    expect(manager.table.restoreRedraw).toHaveBeenCalledTimes(1);
  });

  it('reverts changes using _revertAllChanges when cancelling edits with dirty state', async () => {
    const revertAllChanges = vi.fn(() => Promise.resolve());
    const exitEditMode = vi.fn(() => Promise.resolve());

    // Set up dirty state so cancel will revert
    manager._isDirty = { table: true, sql: false, summary: false, collection: false };
    manager._revertAllChanges = revertAllChanges;
    manager._exitEditMode = exitEditMode;

    await manager.handleNavCancel();

    expect(revertAllChanges).toHaveBeenCalledTimes(1);
    expect(exitEditMode).toHaveBeenCalledWith('cancel');
  });

  it('just exits edit mode when cancelling with no changes', async () => {
    const revertAllChanges = vi.fn(() => Promise.resolve());
    const exitEditMode = vi.fn(() => Promise.resolve());

    // No dirty state
    manager._isDirty = { table: false, sql: false, summary: false, collection: false };
    manager._revertAllChanges = revertAllChanges;
    manager._exitEditMode = exitEditMode;

    await manager.handleNavCancel();

    expect(revertAllChanges).not.toHaveBeenCalled();
    expect(exitEditMode).toHaveBeenCalledWith('cancel');
  });
});
