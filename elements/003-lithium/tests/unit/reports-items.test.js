import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import ReportsManager from '../../src/managers/reports/reports.js';
import { dragItemBox, staticItemText } from '../../src/managers/reports/items.js';

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

import { createBlank } from '../../src/managers/reports/model/document.js';
import { toCssPx } from '../../src/managers/reports/model/units.js';

const html = readFileSync(
  resolve(process.cwd(), 'src/managers/reports/reports.html'),
  'utf8',
);

const css = readFileSync(
  resolve(process.cwd(), 'src/managers/reports/reports.css'),
  'utf8',
);

function itemRule() {
  const start = css.indexOf('.reports-item {');
  const end = css.indexOf('.reports-item[data-align');
  return css.slice(start, end);
}

describe('static text geometry', () => {
  it('reads a stored string and uses Text when the key is omitted', () => {
    expect(staticItemText({ text: 'Static' })).toBe('Static');
    expect(staticItemText({ text: '' })).toBe('');
    expect(staticItemText({})).toBe('Text');
  });

  it('keeps a drag inside the band and floors a resize at 1 CSS pixel', () => {
    const bounds = { width: 768, height: 24 };
    const start = { left: 312, top: 2.4, width: 144, height: 19.2 };
    expect(dragItemBox(start, -1000, -40, bounds, false)).toEqual({
      left: 0, top: 0, width: 144, height: 19.2,
    });
    expect(dragItemBox(start, 1000, 0, bounds, false)).toEqual({
      left: 624, top: 2.4, width: 144, height: 19.2,
    });
    expect(dragItemBox(start, 0, 100, bounds, true)).toEqual({
      left: 312, top: 2.4, width: 144, height: 21.6,
    });
    expect(dragItemBox(start, -1000, -1000, bounds, true)).toEqual({
      left: 312, top: 2.4, width: 1, height: 1,
    });
  });

  it('clips item text with hidden overflow and an inset selection outline', () => {
    const rule = itemRule();
    expect(rule).toContain('overflow: hidden');
    expect(rule).toContain('white-space: nowrap');
    expect(rule).toContain('color: #2a2a2a');
    expect(css).toContain('outline-offset: -1px');
  });
});

