/**
 * Undo stack for a report definition.
 * Each commit stores the diff from the new definition back to the
 * previous one. The open document carries those diffs in `history`.
 * Undo and redo apply one diff, so band and item ids come back unchanged.
 * A new commit drops the redo tail. Clear removes `history` and
 * drops image bytes no item still names.
 */

import { importDefinition } from './document.js';
import { applyPatch, diffValues } from './history-diff.js';
import { pruneUnreferencedImages } from './images.js';

const HISTORY = 'history';
const READ_ERROR = 'Report history could not be read.';

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

/**
 * Definition without its undo trail.
 * @param {object} doc
 * @returns {object}
 */
export function reportBody(doc) {
  const next = clone(doc);
  delete next[HISTORY];
  return next;
}

function withHistory(body, undo, redo) {
  const next = reportBody(body);
  if (undo.length === 0 && redo.length === 0) return next;
  next[HISTORY] = { undo: clone(undo), redo: clone(redo) };
  return next;
}

function readOp(op) {
  if (!op || typeof op !== 'object' || Array.isArray(op)) throw new Error(READ_ERROR);
  if (typeof op.path !== 'string' || op.path === '') throw new Error(READ_ERROR);
  if (op.op === 'del') return { op: 'del', path: op.path };
  if ((op.op === 'set' || op.op === 'ins') && Object.hasOwn(op, 'value')) {
    return { op: op.op, path: op.path, value: clone(op.value) };
  }
  throw new Error(READ_ERROR);
}

function readSteps(value) {
  if (!Array.isArray(value)) throw new Error(READ_ERROR);
  return value.map((step) => {
    if (!Array.isArray(step)) throw new Error(READ_ERROR);
    return step.map(readOp);
  });
}

function readStacks(doc) {
  if (!Object.hasOwn(doc, HISTORY)) return { undo: [], redo: [] };
  const history = doc[HISTORY];
  if (!history || typeof history !== 'object' || Array.isArray(history)) throw new Error(READ_ERROR);
  if (!Object.hasOwn(history, 'undo') || !Object.hasOwn(history, 'redo')) throw new Error(READ_ERROR);
  return {
    undo: readSteps(history.undo),
    redo: readSteps(history.redo),
  };
}

/**
 * @param {object} doc
 */
export function createHistory(doc) {
  const imported = importDefinition(doc);
  const stacks = readStacks(imported);
  let past = stacks.undo;
  let future = stacks.redo;
  let current = withHistory(imported, past, future);

  return {
    get document() {
      return current;
    },
    canUndo() {
      return past.length > 0;
    },
    canRedo() {
      return future.length > 0;
    },
    /**
     * @param {object} next
     * @returns {object}
     */
    commit(next) {
      const stored = importDefinition(next);
      const previous = reportBody(current);
      const upcoming = reportBody(stored);
      const back = diffValues(upcoming, previous);
      if (back.length === 0) return current;
      past = past.concat([back]);
      future = [];
      current = withHistory(upcoming, past, future);
      return current;
    },
    /** @returns {object} */
    undo() {
      if (past.length === 0) return current;
      const body = reportBody(current);
      const restored = applyPatch(body, past[past.length - 1]);
      const forward = diffValues(restored, body);
      past = past.slice(0, -1);
      future = future.concat([forward]);
      current = withHistory(restored, past, future);
      return current;
    },
    /** @returns {object} */
    redo() {
      if (future.length === 0) return current;
      const body = reportBody(current);
      const restored = applyPatch(body, future[future.length - 1]);
      const back = diffValues(restored, body);
      future = future.slice(0, -1);
      past = past.concat([back]);
      current = withHistory(restored, past, future);
      return current;
    },
    /** @returns {object} */
    clear() {
      past = [];
      future = [];
      current = pruneUnreferencedImages(reportBody(current));
      return current;
    },
  };
}
