import { describe, expect, it } from 'vitest';
import { createBlank, addItem } from '../../src/managers/reports/model/document.js';
import {
  applyProperty, propertyRows, selectionScope, FONT_NOTE, OMITTED_NOTE, PLAIN_DECIMAL_NOTE, MASK_ROW_ID,
} from '../../src/managers/reports/report-properties.js';
import { matchesSearch, groupHeaderHtml, tableData } from '../../src/properties/lithium-properties.js';

const attr = (key, name, collection) => ({ key_idx: key, value_txt: name, collection: JSON.stringify(collection) });
const lookups = {
  objectTypes: [
    { key_idx: 0, value_txt: 'Static Text', collection: JSON.stringify({ attributes: [0, 15, 16, 99] }) },
    { key_idx: 1, value_txt: 'Static Number', collection: JSON.stringify({ group: 'Number', attributes: [0] }) },
    { key_idx: 9, value_txt: 'Report Top', collection: JSON.stringify({ attributes: ['Font Size', 'Top Margin'] }) },
  ],
  attributes: [
    attr(0, 'Top', { section: 5, Editor: 'Number' }),
    attr(15, 'Font Family', { section: 6, Editor: 'Text' }),
    attr(16, 'Font Size', { section: 6, Editor: 'Number' }),
    attr(35, 'Top Margin', { section: 2, Editor: 'Number' }),
    attr(99, 'Mystery', { section: 4, Editor: 'Sparkle' }),
  ],
};

describe('Phase 14 gaps', () => {
  it('item without a font shows the report font with the override note', () => {
    const doc = addItem(createBlank(), 'detail', { typeKey: 0, id: 't1' });
    const unknown = [];
    const rows = propertyRows(doc, { bandId: 'detail', itemId: 't1' }, lookups, undefined, unknown);
    const family = rows.find((row) => row.key === 15);
    expect(family.omitted).toBe(true);
    expect(family.note).toBe(FONT_NOTE);
    expect(rows.find((row) => row.key === 99)).toBeUndefined();
    expect(unknown).toEqual([{ key: 99, editor: 'Sparkle' }]);
  });

  it('report defaults carry the omitted note', () => {
    const rows = propertyRows(createBlank(), {}, lookups);
    const size = rows.find((row) => row.key === 16);
    if (size.omitted) expect(size.note).toBe(OMITTED_NOTE);
    else expect(size.note).toBe('');
  });

  it('number item with an empty mask shows the plain-decimal rule', () => {
    const doc = addItem(createBlank(), 'detail', { typeKey: 21, id: 'n1', mask: '' });
    const rows = propertyRows(doc, { bandId: 'detail', itemId: 'n1' }, lookups);
    const mask = rows.find((row) => row.id === MASK_ROW_ID);
    expect(mask.note).toBe(PLAIN_DECIMAL_NOTE);
    expect(mask.readOnly).toBe(false);
    expect(applyProperty(doc, { bandId: 'detail', itemId: 'n1' }, MASK_ROW_ID, ' #,##0.00 ')
      .bands.find((b) => b.id === 'detail').items.find((i) => i.id === 'n1').mask).toBe('#,##0.00');
    const text = addItem(createBlank(), 'detail', { typeKey: 0, id: 't1', mask: '' });
    expect(propertyRows(text, { bandId: 'detail', itemId: 't1' }, lookups).some((row) => row.id === MASK_ROW_ID)).toBe(false);
  });

  it('number editor keeps the unit and rejects junk', () => {
    const doc = createBlank();
    const next = applyProperty(doc, {}, '35', '0.75', {});
    expect(next.page.margin.left).toMatch(/^0\.75 /);
    expect(() => applyProperty(doc, {}, '35', 'wide', {})).toThrow();
  });

  it('search matches label, section, and id; header shows the icon', () => {
    const data = { id: '16', label: 'Font Size', sectionCaption: 'Fonts' };
    expect(matchesSearch(data, 'font s')).toBe(true);
    expect(matchesSearch(data, 'fonts')).toBe(true);
    expect(matchesSearch(data, '16')).toBe(true);
    expect(matchesSearch(data, 'grid')).toBe(false);
    expect(groupHeaderHtml('Fonts', '<fa fa-font></fa>', 2)).toContain('<fa fa-font></fa>');
    expect(groupHeaderHtml('<b>', '<script>', 1)).not.toContain('<script>');
    const rows = tableData([{ id: '16', section: 6 }], [{ id: 6, label: 'Fonts', sortSeq: 1, icon: '<fa fa-font></fa>' }]);
    expect(rows[0].sectionIcon).toBe('<fa fa-font></fa>');
  });

  it('band scope shows band rows and edits the band height', () => {
    const doc = createBlank();
    const sel = { bandId: 'detail', itemId: '', band: true };
    expect(selectionScope(doc, sel)).toBe('band');
    expect(selectionScope(doc, { bandId: 'detail', itemId: '' })).toBe('report');
    const withHeight = { ...lookups, attributes: [...lookups.attributes, attr(3, 'Height', { section: 5, Editor: 'Number' })] };
    const rows = propertyRows(doc, sel, withHeight);
    expect(rows.map((row) => row.key)).toEqual([3]);
    const next = applyProperty(doc, sel, '3', '1.5 in', {});
    expect(next.bands.find((band) => band.id === 'detail').height).toBe('1.5 in');
  });
});
