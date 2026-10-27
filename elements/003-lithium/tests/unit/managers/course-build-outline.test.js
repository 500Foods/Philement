import { describe, it, expect, afterEach } from 'vitest';
import {
  GATE_SCRIPTS, buildGateScriptOptions, gateActionsFor, paintRunPanel,
  publishEnabled, publishPreview,
} from '../../../src/managers/course-builder/course-build-gate.js';
import {
  latestMaterials, latestOfKind, lmsUrlFor, renderOutline, versionsOfKind,
} from '../../../src/managers/course-builder/course-build-outline.js';

const XSS_JSON = JSON.stringify({
  title: '<img src=x onerror="window.__xss=1">',
  modules: ['<script>window.__xss=2</script>', { name: '<svg onload="window.__xss=3">' }],
});

afterEach(() => { delete window.__xss; document.body.replaceChildren(); });

describe('course build outline (Phase 22)', () => {
  it('renders an XSS outline as text only', () => {
    const el = renderOutline(XSS_JSON);
    document.body.append(el);
    expect(el.querySelector('img, script, svg')).toBeNull();
    expect(el.textContent).toContain('<img src=x onerror="window.__xss=1">');
    expect(el.textContent).toContain('<script>window.__xss=2</script>');
    expect(window.__xss).toBeUndefined();
  });

  it('renders a non-JSON body as plain text', () => {
    const el = renderOutline('<img src=x onerror=alert(1)>');
    expect(el.querySelector('img')).toBeNull();
    expect(el.querySelector('pre').textContent).toBe('<img src=x onerror=alert(1)>');
  });

  it('picks latest outline, versions, and latest material per index', () => {
    const arts = [
      { artifact_id: 3, kind: 'outline' }, { artifact_id: 7, kind: 'outline' },
      { artifact_id: 4, kind: 'material', module_index: 2 },
      { artifact_id: 5, kind: 'material', module_index: 1 },
      { artifact_id: 8, kind: 'material', module_index: 2 },
    ];
    expect(latestOfKind(arts, 'outline').artifact_id).toBe(7);
    expect(versionsOfKind(arts, 'outline').map((a) => a.artifact_id)).toEqual([7, 3]);
    expect(latestMaterials(arts).map((a) => [a.module_index, a.artifact_id, a.versions]))
      .toEqual([[1, 5, 1], [2, 8, 2]]);
  });

  it('builds the LMS link from config and the course id only', () => {
    expect(lmsUrlFor({ canvas_course_id: 42 })).toMatch(/\/courses\/42$/);
    expect(lmsUrlFor({ canvas_course_id: null })).toBe('');
    expect(lmsUrlFor({ canvas_course_id: 'javascript:alert(1)' })).toBe('');
    expect(lmsUrlFor({ canvas_course_id: 42 })).not.toMatch(/token/i);
  });

  it('Inspect and Publish buttons emit the locked script names', () => {
    expect(gateActionsFor('inspect').map((a) => [a.label, a.script, a.to])).toEqual([
      ['Inspect Pass', 'Build.InspectPass', 'checklist'],
      ['Inspect Fail', 'Build.InspectFail', 'build'],
    ]);
    expect(gateActionsFor('draft').map((a) => a.script)).toEqual(['Build.Publish']);
    expect(buildGateScriptOptions(GATE_SCRIPTS.publish, 3)).toEqual({
      script: 'Build.Publish', params: { run_id: 3 }, wait: true, timeoutSeconds: 15,
    });
  });

  it('Publish is enabled only at draft and never offered automatically', () => {
    expect(publishEnabled('draft')).toBe(true);
    for (const s of ['build', 'inspect', 'checklist', 'publishing', 'approve', '']) {
      expect(publishEnabled(s)).toBe(false);
    }
    expect(publishPreview('inspect')).toBe(true);
    expect(publishPreview('draft')).toBe(false);
  });

  it('paints the Run tab without executing artifact markup', () => {
    const host = document.createElement('div');
    document.body.append(host);
    const mgr = { elements: { runPane: host } };
    paintRunPanel(mgr, { run_id: 1, state: 'inspect', canvas_course_id: 42 },
      [{ artifact_id: 9, kind: 'outline', body: XSS_JSON }], []);
    expect(host.querySelector('img, script, svg')).toBeNull();
    expect(window.__xss).toBeUndefined();
    const publish = host.querySelector('button[data-gate="publish"]');
    expect(publish.disabled).toBe(true);
    expect(host.querySelector('button[data-gate="inspectPass"]')).not.toBeNull();
    const lms = host.querySelector('a.builder-run-lms');
    expect(lms.getAttribute('rel')).toContain('noopener');
  });
});
