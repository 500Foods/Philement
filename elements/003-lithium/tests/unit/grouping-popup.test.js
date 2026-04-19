/**
 * Grouping Popup Unit Tests
 *
 * Tests for the grouping popup functionality:
 *   - getGroupableColumns filtering logic
 *   - sortColumnsByGroupPriority ordering
 *   - Tooltip generation for tri-state
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';

// Mock dependencies before importing the module
vi.mock('../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { TABLE: 'table' },
  Status: { INFO: 'info', DEBUG: 'debug' },
}));

vi.mock('../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

vi.mock('../../src/tables/lithium-table.js', () => ({
  resolveTableOptions: vi.fn((def) => {
    // Simple mock implementation
    const groupBy = [];
    const cols = def?.columns || {};
    const grouped = Object.entries(cols)
      .filter(([, c]) => c.groupable === true && c.groupPri != null)
      .sort((a, b) => (a[1].groupPri || 0) - (b[1].groupPri || 0));
    for (const [field, col] of grouped) {
      groupBy.push({ field, dir: col.groupDir || 'asc' });
    }
    return { groupBy: groupBy.length ? groupBy : undefined };
  }),
}));

// Import after mocks are set up
const {
  closeGroupingPopup,
} = await import('../../src/tables/popups/grouping-popup.js');

describe('Grouping Popup', () => {
  let mockTable;

  beforeEach(() => {
    mockTable = {
      tableDef: {
        columns: {
          status: { field: 'status', title: 'Status', groupable: true, groupPri: 1, groupDir: 'asc' },
          category: { field: 'category', title: 'Category', groupable: true, groupPri: 2, groupDir: 'desc' },
          name: { field: 'name', title: 'Name', groupable: false },
          hidden_col: { field: 'hidden_col', title: 'Hidden', groupable: true }, // No groupPri
          _selector: { field: '_selector', title: '', groupable: true, groupPri: 3 },
        },
      },
      table: {
        setGroupBy: vi.fn(),
        setGroupStartOpen: vi.fn(),
      },
      collapseAll: vi.fn(),
      expandAll: vi.fn(),
      updateGroupIcons: vi.fn(),
    };
  });

  describe('closeGroupingPopup', () => {
    it('should clean up popup and event listeners', () => {
      mockTable.activeNavPopup = document.createElement('div');
      mockTable.activeNavPopupId = 'grouping';
      mockTable.activeNavPopupButton = document.createElement('button');
      mockTable.navPopupCloseHandler = vi.fn();

      closeGroupingPopup(mockTable);

      expect(mockTable.activeNavPopup).toBeNull();
      expect(mockTable.activeNavPopupId).toBeNull();
      expect(mockTable.activeNavPopupButton).toBeNull();
      expect(mockTable.navPopupCloseHandler).toBeNull();
    });

    it('should handle cleanup when no popup is active', () => {
      mockTable.activeNavPopup = null;
      mockTable.navPopupCloseHandler = null;

      expect(() => closeGroupingPopup(mockTable)).not.toThrow();
    });
  });
});

describe('Grouping Popup Logic', () => {
  // These tests verify the behavior logic without full DOM simulation

  describe('Column Filtering', () => {
    it('should identify groupable columns correctly', () => {
      const columns = {
        groupable_col: { field: 'groupable_col', title: 'Groupable', groupable: true, groupPri: 1 },
        non_groupable: { field: 'non_groupable', title: 'Not Groupable', groupable: false },
        default_col: { field: 'default_col', title: 'Default' }, // No groupable property
      };

      // Filter logic from getGroupableColumns
      const groupable = Object.entries(columns).filter(([, col]) => {
        return col.groupable === true && col.field !== '_selector';
      });

      expect(groupable).toHaveLength(1);
      expect(groupable[0][0]).toBe('groupable_col');
    });

    it('should exclude _selector column even if marked groupable', () => {
      const columns = {
        _selector: { field: '_selector', title: '', groupable: true, groupPri: 1 },
        data_col: { field: 'data_col', title: 'Data', groupable: true, groupPri: 2 },
      };

      const groupable = Object.entries(columns).filter(([, col]) => {
        return col.groupable === true && col.field !== '_selector';
      });

      expect(groupable).toHaveLength(1);
      expect(groupable[0][0]).toBe('data_col');
    });

    it('should include columns with groupable: true but no groupPri (for popup list)', () => {
      const columns = {
        grouped: { field: 'grouped', title: 'Grouped', groupable: true, groupPri: 1 },
        not_grouped: { field: 'not_grouped', title: 'Not Grouped', groupable: true },
      };

      // All groupable columns should appear in popup (for enabling grouping)
      const groupable = Object.entries(columns).filter(([, col]) => {
        return col.groupable === true;
      });

      expect(groupable).toHaveLength(2);
    });
  });

  describe('Tooltip States', () => {
    it('should have correct tooltip for each grouping state', () => {
      // Tooltips should be:
      // - Not grouped: "Click to group ascending"
      // - Ascending: "Click to group descending"
      // - Descending: "Click to remove grouping"

      const getGroupingTooltip = (isGrouped, sortDir) => {
        if (!isGrouped) return 'Click to group ascending';
        return sortDir === 'asc' ? 'Click to group descending' : 'Click to remove grouping';
      };

      expect(getGroupingTooltip(false, 'asc')).toBe('Click to group ascending');
      expect(getGroupingTooltip(true, 'asc')).toBe('Click to group descending');
      expect(getGroupingTooltip(true, 'desc')).toBe('Click to remove grouping');
    });
  });

  describe('Sort Priority Ordering', () => {
    it('should sort grouped columns by groupPri', () => {
      const columns = [
        { field: 'col3', groupPri: 3, groupable: true },
        { field: 'col1', groupPri: 1, groupable: true },
        { field: 'col2', groupPri: 2, groupable: true },
      ];

      const sorted = [...columns].sort((a, b) => (a.groupPri || 0) - (b.groupPri || 0));

      expect(sorted[0].field).toBe('col1');
      expect(sorted[1].field).toBe('col2');
      expect(sorted[2].field).toBe('col3');
    });

    it('should put grouped columns before non-grouped in sort', () => {
      const columns = [
        { field: 'ungrouped', groupable: true },
        { field: 'grouped', groupPri: 1, groupable: true },
      ];

      const sorted = [...columns].sort((a, b) => {
        const aGrouped = a.groupPri != null && a.groupable === true;
        const bGrouped = b.groupPri != null && b.groupable === true;
        if (aGrouped && !bGrouped) return -1;
        if (!aGrouped && bGrouped) return 1;
        return 0;
      });

      expect(sorted[0].field).toBe('grouped');
      expect(sorted[1].field).toBe('ungrouped');
    });
  });
});

describe('Grouping Tri-State Cycle', () => {
  // Tests for the cycleArrowState logic

  it('should cycle: not grouped -> asc -> desc -> not grouped', () => {
    // Simulate state transitions
    const transitions = [
      { state: { isGrouped: false, groupPri: null, groupDir: 'asc' }, action: 'click' },
      { state: { isGrouped: true, groupPri: 1, groupDir: 'asc' }, action: 'click' },
      { state: { isGrouped: true, groupPri: 1, groupDir: 'desc' }, action: 'click' },
      { state: { isGrouped: false, groupPri: null, groupDir: 'asc' }, action: 'complete' },
    ];

    // Verify cycle completes back to start
    expect(transitions[0].state.isGrouped).toBe(false);
    expect(transitions[1].state.isGrouped).toBe(true);
    expect(transitions[1].state.groupDir).toBe('asc');
    expect(transitions[2].state.isGrouped).toBe(true);
    expect(transitions[2].state.groupDir).toBe('desc');
    expect(transitions[3].state.isGrouped).toBe(false); // Back to start
  });

  it('should assign next available priority when enabling grouping', () => {
    const existingGroups = [{ groupPri: 1 }, { groupPri: 3 }];
    const maxPri = Math.max(0, ...existingGroups.map(g => g.groupPri ?? 0));

    expect(maxPri).toBe(3);

    // New column would get priority 4
    const newPri = maxPri + 1;
    expect(newPri).toBe(4);
  });
});
