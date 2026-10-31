import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { toast } from '../../src/shared/toast.js';
import { pageFrame } from '../../src/managers/reports/canvas.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import {
  definitionText,
  openReportText,
  reportDisplayName,
  reportDownloadName,
  reportOutline,
  saveReportFile,
} from '../../src/managers/reports/reports-file.js';
import ReportsManager from '../../src/managers/reports/reports.js';

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
        { line: 'Page header, ph, 0.4 in', items: [] },
        { line: 'Detail, detail, 0.25 in', items: [] },
        { line: 'Page footer, pf, 0.4 in', items: [] },
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
    document.body.appendChild(container);
    manager = new ReportsManager({}, container);
  });

  afterEach(() => {
    manager.destroy();
    container.remove();
    toast.dismissAll();
  });

  async function open() {
    await manager.init();
    const saved = [];
    manager.saveFile = (filename, json) => {
      saved.push({ filename, json });
    };
    return saved;
  }

  function stageText() {
    return manager.elements.stage.textContent;
  }

  function pageEl() {
    return manager.elements.stage.querySelector('#reports-page');
  }

  function pageVar(name) {
    return pageEl().style.getPropertyValue(name);
  }

  it('starts from a blank report and exports the written defaults', async () => {
    const saved = await open();
    expect(manager.elements.name.textContent).toBe('Report');
    expect(container.textContent).not.toMatch(/under development/i);
    manager.elements.exportBtn.click();
    expect(saved[0].filename).toBe('Report.report.json');
    expect(JSON.parse(saved[0].json)).toEqual(createBlank());
    const doc = JSON.parse(saved[0].json);
    expect(doc.page.sizeKey).toBe(0);
    expect(doc.font.style).toBe('Normal');
    expect(doc.grid.style).toBe('dots');
    expect(doc.bands.map((band) => band.id)).toEqual(['ph', 'detail', 'pf']);
    expect(Object.hasOwn(doc.bands[1], 'datasource')).toBe(false);
    const stage = stageText();
    expect(stage).toContain('8.5 in by 11 in, portrait, size 0');
    expect(stage).toContain('0.25 in');
    expect(stage).toContain('dots, 0.125 in');
    expect(stage).toContain('Vanadium Sans, 10 pt, Normal');
    expect(stage).toContain('Page header, ph, 0.4 in');
    expect(stage).toContain('Detail, detail, 0.25 in');
    expect(stage).toContain('Page footer, pf, 0.4 in');
    expect(stage).toContain('Report.report.json');
  });

  it('shows an imported report on the stage and New restores the blank lines', async () => {
    await open();
    manager._importText(loadFixture('detail.definition.json'));
    expect(stageText()).toContain('Orders');
    expect(stageText()).toContain('Detail, detail, 0.25 in, orders');
    expect(stageText()).toContain('customer');
    manager.elements.newBtn.click();
    const stage = stageText();
    expect(stage).toContain('Page header, ph, 0.4 in');
    expect(stage).toContain('Detail, detail, 0.25 in');
    expect(stage).not.toContain('customer');
    expect(stage).not.toContain('orders');
  });

  it('says when the file omits the name and the bands', async () => {
    await open();
    manager._importText('{"format":1}');
    const stage = stageText();
    expect(stage).toContain('The file omits the name.');
    expect(stage).toContain('Omitted');
    expect(stage).toContain('No bands.');
    expect(manager.elements.stage.querySelector('#reports-summary-name').textContent).toBe('Report');
  });

  it('paints a report name as text', async () => {
    await open();
    const name = '<img src=x onerror=alert(1)>';
    manager._importText(JSON.stringify({ format: 1, name }));
    const heading = manager.elements.stage.querySelector('#reports-summary-name');
    expect(heading.textContent).toBe(name);
    expect(manager.elements.stage.querySelector('img')).toBeNull();
  });

  it('draws Letter larger than a 4 in page and insets the fixture margins', async () => {
    await open();
    const letter = pageFrame(createBlank());
    expect(letter).toMatchObject({
      omitted: false,
      widthPx: 816,
      heightPx: 1056,
      leftPx: 24,
      rightPx: 24,
      topPx: 24,
      bottomPx: 24,
    });
    expect(pageVar('--reports-page-width')).toBe('816px');
    expect(pageVar('--reports-page-height')).toBe('1056px');
    expect(pageVar('--reports-margin-left')).toBe('24px');
    expect(pageVar('--reports-margin-right')).toBe('24px');
    expect(pageVar('--reports-margin-top')).toBe('24px');
    expect(pageVar('--reports-margin-bottom')).toBe('24px');
    const guide = pageEl().querySelector('.reports-margin-guide');
    expect(guide).not.toBeNull();
    expect(guide.getAttribute('style')).toBeNull();

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
    expect(pageVar('--reports-page-width')).toBe('384px');
    expect(pageVar('--reports-page-height')).toBe('384px');
    expect(letter.widthPx).toBeGreaterThan(384);
    expect(letter.heightPx).toBeGreaterThan(384);

    manager._importText(loadFixture('detail.definition.json'));
    expect(pageVar('--reports-page-width')).toBe('816px');
    expect(pageVar('--reports-page-height')).toBe('1056px');
    expect(pageVar('--reports-margin-left')).toBe('24px');
    expect(manager.elements.name.textContent).toBe('Orders');
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
      expect(manager.elements.name.textContent).toBe('Orders');
    });
    manager.elements.exportBtn.click();
    expect(saved[0].filename).toBe('Orders.report.json');
    expect(JSON.parse(saved[0].json)).toEqual(JSON.parse(detail));
    manager._importText(saved[0].json);
    expect(manager.elements.name.textContent).toBe('Orders');
    expect(manager.doc).toEqual(JSON.parse(detail));
  });

  it('toasts a bad file and keeps the open name', async () => {
    await open();
    manager._importText(loadFixture('detail.definition.json'));
    expect(manager.elements.name.textContent).toBe('Orders');
    manager._importText('{');
    expect(manager.elements.name.textContent).toBe('Orders');
    expect(manager.doc.name).toBe('Orders');
    expect(stageText()).toContain('customer');
    expect(document.body.textContent).toMatch(/Report file is not JSON/);
    manager._importText('{"format":2,"name":"Other"}');
    expect(manager.elements.name.textContent).toBe('Orders');
    expect(document.body.textContent).toMatch(/Report format must be 1/);
  });

  it('leaves an omitted key omitted on export', async () => {
    const saved = await open();
    manager._importText('{"format":1,"name":"Bare"}');
    expect(manager.elements.name.textContent).toBe('Bare');
    manager.elements.exportBtn.click();
    expect(saved[0].filename).toBe('Bare.report.json');
    expect(JSON.parse(saved[0].json)).toEqual({ format: 1, name: 'Bare' });
  });

  it('New replaces an imported report with the written defaults', async () => {
    const saved = await open();
    manager._importText(loadFixture('detail.definition.json'));
    manager.elements.newBtn.click();
    expect(manager.elements.name.textContent).toBe('Report');
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
    expect(manager.elements.name.textContent).toBe('Report');
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
    expect(manager.elements.name.textContent).toBe('Orders');
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

  it('releases listeners from destroy and from cleanup', async () => {
    const saved = await open();
    manager.destroy();
    manager.elements.exportBtn.click();
    manager.elements.newBtn.click();
    expect(saved).toEqual([]);
    expect(manager.elements.name.textContent).toBe('Report');
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
