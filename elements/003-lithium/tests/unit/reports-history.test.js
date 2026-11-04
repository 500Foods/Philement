import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import {
  addItem,
  deleteItem,
  exportDefinition,
  moveItem,
  resizeBand,
} from '../../src/managers/reports/model/document.js';
import { applyPatch, diffValues } from '../../src/managers/reports/model/history-diff.js';
import { createHistory, reportBody } from '../../src/managers/reports/model/history.js';

function loadDetail() {
  const path = resolve(process.cwd(), 'tests/fixtures/reporting/detail.definition.json');
  return JSON.parse(readFileSync(path, 'utf8'));
}

function customer(doc) {
  return doc.bands.find((band) => band.id === 'detail').items.find((entry) => entry.id === 'customer');
}

describe('report history', () => {
  it('undoes a field move back to the detail fixture', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    const moved = moveItem(history.document, 'detail', 'customer', { left: '1 in' });
    expect(customer(moved).box.left).toBe('1 in');
    expect(customer(moved).id).toBe('customer');
    history.commit(moved);
    expect(customer(fixture).box.left).toBe('0 in');
    history.undo();
    expect(history.canRedo()).toBe(true);
    expect(customer(history.document).id).toBe('customer');
    expect(reportBody(history.document)).toEqual(fixture);
    expect(history.document.history.redo).toHaveLength(1);
  });

  it('drops the redo tail when a new edit is committed', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    history.commit(moveItem(history.document, 'detail', 'customer', { left: '1 in' }));
    history.undo();
    history.commit(moveItem(history.document, 'detail', 'customer', { left: '2 in' }));
    expect(history.canRedo()).toBe(false);
    history.redo();
    expect(customer(history.document).box.left).toBe('2 in');
    history.undo();
    expect(reportBody(history.document)).toEqual(fixture);
    expect(history.document.history.undo).toEqual([]);
  });

  it('restores the same item id across undo and redo', () => {
    const history = createHistory(loadDetail());
    const removed = deleteItem(history.document, 'detail', 'amount');
    history.commit(removed);
    expect(history.document.bands[1].items.map((entry) => entry.id)).toEqual(['customer', 'note']);
    history.undo();
    expect(history.document.bands[1].items.map((entry) => entry.id)).toEqual([
      'customer', 'amount', 'note',
    ]);
    history.redo();
    expect(history.document.bands[1].items.map((entry) => entry.id)).toEqual(['customer', 'note']);
    history.undo();
    const added = addItem(history.document, 'detail', { typeKey: 0 });
    history.commit(added);
    expect(history.document.bands[1].items.at(-1).id).toBe('text_1');
    history.undo();
    history.redo();
    expect(history.document.bands[1].items.at(-1).id).toBe('text_1');
  });

  it('restores a band height', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    history.commit(resizeBand(history.document, 'detail', '0.5 in'));
    expect(history.document.bands[1].height).toBe('0.5 in');
    history.undo();
    expect(history.document.bands[1].height).toBe('0.25 in');
    expect(reportBody(history.document)).toEqual(fixture);
  });

  it('does nothing when there is no snapshot to restore', () => {
    const history = createHistory(loadDetail());
    expect(history.canUndo()).toBe(false);
    expect(history.canRedo()).toBe(false);
    history.undo();
    history.redo();
    expect(history.canUndo()).toBe(false);
    expect(() => history.commit({ format: 2 })).toThrow(/Report format must be 1/);
    expect(history.canUndo()).toBe(false);
  });

  it('keeps a height diff on the document and drops it on clear', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    history.commit(history.document);
    expect(history.canUndo()).toBe(false);
    expect(history.document.history).toBeUndefined();
    history.commit(resizeBand(history.document, 'detail', '0.5 in'));
    const saved = exportDefinition(history.document);
    expect(saved.history.undo).toEqual([
      [{ op: 'set', path: '/bands/1/height', value: '0.25 in' }],
    ]);
    expect(JSON.stringify(saved.history).length).toBeLessThan(JSON.stringify(fixture).length);
    const reopened = createHistory(saved);
    expect(reopened.canUndo()).toBe(true);
    reopened.undo();
    expect(reportBody(reopened.document)).toEqual(fixture);
    expect(reopened.document.history.redo).toEqual([
      [{ op: 'set', path: '/bands/1/height', value: '0.5 in' }],
    ]);
    reopened.clear();
    expect(reopened.canUndo()).toBe(false);
    expect(reopened.canRedo()).toBe(false);
    expect(exportDefinition(reopened.document)).toEqual(fixture);
    expect(() => createHistory({ ...fixture, history: { undo: [{}], redo: [] } }))
      .toThrow(/Report history could not be read/);
  });

  it('diffs an object key that contains a slash', () => {
    const from = { format: 1, note: { 'a/b': 1, keep: true } };
    const to = { format: 1, note: { 'a/b': 2 } };
    const patch = diffValues(from, to);
    expect(patch).toEqual([
      { op: 'set', path: '/note/a~1b', value: 2 },
      { op: 'del', path: '/note/keep' },
    ]);
    expect(applyPatch(from, patch)).toEqual(to);
    expect(applyPatch(to, diffValues(to, from))).toEqual(from);
  });

  it('records an added list entry and leaves the other pictures out', () => {
    const picture = 'B'.repeat(8000);
    const from = {
      format: 1,
      bands: [{ id: 'a', picture }, { id: 'c', picture }],
    };
    const to = {
      format: 1,
      bands: [{ id: 'a', picture }, { id: 'b', picture: 'new' }, { id: 'c', picture }],
    };
    const patch = diffValues(from, to);
    expect(patch).toEqual([
      { op: 'ins', path: '/bands/1', value: { id: 'b', picture: 'new' } },
    ]);
    expect(JSON.stringify(patch)).not.toContain(picture);
    expect(applyPatch(from, patch)).toEqual(to);
    const removed = diffValues(to, from);
    expect(removed).toEqual([{ op: 'del', path: '/bands/1' }]);
    expect(applyPatch(to, removed)).toEqual(from);
    const changed = {
      format: 1,
      bands: [{ id: 'a', picture, caption: 'Now' }, { id: 'c', picture }],
    };
    expect(diffValues(from, changed)).toEqual([
      { op: 'set', path: '/bands/0/caption', value: 'Now' },
    ]);
    const replaced = {
      format: 1,
      bands: [{ id: 'a', picture: 'other' }, { id: 'c', picture }],
    };
    expect(diffValues(replaced, from)).toEqual([
      { op: 'set', path: '/bands/0/picture', value: picture },
    ]);
  });
});
