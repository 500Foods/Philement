/**
 * Preview HTML page. Runs Reports.Render at stage=html and shows the
 * page in a sandboxed iframe. DOMPurify runs before srcdoc. The parent
 * page does not assign the report HTML to innerHTML.
 */

import './reports-preview.css';
import { invokeScript } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { sanitizeHtml } from '../../core/utils.js';
import { composeDefinition, composeResult, composeSources, RENDER_SCRIPT } from './reports-compose.js';
import { pendingQueryDatasources, runQueryDatasources } from './reports-query.js';

const SHELL_HEAD = '<!DOCTYPE html><html><head><meta charset="utf-8"><style>html,body{margin:0;background:#fff}</style></head><body>';
const SHELL_TAIL = '</body></html>';
const CSS_LENGTH = /^\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+))\s*(in|mm|cm|pt|px)\s*$/i;

/**
 * A stored length is "8.5 in". CSS wants "8.5in".
 * @param {string} value
 * @param {string} fallback
 * @returns {string}
 */
export function cssPageLength(value, fallback) {
  const match = typeof value === 'string' ? CSS_LENGTH.exec(value) : null;
  if (!match) return fallback;
  return `${match[1]}${match[2].toLowerCase()}`;
}

/**
 * Paper box for the preview frame. The emitted section wins. Otherwise
 * the definition page. Several pages stack at that height.
 * @param {string} html
 * @param {object} doc
 * @returns {{ width: string, height: string }}
 */
export function previewFrameBox(html, doc) {
  const source = typeof html === 'string' ? html : '';
  const sections = source.match(/<section\b/gi);
  const pages = sections && sections.length ? sections.length : 1;
  const widthInHtml = /<section\b[^>]*\bwidth:\s*([0-9.]+)(in|mm|cm|pt|px)/i.exec(source);
  const heightInHtml = /<section\b[^>]*\bheight:\s*([0-9.]+)(in|mm|cm|pt|px)/i.exec(source);
  const page = doc && doc.page && typeof doc.page === 'object' ? doc.page : {};
  const width = widthInHtml
    ? `${widthInHtml[1]}${widthInHtml[2].toLowerCase()}`
    : cssPageLength(page.width, '8.5in');
  const one = heightInHtml
    ? `${heightInHtml[1]}${heightInHtml[2].toLowerCase()}`
    : cssPageLength(page.height, '11in');
  return { width, height: pages > 1 ? `calc(${one} * ${pages})` : one };
}

/**
 * @param {object} doc
 * @returns {object}
 */
export function previewParams(doc) {
  return { stage: 'html', definition: composeDefinition(doc), sources: composeSources(doc) };
}

/**
 * @param {object} envelope
 * @returns {{ html?: string, log?: Array<object>, error?: string }}
 */
export function previewResult(envelope) {
  const parsed = composeResult(envelope);
  if (parsed.error) return { error: parsed.error };
  const body = parsed.ir;
  if (!body || typeof body.html !== 'string') return { error: 'Preview returned no HTML.' };
  const log = Array.isArray(body.log) ? body.log : [];
  return { html: body.html, log };
}

/**
 * Sanitized document for iframe srcdoc. The shell only clears the
 * browser's default body margin so the page sits at the paper origin.
 * @param {string} html
 * @returns {string}
 */
export function previewSrcdoc(html) {
  const body = sanitizeHtml(typeof html === 'string' ? html : '');
  return `${SHELL_HEAD}${body}${SHELL_TAIL}`;
}

export const reportPreviewMethods = {
  _bindPreview() {
    this._onPreviewRun = () => {
      this.runPreview();
    };
    this.elements.previewBtn.addEventListener('click', this._onPreviewRun);
  },

  _unbindPreview() {
    this.elements.previewBtn?.removeEventListener('click', this._onPreviewRun);
  },

  _sizePreviewFrame(html) {
    const frame = this.elements && this.elements.previewFrame;
    if (!frame) return;
    const box = previewFrameBox(html == null ? this._previewHtml : html, this.doc);
    frame.style.setProperty('--reports-preview-width', box.width);
    frame.style.setProperty('--reports-preview-height', box.height);
  },

  _showPreview(html) {
    const frame = this.elements.previewFrame;
    if (!frame) return;
    this._sizePreviewFrame(html);
    frame.srcdoc = previewSrcdoc(html);
  },

  async runPreview() {
    if (!this.doc || this._previewBusy) return;
    if (!this.app?.api) {
      toast.error('Preview needs a server connection.', { subsystem: 'Manager' });
      return;
    }
    this._previewBusy = true;
    if (this.elements.previewBtn) this.elements.previewBtn.disabled = true;
    try {
      let doc = this.doc;
      if (pendingQueryDatasources(doc).length) {
        const queries = (this._lookups && this._lookups.reportingQueries) || [];
        const run = await runQueryDatasources(this.app.api, doc, queries);
        for (const err of run.errors) {
          toast.error(`Query datasource ${err.name} failed`, { description: err.message, subsystem: 'Manager' });
        }
        doc = run.doc;
      }
      const envelope = await invokeScript(this.app.api, {
        script: RENDER_SCRIPT,
        params: previewParams(doc),
        wait: true,
      });
      const result = previewResult(envelope);
      if (result.error) {
        toast.error('Preview failed', { description: result.error, subsystem: 'Manager' });
        return;
      }
      this._previewHtml = result.html;
      this._previewLog = result.log;
      if (typeof this.showGeneration === 'function') this.showGeneration(result.log);
      this._showPreview(result.html);
    } catch (error) {
      const message = error instanceof Error && error.message ? error.message : 'Preview failed.';
      toast.error('Preview failed', { description: message, subsystem: 'Manager' });
    } finally {
      this._previewBusy = false;
      if (this.elements.previewBtn) this.elements.previewBtn.disabled = false;
    }
  },
};
