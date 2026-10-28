import { describe, it, expect, vi } from 'vitest';

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(async () => [{ key_idx: 7 }]),
}));

const { default: LookupsManager } = await import('../../../src/managers/lookups/lookups.js');

describe('LookupsManager._executeChildSave', () => {
  it('applies the server-generated key_idx on insert without throwing', async () => {
    const rowData = { lookup_id: 59, key_idx: null, value_txt: 'x' };
    const row = { getData: () => rowData, update: vi.fn() };
    const self = {
      app: { api: {} },
      childTable: { originalRowData: rowData, queryRefs: {} },
      _getJsonEditorContent: () => '{}',
      _getSummaryEditorContent: () => '',
      _buildLookupSaveParams: () => ({}),
    };
    await LookupsManager.prototype._executeChildSave.call(self, row);
    expect(row.update).toHaveBeenCalledWith({ key_idx: 7 });
  });
});
