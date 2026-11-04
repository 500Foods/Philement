/**
 * Aggregate number (lookup 053 key 35), aggregate text (key 34),
 * and aggregate datetime (key 38).
 * The item stores `op` and either `field` or `function`.
 * `weighted` also stores `weight`, which stays a field name.
 * mode, min, max, first, and last keep the function's return type.
 * A field has no declared type, so a field-bound item keeps its key.
 * This phase reduces a field. A function name is stored for later.
 */

/** Lookup 053 key 35, Aggregate Number. */
export const AGGREGATE_NUMBER_TYPE = 35;

/** Lookup 053 key 34, Aggregate Text. */
export const AGGREGATE_TEXT_TYPE = 34;

/** Lookup 053 key 38, Aggregate DateTime. */
export const AGGREGATE_DATETIME_TYPE = 38;

/** Ops the picker offers. */
export const AGGREGATE_OPS = Object.freeze([
  'count', 'filled', 'unique', 'sum', 'avg', 'mean', 'weighted', 'median', 'mode',
  'min', 'max', 'first', 'last',
]);

/** Group, page, and report headers and footers. A detail band is refused. */
export const AGGREGATE_BAND_TYPES = Object.freeze([12, 13, 14, 16, 17, 18]);

export const AGGREGATE_OP_ROW_ID = 'aggregate-op';
export const AGGREGATE_FUNCTION_ROW_ID = 'aggregate-function';
export const AGGREGATE_WEIGHT_ROW_ID = 'aggregate-weight';

const RETURN_ANY = new Set(['text', 'number', 'datetime']);
const NUMERIC_OPS = new Set(['sum', 'avg', 'mean', 'weighted', 'median']);
const KEPT_TYPE_OPS = new Set(['mode', 'min', 'max', 'first', 'last']);

/**
 * @param {number} typeKey
 * @returns {boolean}
 */
export function isAggregateType(typeKey) {
  return typeKey === AGGREGATE_NUMBER_TYPE
    || typeKey === AGGREGATE_TEXT_TYPE
    || typeKey === AGGREGATE_DATETIME_TYPE;
}

/**
 * @param {number} typeKey
 * @returns {boolean}
 */
export function aggregateBandAllows(typeKey) {
  return AGGREGATE_BAND_TYPES.includes(typeKey);
}

/**
 * @param {unknown} functions
 * @param {string} name
 * @returns {string}
 */
function returnsOf(functions, name) {
  const list = Array.isArray(functions) ? functions : [];
  for (const entry of list) {
    if (entry && entry.name === name) return entry.returns || 'text';
  }
  return 'text';
}

/**
 * A function is offered for this op when its `returns` can feed it.
 * A missing `returns` is text. Boolean and rows are not inputs.
 * Numeric ops take a number. Mode takes text or a number.
 * Min, max, first, and last take text, a number, or a datetime.
 * @param {string} op
 * @param {string} [returns]
 * @returns {boolean}
 */
export function aggregateFunctionAllowed(op, returns) {
  const type = returns || 'text';
  if (type === 'boolean' || type === 'rows') return false;
  if (!AGGREGATE_OPS.includes(op)) return false;
  if (NUMERIC_OPS.has(op)) return type === 'number';
  if (op === 'mode') return type === 'text' || type === 'number';
  if (KEPT_TYPE_OPS.has(op)) return RETURN_ANY.has(type);
  return RETURN_ANY.has(type);
}

/**
 * @param {string} op
 * @param {string} returns
 * @returns {number}
 */
function keptTypeKey(op, returns) {
  if (op === 'mode') return returns === 'number' ? AGGREGATE_NUMBER_TYPE : AGGREGATE_TEXT_TYPE;
  if (returns === 'datetime') return AGGREGATE_DATETIME_TYPE;
  if (returns === 'number') return AGGREGATE_NUMBER_TYPE;
  return AGGREGATE_TEXT_TYPE;
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
 * Weight exists only while the op is weighted.
 * A kept-type function selects key 34, 35, or 38.
 * A field-bound item keeps a text or datetime key already stored.
 * @param {object} item
 * @param {unknown} functions
 */
function syncAggregateShape(item, functions) {
  if (item.op === 'weighted') {
    if (!Object.hasOwn(item, 'weight')) item.weight = '';
  } else {
    delete item.weight;
  }
  if (!isAggregateType(item.typeKey)) return;
  const previous = item.typeKey;
  const named = typeof item.function === 'string' && item.function !== '';
  if (KEPT_TYPE_OPS.has(item.op) && named) {
    item.typeKey = keptTypeKey(item.op, returnsOf(functions, item.function));
  } else if (KEPT_TYPE_OPS.has(item.op)) {
    if (item.typeKey !== AGGREGATE_TEXT_TYPE && item.typeKey !== AGGREGATE_DATETIME_TYPE) {
      item.typeKey = AGGREGATE_NUMBER_TYPE;
    }
  } else {
    item.typeKey = AGGREGATE_NUMBER_TYPE;
  }
  if (item.typeKey === AGGREGATE_TEXT_TYPE) delete item.mask;
  else if (!Object.hasOwn(item, 'mask') || item.typeKey !== previous) item.mask = '';
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
  syncAggregateShape(item, functions);
}

/**
 * Binding a field clears a function name. Exactly one of the two is set.
 * Weight stays. A field-bound item keeps a stored text or datetime key.
 * @param {object} item
 * @param {unknown} name
 */
export function setAggregateField(item, name) {
  item.field = String(name ?? '').trim();
  if (item.field !== '') delete item.function;
  syncAggregateShape(item);
}

/**
 * Weight is a field name. The row is shown only while the op is weighted.
 * @param {object} item
 * @param {unknown} name
 */
export function setAggregateWeight(item, name) {
  item.weight = String(name ?? '').trim();
}

/**
 * Naming a function clears the field and selects the aggregate key.
 * @param {object} item
 * @param {unknown} name
 * @param {unknown} functions
 */
export function setAggregateFunction(item, name, functions) {
  const text = String(name ?? '').trim();
  if (text === '') {
    delete item.function;
    if (!Object.hasOwn(item, 'field')) item.field = '';
    syncAggregateShape(item, functions);
    return;
  }
  const op = AGGREGATE_OPS.includes(item.op) ? item.op : 'count';
  if (!aggregateFunctionChoices(functions, op).includes(text)) {
    throw new Error(`Function ${text} is not offered for ${op}.`);
  }
  item.function = text;
  delete item.field;
  syncAggregateShape(item, functions);
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
