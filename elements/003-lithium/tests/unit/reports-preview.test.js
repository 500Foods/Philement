import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { composeDefinition, composeSources } from '../../src/managers/reports/reports-compose.js';
import { cssPageLength, previewFrameBox, previewParams, previewResult, previewSrcdoc } from '../../src/managers/reports/reports-preview.js';

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
