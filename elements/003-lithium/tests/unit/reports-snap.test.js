import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import ReportsManager from '../../src/managers/reports/reports.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { toCssPx } from '../../src/managers/reports/model/units.js';
import {
  boxesNear,
  snapBandHeight,
  snapItemBox,
  snapSpacingPx,
  snapToGrid,
} from '../../src/managers/reports/snap.js';

vi.mock('../../src/tables/lithium-table-main.js', () => ({
  LithiumTable: class {
    constructor() {
      this.table = { redraw() {} };
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

describe('grid snap', () => {
  const step = toCssPx('0.125 in');
  const floor = toCssPx('0.15 in');

  it('rounds to the spacing and keeps the band floor', () => {
    expect(step).toBe(12);
    expect(snapToGrid(34, step)).toBe(36);
    expect(snapToGrid(322, step)).toBe(324);
    expect(snapToGrid(120, step)).toBe(120);
    expect(snapToGrid(34, 0)).toBe(34);
    expect(snapBandHeight(34, step, floor)).toBe(36);
    expect(snapBandHeight(16, step, floor)).toBe(floor);
    expect(snapBandHeight(floor, step, floor)).toBe(floor);
  });

  it('snaps a move on both edges and a resize on the far edges', () => {
    const bounds = { width: 768, height: 24 };
    const moved = snapItemBox(
      { left: 322, top: 2.4, width: 144, height: 10 },
      step,
      bounds,
      false,
    );
    expect(moved).toEqual({ left: 324, top: 0, width: 144, height: 10 });
    const blocked = snapItemBox(
      { left: 322, top: 12.4, width: 144, height: 19.2 },
      step,
      bounds,
      false,
    );
    expect(blocked.left).toBe(324);
    expect(blocked.top).toBeCloseTo(4.8, 5);
    const grown = snapItemBox(
      { left: 312, top: 0, width: 146, height: 19.2 },
      step,
      bounds,
      true,
    );
    expect(grown.left).toBe(312);
    expect(grown.top).toBe(0);
    expect(grown.width).toBe(144);
    expect(grown.height).toBe(24);
    expect(boxesNear(moved, { left: 324, top: 0, width: 144, height: 10 })).toBe(true);
    expect(boxesNear(moved, grown)).toBe(false);
  });

  it('reads the document grid and skips a grid that cannot be measured', () => {
    expect(snapSpacingPx(createBlank())).toBe(step);
    expect(snapSpacingPx({ grid: { spacing: 'furlong' } })).toBe(0);
    expect(snapSpacingPx({ grid: { spacing: '0 in' } })).toBe(0);
  });
});

describe('Report Manager snap', () => {
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
    manager.setRecordEditing(true);
  }

  function pointer(type, clientX, clientY) {
    const event = new CustomEvent(type, { bubbles: true, cancelable: true });
    Object.defineProperty(event, 'clientX', { value: clientX });
    Object.defineProperty(event, 'clientY', { value: clientY });
    return event;
  }

  function page() {
    return manager.elements.stage.querySelector('#reports-page');
  }

  it('snaps a band and an item when the pointer stops, and previews the landing', async () => {
    await open();
    expect(manager.elements.snapBtn.getAttribute('aria-pressed')).toBe('true');
    const handle = page().querySelector('[data-band-id="detail"] .reports-band-edge');
    handle.dispatchEvent(pointer('pointerdown', 0, 100));
    document.dispatchEvent(pointer('pointermove', 0, 110));
    const line = page().querySelector('[data-band-id="detail"] .reports-snap-line');
    expect(line).not.toBeNull();
    expect(line.style.top).toBe('36px');
    expect(page().querySelector('[data-band-id="detail"]').classList.contains('reports-band-snapping'))
      .toBe(true);
    document.dispatchEvent(pointer('pointerup', 0, 110));
    const detail = () => manager.doc.bands.find((band) => band.id === 'detail');
    expect(detail().height).toBe('0.375 in');
    expect(page().querySelector('.reports-snap-line')).toBeNull();
    expect(Object.hasOwn(manager.doc, 'snap')).toBe(false);

    manager.elements.snapBtn.click();
    expect(manager.elements.snapBtn.getAttribute('aria-pressed')).toBe('false');
    expect(manager.elements.snapBtn.classList.contains('active')).toBe(false);
    const again = page().querySelector('[data-band-id="detail"] .reports-band-edge');
    again.dispatchEvent(pointer('pointerdown', 0, 100));
    document.dispatchEvent(pointer('pointermove', 0, 110));
    expect(page().querySelector('.reports-snap-line')).toBeNull();
    document.dispatchEvent(pointer('pointerup', 0, 110));
    expect(detail().height).toBe('0.479167 in');

    manager.elements.snapBtn.click();
    expect(manager.elements.snapBtn.getAttribute('aria-pressed')).toBe('true');
    manager.elements.textBtn.click();
    const node = page().querySelector('.reports-item');
    node.dispatchEvent(pointer('pointerdown', 0, 0));
    document.dispatchEvent(pointer('pointermove', 10, 0));
    const ghost = page().querySelector('.reports-snap-preview');
    expect(ghost).not.toBeNull();
    expect(ghost.style.left).toBe('324px');
    document.dispatchEvent(pointer('pointerup', 10, 0));
    expect(detail().items[0].box.left).toBe('3.375 in');
    expect(detail().items[0].box.top).toBe('0.125 in');
    expect(page().querySelector('.reports-snap-preview')).toBeNull();
    expect(JSON.stringify(manager.doc)).not.toMatch(/"snap"/);
  });
});
