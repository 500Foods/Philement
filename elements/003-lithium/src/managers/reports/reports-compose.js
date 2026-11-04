/**
 * Compose page (Phase 19). Runs Reports.Render with stage=compose on
 * the open definition and shows the intermediate as read-only JSON.
 * Sources (Phase 19b): each json datasource sends its own stored
 * `rows` as sources[name] ([] when it has none). A query datasource
 * (Phase 28) sends its rows once a designer Run stored them; with no
 * rows it is left out for Render to fetch (Phase 29). Detail and group bands with no datasource
 * are sent naming the first one, which drives iteration.
 */

import { invokeScript } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { reportBody } from './model/history.js';
import { bandDatasource, datasourceList } from './reports-sample.js';
import { ReportJsonPage } from './reports-json.js';
import { pendingQueryDatasources, runQueryDatasources } from './reports-query.js';

export const RENDER_SCRIPT = 'Reports.Render';

/**
 * @param {object} doc
 * @returns {object}
 */
export function composeSources(doc) {
  const sources = {};
  for (const ds of datasourceList(doc)) {
    if (typeof ds.name !== 'string' || ds.name === '') continue;
    const kind = ds.kind || 'json';
    if (kind === 'json') sources[ds.name] = Array.isArray(ds.rows) ? ds.rows : [];
    else if (kind === 'query' && Array.isArray(ds.rows)) sources[ds.name] = ds.rows;
  }
  return sources;
}

/** Iterating bands: detail (15), group header (14) and footer (16). */
const ITERATING_BANDS = new Set([14, 15, 16]);

/**
 * Definition sent to Render: rows move to sources; bands with
 * no datasource name the first one.
 * @param {object} doc
 * @returns {object}
 */
export function composeDefinition(doc) {
  const body = reportBody(doc);
  if (Array.isArray(body.datasources)) {
    // Rows travel once, in sources; the definition keeps the declaration.
    body.datasources = body.datasources.map((ds) => {
      if (!ds || typeof ds !== 'object' || !Object.hasOwn(ds, 'rows')) return ds;
      const rest = { ...ds };
      delete rest.rows;
      return rest;
    });
  }
  if (!Array.isArray(body.bands)) return body;
  const bands = body.bands.map((band) => {
    if (!band || !ITERATING_BANDS.has(band.typeKey) || band.datasource) return band;
    const name = bandDatasource(doc, band);
    return name ? { ...band, datasource: name } : band;
  });
  return { ...body, bands };
}

/**
 * @param {object} doc
 * @returns {object}
 */
export function composeParams(doc) {
  return { stage: 'compose', definition: composeDefinition(doc), sources: composeSources(doc) };
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
      // Phase 28: Query datasources not yet Run are fetched for this
      // Compose only (the stored definition is not changed).
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
        params: composeParams(doc),
        wait: true,
      });
      const result = composeResult(envelope);
      if (result.error) {
        toast.error('Compose failed', { description: result.error, subsystem: 'Manager' });
        return;
      }
      this._composeIr = result.ir;
      if (typeof this.showGeneration === 'function') this.showGeneration(result.ir && result.ir.log);
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
