/**
 * Preview PDF page.
 * Loads the fixture until a report returns pdf_base64.
 * /Rotate is which side is up. Quarter-turns and flips sit on top.
 * Fit page is its own scale. Numeric 100% is 96 CSS pixels per inch.
 */

import './reports-pdf.css';

import { log, Subsystems, Status } from '../../core/log.js';
import { promptDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import { wheelViewScaleDirection, wheelZoomHeld } from './scale.js';
import {
  PDF_CSS_PER_POINT,
  PDF_FIXTURE_NAME,
  PDF_FIXTURE_URL,
  PDF_PAGE_GAP,
  addQuarter,
  composedRotation,
  flipBoth,
  loadPdfSettings,
  marqueeLocalRect,
  moveThumb,
  pageOccupyingPane,
  pdfFitPageScale,
  pdfFitWidthScale,
  pdfPageKey,
  rememberedPage,
  savePdfSettings,
  scaleAfterZoom,
  scaleLabel,
  searchCountLabel,
  stackScroll,
  toggleFit,
  viewerKeyAction,
  visibleRange,
} from './pdf-view.js';
import {
  getDocument,
  PasswordResponses,
  marqueeBlob,
  paintPage,
  releasePaint,
} from './reports-pdf-render.js';
import { reportPdfFindMethods } from './reports-pdf-find.js';
import { reportPdfThumbMethods } from './reports-pdf-thumbs.js';

const MIN_MARQUEE = 8;

export class ReportPdfPage {
  /**
   * @param {object} manager
   */
  constructor(manager) {
    this.manager = manager;
    this.closed = false;
    this.doc = null;
    this.bytes = null;
    this.task = null;
    this.shells = [];
    this.thumbButtons = [];
    this.layouts = [];
    this.outline = [];
    this.hits = [];
    this.hitIndex = -1;
    this.page = 1;
    this.displayedScale = 1;
    this.generation = 0;
    this.holdPage = 0;
    this.holdToken = 0;
    this.paintQueued = false;
    this.paintDone = Promise.resolve();
    this.loading = null;
    this.needsPassword = false;
    this.seenW = 0;
    this.seenH = 0;
    this.hand = null;
    this.drag = null;
    this.searchToken = 0;
    this.paintWarned = false;
    this.printFrame = null;
    this.printUrl = '';
    this.blobUrls = [];
    this.settings = loadPdfSettings(window.lithiumSettings);
    this.abort = new AbortController();
    const root = manager.container;
    this.toolbar = root.querySelector('#reports-pdf-toolbar');
    this.host = root.querySelector('#reports-pdf');
    this.stack = root.querySelector('#reports-pdf-stack');
    this.track = root.querySelector('#reports-pdf-track');
    this.thumbs = root.querySelector('#reports-pdf-thumbs');
    this.thumbBody = root.querySelector('#reports-pdf-thumb-body');
    this.thumbGrid = root.querySelector('#reports-pdf-thumb-grid');
    this.thumbScale = root.querySelector('#reports-pdf-thumb-scale');
    this.thumbCell = null;
    this.thumbMark = '';
    this.outlinePane = root.querySelector('#reports-pdf-outline');
    this.outlineList = root.querySelector('#reports-pdf-outline-list');
    this.outlineEmpty = root.querySelector('#reports-pdf-outline-empty');
    this.note = root.querySelector('#reports-pdf-note');
    this.pageInput = root.querySelector('#reports-pdf-page-input');
    this.pageCount = root.querySelector('#reports-pdf-page-count');
    this.scaleReadout = root.querySelector('#reports-pdf-scale');
    this.colsReadout = root.querySelector('#reports-pdf-cols');
    this.findBox = root.querySelector('#reports-pdf-find');
    this.findInput = root.querySelector('#reports-pdf-find-input');
    this.findCount = root.querySelector('#reports-pdf-find-count');
    this.resize = null;
    this._bind();
    this._applyVars();
    this._syncChrome();
  }

  destroy() {
    this.closed = true;
    this.generation += 1;
    this.abort.abort();
    if (this.resize) this.resize.disconnect();
    this.resize = null;
    for (const shell of this.shells) releasePaint(shell);
    for (const thumb of this.thumbButtons) this._releaseThumb(thumb);
    if (this.doc) this.doc.destroy();
    this.doc = null;
    if (this.task) {
      this.task.destroy();
      this.task = null;
    }
    this._clearPrint();
    for (const url of this.blobUrls) URL.revokeObjectURL(url);
    this.blobUrls = [];
  }

  async open() {
    if (this.closed) return;
    try {
      if (this.doc) {
        this._whenSized(() => this._showCurrent());
        return;
      }
      if (!this.loading) {
        this.loading = this._load().finally(() => {
          this.loading = null;
        });
      }
      await this.loading;
    } catch (error) {
      this._fail(error);
    }
  }

  async _load() {
    this.needsPassword = false;
    this._showNote('');
    try {
      const response = await fetch(PDF_FIXTURE_URL);
      if (this.closed) return;
      if (!response.ok) throw new Error(`Preview PDF returned ${response.status}.`);
      const bytes = new Uint8Array(await response.arrayBuffer());
      if (this.closed) return;
      this.bytes = bytes;
      const task = getDocument({ data: bytes.slice() });
      this.task = task;
      task.onPassword = (update, reason) => {
        this._askPassword(update, reason);
      };
      const doc = await task.promise;
      if (this.closed) {
        doc.destroy();
        return;
      }
      this.doc = doc;
      this._resetShells();
      for (let number = 1; number <= doc.numPages; number += 1) {
        const proxy = await doc.getPage(number);
        if (this.closed) return;
        this.shells.push(this._makeShell(proxy, number));
        this.thumbButtons.push(this._makeThumb(proxy, number));
      }
      await this._readOutline();
      if (this.closed) return;
      this.page = rememberedPage(this.settings, this._pageKey(), doc.numPages);
      this._showNote('');
      this._whenSized(() => this._showCurrent());
    } catch (error) {
      if (this.closed) return;
      this._resetShells();
      this.doc = null;
      if (this.needsPassword) {
        this._showNote('This PDF needs a password.');
        return;
      }
      this._fail(error);
    }
  }

  _askPassword(update, reason) {
    this._promptPassword(update, reason).catch((error) => {
      if (this.closed) return;
      this.needsPassword = true;
      if (this.task) this.task.destroy();
      this._fail(error);
    });
  }

  async _promptPassword(update, reason) {
    const wrong = reason === PasswordResponses.INCORRECT_PASSWORD;
    const value = await promptDialog({
      title: 'PDF password',
      message: wrong ? 'That password did not open this PDF.' : 'This PDF needs a password.',
      confirmLabel: 'Open',
      cancelLabel: 'Cancel',
    });
    if (this.closed) {
      if (this.task) this.task.destroy();
      return;
    }
    if (value == null) {
      this.needsPassword = true;
      if (this.task) this.task.destroy();
      return;
    }
    update(String(value));
  }

  _bind() {
    const signal = this.abort.signal;
    const click = (id, fn) => {
      const el = this._el(id);
      if (el) el.addEventListener('click', fn, { signal });
    };
    click('#reports-pdf-thumbs-btn', () => this._toggleThumbs());
    click('#reports-pdf-cols-less', () => this._columns(-1));
    click('#reports-pdf-cols-more', () => this._columns(1));
    click('#reports-pdf-thumb-smaller', () => this._thumbStep(-1));
    click('#reports-pdf-thumb-larger', () => this._thumbStep(1));
    click('#reports-pdf-first', () => this._goTo(1));
    click('#reports-pdf-prev', () => this._goTo(this.page - 1));
    click('#reports-pdf-next', () => this._goTo(this.page + 1));
    click('#reports-pdf-last', () => this._goTo(this.shells.length));
    click('#reports-pdf-zoom-out', () => this._zoom(-1));
    click('#reports-pdf-zoom-in', () => this._zoom(1));
    click('#reports-pdf-fit-page', () => this._fit('fit-page'));
    click('#reports-pdf-fit-width', () => this._fit('fit-width'));
    click('#reports-pdf-rotate-left', () => this._turn(-90));
    click('#reports-pdf-rotate-right', () => this._turn(90));
    click('#reports-pdf-flip-h', () => this._flip('h'));
    click('#reports-pdf-flip-v', () => this._flip('v'));
    click('#reports-pdf-flip-both', () => this._flip('both'));
    click('#reports-pdf-find-btn', () => this._toggleSearch());
    click('#reports-pdf-find-prev', () => this._stepHit(-1));
    click('#reports-pdf-find-next', () => this._stepHit(1));
    click('#reports-pdf-outline-btn', () => this._toggleOutline());
    click('#reports-pdf-download', () => this._download());
    click('#reports-pdf-print', () => this._print());
    click('#reports-pdf-pointer-text', () => this._pointer('text'));
    click('#reports-pdf-pointer-hand', () => this._pointer('hand'));
    click('#reports-pdf-pointer-marquee', () => this._pointer('marquee'));
    if (this.pageInput) {
      this.pageInput.addEventListener('change', () => this._commitPage(), { signal });
      this.pageInput.addEventListener('keydown', (event) => {
        if (event.key !== 'Enter') return;
        event.preventDefault();
        this._commitPage();
      }, { signal });
    }
    if (this.findInput) {
      this.findInput.addEventListener('input', () => {
        this._search().catch((error) => this._fail(error));
      }, { signal });
      this.findInput.addEventListener('keydown', (event) => {
        if (event.key !== 'Enter') return;
        event.preventDefault();
        this._stepHit(event.shiftKey ? -1 : 1);
      }, { signal });
    }
    if (this.stack) {
      this.stack.addEventListener('scroll', () => this._onScroll(), { signal });
      this.stack.addEventListener('wheel', (event) => this._onWheel(event), { passive: false, signal });
      this.stack.addEventListener('dblclick', (event) => this._onDouble(event), { signal });
      this.stack.addEventListener('pointerdown', (event) => this._pointerDown(event), { signal });
      this.stack.addEventListener('pointermove', (event) => this._pointerMove(event), { signal });
      this.stack.addEventListener('pointerup', (event) => this._pointerUp(event), { signal });
      this.stack.addEventListener('pointercancel', (event) => this._pointerUp(event), { signal });
    }
    if (this.thumbBody) this.thumbBody.addEventListener('scroll', () => this._schedulePaint(), { signal });
    document.addEventListener('keydown', (event) => this._onKey(event), { signal });
    if (this.stack && typeof ResizeObserver === 'function') {
      this.resize = new ResizeObserver(() => this._onResize());
      this.resize.observe(this.stack);
      if (this.thumbBody) this.resize.observe(this.thumbBody);
    }
  }

  _el(id) {
    return this.manager.container.querySelector(id);
  }

  _fail(error) {
    if (this.closed || this.needsPassword) return;
    const message = error instanceof Error ? error.message : 'Preview PDF failed.';
    log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
    this._showNote(message);
    toast.error('Preview PDF failed', { description: message, subsystem: 'Manager' });
  }

  _showNote(text) {
    if (!this.note) return;
    this.note.hidden = !text;
    this.note.textContent = text || '';
  }

  _pageKey() {
    return pdfPageKey(this.manager._openReportId);
  }

  _save() {
    savePdfSettings(window.lithiumSettings, this.settings);
  }

  _rememberPage() {
    const key = this._pageKey();
    if (this.settings.pages[key] === this.page) return;
    this.settings.pages[key] = this.page;
    this._save();
  }

  _viewport(proxy, scale) {
    return proxy.getViewport({
      scale: scale * PDF_CSS_PER_POINT,
      rotation: composedRotation(proxy.rotate, this.settings.rotate),
    });
  }

  _currentScale() {
    if (this.settings.mode === 'scale') return this.settings.scale;
    const shell = this.shells[this.page - 1];
    if (!shell || !this.stack) return 1;
    const base = this._viewport(shell.proxy, 1);
    if (this.settings.mode === 'fit-width') {
      return pdfFitWidthScale(base.width, this.stack.clientWidth, PDF_PAGE_GAP);
    }
    return pdfFitPageScale(base.width, base.height, this.stack.clientWidth, this.stack.clientHeight, PDF_PAGE_GAP);
  }

  _whenSized(fn) {
    const sized = this.stack && this.stack.clientWidth > 0 && this.stack.clientHeight > 0;
    if (sized) {
      fn();
      return;
    }
    requestAnimationFrame(() => {
      if (!this.closed) fn();
    });
  }

  _showCurrent() {
    this._relayout();
    this._goTo(this.page);
    this.seenW = this.stack.clientWidth;
    this.seenH = this.stack.clientHeight;
    const active = document.activeElement;
    if (!active || active === document.body || active.id === 'reports-pdf-tab') this.stack.focus();
  }

  _resetShells() {
    for (const shell of this.shells) releasePaint(shell);
    for (const thumb of this.thumbButtons) this._releaseThumb(thumb);
    this.shells = [];
    this.thumbButtons = [];
    this.layouts = [];
    if (this.track) this.track.replaceChildren();
    if (this.thumbGrid) this.thumbGrid.replaceChildren();
  }

  _releaseThumb(thumb) {
    const task = thumb.renderTask;
    thumb.renderTask = null;
    if (!task) return;
    task.cancel();
    task.promise?.catch(() => {});
  }

  _makeShell(proxy, pageNumber) {
    const el = document.createElement('div');
    el.className = 'reports-pdf-page';
    const sheet = document.createElement('div');
    sheet.className = 'reports-pdf-sheet';
    const canvas = document.createElement('canvas');
    canvas.className = 'reports-pdf-canvas';
    const textEl = document.createElement('div');
    textEl.className = 'textLayer reports-pdf-text';
    const marquee = document.createElement('div');
    marquee.className = 'reports-pdf-marquee';
    marquee.hidden = true;
    sheet.append(canvas, textEl, marquee);
    el.append(sheet);
    this.track.append(el);
    return {
      proxy,
      pageNumber,
      el,
      sheet,
      canvas,
      textEl,
      marquee,
      textLayer: null,
      textContent: null,
      textItems: [],
      renderTask: null,
      ready: false,
      key: '',
    };
  }

  _makeThumb(proxy, pageNumber) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'reports-pdf-thumb';
    button.title = `Page ${pageNumber}`;
    const sheet = document.createElement('span');
    sheet.className = 'reports-pdf-thumb-sheet';
    const canvas = document.createElement('canvas');
    const num = document.createElement('span');
    num.className = 'reports-pdf-thumb-num';
    num.textContent = String(pageNumber);
    sheet.append(canvas);
    button.append(sheet, num);
    button.addEventListener('click', () => this._goTo(pageNumber), { signal: this.abort.signal });
    this.thumbGrid.append(button);
    return { proxy, pageNumber, button, canvas, renderTask: null, ready: false, key: '' };
  }

  _relayout() {
    if (!this.stack || !this.shells.length) return;
    const scale = this._currentScale();
    this.displayedScale = scale;
    const paneW = this.stack.clientWidth;
    let y = PDF_PAGE_GAP;
    this.layouts = this.shells.map((shell) => {
      const viewport = this._viewport(shell.proxy, scale);
      const left = Math.max(PDF_PAGE_GAP, (paneW - viewport.width) / 2);
      const box = { top: y, left, width: viewport.width, height: viewport.height, viewport };
      y += viewport.height + PDF_PAGE_GAP;
      return box;
    });
    if (this.track) this.track.style.setProperty('--pdf-depth', `${y}px`);
    this.shells.forEach((shell, index) => {
      const box = this.layouts[index];
      shell.el.style.setProperty('--pdf-top', `${box.top}px`);
      shell.el.style.setProperty('--pdf-left', `${box.left}px`);
      shell.el.style.setProperty('--pdf-width', `${box.width}px`);
      shell.el.style.setProperty('--pdf-height', `${box.height}px`);
      const key = this._shellKey(box);
      if (shell.key && shell.key !== key) {
        shell.ready = false;
        shell.key = '';
      }
    });
    this._syncChrome();
  }

  _shellKey(box) {
    return `${Math.round(box.width)}x${Math.round(box.height)}:${this.settings.rotate}`;
  }

  _goTo(pageNumber, point = null) {
    const count = this.shells.length;
    if (!count || !this.stack) return;
    const page = Math.min(count, Math.max(1, Math.round(Number(pageNumber)) || 1));
    this.page = page;
    const box = this.layouts[page - 1];
    let top = box ? box.top : 0;
    if (box && point && (point.top != null || point.left != null)) {
      const [px, py] = box.viewport.convertToViewportPoint(
        point.left == null ? 0 : point.left,
        point.top == null ? 0 : point.top,
      );
      if (point.top != null) {
        const visualY = this.settings.flipV ? box.height - py : py;
        top = box.top + visualY;
      }
      if (point.left != null) {
        const visualX = this.settings.flipH ? box.width - px : px;
        this.stack.scrollLeft = Math.max(0, box.left + visualX - PDF_PAGE_GAP);
      }
    }
    this._hold(page);
    this.stack.scrollTop = Math.max(0, top);
    this._syncChrome();
    this._rememberPage();
    this._schedulePaint();
  }

  _hold(page) {
    this.holdPage = page;
    const token = this.holdToken + 1;
    this.holdToken = token;
    requestAnimationFrame(() => {
      if (this.holdToken === token) this.holdPage = 0;
    });
  }

  _onScroll() {
    if (this.closed || !this.layouts.length) return;
    if (this.holdPage) {
      this._schedulePaint();
      return;
    }
    const page = pageOccupyingPane(
      this.stack.scrollTop,
      this.stack.clientHeight,
      this.layouts.map((box) => box.top),
      this.layouts.map((box) => box.height),
    );
    if (page !== this.page) {
      this.page = page;
      this._syncChrome();
      this._rememberPage();
    }
    this._schedulePaint();
  }

  _onResize() {
    if (this.closed || !this.doc || !this.stack) return;
    const thumbsChanged = this._fitThumbs();
    const width = this.stack.clientWidth;
    const height = this.stack.clientHeight;
    if (!(width > 0) || !(height > 0)) return;
    if (width === this.seenW && height === this.seenH) {
      if (thumbsChanged) this._schedulePaint();
      return;
    }
    this.seenW = width;
    this.seenH = height;
    if (this.settings.mode === 'scale') {
      this._relayout();
      this._schedulePaint();
      return;
    }
    if (this.layouts.length) {
      this.page = pageOccupyingPane(
        this.stack.scrollTop,
        height,
        this.layouts.map((box) => box.top),
        this.layouts.map((box) => box.height),
      );
    }
    this._relayout();
    this._goTo(this.page);
  }

  _schedulePaint() {
    if (this.paintQueued) return this.paintDone;
    this.paintQueued = true;
    this.paintDone = new Promise((resolve) => {
      requestAnimationFrame(() => {
        this.paintQueued = false;
        if (this.closed) {
          resolve();
          return;
        }
        this._paintVisible().then(resolve, resolve);
      });
    });
    return this.paintDone;
  }

  async _paintVisible() {
    const generation = this.generation + 1;
    this.generation = generation;
    const alive = () => !this.closed && this.generation === generation;
    if (!this.layouts.length) return;
    const range = visibleRange(
      this.stack.scrollTop,
      this.stack.clientHeight,
      this.layouts.map((box) => box.top),
      this.layouts.map((box) => box.height),
    );
    const jobs = [];
    for (let index = 0; index < this.shells.length; index += 1) {
      const shell = this.shells[index];
      if (index < range.start || index > range.end) {
        if (shell.ready || shell.renderTask || shell.textLayer) releasePaint(shell);
        continue;
      }
      const box = this.layouts[index];
      const key = this._shellKey(box);
      if (shell.ready && shell.key === key) continue;
      jobs.push(paintPage(shell, box.viewport, alive).then((ok) => {
        if (ok && alive()) shell.key = key;
      }).catch((error) => this._paintError(error, alive)));
    }
    await Promise.all(jobs);
    if (!alive()) return;
    this._markHits();
    await this._paintThumbs(alive);
  }

  _paintError(error, alive) {
    if (!alive() || !error || error.name === 'RenderingCancelledException') return;
    if (this.paintWarned) return;
    this.paintWarned = true;
    this._fail(error);
  }

  _zoom(direction) {
    if (!direction) return;
    const next = scaleAfterZoom(this.settings.mode, this.displayedScale, this.settings.scale, direction);
    this.settings.mode = next.mode;
    this.settings.scale = next.scale;
    this._save();
    this._relayout();
    this._goTo(this.page);
  }

  _fit(mode) {
    this.settings.mode = mode;
    this._save();
    if (this.layouts.length && this.stack) {
      this.page = pageOccupyingPane(
        this.stack.scrollTop,
        this.stack.clientHeight,
        this.layouts.map((box) => box.top),
        this.layouts.map((box) => box.height),
      );
    }
    this._relayout();
    this._goTo(this.page);
  }

  _turn(delta) {
    this.settings.rotate = addQuarter(this.settings.rotate, delta);
    this._save();
    this._relayout();
    this._goTo(this.page);
  }

  _flip(which) {
    if (which === 'both') {
      const next = flipBoth(this.settings.flipH, this.settings.flipV);
      this.settings.flipH = next.flipH;
      this.settings.flipV = next.flipV;
    } else if (which === 'h') this.settings.flipH = !this.settings.flipH;
    else this.settings.flipV = !this.settings.flipV;
    this._save();
    this._applyVars();
    this._syncChrome();
  }

  _pointer(mode) {
    this.settings.pointer = mode;
    this._save();
    this._applyVars();
    this._syncChrome();
  }

  _toggleOutline() {
    this.settings.outlineOpen = !this.settings.outlineOpen;
    this._save();
    this._applyVars();
    this._syncChrome();
  }

  _toggleSearch(force) {
    const open = typeof force === 'boolean' ? force : !this.settings.searchOpen;
    this.settings.searchOpen = open;
    if (!open && this.findInput) {
      this.findInput.value = '';
      this.hits = [];
      this.hitIndex = -1;
      this._markHits();
    }
    this._save();
    this._applyVars();
    this._syncChrome();
    if (open && this.findInput) this.findInput.focus();
  }

  _commitPage() {
    const number = Math.round(Number(this.pageInput.value));
    if (number >= 1) this._goTo(number);
    else this._syncChrome();
  }

  _onWheel(event) {
    if (!wheelZoomHeld(event) || !this.stack?.contains(event.target)) return;
    const direction = wheelViewScaleDirection(event.deltaY);
    if (!direction) return;
    event.preventDefault();
    this._zoom(direction);
  }

  _onDouble(event) {
    if (!event.target?.closest?.('.reports-pdf-sheet')) return;
    event.preventDefault();
    const next = toggleFit(this.settings.mode, this.settings.scale);
    this.settings.mode = next.mode;
    this.settings.scale = next.scale;
    this._save();
    this._relayout();
    this._goTo(this.page);
  }

  _onKey(event) {
    if (this.closed || this.manager._page !== 'pdf') return;
    const target = event.target;
    if (target?.closest?.('.lithium-confirm')) return;
    if (!this.toolbar?.contains(target) && !this.host?.contains(target)) return;
    const field = target && (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.isContentEditable);
    const action = viewerKeyAction(event.key, {
      inField: Boolean(field),
      inText: Boolean(target?.closest?.('.reports-pdf-text')),
      searchOpen: this.settings.searchOpen,
      pointer: this.settings.pointer,
    });
    if (!action) return;
    event.preventDefault();
    if (action === 'close-search') {
      this._toggleSearch(false);
      return;
    }
    if (action === 'page-up' || action === 'page-down') {
      const max = this.stack.scrollHeight - this.stack.clientHeight;
      this.stack.scrollTop = stackScroll(
        this.stack.scrollTop,
        this.stack.clientHeight,
        action === 'page-up' ? -1 : 1,
        max,
      );
      return;
    }
    const key = action === 'first' ? 'Home' : action === 'last' ? 'End' : event.key;
    const next = moveThumb(this.page - 1, key, this.settings.columns, this.shells.length);
    this._goTo(next + 1);
  }

  _pointerDown(event) {
    if (event.button !== 0 || this.settings.pointer === 'text') return;
    const sheet = event.target?.closest?.('.reports-pdf-sheet');
    if (!sheet) return;
    const shell = this.shells.find((item) => item.sheet === sheet);
    if (!shell) return;
    event.preventDefault();
    sheet.setPointerCapture(event.pointerId);
    if (this.settings.pointer === 'hand') {
      this.hand = {
        id: event.pointerId,
        x: event.clientX,
        y: event.clientY,
        left: this.stack.scrollLeft,
        top: this.stack.scrollTop,
      };
      return;
    }
    shell.marquee.hidden = false;
    shell.marquee.style.left = '0px';
    shell.marquee.style.top = '0px';
    shell.marquee.style.width = '0px';
    shell.marquee.style.height = '0px';
    this.drag = {
      id: event.pointerId,
      shell,
      x: event.clientX,
      y: event.clientY,
      rect: sheet.getBoundingClientRect(),
    };
  }

  _pointerMove(event) {
    if (this.hand && event.pointerId === this.hand.id) {
      this.stack.scrollLeft = this.hand.left - (event.clientX - this.hand.x);
      this.stack.scrollTop = this.hand.top - (event.clientY - this.hand.y);
      return;
    }
    if (!this.drag || event.pointerId !== this.drag.id) return;
    const left = Math.min(this.drag.x, event.clientX) - this.drag.rect.left;
    const top = Math.min(this.drag.y, event.clientY) - this.drag.rect.top;
    const box = this.drag.shell.marquee;
    box.style.left = `${left}px`;
    box.style.top = `${top}px`;
    box.style.width = `${Math.abs(event.clientX - this.drag.x)}px`;
    box.style.height = `${Math.abs(event.clientY - this.drag.y)}px`;
  }

  _pointerUp(event) {
    if (this.hand && event.pointerId === this.hand.id) {
      this.hand = null;
      return;
    }
    if (!this.drag || event.pointerId !== this.drag.id) return;
    const { shell } = this.drag;
    this.drag = null;
    const box = shell.marquee;
    const width = Number.parseFloat(box.style.width) || 0;
    const height = Number.parseFloat(box.style.height) || 0;
    const x = Number.parseFloat(box.style.left) || 0;
    const y = Number.parseFloat(box.style.top) || 0;
    box.hidden = true;
    if (width >= MIN_MARQUEE && height >= MIN_MARQUEE) {
      this._copyMarquee(shell, x, y, width, height).catch((error) => this._fail(error));
    }
  }

  async _copyMarquee(shell, x, y, w, h) {
    const rect = shell.sheet.getBoundingClientRect();
    const left = Math.max(0, x);
    const top = Math.max(0, y);
    const local = marqueeLocalRect(
      left,
      top,
      Math.min(rect.width, x + w) - left,
      Math.min(rect.height, y + h) - top,
      rect.width,
      rect.height,
      this.settings.flipH,
      this.settings.flipV,
    );
    const blob = await marqueeBlob(
      shell.canvas,
      local,
      rect.width,
      rect.height,
      this.settings.flipH,
      this.settings.flipV,
    );
    const Item = globalThis.ClipboardItem;
    if (!blob || typeof Item !== 'function' || !navigator.clipboard?.write) {
      throw new Error('The clipboard is not available.');
    }
    await navigator.clipboard.write([new Item({ 'image/png': blob })]);
    if (!this.closed) toast.success('Copied the selection', { subsystem: 'Manager' });
  }

  _download() {
    if (!this.bytes || this.closed) return;
    const url = URL.createObjectURL(new Blob([this.bytes], { type: 'application/pdf' }));
    this.blobUrls.push(url);
    const link = document.createElement('a');
    link.href = url;
    link.download = PDF_FIXTURE_NAME;
    link.hidden = true;
    document.body.appendChild(link);
    link.click();
    link.remove();
    setTimeout(() => URL.revokeObjectURL(url), 4000);
  }

  _print() {
    if (!this.bytes || this.closed) return;
    this._clearPrint();
    const url = URL.createObjectURL(new Blob([this.bytes], { type: 'application/pdf' }));
    this.printUrl = url;
    const frame = document.createElement('iframe');
    frame.className = 'reports-pdf-print';
    frame.title = 'Print PDF';
    frame.src = url;
    frame.addEventListener('load', () => {
      if (this.closed) return;
      try {
        frame.contentWindow?.print();
      } catch (error) {
        this._fail(error instanceof Error ? error : new Error('Print failed.'));
      }
    });
    document.body.appendChild(frame);
    this.printFrame = frame;
  }

  _clearPrint() {
    if (this.printFrame) this.printFrame.remove();
    this.printFrame = null;
    if (this.printUrl) URL.revokeObjectURL(this.printUrl);
    this.printUrl = '';
  }

  _applyVars() {
    if (!this.host) return;
    this.host.style.setProperty('--pdf-cols', String(this.settings.columns));
    this.host.style.setProperty('--pdf-thumb-scale', String(this.settings.thumbPercent / 100));
    this.host.style.setProperty('--pdf-flip-x', this.settings.flipH ? '-1' : '1');
    this.host.style.setProperty('--pdf-flip-y', this.settings.flipV ? '-1' : '1');
    this.host.dataset.pointer = this.settings.pointer;
    if (this.thumbs) this.thumbs.hidden = !this.settings.thumbsOpen;
    if (this.outlinePane) this.outlinePane.hidden = !this.settings.outlineOpen;
    if (this.findBox) this.findBox.hidden = !this.settings.searchOpen;
  }

  _press(id, on) {
    const el = this._el(id);
    if (!el) return;
    el.classList.toggle('active', on);
    el.setAttribute('aria-pressed', on ? 'true' : 'false');
  }

  _syncChrome() {
    this._applyVars();
    if (this.pageInput && document.activeElement !== this.pageInput) {
      this.pageInput.value = String(this.page || 1);
    }
    if (this.pageCount) this.pageCount.textContent = `of ${this.shells.length}`;
    if (this.scaleReadout) this.scaleReadout.textContent = scaleLabel(this.displayedScale);
    if (this.colsReadout) this.colsReadout.textContent = String(this.settings.columns);
    if (this.thumbScale) this.thumbScale.textContent = `${this.settings.thumbPercent}%`;
    this._press('#reports-pdf-thumbs-btn', this.settings.thumbsOpen);
    if (this.findCount) this.findCount.textContent = searchCountLabel(this.hitIndex, this.hits.length);
    this._press('#reports-pdf-fit-page', this.settings.mode === 'fit-page');
    this._press('#reports-pdf-fit-width', this.settings.mode === 'fit-width');
    this._press('#reports-pdf-flip-h', this.settings.flipH);
    this._press('#reports-pdf-flip-v', this.settings.flipV);
    this._press('#reports-pdf-outline-btn', this.settings.outlineOpen);
    this._press('#reports-pdf-find-btn', this.settings.searchOpen);
    this._press('#reports-pdf-pointer-text', this.settings.pointer === 'text');
    this._press('#reports-pdf-pointer-hand', this.settings.pointer === 'hand');
    this._press('#reports-pdf-pointer-marquee', this.settings.pointer === 'marquee');
    this._markCurrent();
  }

  _markCurrent() {
    for (const thumb of this.thumbButtons) {
      const on = thumb.pageNumber === this.page;
      thumb.button.classList.toggle('is-current', on);
      if (on) this._revealIn(this.thumbBody, thumb.button);
    }
    if (!this.outlineList) return;
    let first = null;
    for (const button of this.outlineList.querySelectorAll('.reports-pdf-outline-item')) {
      const on = Number(button.dataset.page) === this.page && Number(button.dataset.page) > 0;
      button.classList.toggle('is-current', on);
      if (on && !first) first = button;
    }
    if (first) this._revealIn(this.outlinePane, first);
  }

  _revealIn(scroller, el) {
    if (!scroller || !el) return;
    const top = el.getBoundingClientRect().top - scroller.getBoundingClientRect().top + scroller.scrollTop;
    const bottom = top + el.offsetHeight;
    if (top < scroller.scrollTop) scroller.scrollTop = top;
    else if (bottom > scroller.scrollTop + scroller.clientHeight) {
      scroller.scrollTop = bottom - scroller.clientHeight;
    }
  }
}

Object.assign(ReportPdfPage.prototype, reportPdfFindMethods);
Object.assign(ReportPdfPage.prototype, reportPdfThumbMethods);
