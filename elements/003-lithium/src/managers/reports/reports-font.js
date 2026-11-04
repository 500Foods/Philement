/**
 * Font settings for the Definition (json) and Compose editors.
 * Same pattern as Query Manager: shared createFontPopup, one saved
 * setting per page in localStorage, applied to the CodeMirror dom.
 */

import { createFontPopup } from '../../core/manager-ui.js';

const DEFAULT_FONT = {
  fontSize: '13px',
  fontFamily: '"Vanadium Mono", var(--font-mono, monospace)',
  fontWeight: 'normal',
};
const FONT_PAGES = new Set(['json', 'compose']);
const fontKey = (page) => `lithium_reports_font_${page}`;

export function loadReportFont(page) {
  try {
    const stored = localStorage.getItem(fontKey(page));
    return stored ? JSON.parse(stored) : null;
  } catch {
    return null;
  }
}

export const reportFontMethods = {
  _fontView(page) {
    const editor = page === 'compose' ? this._compose : this._json;
    return editor && editor.view ? editor.view : null;
  },

  /** Apply the saved font (or clear the override) on a page's editor. */
  _applyReportFont(page) {
    const view = this._fontView(page);
    if (!view || !view.dom) return;
    const saved = loadReportFont(page);
    const style = view.dom.style;
    style.fontSize = saved ? `${parseInt(String(saved.fontSize), 10) || 13}px` : '';
    style.fontFamily = saved ? saved.fontFamily || '' : '';
    style.fontWeight = saved ? saved.fontWeight || '' : '';
    view.requestMeasure();
  },

  _handleFontClick(event) {
    const page = this._page;
    if (!FONT_PAGES.has(page)) return;
    if (!this._fontPopup) {
      this._fontPopup = createFontPopup({
        anchor: this.elements.fontBtn,
        ...DEFAULT_FONT,
        onSave: (state) => {
          const target = this._fontPopupPage;
          if (this._fontResetPending) {
            this._fontResetPending = false;
            try { localStorage.removeItem(fontKey(target)); } catch { /* ignore */ }
          } else {
            try { localStorage.setItem(fontKey(target), JSON.stringify(state)); } catch { /* ignore */ }
          }
          this._applyReportFont(target);
        },
        onCancel: () => {
          this._fontResetPending = false;
        },
        onReset: () => {
          this._fontResetPending = true;
          return { ...DEFAULT_FONT };
        },
      });
    }
    this._fontPopupPage = page;
    this._fontResetPending = false;
    this._fontPopup.setState(loadReportFont(page) || { ...DEFAULT_FONT });
    this._fontPopup.toggle(event);
  },

  _destroyReportFont() {
    if (this._fontPopup) this._fontPopup.destroy();
    this._fontPopup = null;
  },
};
