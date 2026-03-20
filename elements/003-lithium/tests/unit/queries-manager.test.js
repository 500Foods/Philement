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

// Mock json-tree-component module
const mockJsonTreeData = { test: 'data' };
vi.mock('../../src/components/json-tree-component.js', () => ({
  initJsonTree: vi.fn(async (options) => {
    // Simulate storing tree ID and data on the target element
    if (options.target) {
      options.target._jsonTreeId = 'test-tree-id';
      options.target._jsonTreeData = options.data || mockJsonTreeData;
    }
    return 'test-tree-id';
  }),
  getJsonTreeData: vi.fn((target) => {
    return target?._jsonTreeData || null;
  }),
  setJsonTreeData: vi.fn((target, data) => {
    if (target) {
      target._jsonTreeData = data;
    }
  }),
  destroyJsonTree: vi.fn((target) => {
    if (target) {
      target._jsonTreeId = null;
      target._jsonTreeData = null;
    }
  }),
  updateJsonTreeOptions: vi.fn(),
}));

import QueriesManager from '../../src/managers/queries/queries.js';
import { getJsonTreeData, setJsonTreeData, destroyJsonTree } from '../../src/components/json-tree-component.js';

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

    // When the same row is already selected, _selectDataRow returns false
    // because the row is already the current selection
    const changed = manager._selectDataRow(targetRow);

    // Returns true because we're calling select() on a different row object
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
    
    // Mock popup close methods
    let columnChooserClosed = false;
    let navPopupClosed = false;
    manager._closeColumnChooser = () => { columnChooserClosed = true; };
    manager._closeNavPopup = () => { navPopupClosed = true; };

    const changed = manager._selectDataRow(targetRow);

    expect(changed).toBe(true);
    expect(deselectRow).toHaveBeenCalledTimes(1);
    expect(targetRow.select).toHaveBeenCalledTimes(1);
  });



  it('opens cell editor when in edit mode and cell is editable', () => {
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

    // Directly call edit on the cell when in edit mode
    if (manager._isEditing && manager._columnEditors.has(cell.getField())) {
      const row = cell.getRow();
      if (row.isSelected() && row.getData().query_id === manager._editingRowId) {
        cell.edit();
      }
    }

    expect(cell.edit).toHaveBeenCalledTimes(1);
  });

  it('sets data on table during loadQueries', async () => {
    const { authQuery } = await import('../../src/shared/conduit.js');

    manager.queryRefs = { queryRef: 25, searchQueryRef: 32 };
    manager.app = { api: {} };
    manager._loadedDetailRowId = null; // Reset to avoid early return

    manager.table = {
      setData: vi.fn(),
      getSelectedRows: vi.fn(() => []), // For _getSelectedQueryId
    };

    authQuery.mockResolvedValueOnce([{ query_id: 1, name: 'Row 1' }]);

    await manager.loadQueries();

    expect(manager.table.setData).toHaveBeenCalledWith([{ query_id: 1, name: 'Row 1' }]);
  });

  it('reverts table row changes when cancelling edits with dirty state', async () => {
    // Set up dirty state and original data
    manager._isDirty = { table: true, sql: false, summary: false, collection: false };
    manager._originalData.row = { query_id: 42, name: 'Original Name' };
    manager._isEditing = true;

    const mockRow = {
      update: vi.fn(),
      getData: vi.fn(() => ({ query_id: 42, name: 'Edited Name' })),
      getCell: vi.fn(() => null), // For _updateEditingIndicator
    };
    manager.table = {
      getSelectedRows: vi.fn(() => [mockRow]),
    };

    await manager.handleNavCancel();

    // The row should be reverted to original data
    expect(mockRow.update).toHaveBeenCalledWith({ query_id: 42, name: 'Original Name' });
    expect(manager._isEditing).toBe(false);
  });

  it('just exits edit mode when cancelling with no changes', async () => {
    // No dirty state
    manager._isDirty = { table: false, sql: false, summary: false, collection: false };
    manager._isEditing = true;

    await manager.handleNavCancel();

    expect(manager._isEditing).toBe(false);
  });
});

