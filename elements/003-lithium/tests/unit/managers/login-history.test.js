import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, it, expect } from 'vitest';
import { formatLoginTime, buildLoginParams, toLoginTableRow, LOGIN_HISTORY_TABLEDEF_KEY, LOGIN_HISTORY_TABLE_PATH, QUERY_LOGIN_HISTORY, QUERY_LOGIN_SUMMARY }
  from '../../../src/managers/profile-manager/pages/login-history/page-login-history.js';

describe('Login History page helpers', () => {
  it('uses QueryRefs 159/160', () => {
    expect(QUERY_LOGIN_HISTORY).toBe(159);
    expect(QUERY_LOGIN_SUMMARY).toBe(160);
  });
  it('builds params from claims', () => {
    expect(buildLoginParams({ user_id: 7, username: 'andrew' }))
      .toEqual({ INTEGER: { ACCOUNTID: 7 }, STRING: { LOGINID: 'andrew' } });
  });
  it('formats 24h Vancouver time with PCT', () => {
    expect(formatLoginTime('2026-10-27T12:00:00Z')).toBe('2026-10-27 05:00:00 PCT');
    expect(formatLoginTime('2026-10-27 12:00:00')).toBe('2026-10-27 05:00:00 PCT');
    expect(formatLoginTime(null)).toBe('-');
  });
  it('maps QueryRef 159 rows for the LithiumTable', () => {
    expect(toLoginTableRow({ created_at: '2026-10-27T12:00:00Z', action_type_a24: 3, login_id: 'andrew',
      ip_address: '10.0.0.1', action_msecs: '42', app_version: '1.2.3' }, 0)).toEqual({
      row_id: 1, when_txt: '2026-10-27 05:00:00 PCT', result_txt: 'Success', action_type_a24: 3,
      login_id: 'andrew', ip_address: '10.0.0.1', action_msecs: 42, app_version: '1.2.3',
    });
    const r = toLoginTableRow({ action_type_a24: 2 }, 4);
    expect(r.result_txt).toBe('Attempt');
    expect(r.action_msecs).toBeNull();
    expect(r.when_txt).toBe('-');
  });
  it('loads its tableDef from Lookup 059 key 17', () => {
    expect(LOGIN_HISTORY_TABLEDEF_KEY).toBe(17);
    expect(LOGIN_HISTORY_TABLE_PATH).toBe('profile-manager/login-history');
  });
  it('reference tabledef JSON is read-only with the expected columns', () => {
    const def = JSON.parse(readFileSync(resolve(process.cwd(), 'config/tabulator/profile-manager/login-history.json'), 'utf8'));
    expect(def.readonly).toBe(true);
    expect(Object.keys(def.columns)).toEqual(
      ['row_id', 'when_txt', 'result_txt', 'login_id', 'ip_address', 'action_msecs', 'app_version']);
    expect(Object.values(def.columns).every((c) => c.editable === false)).toBe(true);
  });
  it('migration 1408 embeds the same tabledef JSON', () => {
    const lua = readFileSync(resolve(process.cwd(), '../002-helium/acuranzo/migrations/acuranzo_1408.lua'), 'utf8');
    const m = lua.match(/\$\{JSON_INGEST_START\}\[===\[\n([\s\S]*?)\n\]===\]/);
    const ref = JSON.parse(readFileSync(resolve(process.cwd(), 'config/tabulator/profile-manager/login-history.json'), 'utf8'));
    expect(JSON.parse(m[1])).toEqual(ref);
    expect(lua).toMatch(/cfg\.KEY_IDX = "17"/);
  });
  it('labels token issuance rows (type 0)', () => {
    expect(toLoginTableRow({ action_type_a24: 0, action_type_name: 'Token Issued' }).result_txt).toBe('Token Issued');
  });
});
