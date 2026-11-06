/**
 * Thumbnail strip for Preview PDF.
 * Every page uses one cell. The page is contained in that cell
 * and the number sits on the bottom row. 100% is one row as
 * tall as the framed body, so the row has no gap around it.
 */

import {
  THUMB_NUMBER_PX,
  clampColumns,
  stepThumbPercent,
  thumbCellSize,
  thumbContain,
  visibleRange,
} from './pdf-view.js';
import { paintThumb } from './reports-pdf-render.js';

export const reportPdfThumbMethods = {
  _dropThumbs() {
    for (const thumb of this.thumbButtons) {
      thumb.ready = false;
      thumb.key = '';
    }
  },

  /**
   * Cell size follows the framed body. A change throws the bitmaps away.
   * @returns {boolean} true when the cell size changed
   */
  _fitThumbs() {
    const body = this.thumbBody;
    if (!this.host || !body || !this.settings.thumbsOpen) return false;
    const width = body.clientWidth;
    const height = body.clientHeight;
    if (!(width > 0) || !(height > 0)) return false;
    const cell = thumbCellSize(width, height, this.settings.columns, this.settings.thumbPercent);
    const mark = `${cell.width}x${cell.height}`;
    const changed = mark !== this.thumbMark;
    this.thumbMark = mark;
    this.thumbCell = cell;
    this.host.style.setProperty('--pdf-thumb-h', `${cell.height}px`);
    if (changed) this._dropThumbs();
    for (const thumb of this.thumbButtons) {
      const base = this._viewport(thumb.proxy, 1);
      const box = thumbContain(base.width, base.height, cell.width, cell.height, THUMB_NUMBER_PX);
      thumb.box = box;
      thumb.button.style.setProperty('--pdf-page-w', `${box.width}px`);
      thumb.button.style.setProperty('--pdf-page-h', `${box.height}px`);
    }
    return changed;
  },

  async _paintThumbs(alive) {
    const scroller = this.thumbBody;
    if (!this.settings.thumbsOpen || !this.thumbButtons.length || !scroller) return;
    this._fitThumbs();
    const cell = this.thumbCell;
    if (!cell || !(cell.height > 0)) return;
    const tops = this.thumbButtons.map((thumb) => thumb.button.offsetTop);
    const heights = this.thumbButtons.map((thumb) => thumb.button.offsetHeight || cell.height);
    const range = visibleRange(scroller.scrollTop, scroller.clientHeight, tops, heights);
    const jobs = [];
    for (let index = 0; index < this.thumbButtons.length; index += 1) {
      const thumb = this.thumbButtons[index];
      if (index < range.start || index > range.end) continue;
      const box = thumb.box;
      if (!box || !(box.width > 0)) continue;
      const key = `${Math.round(box.width)}x${Math.round(box.height)}:${this.settings.rotate}`;
      if (thumb.ready && thumb.key === key) continue;
      const viewport = this._viewport(thumb.proxy, box.scale);
      jobs.push(paintThumb(thumb, viewport, alive).then((ok) => {
        if (ok && alive()) thumb.key = key;
      }).catch((error) => this._paintError(error, alive)));
    }
    await Promise.all(jobs);
  },

  _columns(direction) {
    const next = clampColumns(this.settings.columns + direction);
    if (next === this.settings.columns) return;
    this.settings.columns = next;
    this._dropThumbs();
    this._save();
    this._fitThumbs();
    this._syncChrome();
    this._schedulePaint();
  },

  _thumbStep(direction) {
    const next = stepThumbPercent(this.settings.thumbPercent, direction);
    if (next === this.settings.thumbPercent) return;
    this.settings.thumbPercent = next;
    this._dropThumbs();
    this._save();
    this._fitThumbs();
    this._syncChrome();
    this._schedulePaint();
  },

  _toggleThumbs() {
    this.settings.thumbsOpen = !this.settings.thumbsOpen;
    this._save();
    this._syncChrome();
    if (!this.settings.thumbsOpen) return;
    requestAnimationFrame(() => {
      if (this.closed) return;
      this._fitThumbs();
      this._schedulePaint();
    });
  },
};
