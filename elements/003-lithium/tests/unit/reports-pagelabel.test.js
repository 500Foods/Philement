import { describe, expect, it } from 'vitest';
import { dropItem } from '../../src/managers/reports/reports-drop.js';
import { PAGE_LABEL_DEFAULT, SYSTEM_TYPES, systemItemLabel } from '../../src/managers/reports/reports-sample.js';

describe('Phase 23 page items', () => {
  it('treats 61, 62 and 64 as system palette items', () => {
    expect(SYSTEM_TYPES).toEqual(expect.arrayContaining([61, 62, 64]));
  });
  it('drops page number and count with no mask, page label with the default template', () => {
    expect(dropItem(61)).toEqual({ typeKey: 61 });
    expect(dropItem(62)).toEqual({ typeKey: 62 });
    expect(dropItem(64)).toEqual({ typeKey: 64, template: 'Page {n} of {c}' });
    expect(PAGE_LABEL_DEFAULT).toBe('Page {n} of {c}');
  });
  it('labels page items on the canvas', () => {
    expect(systemItemLabel({ typeKey: 61 })).toBe('[Page #]');
    expect(systemItemLabel({ typeKey: 62 })).toBe('[Page Count]');
    expect(systemItemLabel({ typeKey: 64, template: 'Pg {n}/{c}' })).toBe('[Pg {n}/{c}]');
    expect(systemItemLabel({ typeKey: 64 })).toBe('[Page {n} of {c}]');
  });
});
