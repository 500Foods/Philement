/**
 * Undo stack for a report definition.
 * Each commit stores a snapshot. Undo and redo restore that snapshot,
 * so band and item ids come back unchanged. A new commit drops the redo tail.
 */

import { importDefinition } from './document.js';

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

/**
 * @param {object} doc
 */
export function createHistory(doc) {
  const past = [];
  let future = [];
  let current = importDefinition(doc);

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
      past.push(clone(current));
      current = stored;
      future = [];
      return current;
    },
    /** @returns {object} */
    undo() {
      if (past.length === 0) return current;
      future.push(clone(current));
      current = past.pop();
      return current;
    },
    /** @returns {object} */
    redo() {
      if (future.length === 0) return current;
      past.push(clone(current));
      current = future.pop();
      return current;
    },
  };
}
