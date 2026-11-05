/**
 * Preview HTML page. Runs Reports.Render at stage=html and shows the
 * page in a sandboxed iframe. DOMPurify runs before srcdoc. The parent
 * page does not assign the report HTML to innerHTML.
 */

import './reports-preview.css';
import { log, Subsystems, Status } from '../../core/log.js';
import { invokeScript } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { sanitizeHtml } from '../../core/utils.js';
import { restorePreviewShapes, SHAPE_PURIFY, shieldPreviewShapes } from './reports-shape.js';
import { composeDefinition, composeResult, composeSources, RENDER_SCRIPT } from './reports-compose.js';
import { pendingQueryDatasources, runQueryDatasources } from './reports-query.js';
import { reportKey, saveListedReport } from './reports-save.js';
import { buildRasterMap } from './reports-rasters.js';

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

/** Default conduit script params cap. Hydrogen's merged `_hydrogen` claims sit inside it. */
export const SCRIPT_PARAMS_CAP_BYTES = 256 * 1024;

/** Bytes left for the server-added `_hydrogen` object. The client does not send that key. */
export const SCRIPT_PARAMS_RESERVE_BYTES = 4096;

export const PREVIEW_OVER_CAP_TOAST = Object.freeze({
  title: 'Saved to preview',
  description: 'This report is larger than the 256 KiB script limit, so it was saved and previewed from the stored revision.',
});

/**
 * UTF-8 length of the compact JSON the client would send as `params`.
 * @param {object} params
 * @returns {number}
 */
export function scriptParamsBytes(params) {
  return new TextEncoder().encode(JSON.stringify(params)).length;
}

/**
 * True when the inline params would not fit under the cap once `_hydrogen` is merged.
 * @param {object} params
 * @param {number} [cap]
 * @param {number} [reserve]
 * @returns {boolean}
 */
export function inlineParamsExceedCap(
  params,
  cap = SCRIPT_PARAMS_CAP_BYTES,
  reserve = SCRIPT_PARAMS_RESERVE_BYTES,
) {
  return scriptParamsBytes(params) > cap - reserve;
}

/**
 * Preview call that names a stored revision. Sources stay inline. The definition does not.
 * @param {object} doc
 * @param {{ report_id: number, rev_id: number }} identity
 * @returns {object}
 */
export function storedPreviewParams(doc, identity) {
  return {
    stage: 'html',
    report_id: identity.report_id,
    rev_id: identity.rev_id,
    sources: composeSources(doc),
  };
}

/**
 * @param {string} title
 * @param {string} message
 * @returns {Error}
 */
function overCapSaveError(title, message) {
  const error = new Error(message);
  error.overCapSave = true;
  error.serverError = { error: title, message };
  return error;
}

/**
 * Inline params, or a save then `report_id` and `rev_id`, when the inline body is over the cap.
 * A failed save throws and does not toast. The canvas is left to the caller.
 * @param {object} manager
 * @param {object} doc
 * @param {{ cap?: number, save?: Function, notify?: Function }} [options]
 * @returns {Promise<{ params: object, stored: boolean }>}
 */
export async function resolvePreviewParams(manager, doc, options = {}) {
  const inline = previewParams(doc);
  const rasters = await buildRasterMap(doc, options.draw);
  if (Object.keys(rasters).length > 0) inline.rasters = rasters;
  if (!inlineParamsExceedCap(inline, options.cap)) return { params: inline, stored: false };
  const row = typeof manager._selectedRow === 'function' ? manager._selectedRow() : null;
  if (!row) {
    throw overCapSaveError('Save is not available', 'Select this report in the list before previewing it.');
  }
  const save = options.save || saveListedReport;
  try {
    await save(manager, row);
  } catch (error) {
    if (error && typeof error === 'object') error.overCapSave = true;
    throw error;
  }
  const helper = manager.editHelper;
  if (typeof helper?.isEditing === 'function' && helper.isEditing()
    && typeof helper._takeSnapshot === 'function') {
    helper._takeSnapshot();
    if (typeof helper.checkDirtyState === 'function') helper.checkDirtyState();
  }
  const saved = manager._openRevision;
  const reportId = reportKey(saved && saved.report_id);
  const revId = reportKey(saved && saved.rev_id);
  if (!reportId || !revId) {
    throw overCapSaveError('Save Failed', 'The insert did not return a report id.');
  }
  const notify = options.notify || ((title, description) => {
    toast.info(title, { description, subsystem: 'Manager' });
  });
  notify(PREVIEW_OVER_CAP_TOAST.title, PREVIEW_OVER_CAP_TOAST.description);
  log(Subsystems.MANAGER, Status.INFO, `[Reports] Preview saved report ${reportId} revision ${revId}`);
  const stored = storedPreviewParams(doc, { report_id: reportId, rev_id: revId });
  if (inline.rasters) stored.rasters = inline.rasters;
  return { params: stored, stored: true };
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
  const entries = Array.isArray(body.log) ? body.log : [];
  return { html: body.html, log: entries };
}

/**
 * Sanitized document for iframe srcdoc. The shell only clears the
 * browser's default body margin so the page sits at the paper origin.
 * @param {string} html
 * @returns {string}
 */
export function previewSrcdoc(html) {
  const shielded = shieldPreviewShapes(typeof html === 'string' ? html : '');
  const clean = sanitizeHtml(shielded.html, SHAPE_PURIFY);
  const body = restorePreviewShapes(clean, shielded.shapes);
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
      let params;
      try {
        const resolved = await resolvePreviewParams(this, doc);
        params = resolved.params;
      } catch (error) {
        if (error && error.overCapSave) {
          const message = error instanceof Error && error.message ? error.message : 'Save failed.';
          toast.error(error.serverError?.error || 'Save Failed', {
            description: error.serverError?.message || message,
            serverError: error.serverError,
            subsystem: 'Conduit',
          });
          return;
        }
        throw error;
      }
      const envelope = await invokeScript(this.app.api, {
        script: RENDER_SCRIPT,
        params,
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
