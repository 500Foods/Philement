/**
 * Course Manager catalog helpers (CATCHUP Phase 13).
 *
 * Staff calls go through auth_query. That lookup keeps the first
 * cache row for a query_ref, and bootstrap orders by query type
 * descending, so a type-1001 reverse row hides a type-1 row that
 * shares the integer. 4008, 4009, and 4010 are already file
 * numbers. 4006 and 4007 are too. Lua still reads those history
 * refs by query_id. The SPA uses 4500–4505, which are not file
 * numbers. 4004 stays the published catalog. Sync and retire stay
 * disabled until those scripts are invokable. 4505 lists
 * course_prices. Extra bind names that the live SQL does not use
 * are ignored. A name the SQL uses and the client omits fails.
 */

export const COURSE_LIST_QUERY_REF = 4500;
export const COURSE_INSERT_QUERY_REF = 4501;
export const COURSE_UPDATE_QUERY_REF = 4502;
export const ENROLMENT_HISTORY_QUERY_REF = 4503;
export const MANAGEMENT_HISTORY_QUERY_REF = 4504;
export const COURSE_PRICES_QUERY_REF = 4505;
export const SCRIPT_DETAIL_QUERY_REF = 87;
export const COURSE_TABLE_PATH = 'courses/course-manager';
export const COURSE_TABLE_KEY_IDX = 14;

export const SYNC_SCRIPT = 'Catalog.SyncFromCanvas';
export const RETIRE_SCRIPT = 'Catalog.Retire';
export const NOT_ENABLED_TIP = 'not enabled on this server';
export const CANVAS_BASE_URL = 'https://canvas.500courses.com';

/** Fields the list search matches. Same names the list query returns. */
export const COURSE_SEARCH_FIELDS = ['code', 'slug', 'title', 'canvas_course_id'];

/**
 * Right-hand tabs. Course is the brochure write. Canvas is the sync
 * surface and the course image. Commerce lists currency prices.
 * History is the audit lines.
 */
export const COURSE_DETAIL_TABS = [
  { id: 'course', label: 'Course', icon: 'book-open' },
  { id: 'canvas', label: 'Canvas', icon: 'graduation-cap' },
  { id: 'commerce', label: 'Commerce', icon: 'credit-card' },
  { id: 'history', label: 'History', icon: 'clock' },
];

/** Currencies stored on course_prices. Display order is fixed. */
export const COURSE_PRICE_CURRENCIES = ['cad', 'usd', 'eur', 'gbp'];

const MAX_INT = 2147483647;
const LIMITS = {
  code: 50,
  slug: 100,
  title: 500,
  imagePath: 500,
};

/**
 * @param {object|null|undefined} row
 * @param {string} name
 * @returns {*}
 */
export function courseCell(row, name) {
  if (!row || typeof row !== 'object') return undefined;
  if (row[name] != null && row[name] !== '') return row[name];
  const upper = name.toUpperCase();
  if (row[upper] != null && row[upper] !== '') return row[upper];
  return undefined;
}

/**
 * @param {object|null|undefined} app
 * @returns {number}
 */
export function courseUserId(app) {
  const id = Number(app && app.auth && app.auth.user && app.auth.user.id);
  return Number.isInteger(id) ? id : 0;
}

/**
 * @param {object|null|undefined} row
 * @param {string} term
 * @returns {boolean}
 */
export function courseMatchesSearch(row, term) {
  const query = String(term || '').trim().toLowerCase();
  if (!query) return true;
  const fields = COURSE_SEARCH_FIELDS;
  return fields.some((name) => String(courseCell(row, name) ?? '').toLowerCase().includes(query));
}

/**
 * Stable text for the editable fields. View-only copy is left out.
 * @param {object|null|undefined} fields
 * @returns {string}
 */
