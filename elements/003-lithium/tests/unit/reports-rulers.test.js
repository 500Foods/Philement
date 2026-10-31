import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { paintReportPage } from '../../src/managers/reports/canvas.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { toCssPx } from '../../src/managers/reports/model/units.js';
import {
  RULER_SIZE,
  bindReportCursor,
  hideReportCursor,
  hideRulerSpan,
  nextRulerUnit,
  paintReportRulers,
  placeReportCursor,
  rulerDisplayUnit,
  rulerSpec,
  rulerTicks,
  rulerUnit,
  showRulerSpan,
} from '../../src/managers/reports/rulers.js';

afterEach(() => {
  document.body.replaceChildren();
  document.head.querySelector('[data-reports-face]')?.remove();
});

/** The corner rule, outside `@layer`, so the test can measure its ink. */
function applyCornerFace() {
  const source = readFileSync(
    resolve(process.cwd(), 'src/managers/reports/reports.css'),
    'utf8',
  );
  const rule = source.match(/\.reports-ruler-corner \{[^}]*\}/);
  const style = document.createElement('style');
  style.dataset.reportsFace = '1';
  style.textContent = rule ? rule[0] : '';
  document.head.append(style);
}

function tick(length, unit, value) {
  const at = toCssPx(`${value} ${unit}`);
  return rulerTicks(length, unit).find((entry) => entry.px === at);
}

function labeled(svg, text) {
  return [...svg.querySelectorAll('text')].find((node) => node.textContent === text);
}

function mouse(type, clientX, clientY) {
  const event = new CustomEvent(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, 'clientX', { value: clientX });
  Object.defineProperty(event, 'clientY', { value: clientY });
  return event;
}

describe('ruler ticks', () => {
  it('labels each inch and ticks every eighth', () => {
    const length = toCssPx('8.5 in');
    const ticks = rulerTicks(length, 'in');
    expect(tick(length, 'in', 0)).toMatchObject({ rank: 'major', label: '' });
    expect(tick(length, 'in', 0.125)).toMatchObject({ rank: 'minor', label: '', px: 12 });
    expect(tick(length, 'in', 0.5)).toMatchObject({ rank: 'medium', label: '', px: 48 });
    expect(tick(length, 'in', 1)).toMatchObject({ rank: 'major', label: '1', px: 96 });
    expect(tick(length, 'in', 1).px).toBe(toCssPx('1 in'));
    expect(tick(length, 'in', 8.5).px).toBe(length);
    expect(ticks.at(-1).px).toBe(length);
    expect(ticks.some((entry) => entry.px > length)).toBe(false);
  });

  it('labels each centimetre and ticks every millimetre', () => {
    const length = toCssPx('2 cm');
    expect(tick(length, 'cm', 0.1)).toMatchObject({
      rank: 'minor',
      px: toCssPx('1 mm'),
    });
    expect(tick(length, 'cm', 0.5)).toMatchObject({ rank: 'medium' });
    expect(tick(length, 'cm', 1)).toMatchObject({
      rank: 'major',
      label: '1',
      px: toCssPx('1 cm'),
    });
    expect(rulerTicks(length, 'cm').at(-1).px).toBe(length);
  });

  it('labels the other document units on the same inch', () => {
    expect(tick(toCssPx('30 mm'), 'mm', 10)).toMatchObject({
      rank: 'major',
      label: '10',
      px: toCssPx('10 mm'),
    });
    expect(tick(toCssPx('30 mm'), 'mm', 1).rank).toBe('minor');
    expect(tick(toCssPx('144 pt'), 'pt', 72)).toMatchObject({
      rank: 'major',
      label: '72',
      px: toCssPx('1 in'),
    });
    expect(tick(toCssPx('144 pt'), 'pt', 9).px).toBe(toCssPx('0.125 in'));
    expect(tick(toCssPx('192 px'), 'px', 96)).toMatchObject({
      rank: 'major',
      label: '96',
      px: 96,
    });
    expect(tick(toCssPx('192 px'), 'px', 12).px).toBe(12);
  });

  it('uses inches when the page omits units and rejects an unknown unit', () => {
    expect(rulerUnit({})).toBe('in');
    expect(rulerUnit({ units: 'CM' })).toBe('cm');
    const odd = rulerSpec({ units: 'furlong' }, 816, 1056);
    expect(odd.error).toMatch(/Unknown unit "furlong"/);
    expect(rulerSpec({ units: 'in' }, 816, 1056).x).toEqual(rulerTicks(816, 'in'));
  });

  it('cycles the corner through inches, centimetres, and points', () => {
    expect(nextRulerUnit('in')).toBe('cm');
    expect(nextRulerUnit('cm')).toBe('pt');
    expect(nextRulerUnit('pt')).toBe('in');
    expect(nextRulerUnit('mm')).toBe('in');
    expect(rulerDisplayUnit('mm', null)).toBe('mm');
    expect(rulerDisplayUnit('mm', 'cm')).toBe('cm');
    expect(rulerDisplayUnit('in', 'furlong')).toBe('in');
    const page = { units: 'in', width: '8.5 in', height: '11 in' };
    const shown = rulerSpec(page, toCssPx('8.5 in'), toCssPx('11 in'), 'cm');
    expect(shown.unit).toBe('cm');
    expect(shown.x.find((entry) => entry.label === '1').px).toBe(toCssPx('1 cm'));
    expect(page.units).toBe('in');
  });
});

