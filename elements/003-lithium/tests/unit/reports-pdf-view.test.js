import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import {
  PDF_CSS_PER_POINT,
  PDF_FIXTURE_URL,
  PDF_WORKER_URL,
  clampColumns,
  composedRotation,
  destTarget,
  findHits,
  flattenOutline,
  flipBoth,
  loadPdfSettings,
  marqueeLocalRect,
  moveThumb,
  normalizePdfSettings,
  pageOccupyingPane,
  pdfFitPageScale,
  pdfPageKey,
  savePdfSettings,
  scaleAfterZoom,
  searchCountLabel,
  stepThumbPercent,
  thumbCellSize,
  thumbContain,
  toggleFit,
  viewerKeyAction,
} from '../../src/managers/reports/pdf-view.js';

const html = readFileSync(resolve(process.cwd(), 'src/managers/reports/reports.html'), 'utf8');
const css = readFileSync(resolve(process.cwd(), 'src/managers/reports/reports-pdf.css'), 'utf8');

describe('preview pdf view math', () => {
  it('keeps a quarter-turn on top of page rotation', () => {
    expect(composedRotation(90, 0)).toBe(90);
    expect(composedRotation(90, 90)).toBe(180);
    expect(composedRotation(270, 90)).toBe(0);
    expect(composedRotation(0, -90)).toBe(270);
  });

  it('turns both mirrors on, then clears them', () => {
    expect(flipBoth(false, true)).toEqual({ flipH: true, flipV: true });
    expect(flipBoth(true, true)).toEqual({ flipH: false, flipV: false });
  });

  it('leaves fit when zooming and toggles back to fit page', () => {
    expect(scaleAfterZoom('fit-page', 0.42, 1, -1).mode).toBe('scale');
    expect(scaleAfterZoom('fit-page', 0.42, 1, -1).scale).toBeCloseTo(0.42 / 1.1);
    expect(toggleFit('fit-width', 1.5)).toEqual({ mode: 'fit-page', scale: 1.5 });
    expect(toggleFit('fit-page', 1.5)).toEqual({ mode: 'scale', scale: 1.5 });
  });

  it('fits a page without clamping, and clamps column count', () => {
    expect(pdfFitPageScale(2000, 2000, 100, 100, 0)).toBeCloseTo(0.05);
    expect(pdfFitPageScale(10, 10, 800, 800, 0)).toBe(80);
    expect(clampColumns(9)).toBe(5);
    expect(clampColumns(0)).toBe(1);
    expect(clampColumns('nope')).toBe(1);
  });

  it('fills one thumbnail row at 100% and steps the zoom by 5%', () => {
    expect(thumbCellSize(300, 400, 1, 100)).toEqual({ width: 300, height: 400 });
    expect(thumbCellSize(300, 400, 3, 50)).toEqual({ width: 50, height: 200 });
    expect(stepThumbPercent(100, 1)).toBe(105);
    expect(stepThumbPercent(100, -1)).toBe(95);
    expect(stepThumbPercent(25, -1)).toBe(25);
    const tall = thumbContain(100, 200, 80, 116, 16);
    expect(tall.scale).toBeCloseTo(0.5);
    expect(tall).toMatchObject({ width: 50, height: 100 });
    const wide = thumbContain(200, 100, 100, 216, 16);
    expect(wide).toMatchObject({ width: 100, height: 50 });
  });

  it('keeps the earlier page when the overlap ties', () => {
    expect(pageOccupyingPane(50, 100, [0, 100], [100, 100])).toBe(1);
  });

  it('moves the thumb by a column, and by one when columns are hidden', () => {
    expect(moveThumb(1, 'ArrowDown', 2, 7)).toBe(3);
    expect(moveThumb(0, 'ArrowLeft', 0, 7)).toBe(0);
    expect(moveThumb(3, 'Home', 2, 7)).toBe(0);
    expect(moveThumb(3, 'End', 2, 7)).toBe(6);
  });

  it('closes search on Escape inside a field and leaves text arrows alone', () => {
    expect(viewerKeyAction('Escape', { inField: true, searchOpen: true })).toBe('close-search');
    expect(viewerKeyAction('ArrowLeft', { inField: true, searchOpen: true })).toBeNull();
    expect(viewerKeyAction('ArrowDown', { inText: true, pointer: 'text' })).toBeNull();
    expect(viewerKeyAction('PageDown', { pointer: 'hand' })).toBe('page-down');
  });

  it('walks a nested outline and reads a point when the destination names one', () => {
    const rows = flattenOutline([{
      title: '  Cover ',
      dest: ['page1', { name: 'Fit' }],
      items: [{ title: 'Title', dest: ['page1', { name: 'XYZ' }, 12, 700, 0], url: '' }],
    }]);
    expect(rows.map((row) => row.title)).toEqual(['Cover', 'Title']);
    expect(rows[1].depth).toBe(1);
    expect(destTarget(rows[0].dest)).toEqual({ left: null, top: null });
    expect(destTarget(rows[1].dest)).toEqual({ left: 12, top: 700 });
    expect(destTarget(['page', { name: 'FitR' }, 1, 2, 3, 40])).toEqual({ left: 1, top: 40 });
  });

  it('counts hits without storing the query on the settings object', () => {
    const hits = findHits([{ pageNumber: 2, text: 'Lua is Great. Lua is Great.' }], 'lua is great');
    expect(hits).toEqual([
      { page: 2, start: 0, length: 12 },
      { page: 2, start: 14, length: 12 },
    ]);
    expect(searchCountLabel(0, 2)).toBe('1 of 2');
    expect(searchCountLabel(-1, 0)).toBe('0 of 0');
    const service = {
      saved: null,
      get() { return { mode: 'fit-page', query: 'secret', pages: { fixture: 3 } }; },
      set(_key, value) { this.saved = value; },
    };
    expect(normalizePdfSettings(service.get()).query).toBeUndefined();
    expect(loadPdfSettings(service).pages.fixture).toBe(3);
    savePdfSettings(service, { ...loadPdfSettings(service), searchOpen: true });
    expect(service.saved.columns).toBe(1);
    expect(service.saved.thumbPercent).toBe(100);
    expect(service.saved.thumbsOpen).toBe(true);
    expect(service.saved.thumbSize).toBeUndefined();
    expect(service.saved.searchOpen).toBe(true);
    const old = normalizePdfSettings({ columns: 0, thumbSize: 96 });
    expect(old.columns).toBe(1);
    expect(old.thumbsOpen).toBe(false);
    expect(old.thumbPercent).toBe(100);
    expect(service.saved.query).toBeUndefined();
    expect(pdfPageKey(0)).toBe('fixture');
    expect(pdfPageKey(12)).toBe('12');
    expect(PDF_FIXTURE_URL).toBe('/assets/documents/test.pdf');
    expect(PDF_WORKER_URL).toBe('/assets/pdf.worker.min.js');
    expect(PDF_CSS_PER_POINT).toBeCloseTo(96 / 72);
  });

  it('mirrors a marquee rectangle back into canvas space', () => {
    const local = marqueeLocalRect(0, 0, 10, 20, 100, 80, true, false);
    expect(local.x).toBeCloseTo(90);
    expect(local.y).toBeCloseTo(0);
    expect(local.w).toBeCloseTo(10);
    expect(local.h).toBeCloseTo(20);
  });
});

