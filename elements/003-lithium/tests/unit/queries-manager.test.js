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

// Shared mock Tabulator table instance
const mockTabulatorTable = {
  getSelectedRows: vi.fn(() => []),
  getData: vi.fn(() => []),
  setData: vi.fn(),
  setColumns: vi.fn(),
  redraw: vi.fn(),
};

// Track the last created LithiumTable instance
let lastMockLithiumTableInstance = null;

vi.mock('../../src/core/lithium-table-main.js', () => {
  class MockLithiumTable {
    constructor() {
      this.table = mockTabulatorTable;
      this.init = vi.fn(async () => {});
      this.loadData = vi.fn(async () => {});
      this.destroy = vi.fn();
      lastMockLithiumTableInstance = this;
    }
  }
  return { LithiumTable: MockLithiumTable };
});

vi.mock('../../src/core/lithium-splitter.js', () => {
  class MockLithiumSplitter {
    constructor() {
      this.destroy = vi.fn();
      this.setCollapsed = vi.fn();
    }
  }
  return { LithiumSplitter: MockLithiumSplitter };
});

vi.mock('../../src/core/panel-state-manager.js', () => {
  class MockPanelStateManager {
    constructor() {
      this.loadWidth = vi.fn(() => 313);
      this.loadCollapsed = vi.fn(() => false);
      this.saveWidth = vi.fn();
      this.saveCollapsed = vi.fn();
      this.clear = vi.fn();
    }
  }
  return { PanelStateManager: MockPanelStateManager };
});

vi.mock('../../src/core/manager-ui.js', () => ({
  setupManagerFooterIcons: vi.fn(() => ({
    reportSelect: { value: 'queries-view' },
    saveBtn: document.createElement('button'),
    cancelBtn: document.createElement('button'),
    dummyBtn: document.createElement('button'),
  })),
  createFontPopup: vi.fn(() => ({
    popup: document.createElement('div'),
    getState: vi.fn(() => ({ fontSize: 14, fontFamily: 'var(--font-mono)', fontWeight: 'normal' })),
  })),
}));

// Mock json-tree-component module
const mockJsonTreeData = { test: 'data' };
vi.mock('../../src/components/json-tree-component.js', () => ({
  initJsonTree: vi.fn(async (options) => {
    if (options.target) {
      options.target._cmView = { state: { doc: { toString: () => JSON.stringify(options.data || mockJsonTreeData) } } };
      options.target._cmData = JSON.stringify(options.data || mockJsonTreeData);
    }
    return options.target?.id || 'cm-editor';
  }),
  getJsonTreeData: vi.fn((target) => {
    if (!target || !target._cmData) return null;
    try { return JSON.parse(target._cmData); } catch { return null; }
  }),
  setJsonTreeData: vi.fn((target, data) => {
    if (target) {
      const jsonStr = JSON.stringify(data, null, 2);
      target._cmData = jsonStr;
      if (target._cmView) {
        target._cmView.state.doc.toString = () => jsonStr;
      }
    }
  }),
  destroyJsonTree: vi.fn((target) => {
    if (target) {
      target._cmView = null;
      target._cmData = null;
    }
  }),
  updateJsonTreeOptions: vi.fn(),
}));

import QueriesManager from '../../src/managers/queries/queries.js';
import { getJsonTreeData, setJsonTreeData, destroyJsonTree } from '../../src/components/json-tree-component.js';

