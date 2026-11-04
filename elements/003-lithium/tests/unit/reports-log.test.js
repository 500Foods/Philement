import { describe, expect, it } from 'vitest';
import {
  generationCounts,
  generationEntries,
  generationFlag,
  generationText,
} from '../../src/managers/reports/reports-log.js';

const columnEnd = (line, label) => {
  const at = line.indexOf(label);
  return at + label.length;
};

describe('generation log text', () => {
  it('shows zero counts and no flag when the log is empty', () => {
    expect(generationText([])).toBe('info  warn  error\n   0     0      0\n\nlevel  message\n');
    expect(generationText({})).toBe(generationText([]));
    expect(generationFlag(generationCounts(generationEntries(null)))).toBe('');
  });

  it('aligns counts on the right and messages in one column', () => {
    const text = generationText([
      { level: 'info', message: 'printed UTC' },
      { level: 'warn', message: 'datasource orders has no array in sources; no rows' },
      { level: 'note' },
    ]);
    const [head, nums, blank, levelHead, infoRow, warnRow, noteRow] = text.split('\n');
    expect(blank).toBe('');
    expect(head).toBe('info  warn  error  flag');
    expect(nums).toBe('   1     1      0  warn');
    expect(columnEnd(nums, '1')).toBe(columnEnd(head, 'info'));
    expect(columnEnd(nums, '0')).toBe(columnEnd(head, 'error'));
    expect(nums.endsWith('warn')).toBe(true);
    expect(columnEnd(nums, 'warn')).toBe(columnEnd(head, 'flag'));
    expect(levelHead).toBe('level  message');
    expect(infoRow.indexOf('printed UTC')).toBe(warnRow.indexOf('datasource'));
    expect(noteRow.indexOf('message') === -1).toBe(true);
    expect(noteRow.startsWith('note   ')).toBe(true);
    expect(generationCounts(generationEntries([
      { level: 'warn', message: 'a' },
      { level: 'error', message: 'b' },
      { level: 'info', message: 'c' },
    ]))).toEqual({ info: 1, warn: 1, error: 1 });
    expect(generationFlag({ info: 1, warn: 1, error: 1 })).toBe('error');
    expect(generationFlag({ info: 2, warn: 0, error: 0 })).toBe('info');
  });

  it('keeps a wrapped message in the message column', () => {
    const text = generationText([{ level: 'warn', message: 'first\nsecond' }]);
    const rows = text.split('\n');
    const messageAt = rows[4].indexOf('first');
    expect(rows[5].slice(messageAt)).toBe('second');
  });
});
