import { describe, it, expect } from 'vitest';
import { readFileSync } from 'fs';
import { resolve } from 'path';

const root = process.cwd();
const ref = JSON.parse(readFileSync(resolve(root, 'config/tabulator/profile-manager/luxon-tokens.json'), 'utf8'));
const lua = readFileSync(resolve(root, '../002-helium/acuranzo/migrations/acuranzo_1409.lua'), 'utf8');

describe('Luxon tokens tabledef (Lookup 059 key 16)', () => {
  it('acuranzo_1409 targets key 16 and the luxon-tokens path', () => {
    expect(lua).toMatch(/cfg\.KEY_IDX = "16"/);
    expect(lua).toMatch(/cfg\.TABLEDEF_NAME = "profile-manager\/luxon-tokens"/);
  });

  it('migration JSON matches the reference copy', () => {
    const start = '${JSON_INGEST_START}[===[\n';
    const i = lua.indexOf(start) + start.length;
    const j = lua.indexOf('\n]===]', i);
    expect(JSON.parse(lua.slice(i, j))).toEqual(ref);
  });

  it('columns cover every field LuxonTokenTable emits', () => {
    expect(Object.keys(ref.columns).sort()).toEqual(['current', 'description', 'example', 'group', 'id', 'token']);
    expect(ref.columns.id.primaryKey).toBe(true);
    expect(ref.readonly).toBe(true);
  });

  it('tabledef-loader maps the path to key 16', () => {
    const loader = readFileSync(resolve(root, 'src/tables/resolution/tabledef-loader.js'), 'utf8');
    expect(loader).toMatch(/'profile-manager\/luxon-tokens': 16/);
  });
});
