import { describe, it, expect } from 'vitest';
import { reportAuditRow } from '../../src/managers/reports/reports-table.js';

describe('reportAuditRow', () => {
  it('returns null for empty input', () => {
    expect(reportAuditRow(null)).toBeNull();
    expect(reportAuditRow(undefined)).toBeNull();
  });

  it('falls back to created_* when updated_id is 0', () => {
    const row = reportAuditRow({
      report_id: 3, created_id: 7, created_at: '2026-11-03 10:00:00', updated_id: 0, updated_at: null,
    });
    expect(row.updated_id).toBe(7);
    expect(row.updated_at).toBe('2026-11-03 10:00:00');
    expect(row.created_by).toBe(7);
    expect(row.report_id).toBe(3);
  });

  it('uses updated_* when set', () => {
    const row = reportAuditRow({
      created_id: 7, created_at: '2026-11-01 10:00:00', updated_id: 9, updated_at: '2026-11-03 11:00:00',
    });
    expect(row.updated_id).toBe(9);
    expect(row.updated_at).toBe('2026-11-03 11:00:00');
    expect(row.created_by).toBe(7);
  });

  it('returns null when no audit pair is set', () => {
    expect(reportAuditRow({ report_id: 1, updated_id: 0, created_id: 0 })).toBeNull();
    expect(reportAuditRow({ report_id: 1 })).toBeNull();
  });
});
