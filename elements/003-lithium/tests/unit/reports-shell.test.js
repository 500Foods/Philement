import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { toast } from '../../src/shared/toast.js';
import { dragBandHeight } from '../../src/managers/reports/bands.js';
import { pageFrame } from '../../src/managers/reports/canvas.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { convertLength, toCssPx } from '../../src/managers/reports/model/units.js';
import { RULER_SIZE } from '../../src/managers/reports/rulers.js';
import { snapBandHeight } from '../../src/managers/reports/snap.js';
import {
  fitWidthScale,
  formatViewScale,
  stepViewScale,
} from '../../src/managers/reports/scale.js';
import {
  definitionText,
  openReportText,
  reportDisplayName,
  reportDownloadName,
  reportOutline,
  saveReportFile,
} from '../../src/managers/reports/reports-file.js';
import ReportsManager from '../../src/managers/reports/reports.js';

vi.mock('../../src/tables/lithium-table-main.js', () => ({
  LithiumTable: class {
    constructor(options) {
      this.options = options;
      this.queryRefs = null;
      this.currentData = [];
      this.table = { redraw() {} };
      this.handleAdd = async () => {};
      this.handleDelete = async () => {};
      this.setSplitter = () => {};
      this.destroy = () => {};
    }

    async init() {}

    async loadData() {}
  },
}));

const html = readFileSync(
  resolve(process.cwd(), 'src/managers/reports/reports.html'),
  'utf8',
);

function loadFixture(name) {
  const path = resolve(process.cwd(), 'tests/fixtures/reporting', name);
  return readFileSync(path, 'utf8');
}

function item(doc, bandId, itemId) {
  const band = doc.bands.find((entry) => entry.id === bandId);
  return band.items.find((entry) => entry.id === itemId);
}

describe('report file helpers', () => {
  it('names a blank report Report and a stored name as written', () => {
    expect(reportDisplayName(createBlank())).toBe('Report');
    expect(reportDisplayName({ format: 1 })).toBe('Report');
    expect(reportDisplayName({ format: 1, name: '' })).toBe('Report');
    expect(reportDisplayName({ format: 1, name: 'Orders' })).toBe('Orders');
    expect(reportDownloadName({ format: 1, name: 'Orders' })).toBe('Orders.report.json');
    expect(reportDownloadName({ format: 1 })).toBe('Report.report.json');
  });

  it('replaces characters that break a download name', () => {
    expect(reportDisplayName({ format: 1, name: 'A/B' })).toBe('A/B');
    expect(reportDownloadName({ format: 1, name: 'A/B' })).toBe('A_B.report.json');
    expect(reportDownloadName({ format: 1, name: 'A\nB' })).toBe('A_B.report.json');
    expect(reportDownloadName({ format: 1, name: '..' })).toBe('Report.report.json');
    expect(reportDownloadName({ format: 1, name: '  ' })).toBe('Report.report.json');
  });

  it('keeps the previous document when the file is bad', () => {
    const current = createBlank();
    const bad = openReportText(current, '{');
    expect(bad.doc).toBe(current);
    expect(bad.error).toMatch(/not JSON/);
    const wrong = openReportText(current, '{"format":2}');
    expect(wrong.doc).toBe(current);
    expect(wrong.error).toBe('Report format must be 1. This file has 2.');
  });

  it('round-trips the detail fixture and leaves omitted keys omitted', () => {
    const detail = loadFixture('detail.definition.json');
    const opened = openReportText(null, detail);
    expect(opened.error).toBeNull();
    expect(reportDisplayName(opened.doc)).toBe('Orders');
    const again = openReportText(null, definitionText(opened.doc));
    expect(again.doc).toEqual(JSON.parse(detail));
    const bare = openReportText(null, '{"format":1}');
    expect(definitionText(bare.doc).trim()).toBe('{\n  "format": 1\n}');
    const titled = openReportText(null, loadFixture('static.definition.json'));
    const title = item(JSON.parse(definitionText(titled.doc)), 'detail', 'title');
    expect(Object.hasOwn(title, 'align')).toBe(false);
    expect(Object.hasOwn(title, 'font')).toBe(false);
    expect(Object.hasOwn(title, 'mask')).toBe(false);
  });

  it('outlines the written defaults, an omitted file, and the detail fixture', () => {
    expect(reportOutline(createBlank())).toEqual({
      name: 'Report',
      nameOmitted: false,
      fileName: 'Report.report.json',
      pageText: '8.5 in by 11 in, portrait, size 0',
      marginText: '0.25 in',
      gridText: 'dots, 0.125 in',
      fontText: 'Vanadium Sans, 10 pt, Normal',
      bands: [
        { line: 'Page header, ph1, 0.4 in', items: [] },
        { line: 'Detail, d1, 0.25 in', items: [] },
        { line: 'Page footer, pf1, 0.4 in', items: [] },
      ],
    });

    const bare = reportOutline({ format: 1 });
    expect(bare.name).toBe('Report');
    expect(bare.nameOmitted).toBe(true);
    expect(bare.pageText).toBe('Omitted');
    expect(bare.marginText).toBe('Omitted');
    expect(bare.gridText).toBe('Omitted');
    expect(bare.fontText).toBe('Omitted');
    expect(bare.bands).toEqual([]);

    const partial = reportOutline({
      format: 1,
      page: { margin: { left: '0.5 in', right: '0.25 in' } },
      grid: {},
      font: { style: 'Bold' },
      bands: [{ typeKey: 99, items: [{ typeKey: 20 }, {}] }],
    });
    expect(partial.pageText).toBe('Omitted');
    expect(partial.marginText).toBe('left 0.5 in, right 0.25 in, top omitted, bottom omitted');
    expect(partial.gridText).toBe('Omitted');
    expect(partial.fontText).toBe('Bold');
    expect(partial.bands).toEqual([
      { line: 'Type 99', items: ['Type 20', 'Item'] },
    ]);

    const detail = reportOutline(JSON.parse(loadFixture('detail.definition.json')));
    expect(detail.name).toBe('Orders');
    expect(detail.nameOmitted).toBe(false);
    const detailBand = detail.bands.find((band) => band.line.includes('orders'));
    expect(detailBand.line).toBe('Detail, detail, 0.25 in, orders');
    expect(detailBand.items).toEqual(['customer', 'amount', 'note']);
  });

  it('downloads the json under the report name', () => {
    const proto = document.createElement('a').constructor.prototype;
    const seen = [];
    const spy = vi.spyOn(proto, 'click').mockImplementation(function onClick() {
      seen.push(this.download);
    });
    saveReportFile('Orders.report.json', '{"format":1}\n');
    expect(seen).toEqual(['Orders.report.json']);
    spy.mockRestore();
  });
});

