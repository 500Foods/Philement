/**
 * Session-log lines for Course Manager Save prices.
 * Amounts are cents. The staff id is omitted. Statement rows log
 * counts only, never the price or account fields inside them.
 */

import { COURSE_PRICE_SAVE_QUERY_REF } from './course-catalog.js';

const PREFIXES = ['CAD', 'USD', 'EUR', 'GBP'];

/**
 * @param {unknown} courseId
 * @param {{ INTEGER?: object }} params
 * @returns {string[]}
 */
export function priceSaveRequestItems(courseId, params) {
  const integer = params && params.INTEGER ? params.INTEGER : {};
  const items = [
    `query_ref=${COURSE_PRICE_SAVE_QUERY_REF}`,
    `course_id=${Number(courseId) || 0}`,
  ];
  PREFIXES.forEach((prefix) => {
    items.push(
      `${prefix} present=${integer[`${prefix}_PRESENT`]} keep=${integer[`${prefix}_KEEP`]} cents=${integer[`${prefix}_CENTS`]} active=${integer[`${prefix}_ACTIVE`]}`,
    );
  });
  return items;
}

/**
 * @param {object|null|undefined} row
 * @returns {boolean}
 */
function isStatementRow(row) {
  return Boolean(row) && typeof row === 'object' && ('affected' in row || 'statement' in row);
}

/**
 * @param {unknown} response
 * @returns {string[]}
 */
export function priceSaveResponseItems(response) {
  const body = response && typeof response === 'object' ? response : {};
  const items = [
    `success=${String(body.success)}`,
    `query_ref=${body.query_ref ?? ''}`,
    `affected_rows=${body.affected_rows ?? ''}`,
    `row_count=${body.row_count ?? ''}`,
  ];
  if (body.message) items.push(`message=${String(body.message).slice(0, 180)}`);
  const rows = Array.isArray(body.rows) ? body.rows : [];
  const statements = rows.filter(isStatementRow);
  if (statements.length === 0) {
    items.push(`statement_rows=0 data_rows=${rows.length}`);
    return items;
  }
  statements.forEach((row) => {
    const error = row.error ? ` error=${String(row.error).slice(0, 120)}` : '';
    items.push(
      `block=${row.block ?? ''} statement=${row.statement ?? ''} affected=${row.affected ?? ''} success=${row.success}${error}`,
    );
  });
  return items;
}

/**
 * A save counts only when the server says success and some statement
 * changed a row. An empty body with affected_rows 0 is the shape of
 * a missing query that was misread as success, and of a write that
 * matched nothing. Either way the typed amount should stay.
 *
 * @param {unknown} response
 * @returns {{ ok: true } | { ok: false, message: string }}
 */
export function priceSaveAccepted(response) {
  const body = response && typeof response === 'object' ? response : {};
  if (body.success !== true) {
    return {
      ok: false,
      message: body.message || 'The server did not accept the price save.',
    };
  }
  const rows = Array.isArray(body.rows) ? body.rows : [];
  const statements = rows.filter(isStatementRow);
  if (statements.some((row) => row.success === false)) {
    const failed = statements.find((row) => row.success === false);
    return { ok: false, message: (failed && failed.error) || 'A price statement failed.' };
  }
  const fromStatements = statements.reduce(
    (sum, row) => sum + (Number(row.affected) || 0),
    0,
  );
  const top = Number(body.affected_rows) || 0;
  if (Math.max(fromStatements, top) <= 0) {
    return { ok: false, message: 'The server changed no price rows.' };
  }
  return { ok: true };
}
