/**
 * LithiumColumnManager Unit Tests
 *
 * Tests the column manager functionality:
 *   - Initialization
 *   - Data loading
 *   - Column definition building
 *   - Persistence
 */

/* globals MouseEvent */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import { LithiumColumnManager } from '../../src/tables/lithium-column-manager.js';
import { LithiumTable } from '../../src/tables/lithium-table-main.js';

// Mock the LithiumTable module
vi.mock('../../src/tables/lithium-table-main.js', () => ({
  LithiumTable: vi.fn(function LithiumTableMock() {
    return {
      init: vi.fn().mockResolvedValue(),
      setData: vi.fn(),
      getColumns: vi.fn().mockReturnValue([]),
      destroy: vi.fn(),
      cssPrefix: 'lithium',
      navigatorContainer: document.createElement('div'),
      onEditModeChange: vi.fn(),
      onDirtyChange: vi.fn(),
      table: {
        getColumns: vi.fn().mockReturnValue([]),
        getRows: vi.fn().mockReturnValue([]),
        redraw: vi.fn(),
        replaceData: vi.fn().mockResolvedValue(),
        clearSort: vi.fn(),
        on: vi.fn(),
      },
    };
  }),
}));

// Mock the icons module
vi.mock('../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

// Mock the log module
vi.mock('../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { TABLE: 'TABLE' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR', DEBUG: 'DEBUG' },
}));

