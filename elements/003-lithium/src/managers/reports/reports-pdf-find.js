/**
 * Search and outline for Preview PDF.
 * Search words stay in the field. They are not written to settings.
 * An outline URL does not leave the page.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import {
  destTarget,
  findHits,
  flattenOutline,
  itemsOverlapping,
  searchCountLabel,
  textFromItems,
} from './pdf-view.js';

export const reportPdfFindMethods = {
  async _collectText() {
    const pages = [];
    for (const shell of this.shells) {
      if (this.closed) return [];
      if (!shell.textContent) shell.textContent = await shell.proxy.getTextContent();
      const items = (shell.textContent.items || []).filter((item) => typeof item.str === 'string');
      shell.textItems = items;
      pages.push({ pageNumber: shell.pageNumber, text: textFromItems(items) });
    }
    return pages;
  },

  async _search() {
    if (!this.findInput || this.closed) return;
    const token = this.searchToken + 1;
    this.searchToken = token;
    const pages = await this._collectText();
    if (this.closed || token !== this.searchToken) return;
    this.hits = findHits(pages, this.findInput.value);
    this.hitIndex = this.hits.length ? 0 : -1;
    this._markHits();
    if (this.hitIndex >= 0) await this._revealHit();
  },

  _stepHit(delta) {
    if (!this.hits.length) return;
    const count = this.hits.length;
    const current = this.hitIndex >= 0 ? this.hitIndex : 0;
    this.hitIndex = (current + delta + count) % count;
    this._revealHit().catch((error) => this._fail(error));
  },

  async _revealHit() {
    const hit = this.hits[this.hitIndex];
    if (!hit) return;
    this._goTo(hit.page);
    await this.paintDone;
    if (this.closed) return;
    this._markHits();
    const shell = this.shells[hit.page - 1];
    const indexes = itemsOverlapping(shell?.textItems || [], hit.start, hit.length);
    const div = shell?.textLayer?.textDivs?.[indexes[0]];
    if (!div || !this.stack) return;
    const stackRect = this.stack.getBoundingClientRect();
    const rect = div.getBoundingClientRect();
    if (rect.top < stackRect.top + 8 || rect.bottom > stackRect.bottom - 8) {
      this._hold(hit.page);
      this.stack.scrollTop += rect.top - stackRect.top - 24;
    }
  },

  _markHits() {
    for (const shell of this.shells) {
      const divs = shell.textLayer?.textDivs || [];
      for (const div of divs) div.classList.remove('is-hit', 'is-hit-current');
    }
    this.hits.forEach((hit, index) => {
      const shell = this.shells[hit.page - 1];
      if (!shell?.textItems || !shell.textLayer) return;
      const current = index === this.hitIndex;
      for (const at of itemsOverlapping(shell.textItems, hit.start, hit.length)) {
        const div = shell.textLayer.textDivs[at];
        if (!div) continue;
        div.classList.add('is-hit');
        if (current) div.classList.add('is-hit-current');
      }
    });
    if (this.findCount) this.findCount.textContent = searchCountLabel(this.hitIndex, this.hits.length);
  },

  async _readOutline() {
    const tree = await this.doc.getOutline();
    if (this.closed) return;
    const rows = [];
    for (const entry of flattenOutline(tree)) {
      if (this.closed) return;
      const row = {
        title: entry.title,
        depth: entry.depth,
        page: 0,
        left: null,
        top: null,
        external: Boolean(entry.url),
      };
      if (!entry.url && entry.dest) {
        try {
          let dest = entry.dest;
          if (typeof dest === 'string') dest = await this.doc.getDestination(dest);
          if (Array.isArray(dest) && dest[0]) {
            const index = await this.doc.getPageIndex(dest[0]);
            const point = destTarget(dest);
            row.page = index + 1;
            row.left = point.left;
            row.top = point.top;
          }
        } catch (error) {
          const message = error instanceof Error ? error.message : 'Outline entry skipped.';
          log(Subsystems.MANAGER, Status.WARN, `[Reports] ${message}`);
        }
      }
      rows.push(row);
    }
    this.outline = rows;
    this._renderOutline();
  },

  _renderOutline() {
    if (!this.outlineList) return;
    this.outlineList.replaceChildren();
    if (this.outlineEmpty) this.outlineEmpty.hidden = this.outline.length > 0;
    for (const row of this.outline) {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'reports-pdf-outline-item';
      if (row.external) button.classList.add('is-external');
      button.textContent = row.title;
      button.dataset.page = String(row.page);
      button.style.setProperty('--pdf-indent', String(row.depth));
      button.addEventListener('click', () => {
        if (row.external || !(row.page >= 1)) return;
        this._goTo(row.page, { left: row.left, top: row.top });
      }, { signal: this.abort.signal });
      this.outlineList.append(button);
    }
    this._markCurrent();
  },
};