export function courseDraftSignature(fields) {
  const source = fields || {};
  return JSON.stringify({
    code: String(source.code ?? ''),
    slug: String(source.slug ?? ''),
    title: String(source.title ?? ''),
    imagePath: String(source.imagePath ?? ''),
    canvasCourseId: String(source.canvasCourseId ?? ''),
    pricingType: String(source.pricingType ?? ''),
    featured: source.featured ? 1 : 0,
    retired: source.retired ? 1 : 0,
    sortOrder: String(source.sortOrder ?? ''),
    tags: String(source.tags ?? ''),
  });
}

/**
 * Map a list row onto the detail form. Display strings are already
 * safe to assign with textContent.
 * @param {object|null|undefined} row
 * @returns {object}
 */
export function courseFormFromRow(row) {
  const imagePath = String(courseCell(row, 'image_path') ?? '');
  const canvasId = courseCell(row, 'canvas_course_id');
  return {
    courseId: Number(courseCell(row, 'course_id')) || 0,
    code: String(courseCell(row, 'code') ?? ''),
    slug: String(courseCell(row, 'slug') ?? ''),
    title: String(courseCell(row, 'title') ?? ''),
    imagePath,
    canvasCourseId: canvasId == null ? '' : String(canvasId),
    pricingType: String(courseCell(row, 'pricing_type') ?? 'free').toLowerCase(),
    featured: Number(courseCell(row, 'is_featured')) === 1,
    sortOrder: String(courseCell(row, 'sort_order') ?? '0'),
    tags: tagsText(courseCell(row, 'tags')),
    summary: String(courseCell(row, 'summary') ?? ''),
    description: String(courseCell(row, 'description') ?? ''),
    published: publishedLabel(courseCell(row, 'published')),
    stripe: stripeLabel(courseCell(row, 'stripe_product_id')),
    synced: syncedLabel(courseCell(row, 'canvas_updated_at')),
    retired: Number(courseCell(row, 'retired')) === 1,
    imageUrl: safeImageUrl(imagePath),
    canvasUrl: canvasCourseUrl(canvasId),
  };
}

/**
 * @param {string} value
 * @returns {string} A safe img src, or '' when the value is not an http(s) or site path
 */
export function safeImageUrl(value) {
  const text = String(value || '').trim();
  if (!text || text.startsWith('//')) return '';
  if (text.startsWith('/') && !text.startsWith('//')) return text;
  try {
    const url = new URL(text);
    if (url.protocol === 'https:' || url.protocol === 'http:') return url.href;
  } catch {
    return '';
  }
  return '';
}

/**
 * @param {unknown} canvasCourseId
 * @returns {string}
 */
export function canvasCourseUrl(canvasCourseId) {
  const text = String(canvasCourseId ?? '').trim();
  if (!/^[1-9][0-9]*$/.test(text)) return '';
  const id = Number(text);
  if (!Number.isSafeInteger(id) || id > MAX_INT) return '';
  return `${CANVAS_BASE_URL}/courses/${id}`;
}

/**
 * @param {unknown} value
 * @returns {string}
 */
export function publishedLabel(value) {
  return Number(value) === 1 ? 'Published' : 'Not published';
}

/**
 * @param {unknown} value
 * @returns {string}
 */
export function stripeLabel(value) {
  const text = value == null ? '' : String(value).trim();
  if (!text) return 'None';
  const lower = text.toLowerCase();
  if (lower.startsWith('sk_') || lower.startsWith('rk_') || lower.includes('whsec_')) {
    return 'Hidden';
  }
  return text;
}

/**
 * @param {unknown} value
 * @returns {string}
 */
export function syncedLabel(value) {
  if (value == null || String(value).trim() === '') return 'Not synced';
  return String(value);
}

/**
 * @param {unknown} retired
 * @returns {string}
 */
export function retireLabel(retired) {
  return Number(retired) === 1 ? 'Unretire' : 'Retire';
}

/**
 * @param {unknown} value
 * @returns {string}
 */
export function tagsText(value) {
  if (value == null || value === '') return '{}';
  if (typeof value === 'object') return JSON.stringify(value, null, 2);
  const text = String(value).trim();
  if (!text) return '{}';
  try {
    const parsed = JSON.parse(text);
    if (parsed && typeof parsed === 'object' && !Array.isArray(parsed)) {
      return JSON.stringify(parsed, null, 2);
    }
  } catch {
    return text;
  }
  return text;
}

