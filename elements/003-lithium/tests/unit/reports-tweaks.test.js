import { describe, expect, it, vi } from 'vitest';
import { LithiumProperties, nextEditableId } from '../../src/properties/lithium-properties.js';
import { applyProperty, propertyRows, rulerSides, selectionScope } from '../../src/managers/reports/report-properties.js';
import { outlineEntries, selectedEntryIndex } from '../../src/managers/reports/reports-panel.js';
import { endMarker, paintReportPage } from '../../src/managers/reports/canvas.js';
import { createBlank } from '../../src/managers/reports/model/document.js';

describe('property panel tweaks', () => {
  it('Tab order skips read-only rows and stops at the ends', () => {
    const rows = [{ id: 'a', readOnly: true }, { id: 'b' }, { id: 'c', readOnly: true }, { id: 'd' }];
    expect(nextEditableId(rows, 'b', 1)).toBe('d');
    expect(nextEditableId(rows, 'd', -1)).toBe('b');
    expect(nextEditableId(rows, 'b', -1)).toBeNull();
    expect(nextEditableId(rows, 'a', 1)).toBe('b');
    expect(nextEditableId(rows, 'zz', 1)).toBeNull();
  });

  it('bottom bar has tight prev/next after expand/collapse and calls onNavigate', () => {
    const element = document.createElement('aside');
    const outline = document.createElement('nav');
    document.body.append(element);
    const onNavigate = vi.fn();
    const props = new LithiumProperties({ element, outline, onEdit: vi.fn(), onNavigate });
    const btn = (action) => element.querySelector(`[data-action="${action}"]`);
    for (const action of ['expand', 'collapse', 'prev', 'next']) {
      expect(btn(action).classList.contains('lithium-properties-btn-tight')).toBe(true);
    }
    expect(btn('prev').querySelector('.fa-angle-up, [class*="angle-up"]')).not.toBeNull();
    expect(btn('next').querySelector('.fa-angle-down, [class*="angle-down"]')).not.toBeNull();
    btn('prev').click();
    btn('next').click();
    expect(onNavigate.mock.calls.map((call) => call[0])).toEqual([-1, 1]);
    props.destroy();
  });
});

describe('End of Report and rulers', () => {
  const doc = createBlank();

  it('outline ends with an End of Report entry that selection can find', () => {
    const entries = outlineEntries(doc);
    expect(entries.at(-1)).toMatchObject({ depth: 1, label: 'End of Report', end: true });
    expect(selectedEntryIndex(entries, { end: true })).toBe(entries.length - 1);
    expect(selectedEntryIndex(entries, {})).toBe(0);
  });

  it('end scope shows one read-only height row and refuses edits', () => {
    expect(selectionScope(doc, { end: true })).toBe('end');
    const rows = propertyRows(doc, { end: true }, { attributes: [] });
    expect(rows).toHaveLength(1);
    expect(rows[0]).toMatchObject({ id: 'end-height', readOnly: true });
    expect(() => applyProperty(doc, { end: true }, 'end-height', '1', {})).toThrow();
  });

  it('ruler sides default on, honour the file, and the toolbar toggle hides all', () => {
    expect(rulerSides(doc)).toEqual({ top: true, left: true, right: true, bottom: true });
    expect(rulerSides({ rulers: { left: false } })).toMatchObject({ left: false, top: true });
    expect(rulerSides(doc, false)).toEqual({ top: false, left: false, right: false, bottom: false });
  });

  it('report scope lists ruler booleans and edits them without touching the original', () => {
    const rows = propertyRows(doc, {}, { attributes: [] });
    const ids = rows.map((row) => row.id).filter((id) => String(id).startsWith('ruler-'));
    expect(ids).toEqual(['ruler-top', 'ruler-left', 'ruler-right', 'ruler-bottom']);
    const next = applyProperty(doc, {}, 'ruler-left', 'False', {});
    const out = next && next.doc ? next.doc : next;
    expect(out.rulers.left).toBe(false);
    expect(doc.rulers && doc.rulers.left).not.toBe(false);
  });

  it('paints the end marker and all four rulers when asked', () => {
    expect(endMarker().classList.contains('reports-band-end')).toBe(true);
    expect(endMarker().textContent).toContain('End of Report');
    const host = document.createElement('div');
    document.body.append(host);
    paintReportPage(host, createBlank(), undefined, 1, { top: true, left: true, right: true, bottom: true });
    expect(host.querySelector('.reports-band-end')).not.toBeNull();
    expect(host.querySelectorAll('.reports-ruler-right').length).toBeGreaterThan(0);
    expect(host.querySelector('.reports-band-ruler-bottom')).not.toBeNull();
    host.replaceChildren();
    paintReportPage(host, createBlank(), undefined, 1, { top: false, left: false, right: false, bottom: false });
    expect(host.querySelector('.reports-ruler-left, .reports-ruler-right, .reports-band-ruler-bottom')).toBeNull();
    host.remove();
  });

  it('right and bottom ruler ticks start at the edge that faces the page', () => {
    const host = document.createElement('div');
    document.body.append(host);
    paintReportPage(host, createBlank(), undefined, 1, { top: true, left: true, right: true, bottom: true });
    const majors = (sel) => [...host.querySelectorAll(`${sel} .reports-ruler-marks line[data-rank="major"]`)];
    const left = majors('.reports-ruler-left')[0];
    const right = majors('.reports-ruler-right')[0];
    expect(left.getAttribute('x2')).toBe('28');
    expect(right.getAttribute('x1')).toBe('0');
    expect(Number(right.getAttribute('x2'))).toBe(28 - Number(left.getAttribute('x1')));
    const top = majors('.reports-ruler-top').find((line) => !line.closest('.reports-band-ruler-bottom'));
    const bottom = majors('.reports-band-ruler-bottom .reports-ruler-top')[0];
    expect(top.getAttribute('y2')).toBe('28');
    expect(bottom.getAttribute('y1')).toBe('0');
    expect(Number(bottom.getAttribute('y2'))).toBe(28 - Number(top.getAttribute('y1')));
    host.remove();
  });
});
