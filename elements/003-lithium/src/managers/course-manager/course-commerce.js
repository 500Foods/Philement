/**
 * Course Manager commerce (CATCHUP Phase 14).
 *
 * Price edits write Lithium cents through QueryRef 4506. A blank
 * amount deactivates the row and leaves the stored cents. Stripe
 * ids change only when Stripe.EnsureProduct runs. That script, and
 * Refund and Deactivate, stay disabled until QueryRef 87 says
 * invokable is 1. Stripe.Mode returns test or live and never a key.
 */

import { log, logGroup, Subsystems, Status } from '../../core/log.js';
import { authQuery, authQueryResponse, invokeScript } from '../../shared/conduit.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import {
  COURSE_ORDERS_QUERY_REF,
  COURSE_PRICE_CURRENCIES,
  COURSE_PRICE_SAVE_QUERY_REF,
  COURSE_PRICES_QUERY_REF,
  NOT_ENABLED_TIP,
  SCRIPT_DETAIL_QUERY_REF,
  actionOutcome,
  courseCell,
  courseUserId,
  formatPriceAmount,
  priceListView,
  scriptDetailParams,
  scriptIsInvokable,
  stripeLabel,
} from './course-catalog.js';
import {
  priceSaveAccepted,
  priceSaveRequestItems,
  priceSaveResponseItems,
} from './course-price-log.js';

const MAX_INT = 2147483647;
const FREE_TIP = 'Free courses do not use Stripe.';
const ENSURE_SCRIPT = 'Stripe.EnsureProduct';
const REFUND_SCRIPT = 'Stripe.Refund';
const DEACTIVATE_SCRIPT = 'Stripe.DeactivateCustomer';
const MODE_SCRIPT = 'Stripe.Mode';

const PREFIX = {
  cad: 'CAD',
  usd: 'USD',
  eur: 'EUR',
  gbp: 'GBP',
};

/**
 * @param {unknown} courseId
 * @param {'ensure'|'rotate'|undefined} action
 * @returns {object|null}
 */
