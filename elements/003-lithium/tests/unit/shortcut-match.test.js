import { describe, expect, it } from 'vitest';
import { matchingShortcut, shortcutCombo, visibleManagerId } from '../../src/core/shortcuts.js';

function key(values) {
  return {
    ctrlKey: false,
    metaKey: false,
    shiftKey: false,
    altKey: false,
    key: '',
    ...values,
  };
}

describe('shortcut matching', () => {
  it('writes Ctrl+Z for a lowercase z and for Cmd', () => {
    expect(shortcutCombo(key({ ctrlKey: true, key: 'z' }))).toBe('Ctrl+Z');
    expect(shortcutCombo(key({ metaKey: true, key: 'y' }))).toBe('Ctrl+Y');
    expect(shortcutCombo(key({ ctrlKey: true, shiftKey: true, key: 'Z' }))).toBe('Ctrl+Shift+Z');
    expect(shortcutCombo(key({ key: 'Enter' }))).toBe('Enter');
  });

  it('runs a manager shortcut only while that slot is visible', () => {
    const undo = { id: 'reports-undo', key: 'Ctrl+Z', options: { managerId: '24' } };
    const zoom = { id: 'zoom', key: 'Ctrl+Shift+Z', options: {} };
    const event = key({ ctrlKey: true, key: 'z' });
    expect(matchingShortcut([zoom, undo], event, '')).toBeNull();
    expect(matchingShortcut([zoom, undo], event, '33')).toBeNull();
    expect(matchingShortcut([zoom, undo], event, '24')).toBe(undo);
    expect(matchingShortcut([zoom, undo], key({ ctrlKey: true, shiftKey: true, key: 'Z' }), '33')).toBe(zoom);
  });

  it('reads the visible manager slot id', () => {
    const root = document.createElement('div');
    const slot = document.createElement('div');
    slot.id = 'manager-slot-24';
    slot.className = 'manager-slot slot-visible';
    root.append(slot);
    expect(visibleManagerId(root)).toBe('24');
    slot.className = 'manager-slot';
    expect(visibleManagerId(root)).toBe('');
  });
});
