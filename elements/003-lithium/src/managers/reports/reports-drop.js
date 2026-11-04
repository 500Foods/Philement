/**
 * Drag the Text or Shape button onto a band.
 * The button still adds a centered item. A drop places the same
 * rectangle on the band under the pointer and commits once.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { addItem, moveItem } from './model/document.js';
import { toCssPx } from './model/units.js';
import { readViewScale } from './scale.js';
import { AGGREGATE_NUMBER_TYPE } from './reports-aggregate.js';
import { PAGE_LABEL_DEFAULT, PAGE_LABEL_TYPE } from './reports-sample.js';
import { SHAPE_TYPE, defaultShapeFields } from './reports-shape.js';

export const TEXT_DROP_KIND = 'reports-text';
export const SHAPE_DROP_KIND = 'reports-shape';

/** Drag kind for a field button: `reports-field:<typeKey>`. */
export const FIELD_DROP_PREFIX = 'reports-field:';
const FIELD_DROP_TYPES = new Set([5, 6, 20, 21, 24, 35, 61, 62, 63, 64]);
const FIELD_GHOST = { 5: 'System date', 6: 'System user', 20: 'Text field', 21: 'Number field', 24: 'Datetime field', 35: 'Aggregate', 61: 'Page number', 62: 'Page count', 63: 'Parameter', 64: 'Page label' };
/**
 * Item a palette drop creates: fields bind `field`, Parameter (63) binds `param`.
 * System Date (5) and new datetime fields (24) start with timezone User and
 * no mask (the default applies); System User (6) takes no mask. Page Number
 * (61) and Page Count (62) start with no mask (plain integer); Page Label (64)
 * starts with template "Page {n} of {c}".
 */
export const dropItem = (typeKey) => {
  if (typeKey === 63) return { typeKey, param: '' };
  if (typeKey === 5) return { typeKey, timezone: 'User' };
  if (typeKey === 6 || typeKey === 61 || typeKey === 62) return { typeKey };
  if (typeKey === PAGE_LABEL_TYPE) return { typeKey, template: PAGE_LABEL_DEFAULT };
  if (typeKey === 24) return { typeKey, field: '', timezone: 'User' };
  if (typeKey === AGGREGATE_NUMBER_TYPE) return { typeKey, op: 'count', field: '' };
  return { typeKey, field: '' };
};

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
    this._fieldDragBtns = [...this.container.querySelectorAll('#reports-items-toolbar [data-reports-category="field"][data-type-key], #reports-items-toolbar [data-reports-category="aggregate"][data-type-key], #reports-items-toolbar [data-reports-category="system"][data-type-key]')];
    this._fieldDragBtns.forEach((btn) => {
      btn.addEventListener('dragstart', this._onTextDragStart);
      btn.addEventListener('dragend', this._onTextDragEnd);
    });
    textBtn.addEventListener('dragstart', this._onTextDragStart);
    textBtn.addEventListener('dragend', this._onTextDragEnd);
    this._shapeDragBtn = this.container.querySelector('#reports-shape-btn');
    this._shapeDragBtn?.addEventListener('dragstart', this._onTextDragStart);
    this._shapeDragBtn?.addEventListener('dragend', this._onTextDragEnd);
    stage.addEventListener('dragover', this._onTextDragOver);
    stage.addEventListener('drop', this._onTextDrop);
  },

  _unbindTextDrop() {
    const elements = this.elements || {};
    if (this._textGhost) this._textGhost.remove();
    this._textGhost = null;
    (this._fieldDragBtns || []).forEach((btn) => {
      btn.removeEventListener('dragstart', this._onTextDragStart);
      btn.removeEventListener('dragend', this._onTextDragEnd);
    });
    this._fieldDragBtns = null;
    elements.textBtn?.removeEventListener('dragstart', this._onTextDragStart);
    elements.textBtn?.removeEventListener('dragend', this._onTextDragEnd);
    this._shapeDragBtn?.removeEventListener('dragstart', this._onTextDragStart);
    this._shapeDragBtn?.removeEventListener('dragend', this._onTextDragEnd);
    this._shapeDragBtn = null;
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
    const typeKey = Number(event.currentTarget && event.currentTarget.dataset && event.currentTarget.dataset.typeKey);
    const isField = FIELD_DROP_TYPES.has(typeKey);
    const isShape = typeKey === SHAPE_TYPE;
    let dragKind = TEXT_DROP_KIND;
    let ghostText = 'Text';
    if (isField) {
      dragKind = `${FIELD_DROP_PREFIX}${typeKey}`;
      ghostText = FIELD_GHOST[typeKey];
    } else if (isShape) {
      dragKind = SHAPE_DROP_KIND;
      ghostText = 'Shape';
    }
    transfer.setData('text/plain', dragKind);
    transfer.effectAllowed = 'copy';
    const ghost = document.createElement('div');
    ghost.className = 'reports-drag-ghost';
    ghost.textContent = ghostText;
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
    const fieldType = typeof kind === 'string' && kind.startsWith(FIELD_DROP_PREFIX)
      ? Number(kind.slice(FIELD_DROP_PREFIX.length))
      : NaN;
    const isField = FIELD_DROP_TYPES.has(fieldType);
    const isShape = kind === SHAPE_DROP_KIND;
    if (kind !== TEXT_DROP_KIND && !isField && !isShape) return;
    event.preventDefault();
    const origin = textDropOrigin(
      body.getBoundingClientRect(),
      event.clientX,
      event.clientY,
      readViewScale(body),
    );
    let next;
    let addedId;
    let spec = { typeKey: 0 };
    if (isField) spec = dropItem(fieldType);
    else if (isShape) spec = { typeKey: SHAPE_TYPE, ...defaultShapeFields('rectangle') };
    try {
      next = addItem(this.doc, bandId, spec);
      const band = next.bands.find((entry) => entry.id === bandId);
      const added = band && band.items[band.items.length - 1];
      if (!added) return;
      addedId = added.id;
      next = moveItem(next, bandId, added.id, {
        left: `${origin.left} px`,
        top: `${origin.top} px`,
      });
    } catch (error) {
      this._failEdit(error, isShape ? 'Shape could not be added.' : 'Static text could not be added.');
      return;
    }
    this._selectedBandId = bandId;
    this._selectedItemId = addedId;
    this._commitDocument(next);
    let name = 'static text';
    if (isField) name = FIELD_GHOST[fieldType].toLowerCase();
    else if (isShape) name = 'shape';
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Dropped ${name} ${addedId} on ${bandId}`);
  },
};
