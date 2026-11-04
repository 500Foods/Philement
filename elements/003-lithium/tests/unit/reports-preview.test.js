import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { composeDefinition, composeSources } from '../../src/managers/reports/reports-compose.js';
import {
  PREVIEW_OVER_CAP_TOAST,
  SCRIPT_PARAMS_RESERVE_BYTES,
  cssPageLength,
  inlineParamsExceedCap,
  previewFrameBox,
  previewParams,
  previewResult,
  previewSrcdoc,
  resolvePreviewParams,
  scriptParamsBytes,
  storedPreviewParams,
} from '../../src/managers/reports/reports-preview.js';

const fixture = (name) => readFileSync(
  resolve(process.cwd(), 'tests/fixtures/reporting', name),
  'utf8',
);

describe('preview params and result', () => {
  it('asks Reports.Render for stage html with the compose definition and sources', () => {
    const doc = JSON.parse(fixture('detail.definition.json'));
    const params = previewParams(doc);
    expect(params.stage).toBe('html');
    expect(params.definition).toEqual(composeDefinition(doc));
    expect(params.sources).toEqual(composeSources(doc));
  });

  it('reads html and log, and turns a script error into one message', () => {
    expect(previewResult({
      status: 'completed',
      result: { html: '<section>North</section>', log: [{ level: 'warn', message: 'x' }] },
    })).toEqual({
      html: '<section>North</section>',
      log: [{ level: 'warn', message: 'x' }],
    });
    expect(previewResult({
      status: 'completed',
      result: '{"html":"<section></section>","log":{}}',
    }).log).toEqual([]);
    expect(previewResult({
      status: 'failed',
      error: 'stage html is not available yet',
    }).error).toBe('stage html is not available yet');
    expect(previewResult({
      status: 'completed',
      result: { error: 'bad band', log: [] },
    }).error).toBe('bad band');
    expect(previewResult({
      status: 'completed',
      result: { pages: [], log: [] },
    }).error).toBe('Preview returned no HTML.');
  });
});

describe('over-cap preview params', () => {
  const detail = () => JSON.parse(fixture('detail.definition.json'));

  it('keeps a small definition inline and measures compact UTF-8 bytes', () => {
    const params = previewParams(detail());
    const bytes = scriptParamsBytes(params);
    expect(bytes).toBe(new TextEncoder().encode(JSON.stringify(params)).length);
    expect(inlineParamsExceedCap(params, bytes + SCRIPT_PARAMS_RESERVE_BYTES)).toBe(false);
    expect(inlineParamsExceedCap(params, bytes + SCRIPT_PARAMS_RESERVE_BYTES - 1)).toBe(true);
  });

  it('names the stored revision and leaves the definition and the session claims out', () => {
    const doc = detail();
    const params = storedPreviewParams(doc, { report_id: 4, rev_id: 2 });
    expect(params).toEqual({
      stage: 'html',
      report_id: 4,
      rev_id: 2,
      sources: composeSources(doc),
    });
    expect(Object.hasOwn(params, 'definition')).toBe(false);
    expect(Object.hasOwn(params, '_hydrogen')).toBe(false);
    expect(PREVIEW_OVER_CAP_TOAST).toEqual({
      title: 'Saved to preview',
      description: 'This report is larger than the 256 KiB script limit, so it was saved and previewed from the stored revision.',
    });
  });

  it('saves once, toasts once, and previews by the new revision when the inline body is over the cap', async () => {
    const doc = detail();
    const notes = [];
    let saves = 0;
    const manager = {
      _selectedRow: () => ({ report_id: 0 }),
      _openRevision: null,
      editHelper: {
        isEditing: () => true,
        _takeSnapshot() { this.snapped = true; },
        checkDirtyState() { this.checked = true; },
      },
    };
    const resolved = await resolvePreviewParams(manager, doc, {
      cap: 32,
      save: async (host) => {
        saves += 1;
        host._openRevision = { report_id: 9, rev_id: 3 };
      },
      notify: (title, description) => notes.push({ title, description }),
    });
    expect(saves).toBe(1);
    expect(manager.editHelper.snapped).toBe(true);
    expect(manager.editHelper.checked).toBe(true);
    expect(notes).toEqual([PREVIEW_OVER_CAP_TOAST]);
    expect(resolved.stored).toBe(true);
    expect(resolved.params.report_id).toBe(9);
    expect(resolved.params.rev_id).toBe(3);
    expect(resolved.params.definition).toBeUndefined();
  });

  it('does not save a definition that fits', async () => {
    let saves = 0;
    const resolved = await resolvePreviewParams({}, detail(), {
      cap: 8 * 1024 * 1024,
      save: async () => { saves += 1; },
      notify: () => { throw new Error('toast'); },
    });
    expect(saves).toBe(0);
    expect(resolved.stored).toBe(false);
    expect(resolved.params.definition).toEqual(composeDefinition(detail()));
  });

  it('leaves the preview unsent when the save fails or no row is selected', async () => {
    const doc = detail();
    await expect(resolvePreviewParams({ _selectedRow: () => null }, doc, { cap: 32 }))
      .rejects.toMatchObject({
        overCapSave: true,
        serverError: { error: 'Save is not available' },
      });
    const failed = new Error('The insert did not return a report id.');
    await expect(resolvePreviewParams({
      _selectedRow: () => ({ report_id: 1 }),
    }, doc, {
      cap: 32,
      save: async () => { throw failed; },
      notify: () => { throw new Error('toast'); },
    })).rejects.toMatchObject({ overCapSave: true, message: failed.message });
  });
});

describe('preview frame box', () => {
  it('uses the emitted section, then the definition page', () => {
    const doc = JSON.parse(fixture('detail.definition.json'));
    expect(cssPageLength('8.5 in', '1in')).toBe('8.5in');
    expect(cssPageLength('nope', '11in')).toBe('11in');
    expect(previewFrameBox('', doc)).toEqual({ width: '8.5in', height: '11in' });
    expect(previewFrameBox(fixture('detail.html'), { page: { width: '4 in', height: '6 in' } })).toEqual({
      width: '8.5in',
      height: '11in',
    });
    const two = `${fixture('detail.html')}\n${fixture('detail.html')}`;
    expect(previewFrameBox(two, doc).height).toBe('calc(11in * 2)');
    expect(previewFrameBox('<section style="width:210mm;height:297mm"></section>', doc)).toEqual({
      width: '210mm',
      height: '297mm',
    });
  });
});

describe('preview srcdoc', () => {
  it('keeps the detail page styles and strips a script', () => {
    const page = fixture('detail.html');
    const doc = previewSrcdoc(page);
    expect(doc.startsWith('<!DOCTYPE html>')).toBe(true);
    expect(doc).toContain('North');
    expect(doc).toContain('1,234.50');
    expect(doc).toContain('position:absolute');
    expect(doc).toContain('font-family:');
    expect(doc).toContain('Vanadium Sans');
    expect(doc).toContain('background:#fff');
    // happy-dom's DOMPurify reads an empty tag name and unwraps <section>.
    // Chrome keeps the section, including break-after and the page box.

    const dirty = previewSrcdoc(
      '<div style="position:absolute">North</div><script>alert(1)</script><img src="x" onerror="alert(1)">',
    );
    expect(dirty).toContain('North');
    expect(dirty).not.toMatch(/<script/i);
    expect(dirty).not.toMatch(/onerror/i);
    expect(dirty).not.toMatch(/alert\(1\)/);
  });
});