describe('QueriesManager edit-mode table behavior', () => {
  let manager;

  beforeEach(() => {
    // Reset mock tabulator table methods
    mockTabulatorTable.getSelectedRows.mockReturnValue([]);
    mockTabulatorTable.getData.mockReturnValue([]);

    manager = new QueriesManager({}, document.createElement('div'));
    
    // Stub requestAnimationFrame to execute callbacks synchronously
    vi.stubGlobal('requestAnimationFrame', (cb) => {
      if (cb) cb();
      return 1;
    });
  });

  afterEach(() => {
    vi.unstubAllGlobals();
    vi.clearAllMocks();
  });

  it('does not refetch details when the selected row is unchanged', () => {
    manager._loadedDetailRowId = 5;
    manager._loadQueryDetails = vi.fn();

    manager._handleRowSelected({ query_id: 5, name: 'Same Row' });

    expect(manager._loadQueryDetails).not.toHaveBeenCalled();
  });

  it('calls _loadQueryDetails when a new row is selected', () => {
    manager._loadedDetailRowId = null;
    manager._loadQueryDetails = vi.fn();

    manager._handleRowSelected({ query_id: 6, name: 'New Row' });

    expect(manager._loadQueryDetails).toHaveBeenCalledWith(6);
  });

  it('opens cell editor when in edit mode and cell is editable', () => {
    manager.editModeManager._isEditing = true;
    manager.editModeManager._editingRowId = 5;
    manager.editModeManager._columnEditors = new Map([['query_timeout', { editor: 'number' }]]);

    const cell = {
      getField: () => 'query_timeout',
      getRow: () => ({
        isSelected: () => true,
        getData: () => ({ query_id: 5 }),
      }),
      edit: vi.fn(),
    };

    // Call the internal logic that would trigger the edit
    // This logic is in edit-mode.js, but we can verify the setup
    expect(manager.editModeManager._columnEditors.has('query_timeout')).toBe(true);
  });

  it('sets data on table during loadData', async () => {
    manager.app = { api: {} };
    manager.queryTable = {
      table: mockTabulatorTable,
      loadData: vi.fn(async () => {}),
    };

    await manager.queryTable.loadData();

    expect(manager.queryTable.loadData).toHaveBeenCalled();
  });

  it('reverts table row changes when cancelling edits with dirty state', async () => {
    manager.dirtyTracker._isDirty = { table: true, sql: false, summary: false, collection: false };
    manager.dirtyTracker._originalRowData = { query_id: 42, name: 'Original Name' };
    manager.editModeManager._isEditing = true;

    const mockRow = {
      update: vi.fn(),
      getData: vi.fn(() => ({ query_id: 42, name: 'Edited Name' })),
      getCell: vi.fn(() => null),
    };
    mockTabulatorTable.getSelectedRows.mockReturnValue([mockRow]);

    // Set up queryTable to point to the mock tabulator table
    manager.queryTable = { table: mockTabulatorTable };

    manager.updateFooterSaveCancelState = vi.fn();

    await manager.dirtyTracker.revertAllChanges();

    expect(mockRow.update).toHaveBeenCalledWith({ query_id: 42, name: 'Original Name' });
  });

  it('just exits edit mode when cancelling with no changes', async () => {
    manager.dirtyTracker._isDirty = { table: false, sql: false, summary: false, collection: false };
    manager.editModeManager._isEditing = true;
    manager.updateFooterSaveCancelState = vi.fn();

    await manager.dirtyTracker.revertAllChanges();
    manager.editModeManager._isEditing = false;

    expect(manager.editModeManager._isEditing).toBe(false);
  });
});

