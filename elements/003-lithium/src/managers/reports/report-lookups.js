/**
 * Lookups for the report property panel. QueryRef 26 is the working
 * lookup path used across Lithium (the plan's "026"); the lookup
 * loader's QueryRef 34 is the Lookups manager list query.
 */

import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { loadReportingQueries } from './reports-query.js';

export const PANEL_LOOKUPS = {
  objectTypes: 53, attributes: 54, pageSizes: 57, paletteGroups: 74, sections: 75,
  /** 076 datasource kinds (Phase 19b). Empty until 1446 applies; the panel falls back. */
  dsKinds: 76,
};

/**
 * Load the panel lookups and (Phase 28) the Reporting query list from
 * QueryRef 173 as `reportingQueries`. Missing ones come back empty.
 */
export async function loadPanelLookups(api) {
  const out = {};
  const reporting = loadReportingQueries(api).then((list) => { out.reportingQueries = list; }, (error) => {
    log(Subsystems.MANAGER, Status.WARN, `[Reports] Reporting query list failed: ${error.message}`);
    out.reportingQueries = [];
  });
  await Promise.all([reporting, ...Object.entries(PANEL_LOOKUPS).map(async ([name, id]) => {
    try {
      const rows = await authQuery(api, 26, { INTEGER: { LOOKUPID: id } });
      out[name] = Array.isArray(rows) ? rows : [];
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, `[Reports] Lookup ${id} failed: ${error.message}`);
      out[name] = [];
    }
  })]);
  return out;
}
