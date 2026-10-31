/**
 * Diffs between two JSON values.
 * A patch applied to the first value produces the second.
 * Paths are JSON Pointers. A list that grows or shrinks records
 * the added or removed entry, not the rest of the list.
 */

const APPLY_ERROR = 'Report history could not be applied.';

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function same(left, right) {
  if (left === right) return true;
  if (typeof left !== typeof right) return false;
  if (left === null || right === null || typeof left !== 'object') return left === right;
  if (Array.isArray(left) || Array.isArray(right)) {
    if (!Array.isArray(left) || !Array.isArray(right) || left.length !== right.length) return false;
    for (let index = 0; index < left.length; index += 1) {
      if (!same(left[index], right[index])) return false;
    }
    return true;
  }
  const leftKeys = Object.keys(left);
  if (leftKeys.length !== Object.keys(right).length) return false;
  for (const key of leftKeys) {
    if (!Object.hasOwn(right, key) || !same(left[key], right[key])) return false;
  }
  return true;
}

function isObject(value) {
  return Boolean(value) && typeof value === 'object' && !Array.isArray(value);
}

function token(key) {
  return String(key).replaceAll('~', '~0').replaceAll('/', '~1');
}

function pointer(parent, key) {
  return `${parent}/${token(key)}`;
}

function unescapeToken(value) {
  return value.replaceAll('~1', '/').replaceAll('~0', '~');
}

function tokensOf(path) {
  if (typeof path !== 'string' || !path.startsWith('/')) throw new Error(APPLY_ERROR);
  return path.slice(1).split('/').map(unescapeToken);
}

/**
 * @param {unknown} from
 * @param {unknown} to
 * @returns {Array<{ op: string, path: string, value?: unknown }>}
 */
export function diffValues(from, to) {
  const patch = [];
  walk('', from, to, patch);
  return patch;
}

function walk(path, from, to, patch) {
  if (same(from, to)) return;
  if (isObject(from) && isObject(to)) {
    const keys = new Set([...Object.keys(from), ...Object.keys(to)]);
    for (const key of keys) {
      const child = pointer(path, key);
      if (!Object.hasOwn(to, key)) patch.push({ op: 'del', path: child });
      else if (!Object.hasOwn(from, key)) patch.push({ op: 'set', path: child, value: clone(to[key]) });
      else walk(child, from[key], to[key], patch);
    }
    return;
  }
  if (Array.isArray(from) && Array.isArray(to)) {
    diffArray(path, from, to, patch);
    return;
  }
  patch.push({ op: 'set', path, value: clone(to) });
}

function diffArray(path, from, to, patch) {
  let start = 0;
  while (start < from.length && start < to.length && same(from[start], to[start])) start += 1;
  let fromEnd = from.length;
  let toEnd = to.length;
  while (fromEnd > start && toEnd > start && same(from[fromEnd - 1], to[toEnd - 1])) {
    fromEnd -= 1;
    toEnd -= 1;
  }
  if (fromEnd - start === toEnd - start) {
    for (let index = start; index < fromEnd; index += 1) {
      walk(`${path}/${index}`, from[index], to[index], patch);
    }
    return;
  }
  for (let index = fromEnd - 1; index >= start; index -= 1) {
    patch.push({ op: 'del', path: `${path}/${index}` });
  }
  for (let index = start; index < toEnd; index += 1) {
    patch.push({ op: 'ins', path: `${path}/${index}`, value: clone(to[index]) });
  }
}

function refuseKey(key) {
  if (key === '__proto__' || key === 'prototype' || key === 'constructor') {
    throw new Error(APPLY_ERROR);
  }
}

function arrayIndex(tokenValue, length, allowAppend) {
  if (!/^(0|[1-9]\d*)$/.test(tokenValue)) throw new Error(APPLY_ERROR);
  const index = Number(tokenValue);
  const max = allowAppend ? length : length - 1;
  if (index > max) throw new Error(APPLY_ERROR);
  return index;
}

function parentOf(root, path) {
  const tokens = tokensOf(path);
  const last = tokens.pop();
  let parent = root;
  for (const key of tokens) {
    if (Array.isArray(parent)) parent = parent[arrayIndex(key, parent.length, false)];
    else if (isObject(parent) && Object.hasOwn(parent, key)) parent = parent[key];
    else throw new Error(APPLY_ERROR);
  }
  return { parent, last };
}

function applyOne(root, op) {
  if (op.path === '') {
    if (op.op !== 'set') throw new Error(APPLY_ERROR);
    return clone(op.value);
  }
  const { parent, last } = parentOf(root, op.path);
  if (Array.isArray(parent)) {
    if (op.op === 'ins') {
      const index = arrayIndex(last, parent.length, true);
      parent.splice(index, 0, clone(op.value));
      return root;
    }
    const index = arrayIndex(last, parent.length, op.op === 'set');
    if (op.op === 'del') parent.splice(index, 1);
    else if (op.op === 'set') parent[index] = clone(op.value);
    else throw new Error(APPLY_ERROR);
    return root;
  }
  if (!isObject(parent)) throw new Error(APPLY_ERROR);
  refuseKey(last);
  if (op.op === 'del') {
    if (!Object.hasOwn(parent, last)) throw new Error(APPLY_ERROR);
    delete parent[last];
  } else if (op.op === 'set') parent[last] = clone(op.value);
  else throw new Error(APPLY_ERROR);
  return root;
}

function deleteRank(path) {
  return tokensOf(path).map((key) => (/^(0|[1-9]\d*)$/.test(key) ? Number(key) : key));
}

function compareDeletes(left, right) {
  const a = deleteRank(left);
  const b = deleteRank(right);
  const length = Math.max(a.length, b.length);
  for (let index = 0; index < length; index += 1) {
    if (a[index] === undefined) return 1;
    if (b[index] === undefined) return -1;
    if (a[index] === b[index]) continue;
    if (typeof a[index] === 'number' && typeof b[index] === 'number') return b[index] - a[index];
    return String(a[index]) < String(b[index]) ? 1 : -1;
  }
  return 0;
}

/**
 * @param {unknown} value
 * @param {Array<{ op: string, path: string, value?: unknown }>} patch
 * @returns {unknown}
 */
export function applyPatch(value, patch) {
  let next = clone(value);
  const deletions = patch.filter((op) => op.op === 'del').map((op) => op.path);
  deletions.sort(compareDeletes);
  for (const path of deletions) next = applyOne(next, { op: 'del', path });
  for (const op of patch) {
    if (op.op === 'set') next = applyOne(next, op);
  }
  const insertions = patch.filter((op) => op.op === 'ins');
  insertions.sort((left, right) => compareDeletes(right.path, left.path));
  for (const op of insertions) next = applyOne(next, op);
  return next;
}