describe('Report Manager shell', () => {
  let container;
  let manager;

  beforeEach(() => {
    localStorage.removeItem('lithium_reports_left_collapsed');
    localStorage.removeItem('lithium_reports_left_width');
    toast.dismissAll();
    const toastBox = document.getElementById('toast-container');
    if (toastBox) toastBox.innerHTML = '';
    vi.stubGlobal('fetch', vi.fn(async (url) => {
      if (url === '/src/managers/reports/reports.html') {
        return new Response(html, { status: 200, headers: { 'content-type': 'text/html' } });
      }
      return new Response('missing', { status: 404 });
    }));
    container = document.createElement('div');
    container.id = 'manager-slot-24';
    container.className = 'manager-slot slot-visible';
    document.body.appendChild(container);
    const host = document.createElement('div');
    container.appendChild(host);
    manager = new ReportsManager({}, host);
  });

  afterEach(() => {
    manager.destroy();
    container.remove();
    toast.dismissAll();
  });

  async function open() {
    await manager.init();
    manager.setRecordEditing(true);
    const saved = [];
    manager.saveFile = (filename, json) => {
      saved.push({ filename, json });
    };
    return saved;
  }

  function stageText() {
    return manager.elements.stage.textContent;
  }

  function outlineText() {
    return container.querySelector('#reports-outline').textContent;
  }

  function pageEl() {
    return manager.elements.stage.querySelector('#reports-page');
  }

  function pageVar(name) {
    return pageEl().style.getPropertyValue(name);
  }

  function bandBody(id) {
    return pageEl().querySelector(`[data-band-id="${id}"] .reports-band-body`);
  }

  function wheel(deltaY, modifiers = {}) {
    const event = new CustomEvent('wheel', { bubbles: true, cancelable: true });
    Object.defineProperty(event, 'deltaY', { value: deltaY });
    Object.defineProperty(event, 'ctrlKey', { value: Boolean(modifiers.ctrlKey) });
    Object.defineProperty(event, 'metaKey', { value: Boolean(modifiers.metaKey) });
    return event;
  }

  function pointer(type, clientY) {
    const event = new CustomEvent(type, { bubbles: true, cancelable: true });
    Object.defineProperty(event, 'clientY', { value: clientY });
    return event;
  }

  function pointerAt(type, clientX, clientY) {
    const event = pointer(type, clientY);
    Object.defineProperty(event, 'clientX', { value: clientX });
    return event;
  }

  function dragBand(id, fromY, toY) {
    const handle = pageEl().querySelector(`[data-band-id="${id}"] .reports-band-edge`);
    handle.dispatchEvent(pointer('pointerdown', fromY));
    if (toY !== fromY) document.dispatchEvent(pointer('pointermove', toY));
    document.dispatchEvent(pointer('pointerup', toY));
  }

  it('starts from a blank report and exports the written defaults', async () => {
    const saved = await open();
    expect(manager.elements.metaName.value).toBe('Report');
    expect(container.textContent).not.toMatch(/under development/i);
    manager.elements.exportBtn.click();
    expect(saved[0].filename).toBe('Report.report.json');
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
    const doc = JSON.parse(saved[0].json);
    expect(doc.page.sizeKey).toBe(0);
    expect(doc.font.style).toBe('Normal');
    expect(doc.grid.style).toBe('dots');
    expect(doc.bands.map((band) => band.id)).toEqual(['ph1', 'd1', 'pf1']);
    expect(Object.hasOwn(doc.bands[1], 'datasource')).toBe(false);
    expect(container.querySelector('.reports-summary')).toBeNull();
    const rows = [...container.querySelectorAll('#reports-outline .reports-outline-row')];
    expect(rows.map((row) => row.textContent)).toEqual(['Report', 'ph1', 'd1', 'pf1', 'End of Report']);
  });

  it('shows an imported report on the stage and New restores the blank lines', async () => {
    await open();
    manager._importText(loadFixture('detail.definition.json'));
    expect(outlineText()).toContain('Orders');
    expect(outlineText()).toContain('customer');
    manager.newReport();
    expect(outlineText()).toContain('d1');
    expect(outlineText()).not.toContain('customer');
    expect(outlineText()).not.toContain('Orders');
  });

  it('says when the file omits the name and the bands', async () => {
    await open();
    manager._importText('{"format":1}');
    const rows = [...container.querySelectorAll('#reports-outline .reports-outline-row')];
    expect(rows.map((row) => row.textContent)).toEqual(['Report', 'End of Report']);
  });

  it('paints a report name as text', async () => {
    await open();
    const name = '<img src=x onerror=alert(1)>';
    manager._importText(JSON.stringify({ format: 1, name }));
    const heading = container.querySelector('#reports-outline .reports-outline-row');
    expect(heading.textContent).toBe(name);
    expect(container.querySelector('#reports-outline img')).toBeNull();
    expect(manager.elements.stage.querySelector('img')).toBeNull();
  });

  it('draws Letter wider than a 4 in page and paints the content width', async () => {
    await open();
    const letter = pageFrame(createBlank());
    expect(letter).toMatchObject({
      omitted: false,
      widthPx: 816,
      heightPx: 1056,
      contentWidthPx: 768,
      leftPx: 24,
      rightPx: 24,
      topPx: 24,
      bottomPx: 24,
    });
    expect(pageVar('--reports-content-width')).toBe('768px');
    expect(pageEl().querySelector('.reports-margin-guide')).toBeNull();
    // Top and bottom rulers share the horizontal ruler class.
    expect(pageEl().querySelectorAll('.reports-ruler-top')).toHaveLength(2);
    expect(pageEl().querySelectorAll('.reports-band-ruler-bottom .reports-ruler-top')).toHaveLength(1);
    expect(pageEl().querySelectorAll('.reports-ruler-left')).toHaveLength(3);
    expect(pageEl().querySelectorAll('.reports-ruler-right')).toHaveLength(3);
    expect(pageEl().querySelector('[data-band-id="ph1"] .reports-ruler-left').getAttribute('height'))
      .toBe(String(toCssPx('0.4 in')));
    expect(pageEl().querySelector('[data-band-id="d1"] .reports-ruler-left').getAttribute('height'))
      .toBe(String(toCssPx('0.25 in')));

    const card = {
      format: 1,
      name: 'Card',
      page: {
        units: 'in',
        width: '4 in',
        height: '4 in',
        margin: {
          left: '0.25 in',
          right: '0.25 in',
          top: '0.25 in',
          bottom: '0.25 in',
        },
      },
    };
    manager.loadReport(card);
    expect(pageVar('--reports-content-width')).toBe('336px');
    expect(letter.contentWidthPx).toBeGreaterThan(336);
    expect(letter.widthPx).toBeGreaterThan(384);

    manager._importText(loadFixture('detail.definition.json'));
    expect(pageVar('--reports-content-width')).toBe('768px');
    expect(manager.elements.metaName.value).toBe('Orders');
  });

  it('paints each band at its stored height, in array order', async () => {
    await open();
    const roles = [...pageEl().querySelectorAll('.reports-band-role')].map((el) => el.textContent);
    expect(roles).toEqual(['Page header', 'Detail', 'Page footer', 'End of Report']);
    expect(bandBody('d1').style.height).toBe('');
    expect(bandBody('d1').style.getPropertyValue('--reports-band-height')).toBe(`${toCssPx('0.25 in')}px`);
    expect(bandBody('ph1').style.getPropertyValue('--reports-band-height')).toBe(`${toCssPx('0.4 in')}px`);
    expect(pageEl().querySelector('.reports-margin-guide')).toBeNull();
    expect(pageEl().querySelector('.reports-band-edge').getAttribute('style')).toBeNull();
    expect(manager.elements.undoBtn.disabled).toBe(true);

    const reversed = createBlank();
    reversed.bands.reverse();
    manager.loadReport(reversed);
    const next = [...pageEl().querySelectorAll('.reports-band-id')].map((el) => el.textContent);
    expect(next).toEqual(['pf1', 'd1', 'ph1']);
  });

  it('resizes Detail, exports the new height, and undo restores it', async () => {
    const saved = await open();
    const handle = pageEl().querySelector('[data-band-id="d1"] .reports-band-edge');
    handle.dispatchEvent(pointer('pointerdown', 100));
    document.dispatchEvent(pointer('pointermove', 196));
    expect(bandBody('d1').style.height).toBe(`${dragBandHeight(toCssPx('0.25 in'), 96)}px`);
    const growing = pageEl().querySelector('[data-band-id="d1"] .reports-ruler-left');
    const inch = [...growing.querySelectorAll('text')].find((node) => node.textContent === '1');
    expect(inch.getAttribute('y')).toBe('94');
    expect([...growing.querySelectorAll('line')].some((node) => node.getAttribute('y1') === '0')).toBe(true);
    document.dispatchEvent(pointer('pointerup', 196));

    const detail = () => manager.doc.bands.find((band) => band.id === 'd1');
    expect(detail().height).toBe('1.25 in');
    expect(bandBody('d1').style.height).toBe('');
    expect(bandBody('d1').style.getPropertyValue('--reports-band-height')).toBe(`${toCssPx('1.25 in')}px`);
    expect(manager.elements.undoBtn.disabled).toBe(false);
    expect(manager.elements.redoBtn.disabled).toBe(true);
    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[0].json).bands.find((band) => band.id === 'd1').height).toBe('1.25 in');

    manager.elements.undoBtn.click();
    expect(detail().height).toBe('0.25 in');
    expect(bandBody('d1').style.getPropertyValue('--reports-band-height')).toBe(`${toCssPx('0.25 in')}px`);
    expect(manager.elements.undoBtn.disabled).toBe(true);
    expect(manager.elements.redoBtn.disabled).toBe(false);
    manager.elements.redoBtn.click();
    expect(detail().height).toBe('1.25 in');
    expect(manager.elements.undoBtn.disabled).toBe(false);
    expect(manager.elements.redoBtn.disabled).toBe(true);
    manager.elements.undoBtn.click();
    manager.elements.exportBtn.click();
    const undone = JSON.parse(saved[1].json);
    expect(undone.bands.find((band) => band.id === 'd1').height).toBe('0.25 in');
    expect(undone.history.redo).toHaveLength(1);
    delete undone.history;
    expect(undone).toEqual(createBlank());
  });

  it('reopens an exported undo trail and clear removes it', async () => {
    const saved = await open();
    dragBand('d1', 100, 196);
    expect(manager.elements.clearBtn.disabled).toBe(false);
    manager.elements.exportBtn.click();
    const exported = JSON.parse(saved[0].json);
    expect(exported.history.undo).toHaveLength(1);
    expect(JSON.stringify(exported.history).length).toBeLessThan(JSON.stringify(createBlank()).length);
    manager._importText(saved[0].json);
    expect(manager.elements.undoBtn.disabled).toBe(false);
    manager.elements.undoBtn.click();
    expect(manager.doc.bands.find((band) => band.id === 'd1').height).toBe('0.25 in');
    expect(manager.elements.redoBtn.disabled).toBe(false);

    manager.elements.clearBtn.click();
    document.querySelector('.lithium-confirm .btn-secondary').click();
    await vi.waitFor(() => {
      expect(document.querySelector('.lithium-confirm')).toBeNull();
    });
    expect(manager.doc.history.redo).toHaveLength(1);
    expect(manager.doc.bands.find((band) => band.id === 'd1').height).toBe('0.25 in');

    manager.elements.clearBtn.click();
    document.querySelector('.lithium-confirm .btn-primary').click();
    await vi.waitFor(() => {
      expect(manager.doc.history).toBeUndefined();
    });
    expect(manager.elements.undoBtn.disabled).toBe(true);
    expect(manager.elements.redoBtn.disabled).toBe(true);
    expect(manager.elements.clearBtn.disabled).toBe(true);
    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[1].json).history).toBeUndefined();
    expect(JSON.parse(saved[1].json).bands.find((band) => band.id === 'd1').height).toBe('0.25 in');
  });

  it('clamps a short drag to 0.15 in and stores a millimetre page in mm', async () => {
    await open();
    dragBand('d1', 100, 100 - 1000);
    expect(manager.doc.bands.find((band) => band.id === 'd1').height).toBe('0.15 in');
    expect(bandBody('d1').style.getPropertyValue('--reports-band-height')).toBe(`${toCssPx('0.15 in')}px`);

    manager.loadReport({
      format: 1,
      name: 'Metric',
      page: {
        units: 'mm',
        width: '210 mm',
        height: '297 mm',
      },
      bands: [{ id: 'd1', typeKey: 15, height: '10 mm', items: [] }],
    });
    const start = toCssPx('10 mm');
    const preview = snapBandHeight(dragBandHeight(start, 96), toCssPx('0.125 in'), toCssPx('0.15 in'));
    dragBand('d1', 40, 40 + 96);
    const height = manager.doc.bands[0].height;
    expect(height).toBe(convertLength(`${preview} px`, 'mm'));
    expect(height.endsWith(' mm')).toBe(true);
  });

  it('undoes with Ctrl+Z and a new report clears that history', async () => {
    await open();
    manager._importText(loadFixture('detail.definition.json'));
    dragBand('detail', 10, 10 + 96);
    expect(manager.doc.name).toBe('Orders');
    expect(manager.doc.bands.find((band) => band.id === 'detail').height).toBe('1.25 in');
    const key = new CustomEvent('keydown', { bubbles: true, cancelable: true });
    Object.defineProperty(key, 'ctrlKey', { value: true });
    Object.defineProperty(key, 'metaKey', { value: false });
    Object.defineProperty(key, 'altKey', { value: false });
    Object.defineProperty(key, 'shiftKey', { value: false });
    Object.defineProperty(key, 'key', { value: 'z' });
    document.dispatchEvent(key);
    expect(manager.doc.name).toBe('Orders');
    expect(manager.doc.bands.find((band) => band.id === 'detail').height).toBe('0.25 in');
    const redoKey = new CustomEvent('keydown', { bubbles: true, cancelable: true });
    Object.defineProperty(redoKey, 'ctrlKey', { value: true });
    Object.defineProperty(redoKey, 'metaKey', { value: false });
    Object.defineProperty(redoKey, 'altKey', { value: false });
    Object.defineProperty(redoKey, 'shiftKey', { value: false });
    Object.defineProperty(redoKey, 'key', { value: 'y' });
    document.dispatchEvent(redoKey);
    expect(manager.doc.bands.find((band) => band.id === 'detail').height).toBe('1.25 in');
    manager.elements.undoBtn.click();
    expect(manager.doc.bands.find((band) => band.id === 'detail').height).toBe('0.25 in');

    dragBand('detail', 10, 10 + 96);
    expect(manager.elements.undoBtn.disabled).toBe(false);
    manager.newReport();
    expect(manager.elements.undoBtn.disabled).toBe(true);
    expect(manager.elements.redoBtn.disabled).toBe(true);
    expect(manager.elements.clearBtn.disabled).toBe(true);
    expect(manager.doc.bands.find((band) => band.id === 'd1').height).toBe('0.25 in');
    expect(manager.doc.name).toBe('Report');
  });

  it('draws an omitted height from the default and a bad length as a note', async () => {
    const saved = await open();
    manager.loadReport({
      format: 1,
      name: 'Plain',
      page: { units: 'in', width: '8.5 in', height: '11 in' },
      bands: [{ id: 'd1', typeKey: 15, items: [] }],
    });
    expect(bandBody('d1').style.getPropertyValue('--reports-band-height')).toBe(`${toCssPx('0.25 in')}px`);
    manager.elements.exportBtn.click();
    expect(Object.hasOwn(JSON.parse(saved[0].json).bands[0], 'height')).toBe(false);

    manager.loadReport({
      format: 1,
      page: { width: '8.5 in', height: '11 in' },
      bands: [{ id: 'd1', typeKey: 15, height: '4', items: [] }],
    });
    expect(pageEl().querySelector('.reports-band-body')).toBeNull();
    expect(pageEl().querySelector('.reports-band-edge')).toBeNull();
    expect(pageEl().querySelector('.reports-band-note').textContent).toMatch(/needs a unit/);
  });

  it('paints the five grid styles from the band origin', async () => {
    const saved = await open();
    const pattern = () => bandBody('d1').querySelector('pattern');
    expect(pattern().getAttribute('x')).toBe('0');
    expect(pattern().getAttribute('y')).toBe('0');
    expect(pattern().getAttribute('width')).toBe(String(toCssPx('0.125 in')));
    const circles = [...bandBody('d1').querySelectorAll('circle')];
    expect(circles).toHaveLength(4);
    expect(circles[0].getAttribute('cx')).toBe('0');
    expect(circles[0].getAttribute('cy')).toBe('0');
    expect(circles[0].getAttribute('r')).toBe('1');
    expect(pageEl().querySelectorAll('.reports-band-grid')).toHaveLength(3);

    const page = createBlank().page;
    const bands = createBlank().bands;
    for (const style of ['grid', 'dotted', 'dashed']) {
      manager.loadReport({
        format: 1, page, bands, grid: { style, spacing: '0.125 in' },
      });
      expect(pattern().getAttribute('x')).toBe('0');
      expect(pattern().getAttribute('width')).toBe('12');
      expect(bandBody('d1').querySelector('path').getAttribute('d')).toBe('M 0 0 V 12 M 0 0 H 12');
    }
    manager.loadReport({
      format: 1, page, bands, grid: { style: 'none', spacing: '1 in' },
    });
    expect(pageEl().querySelector('.reports-band-grid')).toBeNull();
    expect(bandBody('d1').style.getPropertyValue('--reports-grid-background')).toBe('white');

    const bare = createBlank();
    delete bare.grid;
    manager.loadReport(bare);
    expect(bandBody('d1').querySelector('circle')).not.toBeNull();
    manager.elements.exportBtn.click();
    expect(Object.hasOwn(JSON.parse(saved[0].json), 'grid')).toBe(false);

    manager.loadReport({
      format: 1,
      page,
      bands,
      grid: { style: 'dots', spacing: '4' },
    });
    expect(pageEl().querySelector('.reports-band-grid')).toBeNull();
    expect(bandBody('d1')).not.toBeNull();
    expect(stageText()).toMatch(/needs a unit/);
  });

  it('tracks the pointer on the rulers and exports no ruler band', async () => {
    const saved = await open();
    const sheet = () => manager.elements.stage.querySelector('#reports-sheet');
    expect(sheet().querySelector('.reports-ruler-corner').textContent).toBe('in');
    const inch = [...sheet().querySelectorAll('.reports-ruler-top text')]
      .find((node) => node.textContent === '1');
    const inchTick = [...sheet().querySelectorAll('.reports-ruler-top line')]
      .find((node) => node.getAttribute('x1') === String(toCssPx('1 in'))
        && node.getAttribute('data-rank') === 'major');
    expect(inchTick).toBeTruthy();
    expect(inch.getAttribute('x')).toBe(String(toCssPx('1 in') - 2));
    expect(inch.getAttribute('y')).toBe('7');
    const sheetBox = () => manager.elements.stage.querySelector('#reports-sheet');
    const detailBody = () => sheetBox().querySelector('[data-band-id="d1"] .reports-band-body');
    sheetBox().getBoundingClientRect = () => ({
      left: 0, top: 0, width: 796, height: 220, right: 796, bottom: 220,
    });
    detailBody().getBoundingClientRect = () => ({
      left: RULER_SIZE, top: 80, width: 768, height: 24, right: 796, bottom: 104,
    });
    detailBody().dispatchEvent(pointerAt('mousemove', RULER_SIZE + 96, 90));
    const vertical = sheet().querySelector('.reports-cursor-x');
    expect(vertical.hidden).toBe(false);
    expect(vertical.style.left).toBe(`${RULER_SIZE + 96}px`);
    pageEl().dispatchEvent(pointerAt('mouseleave', 0, 0));
    expect(vertical.hidden).toBe(true);

    manager.loadReport(createBlank());
    const again = pageEl();
    const againSheet = again.closest('#reports-sheet');
    const againBody = againSheet.querySelector('[data-band-id="d1"] .reports-band-body');
    againSheet.getBoundingClientRect = () => ({
      left: 0, top: 0, width: 796, height: 220, right: 796, bottom: 220,
    });
    againBody.getBoundingClientRect = () => ({
      left: RULER_SIZE, top: 80, width: 768, height: 24, right: 796, bottom: 104,
    });
    againBody.dispatchEvent(pointerAt('mousemove', RULER_SIZE + 48, 90));
    expect(sheet().querySelector('.reports-cursor-x').hidden).toBe(false);
    expect(sheet().querySelector('.reports-cursor-x').style.left).toBe(`${RULER_SIZE + 48}px`);

    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
    manager.cleanup();
    again.dispatchEvent(pointerAt('mousemove', 96, 24));
    expect(sheet().querySelector('.reports-cursor-x').hidden).toBe(true);
  });

  it('inverts the vertical ruler across the band edge that moved', async () => {
    await open();
    const page = pageEl();
    const handle = page.querySelector('[data-band-id="d1"] .reports-band-edge');
    handle.dispatchEvent(pointer('pointerdown', 400));
    document.dispatchEvent(pointer('pointermove', 496));
    const bar = () => page.querySelector('[data-band-id="d1"] .reports-ruler-left .reports-ruler-span');
    expect(bar().getAttribute('y')).toBe(String(toCssPx('0.25 in')));
    expect(bar().getAttribute('height')).toBe('96');
    expect(page.querySelector('[data-band-id="d1"] .reports-ruler-left').style.height)
      .toBe(`${toCssPx('0.25 in') + 96}px`);
    // The right ruler grows too, with ticks and numbers for the new length.
    const right = page.querySelector('[data-band-id="d1"] .reports-ruler-right');
    const grown = toCssPx('0.25 in') + 96;
    expect(right.style.height).toBe(`${grown}px`);
    const rightTicks = [...right.querySelectorAll('.reports-ruler-marks line')].map((line) => Number(line.getAttribute('y1')));
    expect(Math.max(...rightTicks)).toBeGreaterThan(toCssPx('0.25 in'));
    expect([...right.querySelectorAll('.reports-ruler-marks text')].map((node) => node.textContent)).toContain('1');
    expect(right.querySelector('.reports-ruler-marks line[data-rank="major"]').getAttribute('x1')).toBe('0');
    document.dispatchEvent(pointer('pointerup', 496));
    expect(bar().getAttribute('height')).toBe('0');
  });

  it('cycles the corner through centimetres and points and leaves the file alone', async () => {
    const saved = await open();
    const corner = () => manager.elements.stage.querySelector('.reports-ruler-corner');
    const labelAt = (text) => [...manager.elements.stage.querySelectorAll('.reports-ruler-top text')]
      .find((node) => node.textContent === text);
    expect(corner().textContent).toBe('in');
    corner().click();
    expect(corner().textContent).toBe('cm');
    expect(labelAt('1').getAttribute('x')).toBe(String(toCssPx('1 cm') - 2));
    corner().click();
    expect(corner().textContent).toBe('pt');
    expect(labelAt('72').getAttribute('x')).toBe(String(toCssPx('72 pt') - 2));
    corner().click();
    expect(corner().textContent).toBe('in');
    manager.newReport();
    expect(corner().textContent).toBe('in');
    corner().click();
    expect(corner().textContent).toBe('cm');
    manager.newReport();
    expect(corner().textContent).toBe('in');
    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
  });

  it('draws the page without rulers when the unit is unknown', async () => {
    await open();
    manager.loadReport({
      format: 1,
      name: 'Odd',
      page: { units: 'furlong', width: '8.5 in', height: '11 in' },
    });
    expect(pageEl()).not.toBeNull();
    expect(manager.elements.stage.querySelector('.reports-ruler-top')).toBeNull();
    expect(stageText()).toMatch(/Unknown unit "furlong"/);
    expect(document.body.textContent).toMatch(/Unknown unit "furlong"/);
  });

  it('disconnects the grid and property panel observers on cleanup', async () => {
    let disconnects = 0;
    class FakeObserver {
      observe() {}

      disconnect() {
        disconnects += 1;
      }
    }
    const real = globalThis.ResizeObserver;
    vi.stubGlobal('ResizeObserver', FakeObserver);
    try {
      await open();
      expect(pageEl().querySelector('.reports-band-grid')).not.toBeNull();
      manager.cleanup();
      expect(disconnects).toBe(3);
    } finally {
      vi.stubGlobal('ResizeObserver', real);
    }
  });

  it('omits the page when the file has none and toasts a bad length', async () => {
    await open();
    manager.loadReport({ format: 1 });
    expect(pageEl()).toBeNull();
    expect(stageText()).toContain('The file omits the page.');
    manager.loadReport({ format: 1, page: { width: '4', height: '4 in' } });
    expect(pageEl()).toBeNull();
    expect(stageText()).toMatch(/needs a unit/);
    expect(document.body.textContent).toMatch(/needs a unit/);
  });

  it('imports the detail fixture, exports it, and imports that export', async () => {
    const saved = await open();
    const detail = loadFixture('detail.definition.json');
    const file = { text: async () => detail };
    Object.defineProperty(manager.elements.fileInput, 'files', {
      configurable: true,
      value: [file],
    });
    manager.elements.fileInput.dispatchEvent(new CustomEvent('change'));
    await vi.waitFor(() => {
      expect(manager.elements.metaName.value).toBe('Orders');
    });
    manager.elements.exportBtn.click();
    expect(saved[0].filename).toBe('Orders.report.json');
    expect(JSON.parse(saved[0].json)).toEqual(JSON.parse(detail));
    manager._importText(saved[0].json);
    expect(manager.elements.metaName.value).toBe('Orders');
    expect(manager.doc).toEqual(JSON.parse(detail));
  });

  it('toasts a bad file and keeps the open name', async () => {
    await open();
    manager._importText(loadFixture('detail.definition.json'));
    expect(manager.elements.metaName.value).toBe('Orders');
    manager._importText('{');
    expect(manager.elements.metaName.value).toBe('Orders');
    expect(manager.doc.name).toBe('Orders');
    expect(outlineText()).toContain('customer');
    expect(document.body.textContent).toMatch(/Report file is not JSON/);
    manager._importText('{"format":2,"name":"Other"}');
    expect(manager.elements.metaName.value).toBe('Orders');
    expect(document.body.textContent).toMatch(/Report format must be 1/);
  });

  it('leaves an omitted key omitted on export', async () => {
    const saved = await open();
    manager._importText('{"format":1,"name":"Bare"}');
    expect(manager.elements.metaName.value).toBe('Bare');
    manager.elements.exportBtn.click();
    expect(saved[0].filename).toBe('Bare.report.json');
    expect(JSON.parse(saved[0].json)).toEqual({ format: 1, name: 'Bare' });
  });

  it('New replaces an imported report with the written defaults', async () => {
    const saved = await open();
    manager._importText(loadFixture('detail.definition.json'));
    manager.newReport();
    expect(manager.elements.metaName.value).toBe('Report');
    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
  });

  it('opens the file picker from Import', async () => {
    await open();
    const click = vi.spyOn(manager.elements.fileInput, 'click').mockImplementation(() => {});
    manager.elements.importBtn.click();
    expect(click).toHaveBeenCalledOnce();
  });

  it('ignores a cancelled file picker', async () => {
    await open();
    Object.defineProperty(manager.elements.fileInput, 'files', {
      configurable: true,
      value: [],
    });
    manager.elements.fileInput.dispatchEvent(new CustomEvent('change'));
    await Promise.resolve();
    expect(manager.elements.metaName.value).toBe('Report');
    expect(document.body.textContent).not.toMatch(/Report file/);
  });

  it('keeps the previous document when the file cannot be read', async () => {
    await open();
    manager._importText(loadFixture('detail.definition.json'));
    await manager._importFile({
      text: async () => {
        throw new Error('disk');
      },
    });
    expect(manager.elements.metaName.value).toBe('Orders');
    expect(document.body.textContent).toContain('disk');
  });

  it('does not apply a file that finishes after close', async () => {
    await open();
    const pending = manager._importFile({
      text: () => new Promise((resolve) => {
        manager.destroy();
        resolve(loadFixture('detail.definition.json'));
      }),
    });
    await pending;
    expect(manager.doc.name).toBe('Report');
  });

  function panePadding() {
    const body = manager.elements.stage.querySelector('.reports-stage-body');
    const style = getComputedStyle(body);
    return (Number.parseFloat(style.paddingLeft) || 0) + (Number.parseFloat(style.paddingRight) || 0);
  }

  function viewScale() {
    return manager.elements.stage.querySelector('.reports-canvas').style.getPropertyValue('--reports-scale');
  }

  it('fits a known pane width, zooms around it, and leaves the file alone', async () => {
    const saved = await open();
    expect(viewScale()).toBe('1');
    expect(manager.elements.zoomLabel.textContent).toBe('100%');
    expect(pageVar('--reports-content-width')).toBe('768px');

    Object.defineProperty(manager.elements.stage, 'clientWidth', {
      configurable: true,
      get: () => 1000,
    });
    const pane = 1000 - panePadding();
    // Left and right rulers both count toward the fitted width.
    const fitted = formatViewScale(fitWidthScale(pane, 768 + 2 * RULER_SIZE));
    manager.elements.fitBtn.click();
    expect(viewScale()).toBe(fitted);
    expect(fitted).toBe('1.21');
    expect(pageVar('--reports-content-width')).toBe('768px');
    const inch = [...manager.elements.stage.querySelectorAll('.reports-ruler-top line')]
      .find((node) => node.getAttribute('x1') === '96' && node.getAttribute('data-rank') === 'major');
    expect(inch).toBeTruthy();
    expect(manager.elements.zoomLabel.textContent).toBe('121%');

    manager.elements.zoomInBtn.click();
    const zoomed = formatViewScale(stepViewScale(Number(fitted), 1));
    expect(viewScale()).toBe(zoomed);
    expect(Number(zoomed)).toBeGreaterThan(Number(fitted));
    const corner = () => manager.elements.stage.querySelector('.reports-ruler-corner');
    corner().click();
    expect(corner().textContent).toBe('cm');
    expect(viewScale()).toBe(zoomed);
    manager.elements.zoom100Btn.click();
    expect(viewScale()).toBe('1');
    expect(manager.elements.zoomLabel.textContent).toBe('100%');
    manager.elements.zoomOutBtn.click();
    expect(viewScale()).toBe('0.95');

    manager.newReport();
    expect(viewScale()).toBe(fitted);
    manager.elements.zoomInBtn.click();
    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
  });

  it('steps the outline with arrow keys, reaches End of Report, and a ruler click selects the report', async () => {
    await open();
    const outline = container.querySelector('#reports-outline');
    expect(outline.tabIndex).toBe(0);
    const key = (k) => outline.dispatchEvent(new window.KeyboardEvent('keydown', { key: k, bubbles: true, cancelable: true }));
    key('ArrowDown');
    expect(manager._panelSelection().band).toBeTruthy();
    for (let i = 0; i < 20; i += 1) key('ArrowDown');
    expect(manager._panelSelection().end).toBe(true);
    expect(pageEl().querySelector('.reports-band-end').classList.contains('reports-band-selected')).toBe(true);
    pageEl().querySelector('.reports-ruler-left').dispatchEvent(pointerAt('pointerdown', 5, 50));
    document.dispatchEvent(pointerAt('pointerup', 5, 50));
    const sel = manager._panelSelection();
    expect(Boolean(sel.end || sel.band || sel.itemId)).toBe(false);
  });

  it('the Ruler toggle hides and restores every ruler without editing the file', async () => {
    const saved = await open();
    expect(pageEl().querySelectorAll('.reports-ruler-left').length).toBeGreaterThan(0);
    manager.elements.rulerBtn.click();
    expect(manager.elements.rulerBtn.getAttribute('aria-pressed')).toBe('false');
    expect(pageEl().querySelector('.reports-ruler-top, .reports-ruler-left, .reports-ruler-right')).toBeNull();
    manager.elements.rulerBtn.click();
    expect(pageEl().querySelectorAll('.reports-ruler-right').length).toBeGreaterThan(0);
    manager.elements.exportBtn.click();
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
  });

  it('the Crosshair toggle sits after Ruler and hides the cursor guide lines', async () => {
    await open();
    const button = manager.elements.crosshairBtn;
    expect(manager.elements.rulerBtn.nextElementSibling).toBe(button);
    expect(button.textContent.trim()).toBe('Crosshair');
    expect(button.getAttribute('aria-pressed')).toBe('true');
    button.click();
    expect(button.getAttribute('aria-pressed')).toBe('false');
    expect(manager.elements.stage.classList.contains('reports-crosshair-off')).toBe(true);
    button.click();
    expect(manager.elements.stage.classList.contains('reports-crosshair-off')).toBe(false);
  });

  it('zooms from Ctrl or Cmd plus the wheel over the page and leaves a bare wheel scrolling', async () => {
    const saved = await open();
    const bare = wheel(-1);
    pageEl().dispatchEvent(bare);
    expect(bare.defaultPrevented).toBe(false);
    expect(viewScale()).toBe('1');

    const up = wheel(-1, { ctrlKey: true });
    pageEl().dispatchEvent(up);
    expect(up.defaultPrevented).toBe(true);
    expect(viewScale()).toBe('1.05');
    expect(manager.elements.zoomLabel.textContent).toBe('105%');

    const mac = wheel(-1, { metaKey: true });
    pageEl().dispatchEvent(mac);
    expect(mac.defaultPrevented).toBe(true);
    expect(viewScale()).toBe('1.1');

    const ruler = manager.elements.stage.querySelector('.reports-ruler-top');
    const onRuler = wheel(-1, { ctrlKey: true });
    ruler.dispatchEvent(onRuler);
    expect(onRuler.defaultPrevented).toBe(true);
    expect(viewScale()).toBe('1.15');

    const summary = container.querySelector('#reports-outline');
    const aside = wheel(-1, { ctrlKey: true });
    summary.dispatchEvent(aside);
    expect(aside.defaultPrevented).toBe(false);
    expect(viewScale()).toBe('1.15');

    const flat = wheel(0, { ctrlKey: true });
    pageEl().dispatchEvent(flat);
    expect(flat.defaultPrevented).toBe(false);
    expect(viewScale()).toBe('1.15');

    const down = wheel(40, { ctrlKey: true });
    pageEl().querySelector('.reports-band').dispatchEvent(down);
    expect(down.defaultPrevented).toBe(true);
    expect(viewScale()).toBe('1.1');

    manager.cleanup();
    const after = wheel(-1, { ctrlKey: true });
    pageEl().dispatchEvent(after);
    expect(after.defaultPrevented).toBe(false);
    expect(viewScale()).toBe('1.1');
    manager.elements.exportBtn.click();
    expect(saved).toEqual([]);
  });

  it('drags a band in layout pixels when the view is scaled', async () => {
    await open();
    for (let step = 0; step < 10; step += 1) manager.elements.zoomOutBtn.click();
    expect(viewScale()).toBe('0.5');
    const page = pageEl();
    const body = bandBody('d1');
    const sheet = page.closest('#reports-sheet');
    sheet.getBoundingClientRect = () => ({
      top: 0, left: 0, width: 398, height: 200, right: 398, bottom: 200,
    });
    body.getBoundingClientRect = () => ({
      top: 40, left: 14, right: 398, bottom: 52, width: 384, height: 12,
    });
    const handle = page.querySelector('[data-band-id="d1"] .reports-band-edge');
    handle.dispatchEvent(pointer('pointerdown', 0));
    document.dispatchEvent(pointerAt('pointermove', 62, 48));
    const bar = page.querySelector('[data-band-id="d1"] .reports-ruler-left .reports-ruler-span');
    expect(bar.getAttribute('y')).toBe(String(toCssPx('0.25 in')));
    expect(bar.getAttribute('height')).toBe('96');
    const cursor = body.parentElement.querySelector('.reports-cursor-y');
    expect(cursor.hidden).toBe(false);
    expect(cursor.style.top).toBe(`${(48 - 40) / 0.5}px`);
    expect(body.style.height).toBe(`${dragBandHeight(toCssPx('0.25 in'), 96)}px`);
    document.dispatchEvent(pointer('pointerup', 48));
  });

  it('releases listeners from destroy and from cleanup', async () => {
    const saved = await open();
    manager.destroy();
    manager.elements.exportBtn.click();
    manager.newReport();
    expect(saved).toEqual([]);
    expect(manager.elements.metaName.value).toBe('Report');
    manager.destroy();

    const second = new ReportsManager({}, container);
    const secondSaved = [];
    vi.stubGlobal('fetch', vi.fn(async () => new Response(html, { status: 200 })));
    await second.init();
    second.saveFile = (filename, json) => {
      secondSaved.push({ filename, json });
    };
    second.cleanup();
    second.elements.exportBtn.click();
    expect(secondSaved).toEqual([]);
  });

  it('toasts when the template cannot be loaded', async () => {
    vi.stubGlobal('fetch', vi.fn(async () => new Response('nope', { status: 404 })));
    await manager.init();
    expect(container.textContent).toContain('Failed to load Report Manager');
    expect(document.body.textContent).toContain('Report template returned 404.');
  });
});