describe('ruler paint', () => {
  it('paints both rulers on a blank letter page and leaves the file alone', () => {
    const host = document.createElement('div');
    document.body.append(host);
    const doc = createBlank();
    const frame = paintReportPage(host, doc);
    expect(frame.rulerError).toBeUndefined();
    const sheet = host.querySelector('#reports-sheet');
    expect(sheet.querySelector('.reports-ruler-corner').textContent).toBe('in');
    const top = sheet.querySelector('.reports-ruler-top');
    const left = sheet.querySelector('.reports-ruler-left');
    expect(top.getAttribute('height')).toBe(String(RULER_SIZE));
    expect(left.getAttribute('width')).toBe(String(RULER_SIZE));
    const inch = labeled(top, '1');
    expect(inch.getAttribute('x')).toBe('94');
    expect(inch.getAttribute('y')).toBe('7');
    expect(inch.getAttribute('text-anchor')).toBe('end');
    expect(inch.getAttribute('dominant-baseline')).toBe('text-before-edge');
    const down = labeled(left, '1');
    expect(down.getAttribute('x')).toBe('7');
    expect(down.getAttribute('y')).toBe('94');
    expect(down.getAttribute('text-anchor')).toBe('start');
    expect(labeled(top, '0')).toBeUndefined();
    expect(sheet.querySelector('.reports-ruler-corner').tagName).toBe('BUTTON');
    expect(sheet.querySelector('.reports-ruler-corner').getAttribute('aria-label'))
      .toBe('Show ruler in centimetres');
    applyCornerFace();
    host.style.color = '#e0e0e0';
    expect(getComputedStyle(sheet.querySelector('.reports-ruler-corner')).color)
      .toMatch(/^(?:rgb\(42, 42, 42\)|#2a2a2a)$/);
    const minor = top.querySelector('[data-rank="minor"]');
    expect(minor.getAttribute('x1')).toBe('12');
    expect(minor.getAttribute('y1')).toBe('20.5');
    expect(minor.getAttribute('y2')).toBe(String(RULER_SIZE));
    const major = top.querySelector('[data-rank="major"]');
    expect(major.getAttribute('y1')).toBe('7');
    expect(Number(major.getAttribute('y2')) - Number(major.getAttribute('y1'))).toBe(21);
    expect(sheet.querySelector('.reports-cursor-x').hidden).toBe(true);
    expect(sheet.querySelector('.reports-cursor-y').hidden).toBe(true);
    expect(doc).toEqual(createBlank());
    expect(JSON.stringify(doc)).not.toMatch(/Ruler/);
  });

  it('moves the mouse line and hides it on leave', () => {
    const host = document.createElement('div');
    document.body.append(host);
    paintReportPage(host, createBlank());
    const page = host.querySelector('#reports-page');
    page.getBoundingClientRect = () => ({
      left: 8,
      top: 12,
      width: 816,
      height: 1056,
      right: 824,
      bottom: 1068,
    });
    const sheet = host.querySelector('#reports-sheet');
    const vertical = sheet.querySelector('.reports-cursor-x');
    const horizontal = sheet.querySelector('.reports-cursor-y');
    const cursor = bindReportCursor(page);
    page.dispatchEvent(mouse('mousemove', 8 + 96, 12 + 40));
    expect(vertical.hidden).toBe(false);
    expect(vertical.style.left).toBe(`${RULER_SIZE + 96}px`);
    expect(horizontal.style.top).toBe(`${RULER_SIZE + 40}px`);
    page.dispatchEvent(mouse('mousemove', 8 + 900, 12));
    expect(vertical.hidden).toBe(true);
    page.dispatchEvent(mouse('mouseleave', 0, 0));
    expect(horizontal.hidden).toBe(true);
    cursor.disconnect();
    page.dispatchEvent(mouse('mousemove', 8 + 96, 12 + 40));
    expect(vertical.hidden).toBe(true);
    expect(horizontal.hidden).toBe(true);
  });

  it('maps the mouse line back through the view scale', () => {
    const host = document.createElement('div');
    host.className = 'reports-canvas';
    document.body.append(host);
    paintReportPage(host, createBlank(), undefined, 0.5);
    expect(host.style.getPropertyValue('--reports-scale')).toBe('0.5');
    expect(host.classList.contains('reports-canvas-fitted')).toBe(true);
    const page = host.querySelector('#reports-page');
    expect(page.style.getPropertyValue('--reports-page-width')).toBe('816px');
    page.getBoundingClientRect = () => ({
      left: 0,
      top: 0,
      width: 408,
      height: 528,
      right: 408,
      bottom: 528,
    });
    const cursor = bindReportCursor(page);
    page.dispatchEvent(mouse('mousemove', 48, 20));
    expect(host.querySelector('.reports-cursor-x').style.left).toBe(`${RULER_SIZE + 96}px`);
    expect(host.querySelector('.reports-cursor-y').style.top).toBe(`${RULER_SIZE + 40}px`);
    cursor.disconnect();
  });

  it('skips the rulers when the unit is unknown and still draws the page', () => {
    const host = document.createElement('div');
    document.body.append(host);
    const frame = paintReportPage(host, {
      format: 1,
      page: { units: 'furlong', width: '8.5 in', height: '11 in' },
    });
    expect(frame.rulerError).toMatch(/Unknown unit "furlong"/);
    expect(host.querySelector('#reports-page')).not.toBeNull();
    expect(host.querySelector('.reports-ruler-top')).toBeNull();
    expect(host.textContent).toMatch(/Unknown unit "furlong"/);
    const sheet = document.createElement('div');
    sheet.className = 'reports-sheet';
    expect(paintReportRulers(sheet, { units: 'nope' }, { widthPx: 10, heightPx: 10 }).error)
      .toMatch(/Unknown unit/);
    expect(sheet.childElementCount).toBe(0);
    placeReportCursor(host, 1, 1);
    hideReportCursor(host);
  });

  it('inverts the ruler between two positions and clears that span', () => {
    const host = document.createElement('div');
    document.body.append(host);
    paintReportPage(host, createBlank());
    const page = host.querySelector('#reports-page');
    showRulerSpan(page, 'y', 288, 192);
    const bar = host.querySelector('.reports-ruler-left .reports-ruler-span');
    const clip = host.querySelector('.reports-ruler-left .reports-ruler-span-clip');
    expect(bar.getAttribute('y')).toBe('192');
    expect(bar.getAttribute('height')).toBe('96');
    expect(bar.getAttribute('width')).toBe(String(RULER_SIZE));
    expect(clip.getAttribute('y')).toBe('192');
    expect(clip.getAttribute('height')).toBe('96');
    expect(host.querySelector('.reports-ruler-left .reports-ruler-marks-invert').getAttribute('clip-path'))
      .toBe('url(#reports-ruler-span-y)');
    hideRulerSpan(page, 'y');
    expect(bar.getAttribute('height')).toBe('0');
    showRulerSpan(page, 'nope', 0, 1);
    expect(bar.getAttribute('height')).toBe('0');
  });
});
