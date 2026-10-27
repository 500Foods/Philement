/**
 * Course Builder pure helpers (CATCHUP Phase 23): state -> button
 * visibility, payloads, and filters.
 */
import { describe, it, expect } from 'vitest';
import {
  GATE_ACTIONS, GATE_SCRIPTS, PUBLISH_PREVIEW_STATES, PUBLISH_STATE,
  buildGateScriptOptions, gateActionsFor, publishEnabled, publishPreview,
} from '../../../src/managers/course-builder/course-build-gate.js';
import {
  RUN_STATES, buildRunIdParams, buildRunListParams,
} from '../../../src/managers/course-builder/course-build-runs.js';
import {
  AUTOMATIC_STATES, INTAKE_QUERY_REF, OVERRIDE_QUERY_REF, SUGGESTED_QUERY_REF,
  buildOverrideParams, parseStageValue, stageMove, stageOptions,
} from '../../../src/managers/course-builder/course-suggestions.js';

const GATE_KEYS = {
  submission: ['accept', 'decline'],
  approve: ['approve', 'abandon'],
  inspect: ['inspectPass', 'inspectFail'],
  draft: ['publish'],
};

describe('Phase 23: state -> button visibility', () => {
  it('every P20a state shows exactly its gate keys, or none', () => {
    for (const state of RUN_STATES) {
      expect(gateActionsFor(state).map((a) => a.key)).toEqual(GATE_KEYS[state] || []);
    }
  });

  it('automatic and terminal states have no gate buttons', () => {
    for (const state of [...AUTOMATIC_STATES, 'complete', 'abandoned', 'failed', 'declined',
      'duplicate', 'rejected', 'junk', 'suggested', 'received', 'intake', 'accepted']) {
      expect(gateActionsFor(state)).toEqual([]);
    }
  });

  it('gate states are case/space tolerant and unknown states are empty', () => {
    expect(gateActionsFor('  Submission ').map((a) => a.key)).toEqual(['accept', 'decline']);
    expect(gateActionsFor('bogus')).toEqual([]);
    expect(gateActionsFor(undefined)).toEqual([]);
  });

  it('gateActionsFor returns a copy', () => {
    gateActionsFor('approve').pop();
    expect(GATE_ACTIONS.approve).toHaveLength(2);
  });

  it('Publish enabled only at draft; preview only build/inspect/checklist', () => {
    for (const state of RUN_STATES) {
      expect(publishEnabled(state)).toBe(state === PUBLISH_STATE);
      expect(publishPreview(state)).toBe(PUBLISH_PREVIEW_STATES.includes(state));
    }
    expect(publishEnabled(' DRAFT ')).toBe(true);
    expect(publishEnabled(null)).toBe(false);
    expect(publishPreview('draft')).toBe(false);
  });
});

describe('Phase 23: payloads', () => {
  it('buildGateScriptOptions for every gate script', () => {
    for (const script of Object.values(GATE_SCRIPTS)) {
      expect(buildGateScriptOptions(script, '7')).toEqual({
        script, params: { run_id: 7 }, wait: true, timeoutSeconds: 15,
      });
    }
    expect(Object.keys(GATE_SCRIPTS)).toHaveLength(7);
  });

  it('buildGateScriptOptions rejects bad run ids and unknown scripts', () => {
    for (const bad of [0, -1, 1.5, 'x', null, undefined]) {
      expect(() => buildGateScriptOptions(GATE_SCRIPTS.accept, bad)).toThrow('run_id is required');
    }
    expect(() => buildGateScriptOptions('Build.Research', 1)).toThrow(/Unknown gate script/);
  });

  it('run id params are INTEGER RUN_ID', () => {
    expect(buildRunIdParams('12')).toEqual({ INTEGER: { RUN_ID: 12 } });
  });

  it('override params normalise states and reject unknown ones', () => {
    const ok = buildOverrideParams(5, ' Draft ', 'RESEARCH', 9);
    expect(ok).toEqual({
      ok: true,
      params: { INTEGER: { SUGGESTION_ID: 5, USERID: 9 }, STRING: { FROM_STATE: 'draft', TO_STATE: 'research' } },
    });
    expect(buildOverrideParams(5, 'draft', 'bogus', 9).ok).toBe(false);
    expect(buildOverrideParams(5, '', 'draft', 9).ok).toBe(false);
  });

  it('normal moves keep 4520/4522; override prefix always goes to 4526', () => {
    expect(stageMove('suggested', 'intake').queryRef).toBe(INTAKE_QUERY_REF);
    expect(stageMove('received', 'intake').queryRef).toBe(INTAKE_QUERY_REF);
    expect(stageMove('intake', 'suggested').queryRef).toBe(SUGGESTED_QUERY_REF);
    expect(stageMove('suggested', 'override:intake').queryRef).toBe(OVERRIDE_QUERY_REF);
    expect(stageMove('draft', 'suggested').queryRef).toBe(OVERRIDE_QUERY_REF);
    expect(stageMove('intake', 'intake')).toBeNull();
    expect(stageMove('intake', 'override:bogus')).toBeNull();
  });

  it('override into automatic stages is danger; others are not', () => {
    for (const state of RUN_STATES) {
      const move = stageMove(state === 'complete' ? 'failed' : 'complete', `override:${state}`);
      expect(move.danger).toBe(AUTOMATIC_STATES.includes(state));
    }
  });

  it('parseStageValue splits the override prefix', () => {
    expect(parseStageValue('Override:Build')).toEqual({ override: true, state: 'build' });
    expect(parseStageValue(' intake ')).toEqual({ override: false, state: 'intake' });
  });
});

describe('Phase 23: filters', () => {
  it('run list STATE: named P20a state kept, blank/bogus = all', () => {
    for (const state of RUN_STATES) {
      expect(buildRunListParams(state)).toEqual({ STRING: { STATE: state } });
    }
    for (const bad of ['', 'bogus', null, undefined, 'DRAFT']) {
      expect(buildRunListParams(bad)).toEqual({ STRING: { STATE: '' } });
    }
  });

  it('override group lists every other P20a state once', () => {
    for (const state of RUN_STATES) {
      const values = stageOptions(state).groups[1].options.map((o) => o.value);
      expect(values).toEqual(RUN_STATES.filter((s) => s !== state).map((s) => `override:${s}`));
    }
  });
});