describe('Report Manager static text', () => {
  let container;
  let manager;

  beforeEach(() => {
    localStorage.removeItem('lithium_reports_left_collapsed');
    localStorage.removeItem('lithium_reports_left_width');
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
  });

  async function open() {
    await manager.init();
    const saved = [];
    manager.saveFile = (filename, json) => {
      saved.push({ filename, json });
    };
    return saved;
  }

  function pageEl() {
    return manager.elements.stage.querySelector('#reports-page');
  }

  function bandEl(id) {
    return pageEl().querySelector(`[data-band-id="${id}"]`);
  }

  function textEl() {
    return pageEl().querySelector('.reports-item');
  }

  function pointer(type, clientX, clientY) {
    const event = new CustomEvent(type, { bubbles: true, cancelable: true });
    Object.defineProperty(event, 'clientX', { value: clientX });
    Object.defineProperty(event, 'clientY', { value: clientY });
    return event;
  }

  function key(name) {
    const event = new CustomEvent('keydown', { bubbles: true, cancelable: true });
    Object.defineProperty(event, 'key', { value: name });
    Object.defineProperty(event, 'ctrlKey', { value: false });
    Object.defineProperty(event, 'metaKey', { value: false });
    Object.defineProperty(event, 'shiftKey', { value: false });
    Object.defineProperty(event, 'altKey', { value: false });
    return event;
  }

  function detailItem() {
    return manager.doc.bands.find((band) => band.id === 'detail').items[0];
  }

  it('adds Text to the selected band and exports typeKey 0', async () => {
    const saved = await open();
    expect(bandEl('detail').classList.contains('reports-band-selected')).toBe(true);
    expect(manager.elements.textBtn.disabled).toBe(false);
    expect(manager.elements.deleteBtn.disabled).toBe(true);
    expect(textEl()).toBeNull();

    manager.elements.textBtn.click();
    const placed = detailItem();
    expect(placed).toEqual({
      id: 'i1',
      typeKey: 0,
      align: 'Left',
      text: 'Text',
      box: {
        left: '3.25 in',
        top: '0.025 in',
        width: '1.5 in',
        height: '0.2 in',
      },
    });
    expect(Object.hasOwn(placed, 'font')).toBe(false);
    expect(Object.hasOwn(placed, 'mask')).toBe(false);
    const node = textEl();
    expect(node.textContent).toBe('Text');
    expect(node.classList.contains('reports-item-selected')).toBe(true);
    expect(node.dataset.align).toBe('Left');
    expect(node.dataset.fontStyle).toBe('Normal');
    expect(Number.parseFloat(node.style.left)).toBeCloseTo(toCssPx('3.25 in'), 5);
    expect(Number.parseFloat(node.style.top)).toBeCloseTo(toCssPx('0.025 in'), 5);
    expect(Number.parseFloat(node.style.width)).toBeCloseTo(toCssPx('1.5 in'), 5);
    expect(Number.parseFloat(node.style.height)).toBeCloseTo(toCssPx('0.2 in'), 5);
    expect(node.style.getPropertyValue('--reports-item-font-size')).toBe(`${toCssPx('10 pt')}px`);
    expect(node.style.getPropertyValue('--reports-item-font-family')).toContain('Vanadium Sans');
    expect(manager.elements.deleteBtn.disabled).toBe(false);

    node.dispatchEvent(pointer('pointerdown', 0, 0));
    document.dispatchEvent(pointer('pointerup', 0, 0));
    expect(manager.doc.history.undo).toHaveLength(1);

    manager.elements.undoBtn.click();
    expect(manager.doc.bands.find((band) => band.id === 'detail').items).toEqual([]);
    expect(textEl()).toBeNull();
    expect(manager.elements.deleteBtn.disabled).toBe(true);
    manager.elements.redoBtn.click();
    expect(detailItem().text).toBe('Text');
    expect(textEl().classList.contains('reports-item-selected')).toBe(true);

    manager.elements.exportBtn.click();
    const file = JSON.parse(saved[0].json);
    expect(file.format).toBe(1);
    expect(file.bands.find((band) => band.id === 'detail').items[0]).toEqual(placed);
    expect(file.history.undo).toHaveLength(1);
    manager._importText(saved[0].json);
    expect(textEl().textContent).toBe('Text');
    expect(detailItem()).toEqual(placed);
    expect(manager.elements.undoBtn.disabled).toBe(false);
    expect(textEl().classList.contains('reports-item-selected')).toBe(false);
  });

  it('moves, resizes, and deletes the item, and undo restores it', async () => {
    await open();
    manager.elements.textBtn.click();
    const node = textEl();
    node.dispatchEvent(pointer('pointerdown', 0, 0));
    document.dispatchEvent(pointer('pointermove', 96, 0));
    document.dispatchEvent(pointer('pointerup', 96, 0));
    expect(detailItem().box.left).toBe('4.25 in');
    expect(detailItem().box.top).toBe('0.025 in');
    expect(detailItem().box.width).toBe('1.5 in');
    expect(bandEl('detail').style.getPropertyValue('--reports-band-height')).toBe('');
    expect(bandEl('detail').querySelector('.reports-band-body').style.getPropertyValue('--reports-band-height'))
      .toBe(`${toCssPx('0.25 in')}px`);

    const corner = textEl().querySelector('.reports-item-corner');
    corner.dispatchEvent(pointer('pointerdown', 0, 0));
    document.dispatchEvent(pointer('pointermove', 96, 0));
    document.dispatchEvent(pointer('pointerup', 96, 0));
    expect(detailItem().box).toEqual({
      left: '4.25 in',
      top: '0.025 in',
      width: '2.5 in',
      height: '0.2 in',
    });

    textEl().dispatchEvent(pointer('pointerdown', 0, 0));
    document.dispatchEvent(pointer('pointermove', 1000, 80));
    document.dispatchEvent(pointer('pointerup', 1000, 80));
    expect(detailItem().box.left).toBe('5.5 in');
    expect(detailItem().box.top).toBe('0.05 in');
    expect(detailItem().box.width).toBe('2.5 in');
    expect(detailItem().box.height).toBe('0.2 in');

    manager.elements.deleteBtn.click();
    expect(manager.doc.bands.find((band) => band.id === 'detail').items).toEqual([]);
    document.dispatchEvent(key('Delete'));
    expect(manager.doc.bands.find((band) => band.id === 'detail').items).toEqual([]);
    manager.elements.undoBtn.click();
    expect(detailItem().id).toBe('i1');
    expect(detailItem().box.left).toBe('5.5 in');
    expect(textEl().classList.contains('reports-item-selected')).toBe(true);
    manager.elements.undoBtn.click();
    expect(detailItem().box).toMatchObject({ left: '4.25 in', width: '2.5 in' });
    manager.elements.undoBtn.click();
    expect(detailItem().box).toMatchObject({ left: '4.25 in', width: '1.5 in' });
    manager.elements.undoBtn.click();
    expect(detailItem().box.left).toBe('3.25 in');
    manager.elements.undoBtn.click();
    expect(manager.doc.bands.find((band) => band.id === 'detail').items).toEqual([]);
  });

  it('divides a drag by the view scale and adds text to the band that was clicked', async () => {
    await open();
    manager.elements.textBtn.click();
    for (let step = 0; step < 10; step += 1) manager.elements.zoomOutBtn.click();
    textEl().dispatchEvent(pointer('pointerdown', 0, 0));
    document.dispatchEvent(pointer('pointermove', 48, 0));
    document.dispatchEvent(pointer('pointerup', 48, 0));
    expect(detailItem().box.left).toBe('4.25 in');

    bandEl('ph').querySelector('.reports-band-strip').dispatchEvent(pointer('pointerdown', 0, 0));
    expect(bandEl('ph').classList.contains('reports-band-selected')).toBe(true);
    expect(bandEl('detail').classList.contains('reports-band-selected')).toBe(false);
    expect(textEl().classList.contains('reports-item-selected')).toBe(false);
    expect(manager.elements.deleteBtn.disabled).toBe(true);
    manager.elements.textBtn.click();
    const header = manager.doc.bands.find((band) => band.id === 'ph').items[0];
    expect(header.box.top).toBe('0.1 in');
    expect(header.box.left).toBe('3.25 in');
    expect(header.text).toBe('Text');
    expect(pageEl().querySelectorAll('.reports-item')).toHaveLength(2);
  });

  it('paints a long static string inside the band and leaves other item types off the page', async () => {
    await open();
    const page = createBlank().page;
    const letters = 'A'.repeat(80);
    manager.loadReport({
      format: 1,
      name: 'Long',
      page,
      font: { family: 'Courier New', size: '12 pt', style: 'Bold' },
      bands: [{
        id: 'detail',
        typeKey: 15,
        height: '0.25 in',
        items: [
          {
            id: 'title',
            typeKey: 0,
            text: letters,
            box: { left: '0 in', top: '0 in', width: '0.5 in', height: '0.2 in' },
          },
          {
            id: 'customer',
            typeKey: 20,
            field: 'customer',
            box: { left: '1 in', top: '0 in', width: '1 in', height: '0.2 in' },
          },
        ],
      }],
    });
    const node = textEl();
    expect(node.textContent).toBe(letters);
    expect(node.dataset.fontStyle).toBe('Bold');
    expect(node.style.getPropertyValue('--reports-item-font-family')).toContain('Courier New');
    expect(node.style.getPropertyValue('--reports-item-font-size')).toBe(`${toCssPx('12 pt')}px`);
    expect(pageEl().querySelectorAll('.reports-item')).toHaveLength(1);
    expect(bandEl('detail').querySelector('.reports-band-body').style.getPropertyValue('--reports-band-height'))
      .toBe(`${toCssPx('0.25 in')}px`);
    expect(manager.elements.stage.textContent).toContain('customer');

    manager.loadReport({
      format: 1,
      page,
      bands: [{
        id: 'detail',
        typeKey: 15,
        height: '0.25 in',
        items: [{
          id: 'title',
          typeKey: 0,
          box: { left: '0 in', top: '0 in', width: '1 in', height: '0.2 in' },
        }],
      }],
    });
    expect(textEl().textContent).toBe('Text');
    expect(Object.hasOwn(manager.doc.bands[0].items[0], 'text')).toBe(false);
  });
});
