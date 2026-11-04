/**
 * Compose page (Phase 19). Runs Reports.Render with stage=compose on
 * the open definition and shows the intermediate as read-only JSON.
 * Sources: the session sample under every declared datasource name.
 * A static report (no datasources or no sample) sends empty sources.
 */

import { invokeScript } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { reportBody } from './model/history.js';
import { datasourceNames } from './reports-sample.js';
import { ReportJsonPage } from './reports-json.js';

export const RENDER_SCRIPT = 'Reports.Render';

/**
 * @param {object} doc
 * @param {{ rows: object[] }|null|undefined} sample
 * @returns {object}
 */
export function composeSources(doc, sample) {
  const sources = {};
  if (!sample || !Array.isArray(sample.rows)) return sources;
  for (const name of datasourceNames(doc)) sources[name] = sample.rows;
  return sources;
}

/**
 * @param {object} doc
 * @param {{ rows: object[] }|null|undefined} sample
 * @returns {object}
 */
export function composeParams(doc, sample) {
  return { stage: 'compose', definition: reportBody(doc), sources: composeSources(doc, sample) };
}

const isEmptyObject = (value) => Boolean(value) && typeof value === 'object'
  && !Array.isArray(value) && Object.keys(value).length === 0;

/**
 * Hydrogen encodes an empty Lua table as {}. Show [] for the list keys.
 * @param {object} ir
 * @returns {object}
 */
export function normalizeIr(ir) {
  if (!ir || typeof ir !== 'object') return ir;
  const out = { ...ir };
  if (isEmptyObject(out.log)) out.log = [];
  if (isEmptyObject(out.pages)) out.pages = [];
  if (Array.isArray(out.pages)) {
    out.pages = out.pages.map((page) => {
      if (!page || typeof page !== 'object') return page;
      const next = { ...page };
      if (isEmptyObject(next.bands)) next.bands = [];
      if (Array.isArray(next.bands)) {
        next.bands = next.bands.map((band) => (band && isEmptyObject(band.items)
          ? { ...band, items: [] }
          : band));
      }
      return next;
    });
  }
  return out;
}

/**
 * Read a conduit script envelope.
 * @param {object} envelope
 * @returns {{ ir?: object, error?: string }}
 */
export function composeResult(envelope) {
  const status = envelope && envelope.status ? String(envelope.status) : 'unknown';
  if (status !== 'completed') {
    return { error: envelope && envelope.error ? String(envelope.error) : `Compose ${status}` };
  }
  let body = envelope.result;
  if (typeof body === 'string') {
    try {
      body = JSON.parse(body);
    } catch {
      return { error: 'Compose returned unreadable JSON.' };
    }
  }
  if (!body || typeof body !== 'object') return { error: 'Compose returned no result.' };
  if (body.error) return { error: String(body.error) };
  return { ir: normalizeIr(body) };
}

/** @param {object} ir */
export const composeText = (ir) => JSON.stringify(ir, null, 2);

export const reportComposeMethods = {
  _openCompose() {
    if (!this.elements.composeHost) return;
    if (!this._compose) {
      this._compose = new ReportJsonPage(
        this.elements.composeHost,
        this.elements.composeFooter,
        () => {},
        null,
        composeText,
      );
    }
    this._compose.open(this._composeIr || { pages: [], log: [] }, false);
    this._compose.setEditable(false);
    this._compose.unfold();
  },

  async runCompose() {
    if (!this.doc || this._composeBusy) return;
    if (!this.app?.api) {
      toast.error('Compose needs a server connection.', { subsystem: 'Manager' });
      return;
    }
    this._composeBusy = true;
    if (this.elements.composeBtn) this.elements.composeBtn.disabled = true;
    try {
      const envelope = await invokeScript(this.app.api, {
        script: RENDER_SCRIPT,
        params: composeParams(this.doc, this._sample),
        wait: true,
      });
      const result = composeResult(envelope);
      if (result.error) {
        toast.error('Compose failed', { description: result.error, subsystem: 'Manager' });
        return;
      }
      this._composeIr = result.ir;
      if (this._page === 'compose') this._openCompose();
    } catch (error) {
      const message = error instanceof Error && error.message ? error.message : 'Compose failed.';
      toast.error('Compose failed', { description: message, subsystem: 'Manager' });
    } finally {
      this._composeBusy = false;
      if (this.elements.composeBtn) this.elements.composeBtn.disabled = false;
    }
  },
};