describe('LithiumColumnManager', () => {
  let mockParentTable;
  let mockParentContainer;
  let mockAnchorElement;
  let mockApp;

  beforeEach(() => {
    if (typeof localStorage?.clear === 'function') {
      localStorage.clear();
    } else if (localStorage && typeof localStorage === 'object' && 'store' in localStorage) {
      localStorage.store = {};
    }

    mockParentTable = {
      table: {
        getColumns: vi.fn().mockReturnValue([
          {
            getField: () => 'id',
            getDefinition: () => ({ title: 'ID', coltype: 'integer', width: 80 }),
            isVisible: () => true,
            getElement: () => ({ offsetWidth: 80 }),
          },
          {
            getField: () => 'name',
            getDefinition: () => ({ title: 'Name', coltype: 'string', width: 150 }),
            isVisible: () => true,
            getElement: () => ({ offsetWidth: 150 }),
          },
        ]),
        getColumn: vi.fn().mockReturnValue({
          getDefinition: () => ({ title: 'Test', coltype: 'string' }),
          show: vi.fn(),
          hide: vi.fn(),
          updateDefinition: vi.fn(),
        }),
        redraw: vi.fn(),
      },
    };

    mockParentContainer = document.createElement('div');
    mockAnchorElement = document.createElement('button');
    mockApp = { api: {} };
  });

  describe('constructor', () => {
    it('should initialize with provided options', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        anchorElement: mockAnchorElement,
        parentTable: mockParentTable,
        app: mockApp,
        managerId: 'test-manager',
      });

      expect(manager.parentContainer).toBe(mockParentContainer);
      expect(manager.anchorElement).toBe(mockAnchorElement);
      expect(manager.parentTable).toBe(mockParentTable);
      expect(manager.app).toBe(mockApp);
      expect(manager.managerId).toBe('test-manager');
    });

    it('should use default values for optional parameters', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      expect(manager.managerId).toBe('default');
      expect(manager.cssPrefix).toBe('col-manager');
    });
  });

  describe('loadColumnData', () => {
    it('should load column data from parent table', async () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'test',
      });

      await manager.loadColumnData();

      expect(manager.columnData).toHaveLength(2);
      expect(manager.columnData[0].field_name).toBe('id');
      expect(manager.columnData[0].width).toBe(80);
      expect(manager.columnData[1].field_name).toBe('name');
      expect(manager.columnData[1].width).toBe(150);
    });

    it('should handle missing parent table gracefully', async () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: { table: null },
        managerId: 'test',
      });

      await manager.loadColumnData();

      expect(manager.columnData).toHaveLength(0);
    });
  });

  describe('buildColumnManagerDefinition', () => {
    it('should return valid column definition', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'test',
      });

      const def = manager.buildColumnManagerDefinition();

      expect(def).toHaveProperty('title', 'Column Manager');
      expect(def).toHaveProperty('columns');
      expect(def.columns).toHaveProperty('order');
      expect(def.columns).toHaveProperty('visible');
      expect(def.columns).toHaveProperty('field_name');
      expect(def.columns).toHaveProperty('column_name');
      expect(def.columns).toHaveProperty('format');
      expect(def.columns).toHaveProperty('summary');
      expect(def.columns).toHaveProperty('alignment');
      expect(def.columns).toHaveProperty('width');
      // Manual mode (default) should have drag_handle and movableRows
      expect(def.columns).toHaveProperty('drag_handle');
      expect(def.movableRows).toBe(true);
    });

    it('marks the order column editable only in auto mode', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'test',
      });

      const manualDef = manager.buildColumnManagerDefinition();
      expect(manualDef.columns.order.editable).toBe(false);

      manager.orderingMode = 'auto';
      const autoDef = manager.buildColumnManagerDefinition();
      expect(autoDef.columns.order.editable).toBe(true);
    });
  });

  describe('initColumnTable', () => {
    it('creates a standard LithiumTable with shared template identity', async () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'test-column-manager',
        app: mockApp,
      });

      manager.popup = document.createElement('div');
      manager.popup.innerHTML = `
        <div id="${manager.cssPrefix}-table-container-${manager.managerId}"></div>
        <div id="${manager.cssPrefix}-navigator-${manager.managerId}"></div>
      `;

      manager.columnData = [{ field_name: 'name', field: 'name', order: 1 }];

      await manager.initColumnTable();

      expect(LithiumTable).toHaveBeenCalledWith(expect.objectContaining({
        alwaysEditable: false,
        tablePath: 'column-manager/column-manager',
        storageKey: 'lithium_column_manager',
        useColumnManager: true,
      }));
    });

    it('uses the shared manager-manager identity for depth-2 instances', async () => {
      const popupHost = document.createElement('div');
      popupHost.className = 'col-manager-popup';
      popupHost.appendChild(mockParentContainer);
      document.body.appendChild(popupHost);

      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: {
          ...mockParentTable,
          container: mockParentContainer,
        },
        managerId: 'nested-manager',
        app: mockApp,
      });

      manager.popup = document.createElement('div');
      manager.popup.innerHTML = `
        <div id="${manager.cssPrefix}-table-container-${manager.managerId}"></div>
        <div id="${manager.cssPrefix}-navigator-${manager.managerId}"></div>
      `;

      manager.columnData = [{ field_name: 'name', field: 'name', order: 1 }];

      await manager.initColumnTable();

      expect(LithiumTable).toHaveBeenCalledWith(expect.objectContaining({
        tablePath: 'column-manager/column-manager-manager',
        storageKey: 'lithium_column_manager_manager',
        useColumnManager: false,
      }));

      popupHost.remove();
    });

    it('preserves manager edit callbacks while updating popup button state', async () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'callback-manager',
        app: mockApp,
      });

      manager.popup = document.createElement('div');
      manager.popup.innerHTML = `
        <div id="${manager.cssPrefix}-table-container-${manager.managerId}"></div>
        <div id="${manager.cssPrefix}-navigator-${manager.managerId}"></div>
      `;

      manager.columnData = [{ field_name: 'name', field: 'name', order: 1 }];

      await manager.initColumnTable();

      const tableInstance = LithiumTable.mock.results.at(-1).value;
      expect(typeof tableInstance.onEditModeChange).toBe('function');
      expect(typeof tableInstance.onDirtyChange).toBe('function');
    });

    it('does not block navigator click handlers for popup buttons', async () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'nav-manager',
        app: mockApp,
      });

      manager.popup = document.createElement('div');
      manager.popup.innerHTML = `
        <div id="${manager.cssPrefix}-table-container-${manager.managerId}"></div>
        <div id="${manager.cssPrefix}-navigator-${manager.managerId}"></div>
      `;

      manager.columnData = [{ field_name: 'name', field: 'name', order: 1 }];

      await manager.initColumnTable();

      const tableInstance = LithiumTable.mock.results.at(-1).value;
      tableInstance.navigatorContainer.innerHTML = '<button id="lithium-nav-width"></button>';

      manager.configureNavigatorButtons();

      const widthBtn = tableInstance.navigatorContainer.querySelector('#lithium-nav-width');
      const clickSpy = vi.fn();
      widthBtn.addEventListener('click', clickSpy);

      widthBtn.dispatchEvent(new MouseEvent('click', { bubbles: true }));

      expect(clickSpy).toHaveBeenCalledTimes(1);
    });
  });

  describe('getFormatOptions', () => {
    it('should return format options', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      const options = manager.getFormatOptions();

      expect(options).toBeInstanceOf(Array);
      expect(options.length).toBeGreaterThan(0);
      expect(options[0]).toHaveProperty('value');
      expect(options[0]).toHaveProperty('label');
    });
  });

  describe('getSummaryOptions', () => {
    it('should return summary options', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      const options = manager.getSummaryOptions();

      expect(options).toBeInstanceOf(Array);
      expect(options.length).toBeGreaterThan(0);
    });
  });

  describe('getAlignmentOptions', () => {
    it('should return alignment options', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      const options = manager.getAlignmentOptions();

      expect(options).toEqual([
        { value: 'left', label: 'Left' },
        { value: 'center', label: 'Center' },
        { value: 'right', label: 'Right' },
      ]);
    });
  });

  describe('getColumnGroup', () => {
    it('should categorize ID fields as Identifiers', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      expect(manager.getColumnGroup('user_id')).toBe('Identifiers');
      expect(manager.getColumnGroup('ID')).toBe('Identifiers');
    });

    it('should categorize name fields as Names', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      expect(manager.getColumnGroup('first_name')).toBe('Names');
      expect(manager.getColumnGroup('fullName')).toBe('Names');
    });

    it('should categorize date fields as Dates', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      expect(manager.getColumnGroup('created_date')).toBe('Dates');
      expect(manager.getColumnGroup('updated_time')).toBe('Dates');
    });

    it('should categorize other fields as General', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
      });

      expect(manager.getColumnGroup('email')).toBe('General');
      expect(manager.getColumnGroup('description')).toBe('General');
    });
  });

  describe('persistence', () => {
    it('should save and restore settings', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'test-persistence',
      });

      // Mock localStorage
      const localStorageMock = {
        store: {},
        getItem: vi.fn(key => localStorageMock.store[key] || null),
        setItem: vi.fn((key, value) => {
          localStorageMock.store[key] = value;
        }),
      };
      Object.defineProperty(global, 'localStorage', {
        value: localStorageMock,
        writable: true,
      });

      manager.saveSetting('testKey', 'testValue');
      const restored = manager.restoreSetting('testKey', 'defaultValue');

      expect(localStorageMock.setItem).toHaveBeenCalled();
      expect(restored).toBe('testValue');
    });

    it('should return default value when setting not found', () => {
      const manager = new LithiumColumnManager({
        parentContainer: mockParentContainer,
        parentTable: mockParentTable,
        managerId: 'test-default',
      });

      const localStorageMock = {
        store: {},
        getItem: vi.fn(() => null),
        setItem: vi.fn(),
      };
      Object.defineProperty(global, 'localStorage', {
        value: localStorageMock,
        writable: true,
      });

      const restored = manager.restoreSetting('nonexistent', 'default');

      expect(restored).toBe('default');
    });
  });
});
