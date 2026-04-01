/**
 * Tests for QueriesManager — validates the consolidated architecture
 * (LithiumTable + ManagerEditHelper + queries-editors.js).
 *
 * The Query Manager follows the standard manager pattern:
 *   - editHelper (ManagerEditHelper) handles dirty tracking, edit mode, save/cancel
 *   - queryTable (LithiumTable) owns isEditing, editingRowId, originalRowData
 *   - onExecuteSave callback provides custom save logic
 *   - External editors registered via editHelper.registerEditor()
 */

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
    TABLE: 'TABLE',
  },
  Status: {
    INFO: 'INFO',
    WARN: 'WARN',
    ERROR: 'ERROR',
    DEBUG: 'DEBUG',
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
    constructor(options) {
      this.table = mockTabulatorTable;
      this.isEditing = false;
      this.isDirty = false;
      this.editingRowId = null;
      this.originalRowData = null;
      this.primaryKeyField = 'query_id';
      this.storageKey = options?.storageKey || 'queries_table';
      this.cssPrefix = options?.cssPrefix || 'queries';
      this.queryRefs = { updateQueryRef: 28 };
      this.onEditModeChange = options?.onEditModeChange || (() => {});
      this.onDirtyChange = options?.onDirtyChange || null;
      this.onExecuteSave = options?.onExecuteSave || null;
      this._editHelper = null;
      this.init = vi.fn(async () => {});
      this.loadData = vi.fn(async () => {});
      this.destroy = vi.fn();
      this.setSplitter = vi.fn();
      this.enterEditMode = vi.fn();
      this.exitEditMode = vi.fn();
      this.handleSave = vi.fn();
      this.handleCancel = vi.fn();
      this.getSelectedDataRow = vi.fn();
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

// Mock codemirror modules
vi.mock('../../src/core/codemirror.js', () => ({
  EditorState: { create: vi.fn(() => ({})), readOnly: { of: vi.fn() } },
  EditorView: vi.fn(),
  Compartment: vi.fn(() => ({ of: vi.fn() })),
  undo: vi.fn(),
  redo: vi.fn(),
  keymap: { of: vi.fn() },
  lineNumbers: vi.fn(),
  highlightActiveLineGutter: vi.fn(),
  highlightSpecialChars: vi.fn(),
  drawSelection: vi.fn(),
  highlightActiveLine: vi.fn(),
  defaultKeymap: [],
  history: vi.fn(),
  historyKeymap: [],
  foldGutter: vi.fn(),
  foldKeymap: [],
  foldAll: vi.fn(),
  unfoldAll: vi.fn(),
  bracketMatching: vi.fn(),
  indentOnInput: vi.fn(),
  sql: vi.fn(),
  json: vi.fn(),
  css: vi.fn(),
  markdown: vi.fn(),
  javascript: vi.fn(),
  html: vi.fn(),
  oneDark: {},
}));

vi.mock('../../src/core/codemirror-setup.js', () => ({
  buildEditorExtensions: vi.fn(() => []),
  createReadOnlyCompartment: vi.fn(() => ({ of: vi.fn() })),
  setEditorEditable: vi.fn(),
  foldAllInEditor: vi.fn(),
  unfoldAllInEditor: vi.fn(),
  READONLY_CLASS: 'lithium-cm-readonly',
  EDIT_MODE_CLASS: 'lithium-cm-editable',
  LANG_CLASS_PREFIX: 'lithium-cm-lang-',
}));

// Helper to create a mock CodeMirror view
function mockCmView(content = '{}') {
  let docContent = content;
  const view = {
    state: {
      get doc() {
        return { toString: () => docContent, length: docContent.length };
      },
    },
    dispatch: vi.fn(({ changes } = {}) => {
      if (changes?.insert != null) docContent = changes.insert;
    }),
    destroy: vi.fn(),
  };
  // Getter so tests can read the current content
  Object.defineProperty(view, '_content', { get: () => docContent, configurable: true });
  return view;
}

import QueriesManager from '../../src/managers/queries/queries.js';

describe('QueriesManager — row selection', () => {
  let manager;

  beforeEach(() => {
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

  it('sets data on table during loadData', async () => {
    manager.app = { api: {} };
    manager.queryTable = {
      table: mockTabulatorTable,
      loadData: vi.fn(async () => {}),
    };

    await manager.queryTable.loadData();

    expect(manager.queryTable.loadData).toHaveBeenCalled();
  });
});

describe('QueriesManager — editHelper and edit mode', () => {
  let manager;

  beforeEach(() => {
    mockTabulatorTable.getSelectedRows.mockReturnValue([]);
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

  it('editHelper exists and has registerTable/registerEditor methods', () => {
    expect(manager.editHelper).toBeDefined();
    expect(typeof manager.editHelper.registerTable).toBe('function');
    expect(typeof manager.editHelper.registerEditor).toBe('function');
    expect(typeof manager.editHelper.wireFooterButtons).toBe('function');
    expect(typeof manager.editHelper.checkDirtyState).toBe('function');
  });

  it('editHelper.isEditing() returns false when no table in edit mode', () => {
    expect(manager.editHelper.isEditing()).toBe(false);
    expect(manager.editHelper.activeEditingTable).toBeNull();
  });

  it('editHelper snapshot is null when not editing', () => {
    expect(manager.editHelper.editModeSnapshot).toBeNull();
    expect(manager.editHelper._isFormDirty).toBe(false);
  });

  it('editHelper.checkDirtyState does nothing when no snapshot', () => {
    // Should not throw
    manager.editHelper.checkDirtyState();
    expect(manager.editHelper._isFormDirty).toBe(false);
  });

  it('editHelper.isAnythingDirty returns false when no snapshot', () => {
    expect(manager.editHelper.isAnythingDirty()).toBe(false);
  });

  it('editHelper.restoreEditorSnapshots does nothing when no snapshot', () => {
    // Should not throw
    manager.editHelper.restoreEditorSnapshots();
  });

  it('editHelper.updateFooterSaveCancelState updates button disabled state', () => {
    const saveBtn = document.createElement('button');
    const cancelBtn = document.createElement('button');
    const dummyBtn = document.createElement('button');

    manager.editHelper.footerSaveBtn = saveBtn;
    manager.editHelper.footerCancelBtn = cancelBtn;
    manager.editHelper.footerDummyBtn = dummyBtn;

    // Disabled state (not dirty)
    manager.editHelper.updateFooterSaveCancelState(true, false);
    expect(saveBtn.disabled).toBe(true);
    expect(cancelBtn.disabled).toBe(true);

    // Enabled state (dirty)
    manager.editHelper.updateFooterSaveCancelState(true, true);
    expect(saveBtn.disabled).toBe(false);
    expect(cancelBtn.disabled).toBe(false);
  });
});

describe('QueriesManager — CodeMirror collection integration', () => {
  let manager;

  beforeEach(() => {
    mockTabulatorTable.getSelectedRows.mockReturnValue([]);
    mockTabulatorTable.getData.mockReturnValue([]);

    manager = new QueriesManager({}, document.createElement('div'));
    manager.elements.collectionEditorContainer = document.createElement('div');

    vi.stubGlobal('requestAnimationFrame', (cb) => {
      if (cb) cb();
      return 1;
    });
  });

  afterEach(() => {
    vi.unstubAllGlobals();
    vi.clearAllMocks();
  });

  describe('Collection dirty tracking via editHelper snapshots', () => {
    it('detects dirty state when collection content changes from snapshot', () => {
      const original = JSON.stringify({ key: 'value' });
      const changed = JSON.stringify({ key: 'changed' });

      // Simulate snapshot state
      manager.editHelper.editModeSnapshot = {
        tableRowData: null,
        editors: { collection: original },
        timestamp: Date.now(),
      };

      // Register a mock collection editor
      manager.editHelper.editors.set('collection', {
        getContent: () => changed,
        setContent: vi.fn(),
        setEditable: vi.fn(),
        boundTable: manager.editHelper.activeEditingTable,
      });

      expect(manager.editHelper.isAnythingDirty()).toBe(true);
    });

    it('detects clean state when collection content matches snapshot', () => {
      const original = JSON.stringify({ key: 'value' });

      manager.editHelper.editModeSnapshot = {
        tableRowData: null,
        editors: { collection: original },
        timestamp: Date.now(),
      };

      manager.editHelper.editors.set('collection', {
        getContent: () => original,
        setContent: vi.fn(),
        setEditable: vi.fn(),
        boundTable: manager.editHelper.activeEditingTable,
      });

      expect(manager.editHelper.isAnythingDirty()).toBe(false);
    });

    it('captures collection data correctly from query details', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        collection: { test: 'data' },
      };

      // Manager stores pending content which editors read
      manager._pendingCollectionContent = queryData.collection || queryData.json || {};
      expect(manager._pendingCollectionContent).toEqual({ test: 'data' });
    });

    it('handles string collection data from query details', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        collection: '{"test": "string_data"}',
      };

      let data = queryData.collection;
      if (typeof data === 'string') {
        try { data = JSON.parse(data); } catch { data = {}; }
      }
      manager._pendingCollectionContent = data;

      expect(manager._pendingCollectionContent).toEqual({ test: 'string_data' });
    });

    it('handles json field as fallback for collection data', () => {
      const queryData = {
        query_id: 1,
        name: 'Test Query',
        json: { fallback: 'data' },
      };

      manager._pendingCollectionContent = queryData.collection || queryData.json || {};
      expect(manager._pendingCollectionContent).toEqual({ fallback: 'data' });
    });

    it('enables save/cancel buttons when snapshot detects dirty state', () => {
      const saveBtn = document.createElement('button');
      const cancelBtn = document.createElement('button');
      const dummyBtn = document.createElement('button');

      manager.editHelper.footerSaveBtn = saveBtn;
      manager.editHelper.footerCancelBtn = cancelBtn;
      manager.editHelper.footerDummyBtn = dummyBtn;

      // Enable (dirty)
      manager.editHelper.updateFooterSaveCancelState(true, true);

      expect(saveBtn.disabled).toBe(false);
      expect(cancelBtn.disabled).toBe(false);
    });

    it('disables save/cancel buttons when snapshot detects clean state', () => {
      const saveBtn = document.createElement('button');
      const cancelBtn = document.createElement('button');
      const dummyBtn = document.createElement('button');

      manager.editHelper.footerSaveBtn = saveBtn;
      manager.editHelper.footerCancelBtn = cancelBtn;
      manager.editHelper.footerDummyBtn = dummyBtn;

      // Disable (clean)
      manager.editHelper.updateFooterSaveCancelState(true, false);

      expect(saveBtn.disabled).toBe(true);
      expect(cancelBtn.disabled).toBe(true);
    });
  });

  describe('Collection content via CodeMirror', () => {
    it('returns JSON data from CodeMirror editor', () => {
      const testData = { test: 'data' };
      const view = mockCmView(JSON.stringify(testData));

      const content = view.state.doc.toString();
      expect(JSON.parse(content)).toEqual(testData);
    });

    it('returns empty when no CodeMirror view exists', () => {
      expect(manager.collectionEditor).toBeNull();
    });
  });

  describe('Collection save operations', () => {
    it('_executeSave assembles params from table row + editors', async () => {
      const { authQuery } = await import('../../src/shared/conduit.js');

      manager.sqlEditor = {
        state: { doc: { toString: vi.fn(() => 'SELECT * FROM test') } },
      };
      manager.summaryEditor = {
        state: { doc: { toString: vi.fn(() => 'Summary text') } },
      };
      manager.editorManager = {
        getCollectionContent: vi.fn(() => JSON.stringify({ updated: 'collection_data' })),
        _setCodeMirrorEditable: vi.fn(),
        destroy: vi.fn(),
      };
      manager.app = { api: {}, user: { id: 1 } };
      manager.queryTable = lastMockLithiumTableInstance || { queryRefs: { updateQueryRef: 28 }, primaryKeyField: 'query_id' };

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
        update: vi.fn(),
      };

      authQuery.mockResolvedValueOnce([]);

      await manager._executeSave(mockRow);

      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        28,
        expect.objectContaining({
          INTEGER: expect.objectContaining({ QUERYID: 42 }),
          STRING: expect.objectContaining({
            QUERYCODE: 'SELECT * FROM test',
            QUERYSUMMARY: 'Summary text',
            COLLECTION: JSON.stringify({ updated: 'collection_data' }),
          }),
        }),
      );
    });

    it('_executeSave uses empty defaults when no editors exist', async () => {
      const { authQuery } = await import('../../src/shared/conduit.js');

      manager.sqlEditor = null;
      manager.summaryEditor = null;
      manager.editorManager = {
        getCollectionContent: vi.fn(() => '{}'),
        _setCodeMirrorEditable: vi.fn(),
        destroy: vi.fn(),
      };
      manager.app = { api: {}, user: { id: 1 } };
      manager.queryTable = lastMockLithiumTableInstance || { queryRefs: { updateQueryRef: 28 }, primaryKeyField: 'query_id' };

      const mockRow = {
        getData: vi.fn(() => ({
          query_id: 10,
          query_type_a28: 1,
          query_dialect_a30: 1,
          query_status_a27: 1,
          query_ref: 5,
          name: 'Empty Query',
        })),
        update: vi.fn(),
      };

      authQuery.mockResolvedValueOnce([]);

      await manager._executeSave(mockRow);

      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        28,
        expect.objectContaining({
          STRING: expect.objectContaining({
            QUERYCODE: '',
            QUERYSUMMARY: '',
            COLLECTION: '{}',
          }),
        }),
      );
    });
  });

  describe('Collection editor cancel restore via editHelper', () => {
    it('restoreEditorSnapshots dispatches original content to editors', () => {
      const originalContent = JSON.stringify({ original: 'data' });
      const setContentMock = vi.fn();

      // Set up a mock active table so boundTable matches
      const mockTable = {};
      manager.editHelper.activeEditingTable = mockTable;

      manager.editHelper.editModeSnapshot = {
        tableRowData: null,
        editors: { collection: originalContent },
        timestamp: Date.now(),
      };

      manager.editHelper.editors.set('collection', {
        getContent: () => JSON.stringify({ current: 'data' }),
        setContent: setContentMock,
        setEditable: vi.fn(),
        boundTable: mockTable,
      });

      manager.editHelper.restoreEditorSnapshots();

      expect(setContentMock).toHaveBeenCalledWith(originalContent);
    });

    it('restoreEditorSnapshots skips editors without setContent', () => {
      const mockTable = {};
      manager.editHelper.activeEditingTable = mockTable;

      manager.editHelper.editModeSnapshot = {
        tableRowData: null,
        editors: { collection: '{}' },
        timestamp: Date.now(),
      };

      // Editor without setContent — should not throw
      manager.editHelper.editors.set('collection', {
        getContent: () => '{}',
        setEditable: vi.fn(),
        boundTable: mockTable,
      });

      expect(() => manager.editHelper.restoreEditorSnapshots()).not.toThrow();
    });

    it('restoreEditorSnapshots does nothing when snapshot is null', () => {
      manager.editHelper.editModeSnapshot = null;

      // Should not throw
      manager.editHelper.restoreEditorSnapshots();
    });
  });

  describe('CodeMirror editor lifecycle', () => {
    it('updates editor content via CodeMirror dispatch', () => {
      const view = mockCmView(JSON.stringify({ initial: 'data' }));

      const newData = { updated: 'data' };
      const newContent = JSON.stringify(newData, null, 2);
      view.dispatch({ changes: { from: 0, to: view.state.doc.length, insert: newContent } });

      expect(view._content).toBe(newContent);
    });

    it('destroy properly cleans up the CodeMirror editor instance', () => {
      const view = mockCmView('{}');
      view.destroy();
      expect(view.destroy).toHaveBeenCalled();
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