describe('preview pdf chrome', () => {
  it('puts the viewer toolbar on the pdf page and leaves Documents actions disabled', () => {
    expect(html).toContain('id="reports-pdf-toolbar"');
    expect(html).toContain('id="reports-pdf-stack"');
    expect(html).toContain('id="reports-pdf-outline"');
    const toolbar = html.slice(html.indexOf('id="reports-pdf-toolbar"'), html.indexOf('id="reports-page-report"'));
    expect(toolbar).toContain('id="reports-pdf-thumbs-btn"');
    expect(toolbar).not.toContain('id="reports-pdf-cols-less"');
    expect(toolbar).not.toContain('id="reports-pdf-thumb-smaller"');
    const aside = html.slice(html.indexOf('id="reports-pdf-thumbs"'), html.indexOf('id="reports-pdf-main"'));
    expect(aside).toContain('lithium-properties-bar-top');
    expect(aside).toContain('lithium-properties-bar-bottom');
    expect(aside).toContain('id="reports-pdf-thumb-scale"');
    expect(aside).toContain('id="reports-pdf-cols"');
    expect(html).not.toContain('Preview PDF is not wired yet');
    const around = (id) => {
      const at = html.indexOf(id);
      return html.slice(at, at + 140);
    };
    const ocr = around('id="reports-pdf-ocr-btn"');
    const extract = around('id="reports-pdf-extract-btn"');
    expect(ocr).toContain('disabled');
    expect(ocr).toContain('belongs to Documents');
    expect(extract).toContain('disabled');
    expect(extract).toContain('belongs to Documents');
  });

  it('keeps the page and the thumbs transparent until the bitmap is ready', () => {
    const rule = (selector) => {
      const at = css.indexOf(selector);
      return css.slice(at, css.indexOf('}', at));
    };
    expect(rule('.reports-pdf-sheet {')).toContain('background-color: transparent');
    expect(rule('.reports-pdf-thumb canvas {')).toContain('background-color: transparent');
    expect(rule('.reports-pdf-sheet {')).not.toContain('#fff');
    expect(rule('.reports-pdf-thumb canvas {')).not.toContain('#fff');
  });
});