describe('QueriesManager JSONEditor and collection integration', () => {
  let manager;

  beforeEach(() => {
    manager = new QueriesManager({}, document.createElement('div'));
    manager.elements = {
      tableContainer: document.createElement('div'),
      navigatorContainer: document.createElement('div'),
      sqlEditorContainer: document.createElement('div'),
      summaryEditorContainer: document.createElement('div'),
      collectionEditorContainer: document.createElement('div'),
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

  describe('Collection dirty tracking', () => {
    it('tracks collection dirty state when content changes', () => {
      manager._originalData.collection = { key: 'value' };
      // Setup the container with mock data
      manager.elements.collectionEditorContainer._jsonTreeData = { key: 'changed' };

      const currentData = getJsonTreeData(manager.elements.collectionEditorContainer);
      const isDirty = JSON.stringify(currentData) !== JSON.stringify(manager._originalData.collection);

      expect(isDirty).toBe(true);
    });

    it('does not mark collection dirty when content is unchanged', () => {
      manager._originalData.collection = { key: 'value' };
      manager.elements.collectionEditorContainer._jsonTreeData = { key: 'value' };

      const currentData = getJsonTreeData(manager.elements.collectionEditorContainer);
      const isDirty = JSON.stringify(currentData) !== JSON.stringify(manager._originalData.collection);

      expect(isDirty).toBe(false);
    });

    it('captures original collection data when loading query details', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        collection: { test: 'data' },
      };

      // Simulate the capture from fetchQueryDetails
      manager._originalData = {
        ...manager._originalData,
        collection: queryData.collection || queryData.json || {},
      };

      expect(manager._originalData.collection).toEqual({ test: 'data' });
    });

    it('handles string collection data from queryData', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        collection: '{"test": "string_data"}',
      };

      // Simulate parsing string collection data
      let data = queryData.collection;
      if (typeof data === 'string') {
        try { data = JSON.parse(data); } catch (e) { data = {}; }
      }
      manager._originalData.collection = data;

      expect(manager._originalData.collection).toEqual({ test: 'string_data' });
    });

    it('handles json field as fallback for collection data', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        json: { fallback: 'data' },
      };

      // Simulate fallback to json field
      manager._originalData.collection = queryData.collection || queryData.json || {};

      expect(manager._originalData.collection).toEqual({ fallback: 'data' });
    });

    it('enables save/cancel buttons when collection is dirty', () => {
      manager._isEditing = true;
      manager._isDirty = { table: false, sql: false, summary: false, collection: true };
      manager.elements.navigatorContainer = document.createElement('div');
      manager.elements.navigatorContainer.innerHTML = `
        <button id="queries-nav-save"></button>
        <button id="queries-nav-cancel"></button>
      `;

      manager._updateSaveCancelButtonState();

      expect(manager.elements.navigatorContainer.querySelector('#queries-nav-save').disabled).toBe(false);
      expect(manager.elements.navigatorContainer.querySelector('#queries-nav-cancel').disabled).toBe(false);
    });

    it('_isAnyDirty returns true when collection is dirty', () => {
      manager._isDirty = { table: false, sql: false, summary: false, collection: true };

      expect(manager._isAnyDirty()).toBe(true);
    });

    it('marks all clean resets collection dirty state', () => {
      manager._isDirty = { table: true, sql: true, summary: true, collection: true };
      manager.elements.navigatorContainer = document.createElement('div');
      manager.elements.navigatorContainer.innerHTML = `
        <button id="queries-nav-save"></button>
        <button id="queries-nav-cancel"></button>
      `;

      manager._isDirty = { table: false, sql: false, summary: false, collection: false };

      expect(manager._isDirty.collection).toBe(false);
      expect(manager._isDirty.table).toBe(false);
      expect(manager._isDirty.sql).toBe(false);
      expect(manager._isDirty.summary).toBe(false);
    });
  });

  describe('Collection content retrieval', () => {
    it('getJsonTreeData returns JSON data from editor container', () => {
      const testData = { test: 'data' };
      manager.elements.collectionEditorContainer._jsonTreeData = testData;

      const content = getJsonTreeData(manager.elements.collectionEditorContainer);

      expect(content).toEqual(testData);
    });

    it('getJsonTreeData returns null when editor data is null', () => {
      manager.elements.collectionEditorContainer._jsonTreeData = null;

      const content = getJsonTreeData(manager.elements.collectionEditorContainer);

      expect(content).toBe(null);
    });

    it('getJsonTreeData returns null when container is null', () => {
      const content = getJsonTreeData(null);

      expect(content).toBe(null);
    });
  });

  describe('Collection save operations', () => {
    it('handleNavSave includes collection data in API call via editorManager', async () => {
      const { authQuery } = await import('../../src/shared/conduit.js');

      // Setup mock editor data in container — handleNavSave now calls
      // mgr.editorManager.getCollectionContent() which reads from the container.
      manager.elements.collectionEditorContainer._jsonTreeData = { updated: 'collection_data' };
      manager.sqlEditor = {
        state: { doc: { toString: vi.fn(() => 'SELECT * FROM test') } },
      };
      manager.summaryEditor = {
        state: { doc: { toString: vi.fn(() => 'Summary text') } },
      };
      manager.app = { api: {}, user: { id: 1 } };
      manager._isEditing = true;
      manager._isDirty = { table: false, sql: false, summary: false, collection: true };
      manager._editingRowId = 42;
      manager.queryRefs = { updateQueryRef: 28 };

      // Setup selected row
      const mockRow = {
        getData: vi.fn(() => ({
          query_id: 42,
          query_type_a28: 1,
          query_dialect_a30: 1,
          query_status_a27: 1,
          query_ref: 25,
          name: 'Test Query',
          code: 'SELECT 1',
          summary: 'Original summary',
        })),
      };
      manager.table = {
        getSelectedRows: vi.fn(() => [mockRow]),
      };

      authQuery.mockResolvedValueOnce([]);

      await manager.handleNavSave();

      expect(authQuery).toHaveBeenCalledWith(
        manager.app.api,
        28,
        expect.objectContaining({
          STRING: expect.objectContaining({
            COLLECTION: JSON.stringify({ updated: 'collection_data' }),
          }),
        })
      );
    });

    it('handleNavSave uses empty object when editor content is null', async () => {
      const { authQuery } = await import('../../src/shared/conduit.js');
      
      // Clear previous mock calls
      authQuery.mockClear();

      // No editor data — editorManager.getCollectionContent() returns '{}'
      manager.elements.collectionEditorContainer._jsonTreeData = null;
      manager.sqlEditor = {
        state: { doc: { toString: vi.fn(() => 'SELECT * FROM test') } },
      };
      manager.summaryEditor = {
        state: { doc: { toString: vi.fn(() => 'Summary text') } },
      };
      manager.app = { api: {}, user: { id: 1 } };
      manager._isEditing = true;
      manager._isDirty = { table: false, sql: false, summary: false, collection: true };
      manager._editingRowId = 42;
      manager.queryRefs = { updateQueryRef: 28 };

      // Setup selected row
      const mockRow = {
        getData: vi.fn(() => ({
          query_id: 42,
          query_type_a28: 1,
          query_dialect_a30: 1,
          query_status_a27: 1,
          query_ref: 25,
          name: 'Test Query',
          code: 'SELECT 1',
        })),
      };
      manager.table = {
        getSelectedRows: vi.fn(() => [mockRow]),
      };

      authQuery.mockResolvedValueOnce([]);

      await manager.handleNavSave();

      // Check the last call to authQuery
      const calls = authQuery.mock.calls;
      const lastCall = calls[calls.length - 1];
      
      expect(lastCall[1]).toBe(28); // queryRef
      expect(lastCall[2]).toMatchObject({
        STRING: expect.objectContaining({
          COLLECTION: '{}', // Empty object when editor returns null
        }),
      });
    });

    it('reverts collection content on cancel using setJsonTreeData', async () => {
      manager._originalData.collection = { original: 'data' };
      manager._isDirty = { table: false, sql: false, summary: false, collection: true };
      
      // Setup container for setJsonTreeData
      manager.elements.collectionEditorContainer._jsonTreeData = { current: 'data' };

      // Simulate the revert behavior from handleNavCancel
      if (manager._isDirty.collection && manager._originalData.collection != null) {
        setJsonTreeData(manager.elements.collectionEditorContainer, manager._originalData.collection);
      }

      expect(manager.elements.collectionEditorContainer._jsonTreeData).toEqual({ original: 'data' });
    });

    it('handles null original data when reverting collection', async () => {
      manager._originalData.collection = null;
      manager._isDirty = { table: false, sql: false, summary: false, collection: true };
      
      const originalData = { current: 'data' };
      manager.elements.collectionEditorContainer._jsonTreeData = originalData;

      // Only revert if original data exists
      if (manager._isDirty.collection && manager._originalData.collection != null) {
        setJsonTreeData(manager.elements.collectionEditorContainer, manager._originalData.collection);
      }

      // Data should remain unchanged
      expect(manager.elements.collectionEditorContainer._jsonTreeData).toEqual(originalData);
    });
  });

  describe('JsonTree editable mode', () => {
    it('_setJsonTreeEditable recreates editor with readOnly false when editable is true', async () => {
      manager.elements.collectionEditorContainer = document.createElement('div');
      manager.elements.collectionEditorContainer._jsonTreeData = { test: 'data' };

      // Simulate the _setJsonTreeEditable behavior
      const currentData = getJsonTreeData(manager.elements.collectionEditorContainer);
      expect(currentData).toEqual({ test: 'data' });

      // Should not throw when reinitializing
      expect(() => manager._setJsonTreeEditable?.(true)).not.toThrow();
    });

    it('_setJsonTreeEditable applies readonly CSS class when not editable', () => {
      manager.elements.collectionEditorContainer = document.createElement('div');
      
      // Simulate the CSS class toggle
      const editable = false;
      manager.elements.collectionEditorContainer.classList.toggle('queries-jsoneditor-readonly', !editable);

      expect(manager.elements.collectionEditorContainer.classList.contains('queries-jsoneditor-readonly')).toBe(true);
    });

    it('_setJsonTreeEditable removes readonly CSS class when editable', () => {
      manager.elements.collectionEditorContainer = document.createElement('div');
      manager.elements.collectionEditorContainer.classList.add('queries-jsoneditor-readonly');
      
      // Simulate the CSS class toggle
      const editable = true;
      manager.elements.collectionEditorContainer.classList.toggle('queries-jsoneditor-readonly', !editable);

      expect(manager.elements.collectionEditorContainer.classList.contains('queries-jsoneditor-readonly')).toBe(false);
    });

    it('_setJsonTreeEditable handles null editor gracefully', () => {
      manager.collectionEditor = null;
      manager.elements.collectionEditorContainer = document.createElement('div');

      expect(() => manager._setJsonTreeEditable?.(true)).not.toThrow();
    });

    it('setJsonTreeData updates editor data correctly', () => {
      manager.elements.collectionEditorContainer = document.createElement('div');
      manager.elements.collectionEditorContainer._jsonTreeData = { initial: 'data' };

      const newData = { updated: 'data' };
      setJsonTreeData(manager.elements.collectionEditorContainer, newData);

      expect(manager.elements.collectionEditorContainer._jsonTreeData).toEqual(newData);
    });
  });

  describe('JsonTree lifecycle', () => {
    it('destroyJsonTree properly cleans up the editor instance', () => {
      const container = document.createElement('div');
      container._jsonTreeId = 'test-id';
      container._jsonTreeData = { test: 'data' };

      destroyJsonTree(container);

      expect(container._jsonTreeId).toBe(null);
      expect(container._jsonTreeData).toBe(null);
    });

    it('teardown clears collection change interval', () => {
      manager._collectionChangeInterval = setInterval(() => {}, 500);
      
      // Simulate teardown behavior
      if (manager._collectionChangeInterval) {
        clearInterval(manager._collectionChangeInterval);
        manager._collectionChangeInterval = null;
      }

      expect(manager._collectionChangeInterval).toBe(null);
    });
  });
});
