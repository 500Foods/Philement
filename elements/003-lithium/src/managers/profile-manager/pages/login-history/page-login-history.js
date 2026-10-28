/**
 * Profile Manager - Login History Page Handler
 *
 * Handles the Login History settings page (index: -17).
 * Read-only. Data from QueryRef 159 (rows) and 160 (summary).
 */

import { BaseSettingsPage } from '../settings-page-base.js';
import { getClaims, retrieveJWT } from '../../../../core/jwt.js';
import { authQuery } from '../../../../shared/conduit.js';
import { log, Subsystems, Status } from '../../../../core/log.js';
import { LithiumTable } from '../../../../tables/lithium-table-main.js';

export const QUERY_LOGIN_HISTORY = 159;
export const QUERY_LOGIN_SUMMARY = 160;

const TYPE_LABELS = { 0: 'Signed in', 2: 'Failed', 3: 'Success', 4: 'Passed', 5: 'Signed out' };

/** Format a timestamp as 24-hour America/Vancouver time with a PCT label. */
export function formatLoginTime(value) {
  if (!value) return '-';
  const d = new Date(String(value).includes('T') || /Z|[+-]\d\d:?\d\d$/.test(value) ? value : String(value).replace(' ', 'T') + 'Z');
  if (isNaN(d.getTime())) return String(value);
  const s = new Intl.DateTimeFormat('en-CA', {
    timeZone: 'America/Vancouver', year: 'numeric', month: '2-digit', day: '2-digit',
    hour: '2-digit', minute: '2-digit', second: '2-digit', hourCycle: 'h23',
  }).format(d).replace(',', '');
  return `${s} PCT`;
}

/** Build the parameter payload for both QueryRefs from JWT claims. */
export function buildLoginParams(claims) {
  return {
    INTEGER: { ACCOUNTID: Number(claims?.user_id ?? claims?.id ?? 0) },
    STRING: { LOGINID: String(claims?.username ?? '') },
  };
}

/** Lookup 059 key for the Recent Activity tableDef (acuranzo_1408). */
export const LOGIN_HISTORY_TABLEDEF_KEY = 17;
export const LOGIN_HISTORY_TABLE_PATH = 'profile-manager/login-history';

/** "City, Region, CC" from QueryRef 159 geo_* (Traefik X-GeoIP2-*); '' when none. */
export function formatLoginLocation(r) {
  return [r?.geo_city, r?.geo_region, r?.geo_country]
    .map((v) => (v === null || v === undefined ? '' : String(v).trim()))
    .filter((v) => v && v !== 'Unknown')
    .join(', ');
}

/** Map a QueryRef 159 row to the flat, display-ready shape the tabledef expects. */
export function toLoginTableRow(r, i = 0) {
  const t = Number(r?.action_type_a24);
  const ms = r?.action_msecs;
  return {
    row_id: i + 1,
    when_txt: formatLoginTime(r?.created_at),
    result_txt: r?.action_type_name || TYPE_LABELS[t] || (Number.isNaN(t) ? '' : String(t)),
    action_type_a24: Number.isNaN(t) ? null : t,
    login_id: r?.login_id ?? r?.action ?? '',
    ip_address: r?.ip_address ?? '',
    location_txt: formatLoginLocation(r),
    action_msecs: ms === null || ms === undefined || ms === '' ? null : Number(ms),
    app_version: r?.app_version ?? '',
  };
}

