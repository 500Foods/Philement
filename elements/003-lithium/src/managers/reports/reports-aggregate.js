/**
 * Aggregate number (lookup 053 key 35). Phase 38.
 * The item stores `op` and either `field` or `function`.
 * This phase reduces a field. A function name is stored for later.
 */

/** Lookup 053 key 35, Aggregate Number. */
export const AGGREGATE_NUMBER_TYPE = 35;

/** Ops this phase reduces. Later phases add the rest. */
export const AGGREGATE_OPS = Object.freeze(['count', 'filled', 'unique', 'sum']);

/** Bands that show a group or report total. Page bands are Phase 39. */
export const AGGREGATE_BAND_TYPES = Object.freeze([12, 14, 16, 18]);

export const AGGREGATE_OP_ROW_ID = 'aggregate-op';
export const AGGREGATE_FUNCTION_ROW_ID = 'aggregate-function';

const RETURN_TEXT = new Set(['text', 'number', 'datetime']);

/**
 * @param {number} typeKey
 * @returns {boolean}
 */
export function aggregateBandAllows(typeKey) {
  return AGGREGATE_BAND_TYPES.includes(typeKey);
}

/**
 * A function is offered for this op when its `returns` can feed it.
 * A missing `returns` is text. Boolean and rows are not inputs.
 * @param {string} op
 * @param {string} [returns]
 * @returns {boolean}
 */
export function aggregateFunctionAllowed(op, returns) {
  const type = returns || 'text';
  if (type === 'boolean' || type === 'rows') return false;
  if (op === 'sum') return type === 'number';
  return AGGREGATE_OPS.includes(op) && RETURN_TEXT.has(type);
}

/**
 * Function names the op can reduce, in definition order.
 * @param {unknown} functions
 * @param {string} op
 * @returns {string[]}
 */
export function aggregateFunctionChoices(functions, op) {
  const list = Array.isArray(functions) ? functions : [];
  const names = [];
  for (const entry of list) {
    if (!entry || typeof entry.name !== 'string' || entry.name === '') continue;
    if (aggregateFunctionAllowed(op, entry.returns)) names.push(entry.name);
  }
  return names;
}

/**
 * @param {object} item
 * @param {string} op
 * @param {unknown} functions
 */
export function setAggregateOp(item, op, functions) {
  if (!AGGREGATE_OPS.includes(op)) {
    throw new Error(`Aggregate op must be one of ${AGGREGATE_OPS.join(', ')}.`);
  }
  item.op = op;
  if (typeof item.function === 'string' && item.function !== ''
    && !aggregateFunctionChoices(functions, op).includes(item.function)) {
    delete item.function;
    if (!Object.hasOwn(item, 'field')) item.field = '';
  }
}

/**
 * Binding a field clears a function name. Exactly one of the two is set.
 * @param {object} item
 * @param {unknown} name
 */
export function setAggregateField(item, name) {
  item.field = String(name ?? '').trim();
  if (item.field !== '') delete item.function;
}

/**
 * Naming a function clears the field.
 * @param {object} item
 * @param {unknown} name
 * @param {unknown} functions
 */
export function setAggregateFunction(item, name, functions) {
  const text = String(name ?? '').trim();
  if (text === '') {
    delete item.function;
    if (!Object.hasOwn(item, 'field')) item.field = '';
    return;
  }
  const op = AGGREGATE_OPS.includes(item.op) ? item.op : 'count';
  if (!aggregateFunctionChoices(functions, op).includes(text)) {
    throw new Error(`Function ${text} is not offered for ${op}.`);
  }
  item.function = text;
  delete item.field;
}

/**
 * Canvas label: the op and the bound name, or Aggregate when unbound.
 * @param {object} item
 * @returns {string}
 */
export function aggregateItemLabel(item) {
  const op = item && AGGREGATE_OPS.includes(item.op) ? item.op : 'count';
  const name = item && typeof item.function === 'string' && item.function.trim() !== ''
    ? item.function.trim()
    : item && typeof item.field === 'string' ? item.field.trim() : '';
  return name ? `${op} ${name}` : 'Aggregate';
}
