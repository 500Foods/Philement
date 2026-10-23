/**
 * Course Manager learners (CATCHUP Phase 15).
 *
 * Search is exact: account id, Keycloak subject, or OIDC email.
 * Detail, seats, orders, and the Canvas link are staff reads.
 * Grant, archive, and renew call Enroll.Staff* and stay disabled
 * until QueryRef 87 says the script is invokable. The target
 * account comes from the search. The caller identity stays on
 * the server. Paid seats are not granted here.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { authQuery, invokeScript } from '../../shared/conduit.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import {
  CANVAS_BASE_URL,
  NOT_ENABLED_TIP,
  SCRIPT_DETAIL_QUERY_REF,
  actionOutcome,
  courseCell,
  scriptDetailParams,
  scriptIsInvokable,
} from './course-catalog.js';
import { orderListView, safeStripeId, stripeDashboardUrl } from './course-commerce.js';

export const LEARNER_SEARCH_QUERY_REF = 4508;
export const LEARNER_DETAIL_QUERY_REF = 4509;
export const LEARNER_ENROLLMENTS_QUERY_REF = 4510;
export const LEARNER_ORDERS_QUERY_REF = 4511;
export const LEARNER_CANVAS_QUERY_REF = 4512;

export const STAFF_GRANT_SCRIPT = 'Enroll.StaffGrant';
export const STAFF_ARCHIVE_SCRIPT = 'Enroll.StaffArchive';
export const STAFF_RENEW_SCRIPT = 'Enroll.StaffRenew';

export const CANVAS_OPS_NOTE = 'Relink is an ops step. This screen shows the account id and the Canvas user id. Merging two Canvas users stays with ops.';

const MAX_INT = 2147483647;
const MAX_TEXT = 500;

/**
 * @param {unknown} value
 * @returns {number}
 */
function positiveId(value) {
  const text = String(value ?? '').trim();
  if (!/^[1-9][0-9]*$/.test(text)) return 0;
  const id = Number(text);
  if (!Number.isSafeInteger(id) || id > MAX_INT) return 0;
  return id;
}

/**
 * Account id wins. Otherwise the subject. Otherwise the email.
 * All three binds are always sent.
 * @param {{ email?: unknown, subject?: unknown, accountId?: unknown }} input
 * @returns {{ ok: true, field: string, params: object } | { ok: false, message: string }}
 */
export function buildLearnerSearchParams(input) {
  const source = input || {};
  const account = String(source.accountId ?? '').trim();
  const subject = String(source.subject ?? '').trim();
  const email = String(source.email ?? '').trim();
  let id = 0;
  let field = '';
  if (account) {
    id = positiveId(account);
    if (!id) return { ok: false, message: 'Account id must be a positive integer.' };
    field = 'account';
  } else if (subject) {
    if (subject.length > MAX_TEXT) return { ok: false, message: 'Keycloak sub is too long.' };
    field = 'subject';
  } else if (email) {
    if (email.length > MAX_TEXT) return { ok: false, message: 'Email is too long.' };
    field = 'email';
  } else {
    return { ok: false, message: 'Enter an email, a Keycloak sub, or an account id.' };
  }
  return {
    ok: true,
    field,
    params: {
      INTEGER: { ACCOUNTID: id },
      STRING: {
        SUBJECT: field === 'subject' ? subject : '',
        EMAIL: field === 'email' ? email : '',
      },
    },
  };
}

/**
 * @param {unknown} accountId
 * @returns {{ INTEGER: { ACCOUNTID: number } } | null}
 */
export function learnerAccountParams(accountId) {
  const id = Number(accountId);
  if (!Number.isInteger(id) || id <= 0 || id > MAX_INT) return null;
  return { INTEGER: { ACCOUNTID: id } };
}

/**
 * @param {unknown} canvasUserId
 * @returns {string}
 */
export function canvasUserUrl(canvasUserId) {
  const id = positiveId(canvasUserId);
  if (!id) return '';
  return `${CANVAS_BASE_URL}/users/${id}`;
}

/**
 * @param {unknown} accountId
 * @param {unknown} courseId
 * @returns {object|null}
 */