/**
 * @param {unknown} text
 * @returns {{ ok: true, json: string } | { ok: false, message: string }}
 */
export function parseTags(text) {
  const raw = String(text ?? '').trim();
  if (!raw) return { ok: true, json: '{}' };
  let parsed;
  try {
    parsed = JSON.parse(raw);
  } catch {
    return { ok: false, message: 'Tags must be a JSON object.' };
  }
  if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
    return { ok: false, message: 'Tags must be a JSON object.' };
  }
  return { ok: true, json: JSON.stringify(parsed) };
}

/**
 * @param {Array|null|undefined} rows
 * @returns {boolean}
 */
export function scriptIsInvokable(rows) {
  if (!Array.isArray(rows) || rows.length === 0) return false;
  const flag = rows[0].invokable ?? rows[0].INVOKABLE;
  return Number(flag) === 1;
}

/**
 * @param {string} scriptName - Group.Name
 * @returns {{ STRING: { GROUP_NAME: string, SCRIPT_NAME: string } }}
 */
export function scriptDetailParams(scriptName) {
  const dot = String(scriptName || '').indexOf('.');
  return {
    STRING: {
      GROUP_NAME: dot > 0 ? scriptName.slice(0, dot) : '',
      SCRIPT_NAME: dot > 0 ? scriptName.slice(dot + 1) : '',
    },
  };
}

/**
 * @param {{ invokable: boolean, needsRow: boolean, hasRow: boolean }} state
 * @returns {{ disabled: boolean, title: string }}
 */
export function actionButtonState({ invokable, needsRow, hasRow }) {
  if (!invokable) return { disabled: true, title: NOT_ENABLED_TIP };
  if (needsRow && !hasRow) return { disabled: true, title: 'Select a course' };
  return { disabled: false, title: '' };
}

/**
 * @param {unknown} courseId
 * @param {'one'|'all'} scope
 * @returns {object|null}
 */