export function ensureProductOptions(courseId, action) {
  const id = Number(courseId);
  if (!Number.isInteger(id) || id <= 0) return null;
  const params = { course_id: id };
  if (action === 'rotate') params.action = 'rotate';
  return {
    script: ENSURE_SCRIPT,
    params,
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @param {unknown} orderId
 * @param {unknown} reason
 * @returns {object|null}
 */
export function refundOptions(orderId, reason) {
  const id = Number(orderId);
  if (!Number.isInteger(id) || id <= 0) return null;
  let text = String(reason ?? '');
  if (text.length > 500) text = text.slice(0, 500);
  return {
    script: REFUND_SCRIPT,
    params: { order_id: id, reason: text },
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @param {unknown} accountId
 * @returns {object|null}
 */
export function deactivateOptions(accountId) {
  const id = Number(accountId);
  if (!Number.isInteger(id) || id <= 0) return null;
  return {
    script: DEACTIVATE_SCRIPT,
    params: { account_id: id },
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @returns {object}
 */
export function modeInvokeOptions() {
  return {
    script: MODE_SCRIPT,
    params: {},
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @param {unknown} text
 * @returns {{ ok: true, empty: boolean, cents: number } | { ok: false, message: string }}
 */
export function parsePriceDollars(text) {
  const raw = String(text ?? '').trim();
  if (!raw || raw === '0' || raw === '0.0' || raw === '0.00') {
    return { ok: true, empty: true, cents: 0 };
  }
  if (!/^\d+(\.\d{1,2})?$/.test(raw)) {
    return { ok: false, message: 'Enter a dollar amount with at most two decimal places.' };
  }
  const [whole, frac = ''] = raw.split('.');
  const cents = (Number(whole) * 100) + Number((`${frac}00`).slice(0, 2));
  if (!Number.isSafeInteger(cents) || cents > MAX_INT) {
    return { ok: false, message: 'That amount is too large.' };
  }
  if (cents <= 0) return { ok: true, empty: true, cents: 0 };
  return { ok: true, empty: false, cents };
}

/**
 * @param {unknown} courseId
 * @param {unknown} userId
 * @param {Array<object>} drafts
 * @returns {{ ok: true, params: object } | { ok: false, message: string }}
 */
export function buildPriceSaveParams(courseId, userId, drafts) {
  const id = Number(courseId);
  if (!Number.isInteger(id) || id <= 0) {
    return { ok: false, message: 'Save the course before editing prices.' };
  }
  const byCurrency = new Map((drafts || []).map((row) => [row.currency, row]));
  const integer = {
    COURSEID: id,
    USERID: courseUserId({ auth: { user: { id: userId } } }),
  };
  for (let index = 0; index < COURSE_PRICE_CURRENCIES.length; index += 1) {
    const currency = COURSE_PRICE_CURRENCIES[index];
    const prefix = PREFIX[currency];
    const draft = byCurrency.get(currency) || {};
    const parsed = parsePriceDollars(draft.text);
    if (!parsed.ok) {
      return { ok: false, message: `${prefix}: ${parsed.message}` };
    }
    const hadRow = Boolean(draft.hadRow);
    if (parsed.empty && !hadRow) {
      integer[`${prefix}_PRESENT`] = 0;
      integer[`${prefix}_KEEP`] = 0;
      integer[`${prefix}_CENTS`] = 0;
      integer[`${prefix}_ACTIVE`] = 0;
    } else if (parsed.empty && hadRow) {
      integer[`${prefix}_PRESENT`] = 1;
      integer[`${prefix}_KEEP`] = 1;
      integer[`${prefix}_CENTS`] = Number(draft.storedCents) || 0;
      integer[`${prefix}_ACTIVE`] = 0;
    } else {
      integer[`${prefix}_PRESENT`] = 1;
      integer[`${prefix}_KEEP`] = 0;
      integer[`${prefix}_CENTS`] = parsed.cents;
      integer[`${prefix}_ACTIVE`] = draft.active ? 1 : 0;
    }
  }
  return { ok: true, params: { INTEGER: integer } };
}

/**
 * @param {unknown} courseId
 * @param {unknown} accountText
 * @returns {{ ok: true, params: object } | { ok: false, message: string }}
 */
export function buildOrderParams(courseId, accountText) {
  const id = Number(courseId);
  if (!Number.isInteger(id) || id <= 0) {
    return { ok: false, message: 'Select a course.' };
  }
  const raw = String(accountText ?? '').trim();
  let accountId = 0;
  if (raw) {
    if (!/^[1-9][0-9]*$/.test(raw)) {
      return { ok: false, message: 'Account id must be a positive integer.' };
    }
    accountId = Number(raw);
    if (!Number.isSafeInteger(accountId) || accountId > MAX_INT) {
      return { ok: false, message: 'Account id must be a positive integer.' };
    }
  }
  return { ok: true, params: { INTEGER: { COURSEID: id, ACCOUNTID: accountId } } };
}

/**
 * @param {unknown} value
 * @param {string} prefix
 * @returns {string}
 */
export function safeStripeId(value, prefix) {
  const text = String(value ?? '').trim();
  if (!new RegExp(`^${prefix}[A-Za-z0-9]+$`).test(text)) return '';
  return text;
}

/**
 * @param {unknown} mode
 * @param {'payment'|'customer'} kind
 * @param {unknown} id
 * @returns {string}
 */
export function stripeDashboardUrl(mode, kind, id) {
  const word = String(mode || '');
  if (word !== 'test' && word !== 'live') return '';
  const payment = safeStripeId(id, 'pi_');
  const customer = safeStripeId(id, 'cus_');
  const safe = kind === 'payment' ? payment : customer;
  if (!safe) return '';
  const root = word === 'live'
    ? 'https://dashboard.stripe.com'
    : 'https://dashboard.stripe.com/test';
  const folder = kind === 'payment' ? 'payments' : 'customers';
  return `${root}/${folder}/${safe}`;
}

/**
 * @param {unknown} mode
 * @returns {string}
 */
export function stripeModeLabel(mode) {
  if (mode === 'live') return 'Live';
  if (mode === 'test') return 'Test';
  return 'Mode unknown';
}

/**
 * @param {object|null|undefined} envelope
 * @returns {'test'|'live'|''}
 */
export function stripeModeFromEnvelope(envelope) {
  if (!envelope || envelope.status !== 'completed') return '';
  let body = envelope.result;
  if (typeof body === 'string') {
    try {
      body = JSON.parse(body);
    } catch {
      return '';
    }
  }
  const mode = body && String(body.mode || '').toLowerCase();
  if (mode === 'live' || mode === 'test') return mode;
  return '';
}

/**
 * @param {{ invokable: boolean, hasRow: boolean, paid: boolean }} state
 * @returns {{ disabled: boolean, title: string }}
 */
export function commerceActionState({ invokable, hasRow, paid }) {
  if (!hasRow) return { disabled: true, title: 'Select a course' };
  if (!paid) return { disabled: true, title: FREE_TIP };
  if (!invokable) return { disabled: true, title: NOT_ENABLED_TIP };
  return { disabled: false, title: '' };
}

/**
 * @param {{ invokable: boolean, status: unknown }} state
 * @returns {{ disabled: boolean, title: string }}
 */
export function refundButtonState({ invokable, status }) {
  if (!invokable) return { disabled: true, title: NOT_ENABLED_TIP };
  if (String(status || '').toLowerCase() !== 'completed') {
    return { disabled: true, title: 'Only a completed order can be refunded.' };
  }
  return { disabled: false, title: '' };
}

/**
 * @param {{ invokable: boolean, accountId: unknown }} state
 * @returns {{ disabled: boolean, title: string }}
 */
export function deactivateButtonState({ invokable, accountId }) {
  if (!invokable) return { disabled: true, title: NOT_ENABLED_TIP };
  const id = Number(accountId);
  if (!Number.isInteger(id) || id <= 0) {
    return { disabled: true, title: 'This order has no account.' };
  }
  return { disabled: false, title: '' };
}

/**
 * @param {Array<object>|null|undefined} rows
 * @returns {Map<string, number>}
 */
function centsByCurrency(rows) {
  const cents = new Map();
  (Array.isArray(rows) ? rows : []).forEach((row) => {
    const currency = String(courseCell(row, 'currency') || '').trim().toLowerCase();
    const amount = Number(courseCell(row, 'unit_amount_cents'));
    if (currency && Number.isFinite(amount)) cents.set(currency, amount);
  });
  return cents;
}

/**
 * @param {Array<object>|null|undefined} rows
 * @returns {Array<object>}
 */
export function orderListView(rows) {
  return (Array.isArray(rows) ? rows : []).map((row) => {
    const amount = formatPriceAmount(courseCell(row, 'total_cents'));
    return {
      orderId: Number(courseCell(row, 'order_id')) || 0,
      orderNumber: String(courseCell(row, 'order_number') ?? ''),
      accountId: Number(courseCell(row, 'account_id')) || 0,
      status: String(courseCell(row, 'status') ?? ''),
      currency: String(courseCell(row, 'currency') ?? '').toUpperCase(),
      amount: amount === 'None' ? '' : amount,
      intent: stripeLabel(courseCell(row, 'stripe_intent_id')),
      paymentId: safeStripeId(courseCell(row, 'stripe_intent_id'), 'pi_'),
      customer: stripeLabel(courseCell(row, 'stripe_customer_id')),
      customerId: safeStripeId(courseCell(row, 'stripe_customer_id'), 'cus_'),
      created: String(courseCell(row, 'created_at') ?? ''),
      refunded: String(courseCell(row, 'refunded_at') ?? ''),
    };
  }).filter((row) => row.orderId > 0);
}

export class CourseCommerce {
  /**
   * @param {object} manager
   */
  constructor(manager) {
    this.manager = manager;
    this._editable = false;
    this._busy = false;
    this._courseId = 0;
    this._priceToken = 0;
    this._orderToken = 0;
    this._mode = '';
    this._ensureInvokable = false;
    this._refundInvokable = false;
    this._deactivateInvokable = false;
    this._loaded = new Map();
    this._baseline = '';
    this._abort = new AbortController();
  }

  /**
   * @param {HTMLElement} pane
   */
  mount(pane) {
    this.pane = pane;
    const rows = COURSE_PRICE_CURRENCIES.map((currency) => `
      <div class="course-price-row" data-currency="${currency}">
        <span class="course-price-code">${currency.toUpperCase()}</span>
        <span class="course-price-values">
          <span class="course-price-amount"></span>
          <input class="input course-price-input" type="text" inputmode="decimal" autocomplete="off" disabled>
        </span>
        <span class="course-price-stripe"></span>
        <span class="course-price-state"></span>
        <label class="course-price-active-label">
          <input class="course-price-active" type="checkbox" disabled>
          Active
        </label>
      </div>
    `).join('');
    pane.innerHTML = `
      <div class="course-form">
        <p id="course-stripe-mode" class="course-field-hint">Mode unknown</p>
        <section class="course-section is-empty" id="course-prices">
          <span class="course-section-label">Prices</span>
          <p id="course-prices-status" class="course-field-hint"></p>
          <p class="course-field-hint">Amounts are in dollars. A blank amount deactivates that currency and keeps the saved cents.</p>
          ${rows}
          <div class="course-actions">
            <span class="course-action-tip" id="course-price-save-tip">
              <button id="course-price-save" class="btn btn-sm btn-secondary" type="button" disabled>Save prices</button>
            </span>
          </div>
        </section>
        <div class="course-field">
          <span class="course-field-label">Stripe product</span>
          <div id="course-stripe" class="course-readonly-line"></div>
        </div>
        <div class="course-actions">
          <span class="course-action-tip" id="course-ensure-tip" title="${NOT_ENABLED_TIP}">
            <button id="course-ensure-btn" class="btn btn-sm btn-secondary" type="button" disabled>Ensure Product</button>
          </span>
          <span class="course-action-tip" id="course-rotate-tip" title="${NOT_ENABLED_TIP}">
            <button id="course-rotate-btn" class="btn btn-sm btn-secondary" type="button" disabled>Update Stripe prices</button>
          </span>
        </div>
        <section class="course-section" id="course-orders-section">
          <span class="course-section-label">Orders</span>
          <p id="course-orders-status" class="course-field-hint"></p>
          <div class="course-field">
            <label for="course-order-account">Account id</label>
            <input id="course-order-account" class="input" type="text" inputmode="numeric" autocomplete="off">
          </div>
          <div class="course-field">
            <label for="course-refund-reason">Refund reason</label>
            <input id="course-refund-reason" class="input" type="text" maxlength="500" autocomplete="off">
          </div>
          <div class="course-actions">
            <button id="course-orders-load" class="btn btn-sm btn-secondary" type="button">Load orders</button>
          </div>
          <div id="course-orders" class="course-orders"></div>
        </section>
      </div>
    `;
    const signal = this._abort.signal;
    pane.querySelector('#course-price-save').addEventListener('click', () => this.savePrices(), { signal });
    pane.querySelector('#course-ensure-btn').addEventListener('click', () => this.ensure(), { signal });
    pane.querySelector('#course-rotate-btn').addEventListener('click', () => this.rotate(), { signal });
    pane.querySelector('#course-orders-load').addEventListener('click', () => this.loadOrders(), { signal });
    pane.querySelector('#course-orders').addEventListener('click', (event) => this._onOrderClick(event), { signal });
    COURSE_PRICE_CURRENCIES.forEach((currency) => {
      const row = pane.querySelector(`[data-currency="${currency}"]`);
      const input = row.querySelector('.course-price-input');
      const box = row.querySelector('.course-price-active');
      input.addEventListener('input', () => {
        const parsed = parsePriceDollars(input.value);
        const loaded = this._loaded.get(currency);
        if (loaded && !loaded.hadRow && !box.dataset.touched && parsed.ok && !parsed.empty) {
          box.checked = true;
        }
      }, { signal });
      box.addEventListener('change', () => {
        box.dataset.touched = '1';
      }, { signal });
    });
    this.paintActions();
  }

  destroy() {
    this._abort.abort();
    this._priceToken += 1;
    this._orderToken += 1;
  }

  /**
   * @param {boolean} editable
   */
  setEditable(editable) {
    this._editable = Boolean(editable);
    if (!this.pane) return;
    this.pane.querySelectorAll('.course-price-input, .course-price-active').forEach((node) => {
      node.disabled = !this._editable;
    });
    this.pane.querySelector('#course-prices')?.classList.toggle('is-editing', this._editable);
    this.paintActions();
  }

  /**
   * @param {Array<object>|null|undefined} view
   * @param {string} status
   */
  paintPrices(view, status) {
    const section = this.pane && this.pane.querySelector('#course-prices');
    if (!section) return;
    const statusNode = section.querySelector('#course-prices-status');
    statusNode.textContent = status || '';
    section.classList.toggle('is-empty', Boolean(status));
    const rows = view || [];
    this._loaded = new Map();
    section.querySelectorAll('.course-price-row').forEach((row) => {
      const item = rows.find((entry) => entry.currency === row.dataset.currency);
      row.querySelector('.course-price-amount').textContent = item ? item.amount : '';
      row.querySelector('.course-price-stripe').textContent = item ? item.stripe : '';
      row.querySelector('.course-price-state').textContent = item ? item.active : '';
      const input = row.querySelector('.course-price-input');
      const box = row.querySelector('.course-price-active');
      input.value = item && item.amount !== 'None' ? item.amount : '';
      input.disabled = !this._editable;
      box.checked = Boolean(item && item.active === 'Active');
      box.disabled = !this._editable;
      delete box.dataset.touched;
      if (item) {
        this._loaded.set(item.currency, {
          hadRow: item.amount !== 'None',
          storedCents: Number(item.storedCents) || 0,
        });
      }
    });
    this._baseline = this._signature();
  }

  /**
   * @param {string} status
   */
  clearCourse(status) {
    this._courseId = 0;
    this._priceToken += 1;
    this._orderToken += 1;
    this.paintPrices([], status || 'Select a course.');
    this._paintOrders([], status || 'Select a course.');
    this.paintActions();
  }

  paintMode() {
    const node = this.pane && this.pane.querySelector('#course-stripe-mode');
    if (node) node.textContent = stripeModeLabel(this._mode);
  }

  paintActions() {
    if (!this.pane) return;
    const hasRow = this._hasRow();
    const paid = hasRow && this.manager.elements.pricing
      && this.manager.elements.pricing.value === 'paid';
    const ensure = commerceActionState({
      invokable: this._ensureInvokable,
      hasRow,
      paid,
    });
    this._applyButton(
      this.pane.querySelector('#course-ensure-btn'),
      this.pane.querySelector('#course-ensure-tip'),
      ensure,
    );
    this._applyButton(
      this.pane.querySelector('#course-rotate-btn'),
      this.pane.querySelector('#course-rotate-tip'),
      ensure,
    );
    let saveTitle = '';
    let saveDisabled = this._busy;
    if (!hasRow) {
      saveDisabled = true;
      saveTitle = 'Save the course before editing prices.';
    } else if (!this._editable) {
      saveDisabled = true;
      saveTitle = 'Start edit to change prices.';
    }
    this._applyButton(
      this.pane.querySelector('#course-price-save'),
      this.pane.querySelector('#course-price-save-tip'),
      { disabled: saveDisabled, title: saveTitle },
    );
  }

  async loadFlags() {
    const api = this.manager.app && this.manager.app.api;
    if (!api) return;
    const [ensureRows, refundRows, deactivateRows] = await Promise.all([
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(ENSURE_SCRIPT)).catch(() => null),
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(REFUND_SCRIPT)).catch(() => null),
      authQuery(api, SCRIPT_DETAIL_QUERY_REF, scriptDetailParams(DEACTIVATE_SCRIPT)).catch(() => null),
    ]);
    this._ensureInvokable = scriptIsInvokable(ensureRows);
    this._refundInvokable = scriptIsInvokable(refundRows);
    this._deactivateInvokable = scriptIsInvokable(deactivateRows);
    try {
      const envelope = await invokeScript(api, modeInvokeOptions());
      this._mode = stripeModeFromEnvelope(envelope);
    } catch (error) {
      this._mode = '';
      log(Subsystems.MANAGER, Status.ERROR, `Stripe mode failed: ${error.message}`);
    }
    this.paintMode();
    this.paintActions();
  }

  /**
   * @param {unknown} courseId
   */
  async load(courseId) {
    const id = Number(courseId);
    if (!Number.isInteger(id) || id <= 0) {
      this.clearCourse('Select a course.');
      return;
    }
    if (this._courseId && this._courseId !== id && this._isDirty()) {
      toast.info('Price changes were not saved.');
    }
    this._courseId = id;
    await this._loadPrices(id);
    await this.loadOrders();
    this.paintActions();
  }

  async savePrices() {
    if (this._busy || !this._hasRow()) return;
    const built = buildPriceSaveParams(this._courseId, courseUserId(this.manager.app), this._readDrafts());
    if (!built.ok) {
      toast.error('Prices were not saved', { description: built.message });
      return;
    }
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      toast.error('Prices were not saved', { description: 'No server connection.' });
      return;
    }
    logGroup(
      Subsystems.MANAGER,
      Status.INFO,
      '[Course] Price save',
      priceSaveRequestItems(this._courseId, built.params),
    );
    this._busy = true;
    this.paintActions();
    try {
      const response = await authQueryResponse(api, COURSE_PRICE_SAVE_QUERY_REF, built.params);
      const accepted = priceSaveAccepted(response);
      logGroup(
        Subsystems.MANAGER,
        accepted.ok ? Status.INFO : Status.WARN,
        '[Course] Price save response',
        priceSaveResponseItems(response),
      );
      if (!accepted.ok) {
        toast.error('Prices were not saved', { description: accepted.message });
        return;
      }
      toast.success('Prices saved');
      await this._loadPrices(this._courseId);
    } catch (error) {
      const server = error && error.serverError;
      logGroup(
        Subsystems.MANAGER,
        Status.ERROR,
        '[Course] Price save failed',
        [
          `message=${error.message}`,
          `query_ref=${server && server.queryRef != null ? server.queryRef : COURSE_PRICE_SAVE_QUERY_REF}`,
        ],
      );
      toast.error('Prices were not saved', { description: error.message });
    } finally {
      this._busy = false;
      this.paintActions();
    }
  }

  async ensure() {
    await this._runStripe(ensureProductOptions(this._courseId), 'Stripe product checked');
  }

  async rotate() {
    await this._runStripe(ensureProductOptions(this._courseId, 'rotate'), 'Stripe prices updated');
  }

  async loadOrders() {
    const statusNode = this.pane && this.pane.querySelector('#course-orders-status');
    const built = buildOrderParams(
      this._courseId,
      this.pane && this.pane.querySelector('#course-order-account')
        ? this.pane.querySelector('#course-order-account').value
        : '',
    );
    if (!built.ok) {
      if (statusNode && built.message !== 'Select a course.') {
        toast.error('Orders were not loaded', { description: built.message });
      }
      this._paintOrders([], built.message);
      return;
    }
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      this._paintOrders([], 'Orders are unavailable.');
      return;
    }
    const token = this._orderToken + 1;
    this._orderToken = token;
    try {
      const rows = await authQuery(api, COURSE_ORDERS_QUERY_REF, built.params);
      if (this._orderToken !== token) return;
      const view = orderListView(rows);
      this._paintOrders(view, view.length ? '' : 'No orders.');
    } catch (error) {
      if (this._orderToken !== token) return;
      log(Subsystems.MANAGER, Status.ERROR, `Course orders failed: ${error.message}`);
      this._paintOrders([], 'Orders are unavailable.');
    }
  }

  /**
   * @param {object|null} options
   * @param {string} successLabel
   */
  async _runStripe(options, successLabel) {
    if (!options || this._busy || !this._ensureInvokable) return;
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      toast.error('Action failed', { description: 'No server connection.' });
      return;
    }
    this._busy = true;
    this.paintActions();
    try {
      const envelope = await invokeScript(api, options);
      const outcome = actionOutcome(envelope);
      if (!outcome.ok) {
        toast.error('Action failed', { description: outcome.detail });
        return;
      }
      const result = scriptBody(envelope);
      if (result && result.stripe_product_id && this.manager.elements.stripe) {
        this.manager.elements.stripe.textContent = stripeLabel(result.stripe_product_id);
      }
      toast.success(successLabel);
      await this._loadPrices(this._courseId);
    } catch (error) {
      toast.error('Action failed', { description: error.message });
    } finally {
      this._busy = false;
      this.paintActions();
    }
  }

  /**
   * @param {MouseEvent} event
   */
  async _onOrderClick(event) {
    const button = event.target instanceof Element
      ? event.target.closest('button')
      : null;
    if (!button || button.disabled || this._busy) return;
    const article = button.closest('.course-order');
    if (!article) return;
    const orderId = Number(article.dataset.orderId);
    const accountId = Number(article.dataset.accountId);
    if (button.classList.contains('course-order-refund')) {
      const accepted = await confirmDialog({
        title: 'Refund order',
        message: `Refund order ${orderId}?`,
        confirmLabel: 'Refund',
        danger: true,
      });
      if (!accepted) return;
      const reason = this.pane.querySelector('#course-refund-reason').value;
      await this._runOrder(refundOptions(orderId, reason), 'Refund recorded');
      return;
    }
    if (button.classList.contains('course-order-deactivate')) {
      const accepted = await confirmDialog({
        title: 'Deactivate customer',
        message: `Deactivate the Stripe customer for account ${accountId}? This revokes seats.`,
        confirmLabel: 'Deactivate',
        danger: true,
      });
      if (!accepted) return;
      await this._runOrder(deactivateOptions(accountId), 'Customer deactivated');
    }
  }

  /**
   * @param {object|null} options
   * @param {string} successLabel
   */
  async _runOrder(options, successLabel) {
    if (!options || this._busy) return;
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      toast.error('Action failed', { description: 'No server connection.' });
      return;
    }
    this._busy = true;
    this.paintActions();
    try {
      const envelope = await invokeScript(api, options);
      const outcome = actionOutcome(envelope);
      if (!outcome.ok) {
        toast.error('Action failed', { description: outcome.detail });
        return;
      }
      toast.success(successLabel);
      await this.loadOrders();
    } catch (error) {
      toast.error('Action failed', { description: error.message });
    } finally {
      this._busy = false;
      this.paintActions();
    }
  }

  /**
   * @param {number} courseId
   */
  async _loadPrices(courseId) {
    const api = this.manager.app && this.manager.app.api;
    if (!api) {
      this.paintPrices([], 'Prices are unavailable.');
      return;
    }
    const token = this._priceToken + 1;
    this._priceToken = token;
    try {
      const rows = await authQuery(api, COURSE_PRICES_QUERY_REF, { INTEGER: { COURSEID: courseId } });
      if (this._priceToken !== token) return;
      const view = priceListView(rows);
      const cents = centsByCurrency(rows);
      view.forEach((item) => {
        item.storedCents = cents.get(item.currency) || 0;
      });
      this.paintPrices(view, '');
    } catch (error) {
      if (this._priceToken !== token) return;
      log(Subsystems.MANAGER, Status.ERROR, `Course prices failed: ${error.message}`);
      this.paintPrices([], 'Prices are unavailable.');
    }
  }

  /**
   * @param {Array<object>} view
   * @param {string} status
   */
  _paintOrders(view, status) {
    const statusNode = this.pane && this.pane.querySelector('#course-orders-status');
    const host = this.pane && this.pane.querySelector('#course-orders');
    if (!statusNode || !host) return;
    statusNode.textContent = status || '';
    host.replaceChildren();
    view.forEach((order) => {
      host.appendChild(this._orderArticle(order));
    });
  }

  /**
   * @param {object} order
   * @returns {HTMLElement}
   */
  _orderArticle(order) {
    const article = document.createElement('article');
    article.className = 'course-order';
    article.dataset.orderId = String(order.orderId);
    article.dataset.accountId = String(order.accountId);
    article.append(
      line('Number', order.orderNumber || String(order.orderId)),
      line('Status', order.status),
      line('Account', String(order.accountId || '')),
      line('Amount', `${order.amount} ${order.currency}`.trim()),
      line('Payment', order.intent),
      line('Customer', order.customer),
      line('Created', order.created),
    );
    if (order.refunded) article.append(line('Refunded', order.refunded));
    const paymentUrl = stripeDashboardUrl(this._mode, 'payment', order.paymentId);
    const customerUrl = stripeDashboardUrl(this._mode, 'customer', order.customerId);
    if (paymentUrl) article.append(link(paymentUrl, 'Open payment'));
    if (customerUrl) article.append(link(customerUrl, 'Open customer'));
    const actions = document.createElement('div');
    actions.className = 'course-actions';
    actions.append(
      actionButton('course-order-refund', 'Refund', refundButtonState({
        invokable: this._refundInvokable,
        status: order.status,
      })),
      actionButton('course-order-deactivate', 'Deactivate', deactivateButtonState({
        invokable: this._deactivateInvokable,
        accountId: order.accountId,
      })),
    );
    article.append(actions);
    return article;
  }

  _hasRow() {
    return Number.isInteger(this._courseId) && this._courseId > 0;
  }

  _readDrafts() {
    return COURSE_PRICE_CURRENCIES.map((currency) => {
      const row = this.pane.querySelector(`[data-currency="${currency}"]`);
      const loaded = this._loaded.get(currency) || { hadRow: false, storedCents: 0 };
      return {
        currency,
        text: row.querySelector('.course-price-input').value,
        active: row.querySelector('.course-price-active').checked,
        hadRow: loaded.hadRow,
        storedCents: loaded.storedCents,
      };
    });
  }

  _signature() {
    if (!this.pane) return '';
    return JSON.stringify(COURSE_PRICE_CURRENCIES.map((currency) => {
      const row = this.pane.querySelector(`[data-currency="${currency}"]`);
      return {
        currency,
        text: row.querySelector('.course-price-input').value,
        active: row.querySelector('.course-price-active').checked,
      };
    }));
  }

  _isDirty() {
    return this._signature() !== this._baseline;
  }

  /**
   * @param {HTMLButtonElement|null} button
   * @param {HTMLElement|null} tip
   * @param {{ disabled: boolean, title: string }} state
   */
  _applyButton(button, tip, state) {
    if (!button || !tip) return;
    button.disabled = state.disabled || this._busy;
    tip.title = state.title;
  }
}

/**
 * @param {object|null|undefined} envelope
 * @returns {object|null}
 */
function scriptBody(envelope) {
  if (!envelope) return null;
  let body = envelope.result;
  if (typeof body === 'string') {
    try {
      body = JSON.parse(body);
    } catch {
      return null;
    }
  }
  return body && typeof body === 'object' ? body : null;
}

/**
 * @param {string} label
 * @param {string} value
 * @returns {HTMLElement}
 */
function line(label, value) {
  const row = document.createElement('div');
  row.className = 'course-order-line';
  const name = document.createElement('span');
  name.className = 'course-order-label';
  name.textContent = label;
  const text = document.createElement('span');
  text.textContent = value || '';
  row.append(name, text);
  return row;
}

/**
 * @param {string} href
 * @param {string} label
 * @returns {HTMLAnchorElement}
 */
function link(href, label) {
  const anchor = document.createElement('a');
  anchor.className = 'course-order-link';
  anchor.href = href;
  anchor.target = '_blank';
  anchor.rel = 'noopener noreferrer';
  anchor.textContent = label;
  return anchor;
}

/**
 * @param {string} className
 * @param {string} label
 * @param {{ disabled: boolean, title: string }} state
 * @returns {HTMLElement}
 */
function actionButton(className, label, state) {
  const tip = document.createElement('span');
  tip.className = 'course-action-tip';
  tip.title = state.title;
  const button = document.createElement('button');
  button.type = 'button';
  button.className = `btn btn-sm btn-secondary ${className}`;
  button.textContent = label;
  button.disabled = state.disabled;
  tip.append(button);
  return tip;
}
