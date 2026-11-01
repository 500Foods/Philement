/**
 * Pointer editing for the report page.
 * A band edge resizes the band. An item moves, and its corner
 * resizes it. Both commit on pointerup. A band click selects
 * that band and clears the item.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';
import { dragBandHeight } from './bands.js';
import { bandItemBounds, dragItemBox, itemBox } from './items.js';
import { bandHeight, moveItem, resizeBand, resizeItem } from './model/document.js';
import { hideRulerSpan, pageSpan, setRulerLength, showRulerSpan, trackReportCursor } from './rulers.js';
import { readViewScale } from './scale.js';
import { boxesNear, snapBandHeight, snapItemBox, snapSpacingPx } from './snap.js';
import { toCssPx } from './model/units.js';

function setToolbarToggle(button, on) {
  if (!button) return;
  button.setAttribute('aria-pressed', on ? 'true' : 'false');
  button.classList.toggle('active', on);
}

export const reportPointerMethods = {
  _bindSnap() {
    const button = this.elements.snapBtn;
    if (!button) return;
    if (this._snap === undefined) this._snap = true;
    setToolbarToggle(button, this._snap);
    this._onSnap = () => {
      this._snap = !this._snap;
      setToolbarToggle(button, this._snap);
      if (!this._snap) this._clearSnapPreview();
    };
    button.addEventListener('click', this._onSnap);
  },

  _unbindSnap() {
    const button = this.elements && this.elements.snapBtn;
    if (button && this._onSnap) button.removeEventListener('click', this._onSnap);
    this._onSnap = null;
    this._clearSnapPreview();
  },

  _syncGridVisible() {
    const stage = this.elements && this.elements.stage;
    if (stage) stage.classList.toggle('reports-grid-off', this._gridVisible === false);
    setToolbarToggle(this.elements && this.elements.gridBtn, this._gridVisible !== false);
  },

  _bindGrid() {
    const button = this.elements.gridBtn;
    if (!button) return;
    if (this._gridVisible === undefined) this._gridVisible = true;
    this._syncGridVisible();
    this._onGrid = () => {
      this._gridVisible = !this._gridVisible;
      this._syncGridVisible();
    };
    button.addEventListener('click', this._onGrid);
  },

  _unbindGrid() {
    const button = this.elements && this.elements.gridBtn;
    if (button && this._onGrid) button.removeEventListener('click', this._onGrid);
    this._onGrid = null;
  },

  _snapSpacing() {
    if (this._snap === false || !this.doc) return 0;
    return snapSpacingPx(this.doc);
  },

  _clearSnapPreview() {
    const stage = this.elements && this.elements.stage;
    if (!stage) return;
    stage.querySelectorAll('.reports-snap-preview, .reports-snap-line').forEach((node) => node.remove());
    stage.querySelectorAll('.reports-band-snapping').forEach((node) => {
      node.classList.remove('reports-band-snapping');
    });
  },

  _showBandSnap(body, livePx, snappedPx) {
    this._clearSnapPreview();
    if (!body || !(snappedPx > 0) || Math.abs(snappedPx - livePx) < 0.5) return;
    const line = document.createElement('div');
    line.className = 'reports-snap-line';
    line.style.top = `${snappedPx}px`;
    body.append(line);
    const band = body.closest('.reports-band');
    if (band) band.classList.add('reports-band-snapping');
  },

  _showItemSnap(body, live, snapped) {
    this._clearSnapPreview();
    if (!body || !snapped || boxesNear(live, snapped)) return;
    const ghost = document.createElement('div');
    ghost.className = 'reports-snap-preview';
    ghost.style.left = `${snapped.left}px`;
    ghost.style.top = `${snapped.top}px`;
    ghost.style.width = `${snapped.width}px`;
    ghost.style.height = `${snapped.height}px`;
    body.append(ghost);
  },

  _beginPointer(event) {
    if (this._drag || !this.doc) return;
    if (event.button !== undefined && event.button !== 0) return;
    const stage = this.elements.stage;
    const target = event.target;
    if (!target || !target.closest || !stage || !stage.contains(target)) return;
    const itemEl = target.closest('.reports-item');
    if (itemEl) {
      if (!this._recordEditing) {
        this._selectPointerItem(itemEl);
        return;
      }
      this._beginItemDrag(event, itemEl, Boolean(target.closest('.reports-item-corner')));
      return;
    }
    if (target.closest('.reports-band-edge')) {
      if (!this._recordEditing) return;
      this._beginBandDrag(event);
      return;
    }
    const bandEl = target.closest('.reports-band');
    if (bandEl && bandEl.dataset.bandId) {
      this._selectedBandId = bandEl.dataset.bandId;
      this._selectedItemId = '';
      this._markSelection();
    }
  },

  _selectPointerItem(itemEl) {
    const bandEl = itemEl.closest('.reports-band');
    const bandId = bandEl && bandEl.dataset.bandId;
    const itemId = itemEl.dataset.itemId;
    if (!bandId || !itemId) return;
    this._selectedBandId = bandId;
    this._selectedItemId = itemId;
    this._markSelection();
  },

  _clearItemDrag() {
    this._clearSnapPreview();
    const drag = this._drag;
    if (drag && drag.page && (drag.kind === 'move' || drag.kind === 'resize')) {
      hideRulerSpan(drag.page, 'x');
      hideRulerSpan(drag.page, 'y');
    }
    if (this._onItemMove) {
      document.removeEventListener('pointermove', this._onItemMove);
      document.removeEventListener('pointerup', this._onItemUp);
      document.removeEventListener('pointercancel', this._onItemUp);
    }
    this._onItemMove = null;
    this._onItemUp = null;
    if (this._drag && (this._drag.kind === 'move' || this._drag.kind === 'resize')) this._drag = null;
  },

  _beginItemDrag(event, itemEl, resize) {
    if (!this._recordEditing) return;
    const bandEl = itemEl.closest('.reports-band');
    const bandId = bandEl && bandEl.dataset.bandId;
    const itemId = itemEl.dataset.itemId;
    if (!bandId || !itemId) return;
    this._selectedBandId = bandId;
    this._selectedItemId = itemId;
    this._markSelection();
    const left = Number.parseFloat(itemEl.style.left);
    const top = Number.parseFloat(itemEl.style.top);
    const width = Number.parseFloat(itemEl.style.width);
    const height = Number.parseFloat(itemEl.style.height);
    if (![left, top, width, height].every(Number.isFinite)) return;
    const band = this.doc.bands.find((entry) => entry.id === bandId);
    let bounds;
    try {
      bounds = bandItemBounds(this.doc, band);
    } catch (error) {
      this._failEdit(error, 'Item could not be moved.');
      return;
    }
    event.preventDefault();
    this._drag = {
      kind: resize ? 'resize' : 'move',
      bandId,
      itemId,
      itemEl,
      box: { left, top, width, height },
      bounds,
      scale: readViewScale(itemEl),
      startX: event.clientX,
      startY: event.clientY,
    };
    this._onItemMove = (move) => this._moveItemDrag(move);
    this._onItemUp = () => this._endItemDrag();
    document.addEventListener('pointermove', this._onItemMove);
    document.addEventListener('pointerup', this._onItemUp);
    document.addEventListener('pointercancel', this._onItemUp);
  },

  _moveItemDrag(event) {
    const drag = this._drag;
    if (!drag || drag.kind === 'band') return;
    const scale = drag.scale > 0 ? drag.scale : 1;
    const dx = (event.clientX - drag.startX) / scale;
    const dy = (event.clientY - drag.startY) / scale;
    const preview = dragItemBox(drag.box, dx, dy, drag.bounds, drag.kind === 'resize');
    drag.preview = preview;
    drag.itemEl.style.left = `${preview.left}px`;
    drag.itemEl.style.top = `${preview.top}px`;
    drag.itemEl.style.width = `${preview.width}px`;
    drag.itemEl.style.height = `${preview.height}px`;
    const page = drag.itemEl.closest('#reports-page');
    const body = drag.itemEl.closest('.reports-band-body');
    if (!page || !body) return;
    drag.page = page;
    const span = pageSpan(drag.itemEl.getBoundingClientRect(), body.getBoundingClientRect(), drag.scale);
    showRulerSpan(body, 'x', span.x0, span.x1);
    showRulerSpan(body, 'y', span.y0, span.y1);
    trackReportCursor(page, event, body);
    const spacing = this._snapSpacing();
    if (!(spacing > 0)) {
      this._clearSnapPreview();
      return;
    }
    this._showItemSnap(
      body,
      preview,
      snapItemBox(preview, spacing, drag.bounds, drag.kind === 'resize'),
    );
  },

  _endItemDrag() {
    const drag = this._drag;
    this._clearItemDrag();
    if (!drag || drag.kind === 'band' || this._closed || !this.doc || !drag.preview) return;
    const spacing = this._snapSpacing();
    const box = spacing > 0
      ? snapItemBox(drag.preview, spacing, drag.bounds, drag.kind === 'resize')
      : drag.preview;
    let next;
    try {
      if (drag.kind === 'resize') {
        next = resizeItem(this.doc, drag.bandId, drag.itemId, {
          width: `${box.width} px`,
          height: `${box.height} px`,
        });
      } else {
        next = moveItem(this.doc, drag.bandId, drag.itemId, {
          left: `${box.left} px`,
          top: `${box.top} px`,
        });
      }
    } catch (error) {
      this._failEdit(error, 'Item could not be moved.');
      this._showDocument(this.doc);
      return;
    }
    const before = itemBox(this.doc, drag.bandId, drag.itemId);
    const after = itemBox(next, drag.bandId, drag.itemId);
    if (before && after && before.left === after.left && before.top === after.top
      && before.width === after.width && before.height === after.height) {
      this._showDocument(this.doc);
      return;
    }
    this._commitDocument(next);
    const placed = after || {};
    log(Subsystems.MANAGER, Status.INFO, `[Reports] ${drag.kind === 'resize' ? 'Resized' : 'Moved'} ${drag.itemId} to ${placed.left}, ${placed.top}, ${placed.width} by ${placed.height}`);
  },

  _clearBandDrag() {
    this._clearSnapPreview();
    if (this._drag && this._drag.page) hideRulerSpan(this._drag.page, 'y');
    if (this._onBandMove) {
      document.removeEventListener('pointermove', this._onBandMove);
      document.removeEventListener('pointerup', this._onBandUp);
      document.removeEventListener('pointercancel', this._onBandUp);
    }
    this._onBandMove = null;
    this._onBandUp = null;
    this._drag = null;
  },

  _beginBandDrag(event) {
    if (!this._recordEditing || this._drag || !this.doc || !Array.isArray(this.doc.bands)) return;
    if (event.button !== undefined && event.button !== 0) return;
    const edge = event.target && event.target.closest && event.target.closest('.reports-band-edge');
    if (!edge || !this.elements.stage || !this.elements.stage.contains(edge)) return;
    const bandEl = edge.closest('.reports-band');
    const id = bandEl && bandEl.dataset.bandId;
    const body = bandEl && bandEl.querySelector('.reports-band-body');
    const band = id && this.doc.bands.find((entry) => entry.id === id);
    if (!band || !body) return;
    let startPx;
    try {
      startPx = toCssPx(bandHeight(band));
    } catch {
      return;
    }
    event.preventDefault();
    const page = body.closest('#reports-page');
    const scale = readViewScale(page || body);
    this._drag = {
      kind: 'band', id, startPx, startY: event.clientY, body, page, bandEl, scale,
    };
    this._onBandMove = (move) => this._moveBandDrag(move);
    this._onBandUp = () => this._endBandDrag();
    document.addEventListener('pointermove', this._onBandMove);
    document.addEventListener('pointerup', this._onBandUp);
    document.addEventListener('pointercancel', this._onBandUp);
  },

  _moveBandDrag(event) {
    if (!this._drag) return;
    const scale = this._drag.scale > 0 ? this._drag.scale : 1;
    const next = dragBandHeight(this._drag.startPx, (event.clientY - this._drag.startY) / scale);
    this._drag.previewPx = next;
    this._drag.body.style.height = `${next}px`;
    const ruler = this._drag.bandEl && this._drag.bandEl.querySelector('.reports-ruler-left');
    setRulerLength(ruler, next);
    const spacing = this._snapSpacing();
    if (spacing > 0) {
      this._showBandSnap(this._drag.body, next, snapBandHeight(next, spacing, toCssPx('0.15 in')));
    } else {
      this._clearSnapPreview();
    }
    if (!this._drag.page) return;
    showRulerSpan(this._drag.bandEl, 'y', this._drag.startPx, next);
    trackReportCursor(this._drag.page, event, this._drag.body);
  },

  _endBandDrag() {
    const drag = this._drag;
    this._clearBandDrag();
    if (!drag || this._closed || !this.doc) return;
    if (drag.previewPx === undefined) return;
    const spacing = this._snapSpacing();
    const heightPx = spacing > 0
      ? snapBandHeight(drag.previewPx, spacing, toCssPx('0.15 in'))
      : drag.previewPx;
    let next;
    try {
      next = resizeBand(this.doc, drag.id, `${heightPx} px`);
    } catch (error) {
      const message = error instanceof Error && error.message
        ? error.message
        : 'Band could not be resized.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error(message, { subsystem: 'Manager' });
      this._showDocument(this.doc);
      return;
    }
    const before = this.doc.bands.find((entry) => entry.id === drag.id);
    const after = next.bands.find((entry) => entry.id === drag.id);
    if (before && after && before.height === after.height) {
      this._showDocument(this.doc);
      return;
    }
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Resized ${drag.id} to ${after.height}`);
  }
};