export function syncInvokeOptions(courseId, scope) {
  if (scope === 'all') {
    return {
      script: SYNC_SCRIPT,
      params: { action: 'all' },
      wait: true,
      timeoutSeconds: 15,
    };
  }
  const id = Number(courseId);
  if (!Number.isInteger(id) || id <= 0) return null;
  return {
    script: SYNC_SCRIPT,
    params: { course_id: id },
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @param {unknown} courseId
 * @param {unknown} retired
 * @returns {object|null}
 */
export function retireInvokeOptions(courseId, retired) {
  const id = Number(courseId);
  if (!Number.isInteger(id) || id <= 0) return null;
  return {
    script: RETIRE_SCRIPT,
    params: {
      course_id: id,
      action: Number(retired) === 1 ? 'unretire' : 'retire',
    },
    wait: true,
    timeoutSeconds: 15,
  };
}

/**
 * @param {object|null|undefined} envelope
 * @returns {{ ok: boolean, status: string, detail: string }}
 */
export function actionOutcome(envelope) {
  const status = envelope && envelope.status ? String(envelope.status) : 'unknown';
  if (status === 'completed') return { ok: true, status, detail: '' };
  const detail = envelope && envelope.error ? String(envelope.error) : status;
  return { ok: false, status, detail };
}

/**
 * @param {unknown} courseId
 * @returns {{ INTEGER: { COURSEID: number } }|null}
 */
export function historyParams(courseId) {
  const id = Number(courseId);
  if (!Number.isInteger(id) || id <= 0) return null;
  return { INTEGER: { COURSEID: id } };
}

/**
 * @param {Array|null|undefined} rows
 * @returns {string}
 */
export function historyText(rows) {
  if (!Array.isArray(rows) || rows.length === 0) return '';
  return rows
    .map((row) => String((row && (row.line ?? row.LINE)) ?? ''))
    .join('\n');
}

/**
 * @param {string} value
 * @param {string} label
 * @param {number} max
 * @returns {{ ok: true, text: string } | { ok: false, message: string }}
 */
function requiredText(value, label, max) {
  const text = String(value ?? '').trim();
  if (!text) return { ok: false, message: `${label} is required.` };
  if (text.length > max) return { ok: false, message: `${label} is too long.` };
  return { ok: true, text };
}

/**
 * @param {string} value
 * @param {number} max
 * @returns {{ ok: true, text: string } | { ok: false, message: string }}
 */
function optionalText(value, max) {
  const text = String(value ?? '').trim();
  if (text.length > max) return { ok: false, message: 'Image path is too long.' };
  return { ok: true, text };
}

/**
 * @param {unknown} value
 * @returns {{ ok: true, text: string } | { ok: false, message: string }}
 */
function pricingValue(value) {
  const text = String(value ?? '').trim().toLowerCase();
  if (text !== 'free' && text !== 'paid') {
    return { ok: false, message: 'Pricing must be free or paid.' };
  }
  return { ok: true, text };
}

/**
 * @param {unknown} value
 * @returns {{ ok: true, text: string } | { ok: false, message: string }}
 */
function canvasValue(value) {
  const text = String(value ?? '').trim();
  if (!text) return { ok: true, text: '' };
  if (!/^[1-9][0-9]*$/.test(text)) {
    return { ok: false, message: 'Canvas course id must be a positive integer.' };
  }
  const id = Number(text);
  if (!Number.isSafeInteger(id) || id > MAX_INT) {
    return { ok: false, message: 'Canvas course id must be a positive integer.' };
  }
  return { ok: true, text };
}

/**
 * @param {unknown} value
 * @returns {{ ok: true, number: number } | { ok: false, message: string }}
 */
function sortValue(value) {
  const text = String(value ?? '').trim();
  if (!text) return { ok: true, number: 0 };
  if (!/^-?[0-9]+$/.test(text)) {
    return { ok: false, message: 'Sort order must be an integer.' };
  }
  const number = Number(text);
  if (!Number.isSafeInteger(number) || number > MAX_INT || number < -MAX_INT) {
    return { ok: false, message: 'Sort order must be an integer.' };
  }
  return { ok: true, number };
}

/**
 * @param {object} fields
 * @returns {object|null}
 */
function sharedFields(fields) {
  const code = requiredText(fields.code, 'Code', LIMITS.code);
  if (!code.ok) return code;
  const slug = requiredText(fields.slug, 'Slug', LIMITS.slug);
  if (!slug.ok) return slug;
  const imagePath = optionalText(fields.imagePath, LIMITS.imagePath);
  if (!imagePath.ok) return imagePath;
  const canvas = canvasValue(fields.canvasCourseId);
  if (!canvas.ok) return canvas;
  const pricing = pricingValue(fields.pricingType);
  if (!pricing.ok) return pricing;
  const sortOrder = sortValue(fields.sortOrder);
  if (!sortOrder.ok) return sortOrder;
  const tags = parseTags(fields.tags);
  if (!tags.ok) return tags;
  return {
    ok: true,
    code: code.text,
    slug: slug.text,
    imagePath: imagePath.text,
    canvas: canvas.text,
    pricing: pricing.text,
    sortOrder: sortOrder.number,
    tags: tags.json,
    featured: fields.featured ? 1 : 0,
    retired: fields.retired ? 1 : 0,
  };
}

/**
 * @param {object} fields
 * @param {number} userId
 * @returns {{ ok: true, params: object } | { ok: false, message: string }}
 */
export function buildCourseInsertParams(fields, userId) {
  const title = requiredText(fields && fields.title, 'Title', LIMITS.title);
  if (!title.ok) return title;
  const shared = sharedFields(fields || {});
  if (!shared.ok) return shared;
  const id = courseUserId({ auth: { user: { id: userId } } });
  return {
    ok: true,
    params: {
      STRING: {
        CODE: shared.code,
        SLUG: shared.slug,
        TITLE: title.text,
        IMAGE_PATH: shared.imagePath,
        CANVAS_COURSE_ID: shared.canvas,
        PRICING_TYPE: shared.pricing,
        TAGS: shared.tags,
      },
      INTEGER: {
        IS_FEATURED: shared.featured,
        RETIRED: shared.retired,
        SORT_ORDER: shared.sortOrder,
        CREATEDID: id,
        UPDATEDID: id,
      },
    },
  };
}

/**
 * @param {object} fields
 * @param {number} userId
 * @returns {{ ok: true, params: object } | { ok: false, message: string }}
 */
export function buildCourseUpdateParams(fields, userId) {
  const courseId = Number(fields && fields.courseId);
  if (!Number.isInteger(courseId) || courseId <= 0) {
    return { ok: false, message: 'Select a course before saving.' };
  }
  const shared = sharedFields(fields || {});
  if (!shared.ok) return shared;
  return {
    ok: true,
    params: {
      STRING: {
        CODE: shared.code,
        SLUG: shared.slug,
        IMAGE_PATH: shared.imagePath,
        CANVAS_COURSE_ID: shared.canvas,
        PRICING_TYPE: shared.pricing,
        TAGS: shared.tags,
      },
      INTEGER: {
        IS_FEATURED: shared.featured,
        RETIRED: shared.retired,
        SORT_ORDER: shared.sortOrder,
        USERID: courseUserId({ auth: { user: { id: userId } } }),
        COURSEID: courseId,
      },
    },
  };
}

/**
 * Keep a suffix inside a column limit.
 * @param {string} value
 * @param {string} suffix
 * @param {number} max
 * @returns {string}
 */
function clipJoin(value, suffix, max) {
  const base = String(value || '');
  const room = max - suffix.length;
  const head = room > 0 ? base.slice(0, room) : '';
  return `${head}${suffix}`.slice(0, max);
}

/**
 * Brochure copy for the navigator duplicate button. The row is not
 * inserted until Save. Canvas id and the Stripe product are cleared
 * so the copy does not point at the same course.
 * @param {object|null|undefined} row
 * @returns {object}
 */
export function courseDuplicateRow(row) {
  const form = courseFormFromRow(row);
  return {
    code: clipJoin(form.code, '-copy', LIMITS.code),
    slug: clipJoin(form.slug, '-copy', LIMITS.slug),
    title: clipJoin(form.title, ' (Copy)', LIMITS.title),
    image_path: form.imagePath,
    canvas_course_id: null,
    pricing_type: form.pricingType === 'paid' ? 'paid' : 'free',
    is_featured: form.featured ? 1 : 0,
    sort_order: form.sortOrder,
    tags: form.tags,
    published: 0,
    retired: 0,
    stripe_product_id: null,
    summary: '',
    description: '',
  };
}

/**
 * Minor units as a decimal amount. 1999 is 19.99.
 * @param {unknown} cents
 * @returns {string}
 */
export function formatPriceAmount(cents) {
  const amount = Number(cents);
  if (!Number.isFinite(amount)) return 'None';
  return (amount / 100).toFixed(2);
}

/**
 * One display row per catalog currency. A missing currency is None.
 * @param {Array<object>|null|undefined} rows
 * @returns {Array<{ currency: string, amount: string, stripe: string, active: string }>}
 */
export function priceListView(rows) {
  const byCurrency = new Map();
  (Array.isArray(rows) ? rows : []).forEach((row) => {
    const currency = String(courseCell(row, 'currency') || '').trim().toLowerCase();
    if (!currency) return;
    byCurrency.set(currency, {
      currency,
      amount: formatPriceAmount(courseCell(row, 'unit_amount_cents')),
      stripe: stripeLabel(courseCell(row, 'stripe_price_id')),
      active: Number(courseCell(row, 'is_active')) === 0 ? 'Inactive' : 'Active',
    });
  });
  return COURSE_PRICE_CURRENCIES.map((currency) => byCurrency.get(currency) || {
    currency,
    amount: 'None',
    stripe: 'None',
    active: '',
  });
}