export class LoginHistoryPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -17 });
    this.app = options.app || null;
    this._loading = null;
    this._table = null;
    this._rows = [];
  }

  async onInit() {
    log(Subsystems.MANAGER, Status.DEBUG, '[LoginHistoryPage] Initialized');
    await this._initTable();
    await this.loadData();
    // SettingsTab calls onShow() right after init; skip that one so the page
    // doesn't fire QueryRef 159 + 160 twice on first display.
    this._skipNextShow = true;
  }

  /** Initialize the Recent Activity LithiumTable (read-only). */
  async _initTable() {
    const tableContainer = this.container?.querySelector('#login-history-table-container');
    const navContainer = this.container?.querySelector('#login-history-navigator');
    if (!tableContainer || this._table) return;
    try {
      this._table = new LithiumTable({
        container: tableContainer,
        navigatorContainer: navContainer,
        tablePath: LOGIN_HISTORY_TABLE_PATH,
        lookupKeyIdx: LOGIN_HISTORY_TABLEDEF_KEY,
        cssPrefix: 'profile-login-history',
        storageKey: 'profile_login_history',
        app: this.app,
        readonly: true,
        onRefresh: () => this.loadData(),
      });
      await this._table.init();
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[LoginHistoryPage] Table init failed: ${err?.message || err}`);
      this._table = null;
    }
  }

  onShow() {
    if (this._skipNextShow) { this._skipNextShow = false; return; }
    this.loadData();
  }
  refresh() { return this.loadData(); }
  canSave() { return false; }

  async loadData() {
    if (this._loading) return this._loading;
    this._loading = this._load().finally(() => { this._loading = null; });
    return this._loading;
  }

  async _load() {
    // getClaims() needs the token string; with no argument it returns null,
    // which previously made this method return silently (no 159/160 calls).
    const claims = getClaims(retrieveJWT());
    const api = this.app?.api;
    if (!claims || !api || !this.container) {
      const why = !claims ? 'no JWT claims' : !api ? 'no app.api' : 'no container';
      log(Subsystems.MANAGER, Status.WARN, `[LoginHistoryPage] Load skipped: ${why}`);
      this.renderSummary(null);
      this.renderRows(null);
      return;
    }
    const params = buildLoginParams(claims);
    log(Subsystems.MANAGER, Status.INFO, `[LoginHistoryPage] Loading QueryRef ${QUERY_LOGIN_HISTORY} + ${QUERY_LOGIN_SUMMARY} (account ${params.INTEGER.ACCOUNTID})`);
    const [rows, summary] = await Promise.all([
      authQuery(api, QUERY_LOGIN_HISTORY, params).catch((e) => this._fail(QUERY_LOGIN_HISTORY, e)),
      authQuery(api, QUERY_LOGIN_SUMMARY, params).catch((e) => this._fail(QUERY_LOGIN_SUMMARY, e)),
    ]);
    this.renderSummary(Array.isArray(summary) ? summary[0] : null);
    this.renderRows(Array.isArray(rows) ? rows : null);
    log(Subsystems.MANAGER, Status.INFO, `[LoginHistoryPage] QueryRef ${QUERY_LOGIN_HISTORY}: ${Array.isArray(rows) ? rows.length : 'failed'} rows; QueryRef ${QUERY_LOGIN_SUMMARY}: ${Array.isArray(summary) ? 'ok' : 'failed'}`);
  }

  _fail(ref, err) {
    log(Subsystems.MANAGER, Status.WARN, `[LoginHistoryPage] QueryRef ${ref} failed: ${err?.message || err}`);
    return null;
  }

  renderSummary(s) {
    const set = (sel, v) => { const el = this.container.querySelector(sel); if (el) el.textContent = v ?? '-'; };
    set('#login-total', s?.total_logins);
    set('#login-last', s?.last_login ? formatLoginTime(s.last_login) : '-');
    set('#login-failed', s?.failed_attempts);
    set('#login-sessions', s?.active_sessions);
  }

  renderRows(rows) {
    const empty = this.container?.querySelector('#login-history-empty');
    this._rows = Array.isArray(rows) ? rows.map(toLoginTableRow) : [];
    if (empty) {
      empty.hidden = this._rows.length > 0;
      empty.textContent = rows ? 'No login activity recorded.' : 'Login history is unavailable.';
    }
    const t = this._table;
    if (!t?.table) return;
    t.currentData = this._rows;
    t.table.blockRedraw?.();
    try {
      t.table.setData(this._rows);
      t.discoverColumns?.(this._rows);
    } finally {
      t.table.restoreRedraw?.();
    }
  }

  destroy() {
    try { this._table?.destroy(); } catch { /* ignore */ }
    this._table = null;
    super.destroy();
  }

}

export default LoginHistoryPage;