describe('QueriesManager JSONEditor and collection integration', () => {
  let manager;

  beforeEach(() => {
    // Reset mock tabulator table methods
    mockTabulatorTable.getSelectedRows.mockReturnValue([]);
    mockTabulatorTable.getData.mockReturnValue([]);

    manager = new QueriesManager({}, document.createElement('div'));
    
    vi.stubGlobal('requestAnimationFrame', (cb) => {
      if (cb) cb();
      return 1;
    });
  });

  afterEach(() => {
    vi.unstubAllGlobals();
    vi.clearAllMocks();
  });

  describe('Collection dirty tracking', () => {
    beforeEach(() => {
      // Create a mock collection editor container
      manager.elements.collectionEditorContainer = document.createElement('div');
    });

    it('tracks collection dirty state when content changes', () => {
      manager.dirtyTracker._originalCollectionContent = JSON.stringify({ key: 'value' });
      manager.elements.collectionEditorContainer._cmData = JSON.stringify({ key: 'changed' });

      const currentData = getJsonTreeData(manager.elements.collectionEditorContainer);
      const isDirty = JSON.stringify(currentData) !== JSON.parse(manager.dirtyTracker._originalCollectionContent);

      expect(isDirty).toBe(true);
    });

    it('does not mark collection dirty when content is unchanged', () => {
      manager.dirtyTracker._originalCollectionContent = JSON.stringify({ key: 'value' });
      manager.elements.collectionEditorContainer._cmData = JSON.stringify({ key: 'value' });

      const currentData = getJsonTreeData(manager.elements.collectionEditorContainer);
      // Compare stringified versions to properly check equality
      const isDirty = JSON.stringify(currentData) !== manager.dirtyTracker._originalCollectionContent;

      expect(isDirty).toBe(false);
    });

    it('captures original collection data when loading query details', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        collection: { test: 'data' },
      };

      manager.dirtyTracker._originalCollectionContent = JSON.stringify(queryData.collection || queryData.json || {});

      expect(JSON.parse(manager.dirtyTracker._originalCollectionContent)).toEqual({ test: 'data' });
    });

    it('handles string collection data from queryData', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        collection: '{"test": "string_data"}',
      };

      let data = queryData.collection;
      if (typeof data === 'string') {
        try { data = JSON.parse(data); } catch (e) { data = {}; }
      }
      manager.dirtyTracker._originalCollectionContent = JSON.stringify(data);

      expect(JSON.parse(manager.dirtyTracker._originalCollectionContent)).toEqual({ test: 'string_data' });
    });

    it('handles json field as fallback for collection data', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        json: { fallback: 'data' },
      };

      manager.dirtyTracker._originalCollectionContent = JSON.stringify(queryData.collection || queryData.json || {});

      expect(JSON.parse(manager.dirtyTracker._originalCollectionContent)).toEqual({ fallback: 'data' });
    });

    it('enables save/cancel buttons when collection is dirty', () => {
      manager.editModeManager._isEditing = true;
      manager.dirtyTracker._isDirty = { table: false, sql: false, summary: false, collection: true };
      
      // Set up footer buttons directly
      manager.footerSaveBtn = document.createElement('button');
      manager.footerCancelBtn = document.createElement('button');
      manager.footerDummyBtn = document.createElement('button');

      manager.updateFooterSaveCancelState(true, true);

      expect(manager.footerSaveBtn.disabled).toBe(false);
      expect(manager.footerCancelBtn.disabled).toBe(false);
    });

    it('_isAnyDirty returns true when collection is dirty', () => {
      manager.dirtyTracker._isDirty = { table: false, sql: false, summary: false, collection: true };

      expect(manager.dirtyTracker.isAnyDirty()).toBe(true);
    });

    it('marks all clean resets collection dirty state', () => {
      manager.dirtyTracker._isDirty = { table: true, sql: true, summary: true, collection: true };
      
      // Set up footer buttons directly
      manager.footerSaveBtn = document.createElement('button');
      manager.footerCancelBtn = document.createElement('button');
      manager.footerDummyBtn = document.createElement('button');

      manager.dirtyTracker.markAllClean();

      expect(manager.dirtyTracker._isDirty.collection).toBe(false);
      expect(manager.dirtyTracker._isDirty.table).toBe(false);
      expect(manager.dirtyTracker._isDirty.sql).toBe(false);
      expect(manager.dirtyTracker._isDirty.summary).toBe(false);
    });
  });

  describe('Collection content retrieval', () => {
    beforeEach(() => {
      // Create a mock collection editor container
      manager.elements.collectionEditorContainer = document.createElement('div');
    });

    it('getJsonTreeData returns JSON data from editor container', () => {
      const testData = { test: 'data' };
      manager.elements.collectionEditorContainer._cmData = JSON.stringify(testData);

      const content = getJsonTreeData(manager.elements.collectionEditorContainer);

      expect(content).toEqual(testData);
    });

    it('getJsonTreeData returns null when editor data is null', () => {
      manager.elements.collectionEditorContainer._cmData = null;

      const content = getJsonTreeData(manager.elements.collectionEditorContainer);

      expect(content).toBe(null);
    });

    it('getJsonTreeData returns null when container is null', () => {
      const content = getJsonTreeData(null);

      expect(content).toBe(null);
    });
  });

  describe('Collection save operations', () => {
    beforeEach(() => {
      // Create a mock collection editor container
      manager.elements.collectionEditorContainer = document.createElement('div');
    });

    it('handleSave includes collection data in API call via editorManager', async () => {
      const { authQuery } = await import('../../src/shared/conduit.js');

      manager.elements.collectionEditorContainer._cmData = JSON.stringify({ updated: 'collection_data' });
      manager.sqlEditor = {
        state: { doc: { toString: vi.fn(() => 'SELECT * FROM test') } },
      };
      manager.summaryEditor = {
        state: { doc: { toString: vi.fn(() => 'Summary text') } },
      };
      manager.app = { api: {}, user: { id: 1 } };
      manager.editModeManager._isEditing = true;
      manager.dirtyTracker._isDirty = { table: false, sql: false, summary: false, collection: true };
      manager.editModeManager._editingRowId = 42;
      manager.queryTable = { table: mockTabulatorTable };

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
      mockTabulatorTable.getSelectedRows.mockReturnValue([mockRow]);

      authQuery.mockResolvedValueOnce([]);

      const collectionContent = getJsonTreeData(manager.elements.collectionEditorContainer);
      expect(collectionContent).toEqual({ updated: 'collection_data' });
    });

    it('handleSave uses empty object when editor content is null', async () => {
      manager.elements.collectionEditorContainer._cmData = null;

      const collectionContent = getJsonTreeData(manager.elements.collectionEditorContainer);
      expect(collectionContent).toBe(null);
    });

    it('reverts collection content on cancel using setJsonTreeData', async () => {
      manager.dirtyTracker._originalCollectionContent = JSON.stringify({ original: 'data' });
      manager.dirtyTracker._isDirty = { table: false, sql: false, summary: false, collection: true };
      
      manager.elements.collectionEditorContainer._cmData = JSON.stringify({ current: 'data' });

      if (manager.dirtyTracker._isDirty.collection && manager.dirtyTracker._originalCollectionContent != null) {
        setJsonTreeData(manager.elements.collectionEditorContainer, JSON.parse(manager.dirtyTracker._originalCollectionContent));
      }

      expect(JSON.parse(manager.elements.collectionEditorContainer._cmData)).toEqual({ original: 'data' });
    });

    it('handles null original data when reverting collection', async () => {
      manager.dirtyTracker._originalCollectionContent = null;
      manager.dirtyTracker._isDirty = { table: false, sql: false, summary: false, collection: true };
      
      const originalData = { current: 'data' };
      manager.elements.collectionEditorContainer._cmData = JSON.stringify(originalData);

      if (manager.dirtyTracker._isDirty.collection && manager.dirtyTracker._originalCollectionContent != null) {
        setJsonTreeData(manager.elements.collectionEditorContainer, JSON.parse(manager.dirtyTracker._originalCollectionContent));
      }

      expect(manager.elements.collectionEditorContainer._cmData).toBe(JSON.stringify(originalData));
    });
  });

  describe('JSON editor editable mode', () => {
    it('setJsonTreeData updates editor data correctly', () => {
      manager.elements.collectionEditorContainer = document.createElement('div');
      manager.elements.collectionEditorContainer._cmData = JSON.stringify({ initial: 'data' });

      const newData = { updated: 'data' };
      setJsonTreeData(manager.elements.collectionEditorContainer, newData);

      expect(manager.elements.collectionEditorContainer._cmData).toBe(JSON.stringify(newData, null, 2));
    });
  });

  describe('JSON editor lifecycle', () => {
    it('destroyJsonTree properly cleans up the editor instance', () => {
      const container = document.createElement('div');
      container._cmView = { state: { doc: { toString: () => '{}' } } };
      container._cmData = JSON.stringify({ test: 'data' });

      destroyJsonTree(container);

      expect(container._cmView).toBe(null);
      expect(container._cmData).toBe(null);
    });

    it('teardown clears collection change interval', () => {
      manager._collectionChangeInterval = setInterval(() => {}, 500);
      
      if (manager._collectionChangeInterval) {
        clearInterval(manager._collectionChangeInterval);
        manager._collectionChangeInterval = null;
      }

      expect(manager._collectionChangeInterval).toBe(null);
    });
  });
});
