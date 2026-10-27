import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect } from 'vitest';
import {
  GATE_ACTIONS, GATE_SCRIPTS, buildGateScriptOptions, findRunForSuggestion,
  gateActionsFor, gateOutcome, recentEvents, splitRunDetail,
} from '../../../src/managers/course-builder/course-build-gate.js';

const here = dirname(fileURLToPath(import.meta.url));
const mig = (n) => readFileSync(join(
  here, `../../../../002-helium/courses/migrations/courses_${n}.lua`,
), 'utf8');

describe('course build gate (Phase 21)', () => {
  it('shows Accept/Decline only at submission and Approve/Abandon only at approve', () => {
    expect(gateActionsFor('submission').map((a) => a.script)).toEqual(['Build.Accept', 'Build.Decline']);
    expect(gateActionsFor('approve').map((a) => a.script)).toEqual(['Build.Approve', 'Build.Abandon']);
    for (const state of ['intake', 'accepted', 'outline', 'research', 'declined', '', null]) {
      expect(gateActionsFor(state)).toEqual([]);
    }
  });

  it('never offers a Research button', () => {
    const all = Object.values(GATE_ACTIONS).flat();
    expect(all.some((a) => /research/i.test(a.label))).toBe(false);
    expect(all.filter((a) => a.to === 'research').map((a) => a.script)).toEqual(['Build.Approve']);
  });

  it('maps a button to the invokeScript payload', () => {
    expect(buildGateScriptOptions('Build.Decline', '7')).toEqual({
      script: 'Build.Decline', params: { run_id: 7 }, wait: true, timeoutSeconds: 15,
    });
    expect(() => buildGateScriptOptions('Build.Decline', 0)).toThrow();
    expect(() => buildGateScriptOptions('Build.Research', 1)).toThrow();
  });

  it('script transitions match the migrations', () => {
    const files = { accept: 4124, decline: 4125, approve: 4126, abandon: 4127 };
    for (const action of Object.values(GATE_ACTIONS).flat()) {
      const src = mig(files[action.key]);
      const name = GATE_SCRIPTS[action.key].split('.')[1];
      expect(src).toContain(`cfg.SCRIPT_NAME = "${name}"`);
      expect(src).toContain(`to = "${action.to}"`);
    }
    expect(mig(4123)).toContain('cfg.SCRIPT_NAME = "Gate"');
  });

  it('reads wrong_state failures from the script result', () => {
    const out = gateOutcome({
      status: 'completed',
      result: { ok: false, code: 'wrong_state', message: 'Run is intake, expected submission' },
    });
    expect(out).toEqual({ ok: false, code: 'wrong_state', message: 'Run is intake, expected submission' });
    expect(gateOutcome({ status: 'completed', result: { ok: true, state: 'declined' } }).ok).toBe(true);
  });

  it('finds the run and splits detail rows', () => {
    expect(findRunForSuggestion([{ run_id: 2, suggestion_id: 5 }], 5).run_id).toBe(2);
    expect(findRunForSuggestion([], 5)).toBeNull();
    const { run, artifacts } = splitRunDetail([
      { run_id: 2, state: 'approve', artifact_id: 9, artifact_kind: 'outline', module_index: null, artifact_body: '<b>x</b>' },
    ]);
    expect(run.state).toBe('approve');
    expect(artifacts).toEqual([expect.objectContaining({ artifact_id: 9, kind: 'outline', body: '<b>x</b>' })]);
    expect(splitRunDetail([{ run_id: 2, artifact_id: null }]).artifacts).toEqual([]);
    expect(recentEvents([{ action_id: 1 }, { action_id: 2 }]).map((e) => e.action_id)).toEqual([2, 1]);
  });
});
