/**
 * Drag the Text button onto a band.
 * The button still adds a centered item. A drop places the same
 * rectangle on the band under the pointer and commits once.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { addItem, moveItem } from './model/document.js';
import { toCssPx } from './model/units.js';
import { readViewScale } from './scale.js';

export const TEXT_DROP_KIND = 'reports-text';

/**
 * Top-left of a default text box centered on the pointer.
 * Coordinates are CSS pixels in the band body, before view scale.
 * @param {{ left: number, top: number }} bodyRect
 * @param {number} clientX
 * @param {number} clientY
 * @param {number} scale
 * @returns {{ left: number, top: number }}
 */
export function textDropOrigin(bodyRect, clientX, clientY, scale) {
  const factor = scale > 0 ? scale : 1;
  const width = toCssPx('1.5 in');
  const height = toCssPx('0.2 in');
  return {
    left: (clientX - bodyRect.left) / factor - width / 2,
    top: (clientY - bodyRect.top) / factor - height / 2,
  };
}

export const reportDropMethods = {
  _bindTextDrop() {
    const { textBtn, stage } = this.elements;
    this._onTextDragStart = (event) => this._startTextDrag(event);
    this._onTextDragEnd = () => this._endTextDrag();
    this._onTextDragOver = (event) => this._overTextDrag(event);
    this._onTextDrop = (event) => this._dropTextDrag(event);
    textBtn.addEventListener('dragstart', this._onTextDragStart);
    textBtn.addEventListener('dragend', this._onTextDragEnd);
    stage.addEventListener('dragover', this._onTextDragOver);
    stage.addEventListener('drop', this._onTextDrop);
  },

  _unbindTextDrop() {
    const elements = this.elements || {};
    if (this._textGhost) this._textGhost.remove();
    this._textGhost = null;
    elements.textBtn?.removeEventListener('dragstart', this._onTextDragStart);
    elements.textBtn?.removeEventListener('dragend', this._onTextDragEnd);
    elements.stage?.removeEventListener('dragover', this._onTextDragOver);
    elements.stage?.removeEventListener('drop', this._onTextDrop);
  },

  _startTextDrag(event) {
    if (!this._recordEditing || !this.doc) {
      event.preventDefault();
      return;
    }
    this._textDragged = true;
    const transfer = event.dataTransfer;
    if (!transfer) return;
    transfer.setData('text/plain', TEXT_DROP_KIND);
    transfer.effectAllowed = 'copy';
    const ghost = document.createElement('div');
    ghost.className = 'reports-drag-ghost';
    ghost.textContent = 'Text';
    document.body.append(ghost);
    if (typeof transfer.setDragImage === 'function') transfer.setDragImage(ghost, 72, 10);
    this._textGhost = ghost;
  },

  _endTextDrag() {
    if (this._textGhost) this._textGhost.remove();
    this._textGhost = null;
    this._textDragged = true;
    setTimeout(() => {
      this._textDragged = false;
    }, 0);
  },

  _overTextDrag(event) {
    if (!this._recordEditing) return;
    const band = event.target && event.target.closest && event.target.closest('.reports-band');
    if (!band) return;
    event.preventDefault();
    if (event.dataTransfer) event.dataTransfer.dropEffect = 'copy';
  },

  _dropTextDrag(event) {
    if (!this._recordEditing || this._drag || this._closed || !this.doc) return;
    const bandEl = event.target && event.target.closest && event.target.closest('.reports-band');
    const body = bandEl && bandEl.querySelector('.reports-band-body');
    const bandId = bandEl && bandEl.dataset.bandId;
    if (!bandId || !body) return;
    const kind = event.dataTransfer && event.dataTransfer.getData('text/plain');
    if (kind !== TEXT_DROP_KIND) return;
    event.preventDefault();
    const origin = textDropOrigin(
      body.getBoundingClientRect(),
      event.clientX,
      event.clientY,
      readViewScale(body),
    );
    let next;
    let addedId;
    try {
      next = addItem(this.doc, bandId, { typeKey: 0 });
      const band = next.bands.find((entry) => entry.id === bandId);
      const added = band && band.items[band.items.length - 1];
      if (!added) return;
      addedId = added.id;
      next = moveItem(next, bandId, added.id, {
        left: `${origin.left} px`,
        top: `${origin.top} px`,
      });
    } catch (error) {
      this._failEdit(error, 'Static text could not be added.');
      return;
    }
    this._selectedBandId = bandId;
    this._selectedItemId = addedId;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Dropped static text ${addedId} on ${bandId}`);
  },
};