export function staffGrantOptions(accountId, courseId) {
  const account = Number(accountId);
  const course = Number(courseId);
  if (!Number.isInteger(account) || account <= 0 || account > MAX_INT) return null;
  if (!Number.isInteger(course) || course <= 0 || course > MAX_INT) return null;
  return {
    script: STAFF_GRANT_SCRIPT,
    params: { account_id: account, course_id: course },
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @param {unknown} accountId
 * @param {unknown} courseId
 * @param {unknown} action
 * @returns {object|null}
 */
export function staffArchiveOptions(accountId, courseId, action) {
  if (action !== 'archive' && action !== 'unarchive') return null;
  const base = staffGrantOptions(accountId, courseId);
  if (!base) return null;
  return {
    ...base,
    script: STAFF_ARCHIVE_SCRIPT,
    params: { account_id: base.params.account_id, course_id: base.params.course_id, action },
  };
}

/**
 * @param {unknown} accountId
 * @param {unknown} courseId
 * @returns {object|null}
 */
export function staffRenewOptions(accountId, courseId) {
  const base = staffGrantOptions(accountId, courseId);
  if (!base) return null;
  return { ...base, script: STAFF_RENEW_SCRIPT };
}

/**
 * @param {object} state
 * @returns {{ disabled: boolean, title: string }}
 */
export function grantButtonState(state) {
  const source = state || {};
  const courseId = Number(source.courseId);
  const accountId = Number(source.accountId);
  if (!source.invokable) return { disabled: true, title: NOT_ENABLED_TIP };
  if (!Number.isInteger(courseId) || courseId <= 0) {
    return { disabled: true, title: 'Select a course' };
  }
  if (!Number.isInteger(accountId) || accountId <= 0) {
    return { disabled: true, title: 'Search for an account' };
  }
  if (source.retired) return { disabled: true, title: 'Retired courses are not granted.' };
  if (String(source.pricingType || '').toLowerCase() !== 'free') {
    return { disabled: true, title: 'Free courses only. Paid seats stay on checkout.' };
  }
  if (source.published !== 'Published') {
    return { disabled: true, title: 'The course is not published.' };
  }
  return { disabled: false, title: '' };
}

/**
 * @param {{ invokable: boolean, archived: boolean }} state
 * @returns {{ disabled: boolean, title: string, label: string }}
 */
export function archiveButtonState(state) {
  const archived = Boolean(state && state.archived);
  const label = archived ? 'Unarchive' : 'Archive';
  if (!state || !state.invokable) return { disabled: true, title: NOT_ENABLED_TIP, label };
  return { disabled: false, title: '', label };
}

/**
 * @param {{ invokable: boolean, renewPolicy: unknown, archived: boolean }} state
 * @returns {{ disabled: boolean, title: string }}
 */
export function renewButtonState(state) {
  const source = state || {};
  const policy = String(source.renewPolicy || '').toLowerCase();
  if (!source.invokable) return { disabled: true, title: NOT_ENABLED_TIP };
  if (source.archived) return { disabled: true, title: 'Unarchive the seat before renewing it.' };
  if (policy === 'paid_renew') return { disabled: true, title: 'Paid renewal stays on checkout.' };
  if (policy !== 'free_renew') return { disabled: true, title: 'This seat cannot be renewed.' };
  return { disabled: false, title: '' };
}

/**
 * @param {unknown} value
 * @returns {string}
 */
export function stripeCustomerLabel(value) {
  const raw = String(value ?? '').trim();
  if (!raw) return 'None';
  return safeStripeId(raw, 'cus_') || 'Hidden';
}

/**
 * @param {object|null|undefined} envelope
 * @returns {{ ok: true, body: object } | { ok: false, code: string, message: string }}
 */
export function staffOutcome(envelope) {
  const outcome = actionOutcome(envelope);
  if (!outcome.ok) {
    return { ok: false, code: '', message: outcome.detail || 'Action failed' };
  }
  let body = envelope && envelope.result;
  if (typeof body === 'string') {
    try {
      body = JSON.parse(body);
    } catch {
      body = null;
    }
  }
  if (!body || typeof body !== 'object') return { ok: true, body: {} };
  if (body.ok === false) {
    return {
      ok: false,
      code: String(body.code || ''),
      message: String(body.message || body.code || 'Action failed'),
    };
  }
  return { ok: true, body };
}

/**
 * @param {string} tag
 * @param {string} className
 * @param {string} [text]
 * @returns {HTMLElement}
 */
function node(tag, className, text) {
  const element = document.createElement(tag);
  element.className = className;
  if (text != null) element.textContent = text;
  return element;
}

/**
 * @param {string} label
 * @param {string} value
 * @returns {HTMLElement}
 */
function readLine(label, value) {
  const row = node('div', 'course-field');
  row.append(node('span', 'course-field-label', label));
  row.append(node('div', 'course-readonly-line', value || 'None'));
  return row;
}

/**
 * @param {unknown} value
 * @returns {string}
 */
function flagText(value) {
  if (value == null || value === '') return 'None';
  return Number(value) === 1 ? 'Yes' : 'No';
}

export class CourseLearners {
  /**
   * @param {object} manager
   */
  constructor(manager) {
    this.manager = manager;
    this.pane = null;
    this._grantInvokable = false;
    this._archiveInvokable = false;
    this._renewInvokable = false;
    this._busy = false;
    this._accountId = 0;
    this._searchToken = 0;
    this._detailToken = 0;
  }

  /**
   * @param {HTMLElement|null} pane
   */
  mount(pane) {
    this.pane = pane;
    if (!pane) return;
    pane.innerHTML = `
      <div class="course-form" id="course-learners">
        <div class="course-field">
          <label for="learner-email">Email</label>
          <input id="learner-email" class="input" type="text" autocomplete="off">
        </div>
        <div class="course-field">
          <label for="learner-subject">Keycloak sub</label>
          <input id="learner-subject" class="input" type="text" autocomplete="off">
        </div>
        <div class="course-field">
          <label for="learner-account">Account id</label>
          <input id="learner-account" class="input" type="text" inputmode="numeric" autocomplete="off">
        </div>
        <div class="course-actions">
          <button id="learner-search-btn" class="btn btn-sm btn-secondary" type="button">Search</button>
        </div>
        <div id="learner-results" class="course-learner-list"></div>
        <section id="learner-detail" class="course-learner-detail" hidden>
          <div id="learner-identity"></div>
          <p id="learner-canvas-note" class="course-learner-ops"></p>
          <div class="course-actions">
            <span class="course-action-tip" id="learner-grant-tip">
              <button id="learner-grant-btn" class="btn btn-sm btn-secondary" type="button" disabled>Grant this course</button>
            </span>
            <button id="learner-canvas-btn" class="btn btn-sm btn-secondary" type="button" disabled>Open Canvas user</button>
          </div>
          <div class="course-field">
            <span class="course-field-label">Seats</span>
            <div id="learner-seats" class="course-learner-seats"></div>
          </div>
          <div class="course-field">
            <span class="course-field-label">Orders</span>
            <div id="learner-orders" class="course-learner-orders"></div>
          </div>
        </section>
      </div>
    `;
    pane.querySelector('#learner-search-btn')?.addEventListener('click', () => {
      this.search().catch((error) => {
        toast.error('Search failed', { description: error.message });
      });
    });
    pane.querySelector('#learner-grant-btn')?.addEventListener('click', () => {
      this.grant().catch((error) => {
        toast.error('Grant failed', { description: error.message });
      });
    });
    pane.querySelector('#learner-canvas-btn')?.addEventListener('click', () => this.openCanvasUser());
    pane.querySelector('#learner-seats')?.addEventListener('click', (event) => {
      this.onSeatClick(event).catch((error) => {
        toast.error('Seat update failed', { description: error.message });
      });
    });
    pane.querySelector('#learner-canvas-note').textContent = CANVAS_OPS_NOTE;
    this.paintGrant();
  }

  destroy() {
    this._searchToken += 1;
    this._detailToken += 1;
    this.pane = null;
  }

  async loadFlags() {
    const api = this.manager.app && this.manager.app.api;
    if (!api) return;
    const [grantRows, archiveRows, renewRows] = await Promise.all([
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(STAFF_GRANT_SCRIPT)).catch(() => null),
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(STAFF_ARCHIVE_SCRIPT)).catch(() => null),
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(STAFF_RENEW_SCRIPT)).catch(() => null),
    ]);
    this._grantInvokable = scriptIsInvokable(grantRows);
    this._archiveInvokable = scriptIsInvokable(archiveRows);
    this._renewInvokable = scriptIsInvokable(renewRows);
    this.paintGrant();
  }

  /**
   * @returns {{ courseId: number, pricingType: string, retired: boolean, published: string }}
   */
  courseContext() {
    const manager = this.manager;
    const id = manager._mode === 'edit' ? Number(manager._selectedId) : 0;
    const elements = manager.elements || {};
    return {
      courseId: Number.isInteger(id) && id > 0 ? id : 0,
      pricingType: elements.pricing ? elements.pricing.value : '',
      retired: Boolean(elements.retired && elements.retired.checked),
      published: elements.published ? elements.published.textContent : '',
    };
  }

  paintGrant() {
    if (!this.pane) return;
    const button = this.pane.querySelector('#learner-grant-btn');
    const tip = this.pane.querySelector('#learner-grant-tip');
    if (!button || !tip) return;
    const course = this.courseContext();
    const state = grantButtonState({
      invokable: this._grantInvokable,
      courseId: course.courseId,
      accountId: this._accountId,
      pricingType: course.pricingType,
      retired: course.retired,
      published: course.published,
    });
    button.disabled = state.disabled || this._busy;
    tip.title = state.title;
  }

  async search() {
    if (!this.pane || this._busy) return;
    const built = buildLearnerSearchParams({
      email: this.pane.querySelector('#learner-email').value,
      subject: this.pane.querySelector('#learner-subject').value,
      accountId: this.pane.querySelector('#learner-account').value,
    });
    const results = this.pane.querySelector('#learner-results');
    if (!built.ok) {
      toast.error('Search failed', { description: built.message });
      return;
    }
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      toast.error('Search failed', { description: 'No server connection.' });
      return;
    }
    const token = this._searchToken + 1;
    this._searchToken = token;
    this._detailToken += 1;
    this._accountId = 0;
    this.hideDetail();
    results.textContent = '';
    try {
      const rows = await authQuery(api, LEARNER_SEARCH_QUERY_REF, built.params);
      if (this._searchToken !== token) return;
      const list = Array.isArray(rows) ? rows : [];
      const idNote = built.field === 'account' ? ` ${built.params.INTEGER.ACCOUNTID}` : '';
      log(Subsystems.MANAGER, Status.INFO,
        `Learner search by ${built.field}${idNote}: ${list.length} row(s)`);
      if (list.length === 0) {
        results.textContent = 'No accounts.';
        return;
      }
      if (list.length === 1) {
        await this.openAccount(courseCell(list[0], 'account_id'), token);
        return;
      }
      list.forEach((row) => {
        const id = Number(courseCell(row, 'account_id')) || 0;
        const button = node('button', 'btn btn-sm btn-secondary course-learner-hit');
        button.type = 'button';
        button.textContent = [
          courseCell(row, 'name'),
          courseCell(row, 'email'),
          courseCell(row, 'subject'),
          id ? `#${id}` : '',
        ].filter((part) => part != null && String(part) !== '').join(' · ');
        button.addEventListener('click', () => {
          this.openAccount(id, token).catch((error) => {
            toast.error('Account failed', { description: error.message });
          });
        });
        results.append(button);
      });
    } catch (error) {
      if (this._searchToken !== token) return;
      log(Subsystems.MANAGER, Status.ERROR, `Learner search failed: ${error.message}`);
      results.textContent = 'Search failed.';
      toast.error('Search failed', { description: error.message });
    }
  }

  hideDetail() {
    const detail = this.pane && this.pane.querySelector('#learner-detail');
    if (detail) detail.hidden = true;
    this.paintGrant();
  }

  /**
   * @param {unknown} accountId
   * @param {number} searchToken
   */
  async openAccount(accountId, searchToken) {
    const params = learnerAccountParams(accountId);
    const api = this.manager.app && this.manager.app.api;
    if (!params || !api || !this.pane) return;
    if (searchToken != null && searchToken !== this._searchToken) return;
    const token = this._detailToken + 1;
    this._detailToken = token;
    const id = params.INTEGER.ACCOUNTID;
    try {
      const [detailRows, seatRows, orderRows, linkRows] = await Promise.all([
        authQuery(api, LEARNER_DETAIL_QUERY_REF, params),
        authQuery(api, LEARNER_ENROLLMENTS_QUERY_REF, params),
        authQuery(api, LEARNER_ORDERS_QUERY_REF, params),
        authQuery(api, LEARNER_CANVAS_QUERY_REF, params),
      ]);
      if (this._detailToken !== token || (searchToken != null && this._searchToken !== searchToken)) return;
      const detail = Array.isArray(detailRows) ? detailRows[0] : null;
      if (!detail) {
        this._accountId = 0;
        this.hideDetail();
        toast.error('Account failed', { description: 'No account.' });
        return;
      }
      this._accountId = id;
      this.paintDetail(detail, linkRows, seatRows, orderRows);
      log(Subsystems.MANAGER, Status.INFO, `Learner account ${id} opened`);
    } catch (error) {
      if (this._detailToken !== token) return;
      log(Subsystems.MANAGER, Status.ERROR, `Learner account ${id} failed: ${error.message}`);
      toast.error('Account failed', { description: error.message });
    }
  }

  /**
   * @param {object} detail
   * @param {Array} linkRows
   * @param {Array} seatRows
   * @param {Array} orderRows
   */
  paintDetail(detail, linkRows, seatRows, orderRows) {
    const section = this.pane.querySelector('#learner-detail');
    const identity = this.pane.querySelector('#learner-identity');
    section.hidden = false;
    identity.textContent = '';
    const link = Array.isArray(linkRows) && linkRows[0] ? linkRows[0] : {};
    const fields = [
      ['Account id', String(courseCell(detail, 'account_id') ?? '')],
      ['Name', String(courseCell(detail, 'name') ?? '')],
      ['First name', String(courseCell(detail, 'first_name') ?? '')],
      ['Last name', String(courseCell(detail, 'last_name') ?? '')],
      ['Stripe customer', stripeCustomerLabel(courseCell(detail, 'stripe_customer_id'))],
      ['Currency', String(courseCell(detail, 'currency') ?? '')],
      ['Language', String(courseCell(detail, 'preferred_language') ?? '')],
      ['Referral', String(courseCell(detail, 'referral_source') ?? '')],
      ['Learner type', String(courseCell(detail, 'learner_type') ?? '')],
      ['Country', String(courseCell(detail, 'country') ?? '')],
      ['Age band', String(courseCell(detail, 'age_band') ?? '')],
      ['Notify new course', flagText(courseCell(detail, 'notify_new_course'))],
      ['Notify expiring', flagText(courseCell(detail, 'notify_expiring'))],
      ['Notify expired', flagText(courseCell(detail, 'notify_expired'))],
      ['Notify weekly summary', flagText(courseCell(detail, 'notify_weekly_summary'))],
      ['Canvas user id', String(courseCell(link, 'canvas_user_id') ?? '')],
      ['Canvas email', String(courseCell(link, 'canvas_email') ?? '')],
      ['Canvas last seen', String(courseCell(link, 'last_seen_at') ?? '')],
    ];
    fields.forEach(([label, value]) => identity.append(readLine(label, value)));
    const canvasButton = this.pane.querySelector('#learner-canvas-btn');
    const url = canvasUserUrl(courseCell(link, 'canvas_user_id'));
    canvasButton.disabled = !url;
    canvasButton.dataset.url = url;
    this.paintSeats(seatRows);
    this.paintOrders(orderRows);
    this.paintGrant();
  }

  /**
   * @param {Array} rows
   */
  paintSeats(rows) {
    const host = this.pane.querySelector('#learner-seats');
    host.textContent = '';
    const list = Array.isArray(rows) ? rows : [];
    if (list.length === 0) {
      host.textContent = 'No seats.';
      return;
    }
    list.forEach((row) => {
      const courseId = Number(courseCell(row, 'course_id')) || 0;
      const archived = Boolean(courseCell(row, 'archived_at'));
      const policy = String(courseCell(row, 'renew_policy') ?? '');
      const article = node('div', 'course-learner-seat');
      article.dataset.courseId = String(courseId);
      article.dataset.archived = archived ? '1' : '0';
      article.dataset.policy = policy;
      const title = String(courseCell(row, 'title') || courseCell(row, 'code') || `Course ${courseId}`);
      const amount = [
        title,
        String(courseCell(row, 'status') ?? ''),
        policy,
        archived ? 'archived' : 'open',
      ].filter(Boolean).join(' · ');
      article.append(node('span', 'course-learner-seat-label', amount));
      const archiveState = archiveButtonState({ invokable: this._archiveInvokable, archived });
      const renewState = renewButtonState({
        invokable: this._renewInvokable,
        renewPolicy: policy,
        archived,
      });
      article.append(this.actionButton('course-learner-archive', archiveState.label, archiveState));
      article.append(this.actionButton('course-learner-renew', 'Renew', renewState));
      host.append(article);
    });
  }

  /**
   * @param {string} className
   * @param {string} label
   * @param {{ disabled: boolean, title: string }} state
   * @returns {HTMLElement}
   */
  actionButton(className, label, state) {
    const tip = node('span', 'course-action-tip');
    tip.title = state.title || '';
    const button = node('button', `btn btn-sm btn-secondary ${className}`, label);
    button.type = 'button';
    button.disabled = state.disabled || this._busy;
    tip.append(button);
    return tip;
  }

  /**
   * @param {Array} rows
   */
  paintOrders(rows) {
    const host = this.pane.querySelector('#learner-orders');
    host.textContent = '';
    const view = orderListView(rows);
    if (view.length === 0) {
      host.textContent = 'No orders.';
      return;
    }
    const mode = this.manager.commerce ? this.manager.commerce._mode : '';
    view.forEach((order) => {
      const article = node('div', 'course-learner-order');
      const text = [
        order.orderNumber || `#${order.orderId}`,
        order.status,
        order.currency,
        order.amount,
      ].filter(Boolean).join(' · ');
      article.append(node('span', 'course-learner-order-label', text));
      const url = stripeDashboardUrl(mode, 'payment', order.paymentId);
      if (url) {
        const anchor = node('a', 'course-learner-order-link', 'Payment');
        anchor.href = url;
        anchor.target = '_blank';
        anchor.rel = 'noopener noreferrer';
        article.append(anchor);
      }
      host.append(article);
    });
  }

  openCanvasUser() {
    const button = this.pane && this.pane.querySelector('#learner-canvas-btn');
    const url = button && button.dataset.url;
    if (!url) return;
    window.open(url, '_blank', 'noopener,noreferrer');
  }

  /**
   * @param {MouseEvent} event
   */
  async onSeatClick(event) {
    const button = event.target instanceof Element ? event.target.closest('button') : null;
    if (!button || button.disabled || this._busy) return;
    const article = button.closest('.course-learner-seat');
    if (!article) return;
    const courseId = Number(article.dataset.courseId);
    if (button.classList.contains('course-learner-archive')) {
      const action = article.dataset.archived === '1' ? 'unarchive' : 'archive';
      const accepted = await confirmDialog({
        title: action === 'archive' ? 'Archive seat' : 'Unarchive seat',
        message: action === 'archive'
          ? 'Remove this person from the Canvas course? The seat stays on the account.'
          : 'Put this person back in the Canvas course?',
        confirmLabel: action === 'archive' ? 'Archive' : 'Unarchive',
      });
      if (!accepted) return;
      await this.runSeat(
        staffArchiveOptions(this._accountId, courseId, action),
        action === 'archive' ? 'Seat archived' : 'Seat restored',
      );
      return;
    }
    if (button.classList.contains('course-learner-renew')) {
      const accepted = await confirmDialog({
        title: 'Renew seat',
        message: 'Extend this free seat by 90 days?',
        confirmLabel: 'Renew',
      });
      if (!accepted) return;
      await this.runSeat(staffRenewOptions(this._accountId, courseId), 'Seat renewed');
    }
  }

  async grant() {
    if (this._busy) return;
    const course = this.courseContext();
    const state = grantButtonState({
      invokable: this._grantInvokable,
      courseId: course.courseId,
      accountId: this._accountId,
      pricingType: course.pricingType,
      retired: course.retired,
      published: course.published,
    });
    if (state.disabled) return;
    const accepted = await confirmDialog({
      title: 'Grant this course',
      message: `Grant this free course to account ${this._accountId}?`,
      confirmLabel: 'Grant',
    });
    if (!accepted) return;
    await this.runSeat(
      staffGrantOptions(this._accountId, course.courseId),
      'Seat granted',
    );
  }

  /**
   * @param {object|null} options
   * @param {string} successLabel
   */
  async runSeat(options, successLabel) {
    if (!options || this._busy) return;
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      toast.error('Action failed', { description: 'No server connection.' });
      return;
    }
    this._busy = true;
    this.paintGrant();
    try {
      const envelope = await invokeScript(api, options);
      const result = staffOutcome(envelope);
      const body = result.ok ? result.body : {};
      log(Subsystems.MANAGER, Status.INFO,
        `${options.script} account ${options.params.account_id} course ${options.params.course_id} ok=${result.ok} code=${result.ok ? '' : result.code}`);
      if (!result.ok) {
        toast.error('Action failed', { description: result.message });
        return;
      }
      if (body.already === true) {
        toast.success('Seat already exists');
      } else if (options.script === STAFF_GRANT_SCRIPT && body.canvas_ok === false) {
        toast.success(successLabel, { description: 'Canvas was not updated.' });
      } else {
        toast.success(successLabel);
      }
      await this.openAccount(this._accountId, this._searchToken);
    } catch (error) {
      toast.error('Action failed', { description: error.message });
    } finally {
      this._busy = false;
      this.paintGrant();
    }
  }
}
