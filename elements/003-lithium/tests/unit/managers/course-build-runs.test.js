import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect } from 'vitest';
import {
  RUN_LIST_QUERY_REF, RUN_GET_QUERY_REF, RUN_EVENTS_QUERY_REF,
  RUN_TABLE_DEF, RUN_STATES, buildRunListParams, buildRunIdParams,
} from '../../../src/managers/course-builder/course-build-runs.js';

const here = dirname(fileURLToPath(import.meta.url));
const mig = (n) => readFileSync(join(
  here, `../../../../002-helium/courses/migrations/courses_${n}.lua`,
), 'utf8');

describe('course build runs (Phase 20 / CB-2)', () => {
  it('query refs match the migrations', () => {
    expect(mig(4120)).toContain(`cfg.QUERY_REF = "${RUN_LIST_QUERY_REF}"`);
    expect(mig(4121)).toContain(`cfg.QUERY_REF = "${RUN_GET_QUERY_REF}"`);
    expect(mig(4122)).toContain(`cfg.QUERY_REF = "${RUN_EVENTS_QUERY_REF}"`);
  });

  it('tableDef fields are selected by 4523', () => {
    const sql = mig(4120);
    for (const field of Object.keys(RUN_TABLE_DEF.columns)) {
      expect(sql).toMatch(new RegExp(`\\b${field}\\b`));
    }
    expect(RUN_TABLE_DEF.columns.run_id.primaryKey).toBe(true);
  });

  it('list params keep only P20a states', () => {
    expect(RUN_STATES).toHaveLength(22);
    expect(buildRunListParams('intake')).toEqual({ STRING: { STATE: 'intake' } });
    expect(buildRunListParams('bogus')).toEqual({ STRING: { STATE: '' } });
    expect(buildRunListParams()).toEqual({ STRING: { STATE: '' } });
  });

  it('run id params are INTEGER', () => {
    expect(buildRunIdParams('7')).toEqual({ INTEGER: { RUN_ID: 7 } });
  });
});
