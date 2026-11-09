import { describe, expect, it } from 'vitest';
import { createBlank, addItem } from '../../src/managers/reports/model/document.js';
import {
  CLIENT_SECTIONS, applyProperty, attributeKeys, propertyRows, withUnit,
  fontUnitsOf, positionUnitsOf, pageUnitsOf, displayLengthValue, FONT_SIZE_LIST,
} from '../../src/managers/reports/report-properties.js';
import { fontStyleCss } from '../../src/properties/lithium-properties.js';
import { tableData } from '../../src/properties/lithium-properties.js';
import { outlineEntries } from '../../src/managers/reports/reports-panel.js';
import { createHistory } from '../../src/managers/reports/model/history.js';

const attr = (key, name, collection) => ({ key_idx: key, value_txt: name, collection: JSON.stringify(collection) });
const lookups = {
  objectTypes: [
    { key_idx: 0, value_txt: 'Static Text', collection: JSON.stringify({ attributes: [0, 1, 2, 3, 4, 14, 29, 55, 56] }) },
    { key_idx: 9, value_txt: 'Report Top', collection: JSON.stringify({ attributes: ['Title', 'Link', 'Page Size', 'Position Units', 'Font Units', 'Page Units', 'Grid Color', 'Grid Background Color', 'Font Family', 'Font Size', 'Font Style'] }) },
  ],
  attributes: [
    attr(0, 'Top', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(1, 'Left', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(2, 'Width', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(3, 'Height', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(4, 'H. Align', { Group: 'Formatting', section: 7, Editor: 'List', List: ['Left', 'Center', 'Right', 'Justified'] }),
    attr(55, 'Fore Color', { Group: 'Formatting', section: 7, Label: 'Fore Color', Editor: 'Color' }),
    attr(56, 'Back Color', { Group: 'Formatting', section: 7, Label: 'Back Color', Editor: 'Color' }),
    attr(6, 'Object Name', { Group: 'Object', section: 4, Label: 'Name', Editor: 'Text' }),
    attr(14, 'Rotation', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(15, 'Font Family', { Group: 'Fonts', section: 6, Label: 'Font Family', Editor: 'List', List: ['Vanadium Sans', 'Helvetica'] }),
    attr(16, 'Font Size', { Group: 'Fonts', section: 6, Label: 'Font Size', Editor: 'Number' }),
    attr(17, 'Font Style', { Group: 'Fonts', section: 6, Label: 'Font Style', Editor: 'List', List: ['Normal', 'Bold', 'Italic', 'Bold + Italic'] }),
    attr(18, 'Title', { Group: 'Report', section: 1, Label: 'Title', Editor: 'Text' }),
    attr(34, 'Page Units', { Group: 'Units', section: 17, Label: 'Page Units', Editor: 'List', List: ['in', 'mm', 'cm', 'px', 'pt'] }),
    attr(53, 'Position Units', { Group: 'Units', section: 17, Label: 'Position Units', Editor: 'List', List: ['in', 'mm', 'cm', 'px', 'pt'] }),
    attr(54, 'Font Units', { Group: 'Units', section: 17, Label: 'Font Units', Editor: 'List', List: ['pt', 'in', 'mm', 'cm', 'px'] }),
    attr(21, 'Grid Color', { Group: 'Grid', section: 8, Label: 'Color', Editor: 'Color' }),
    attr(24, 'Grid Background Color', { Group: 'Grid', section: 8, Label: 'Background', Editor: 'Color' }),
    attr(29, 'Link', { Group: 'Object', section: 4, Label: 'Link', Editor: 'None' }),
    attr(30, 'Page Size', { Group: 'Page Layout', section: 2, Label: 'Page Size', Editor: 'List', List: ['US Letter'] }),
  ],
  pageSizes: [
    { key_idx: 3, value_txt: 'US Legal Landscape', collection: JSON.stringify({ 'Page Width': '14 in', 'Page Height': '8.5 in', 'Page Orientation': 'Landscape', 'Page Margin Top': '0.5 in' }) },
  ],
};

function withText() {
  return addItem(createBlank(), 'd1', { typeKey: 0, id: 't1' });
}

describe('report property rows', () => {
  it('attribute list filters the panel', () => {
    const doc = withText();
    const item = attributeKeys(doc, { bandId: 'd1', itemId: 't1' }, lookups);
    expect(item).toEqual([0, 1, 2, 3, 4, 14, 29, 55, 56]);
    expect(item).not.toContain(24);
    const report = attributeKeys(doc, { bandId: 'd1', itemId: '' }, lookups);
    expect(report).toEqual([6, 18, 29, 30, 53, 54, 34, 21, 24, 15, 16, 17]);
    const sel = { bandId: 'd1', itemId: 't1' };
    const labels = propertyRows(doc, sel, lookups).map((row) => row.label);
    expect(labels).toEqual([
      'ID', 'Text', 'Top', 'Left', 'Width', 'Height', 'Rotation',
      'H. Align', 'V. Align', 'Fore Color', 'Back Color',
      'Wrap', 'Fit height', 'Line height', 'Notes', 'Link',
    ]);
    const data = tableData(propertyRows(doc, sel, lookups), [
      { id: 4, label: 'Object', sortSeq: 4 },
      { id: 5, label: 'Position', sortSeq: 5 },
      { id: 7, label: 'Formatting', sortSeq: 7 },
      ...CLIENT_SECTIONS,
    ]);
    const object = data.filter((row) => row.sectionCaption === 'Object').map((row) => row.label);
    expect(object).toEqual(['ID', 'Text', 'Link']);
    expect(object[0]).toBe('ID');
    expect(data.find((row) => row.label === 'ID')).toMatchObject({ value: 't1', readOnly: true });
    expect(data.filter((row) => row.sectionCaption === 'Notes').map((row) => row.label)).toEqual(['Notes']);
    expect(data.filter((row) => row.sectionCaption === 'Formatting').map((row) => row.label).slice(0, 4))
      .toEqual(['H. Align', 'V. Align', 'Fore Color', 'Back Color']);
  });

  it('reads editors: Color from 054 on 21 and 24, text on Link, 057 on Page Size', () => {
    const rows = propertyRows(createBlank(), {}, lookups);
    const editor = (key) => rows.find((row) => row.key === key).editor;
    expect(editor(21)).toBe('Color');
    expect(editor(24)).toBe('Color');
    const plain = { ...lookups, attributes: lookups.attributes.map((row) => (row.key_idx === 21 ? attr(21, 'Grid Color', { Group: 'Grid', section: 8, Editor: 'Text' }) : row)) };
    expect(propertyRows(createBlank(), {}, plain).find((row) => row.key === 21).editor).toBe('Text');
    expect(editor(29)).toBe('Text');
    expect(editor(30)).toBe('PageSize');
    expect(rows.find((row) => row.key === 30).list).toBeNull();
  });

  it('shows omitted values as defaults', () => {
    const doc = createBlank();
    delete doc.grid.background;
    delete doc.font;
    const rows = propertyRows(doc, {}, lookups);
    const bg = rows.find((row) => row.key === 24);
    expect(bg).toMatchObject({ value: 'white', omitted: true });
    expect(rows.find((row) => row.key === 16)).toMatchObject({ value: '10 pt', omitted: true });
    expect(rows.find((row) => row.key === 18)).toMatchObject({ value: 'Report', omitted: false });
    const link = propertyRows(withText(), { bandId: 'd1', itemId: 't1' }, lookups).find((row) => row.key === 29);
    expect(link).toMatchObject({ value: '', omitted: true });
  });

  it('applies report and item edits as new documents', () => {
    const doc = withText();
    const titled = applyProperty(doc, {}, '18', 'Orders');
    expect(titled.title).toBe('Orders');
    expect(doc.title).toBe('Report');
    const sized = applyProperty(doc, {}, '30', 3, { pageSizes: lookups.pageSizes });
    expect(sized.page).toMatchObject({ sizeKey: 3, width: '14 in', height: '8.5 in', orientation: 'landscape' });
    expect(sized.page.margin.top).toBe('0.5 in');
    const sel = { bandId: 'd1', itemId: 't1' };
    const moved = applyProperty(doc, sel, '1', '0.5');
    expect(moved.bands[1].items[0].box.left).toBe('0.5 in');
    const texted = applyProperty(doc, sel, 'text', 'Hello');
    expect(texted.bands[1].items[0].text).toBe('Hello');
    const linked = applyProperty(doc, sel, '29', 'https://example.com');
    expect(linked.bands[1].items[0].link).toBe('https://example.com');
    expect(() => applyProperty(doc, sel, '24', 'red')).toThrow(/read only/);
    expect(withUnit('2', doc)).toBe('2 in');
    expect(withUnit('2 cm', doc)).toBe('2 cm');
  });

  it('makes each edit one undo step', () => {
    const doc = createBlank();
    const history = createHistory(doc);
    history.commit(applyProperty(history.document, {}, '6', 'Orders'));
    history.commit(applyProperty(history.document, {}, '18', 'Order list'));
    expect(history.document).toMatchObject({ name: 'Orders', title: 'Order list' });
    expect(history.undo().title).toBe('Report');
    expect(history.undo().name).toBe('Report');
  });

  it('groups table data by section order and lists the outline', () => {
    const rows = propertyRows(createBlank(), {}, lookups);
    const data = tableData(rows, [
      { id: 1, label: 'Report', sortSeq: 1 },
      { id: 2, label: 'Page Layout', sortSeq: 2 },
      { id: 4, label: 'Object', sortSeq: 4 },
      { id: 6, label: 'Fonts', sortSeq: 6 },
      { id: 8, label: 'Grid', sortSeq: 8 },
    ]);
    expect([...new Set(data.map((row) => row.sectionName))]).toEqual(['00 Report', '01 Page Layout', '02 Object', '03 Fonts', '04 Grid', '05 Other']);
    expect(outlineEntries(withText()).map((entry) => entry.label)).toEqual(['Report', 'ph1', 'd1', 't1 Text', 'pf1', 'End of Report']);
  });

  it('stores notes on the report, the band, and the item, and drops a blank', () => {
    const doc = withText();
    const sel = { bandId: 'd1', itemId: 't1' };
    const itemNoted = applyProperty(doc, sel, 'notes', 'keep\nleft');
    expect(itemNoted.bands[1].items[0].notes).toBe('keep\nleft');
    expect(applyProperty(itemNoted, sel, 'notes', '  ').bands[1].items[0].notes).toBeUndefined();
    const bandSel = { band: true, bandId: 'd1' };
    expect(propertyRows(doc, bandSel, lookups).find((row) => row.id === 'object-id').value).toBe('d1');
    const bandNoted = applyProperty(doc, bandSel, 'notes', 'band note');
    expect(bandNoted.bands.find((band) => band.id === 'd1').notes).toBe('band note');
    const reportNoted = applyProperty(doc, {}, 'notes', 'cover');
    expect(reportNoted.notes).toBe('cover');
    expect(propertyRows(doc, {}, lookups).some((row) => row.label === 'ID')).toBe(false);
    expect(() => applyProperty(doc, sel, 'object-id', 'x')).toThrow(/read only/);
  });
});

describe('Phase 76 units and font editors', () => {
  it('resolves Position/Font/Page Units with defaults', () => {
    const doc = createBlank();
    expect(positionUnitsOf(doc)).toBe('in');
    expect(fontUnitsOf(doc)).toBe('pt');
    expect(pageUnitsOf(doc)).toBe('in');
    doc.page.positionUnits = 'mm';
    doc.page.fontUnits = 'px';
    doc.page.units = 'cm';
    expect(withUnit('2', doc)).toBe('2 mm');
    expect(withUnit('2', doc, 'font')).toBe('2 px');
    expect(withUnit('2', doc, 'page')).toBe('2 cm');
    expect(withUnit('2 in', doc, 'font')).toBe('2 in');
  });

  it('bare position and font size use the matching unit; changing units does not rewrite', () => {
    const doc = withText();
    const sel = { bandId: 'd1', itemId: 't1' };
    const moved = applyProperty(doc, sel, '1', '0.5');
    expect(moved.bands[1].items[0].box.left).toBe('0.5 in');
    const sized = applyProperty(doc, sel, '16', '14');
    expect(sized.bands[1].items[0].font.size).toBe('14 pt');
    const next = applyProperty(moved, {}, '53', 'mm');
    expect(next.page.positionUnits).toBe('mm');
    expect(next.bands[1].items[0].box.left).toBe('0.5 in');
  });

  it('shows Units on the report and Font preview editors', () => {
    const rows = propertyRows(createBlank(), {}, lookups);
    expect(rows.find((row) => row.key === 53)).toMatchObject({ label: 'Position Units', value: 'in' });
    expect(rows.find((row) => row.key === 54)).toMatchObject({ label: 'Font Units', value: 'pt' });
    expect(rows.find((row) => row.key === 34)).toMatchObject({ label: 'Page Units', value: 'in' });
    expect(rows.find((row) => row.key === 15)).toMatchObject({
      editor: 'FontFamily',
      list: ['Vanadium Sans', 'Helvetica'],
    });
    expect(rows.find((row) => row.key === 17)).toMatchObject({
      editor: 'FontStyle',
      list: ['Normal', 'Bold', 'Italic', 'Bold + Italic'],
    });
    expect(rows.find((row) => row.key === 16)).toMatchObject({
      editor: 'FontSize',
      display: '10 pt',
      list: FONT_SIZE_LIST.map((n) => ({ value: String(n), label: String(n) })),
    });
    expect(rows.find((row) => row.key === 16).display).toBe('10 pt');
    const itemRows = propertyRows(withText(), { bandId: 'd1', itemId: 't1' }, lookups);
    expect(itemRows.find((row) => row.key === 0).display).toBeDefined();
    expect(displayLengthValue('1.2345 in')).toBe('1.23 in');
    expect(fontStyleCss('Bold + Italic')).toEqual({ fontWeight: '700', fontStyle: 'italic' });
    expect(fontStyleCss('Normal')).toEqual({ fontWeight: '400', fontStyle: 'normal' });
  });
});


describe('Formatting lead order (Phase 78)', () => {
  it('puts H. Align, V. Align, Fore Color, Back Color first in Formatting', () => {
    const doc = addItem(createBlank(), 'd1', { typeKey: 0, id: 't1', text: 'Hi' });
    const sel = { bandId: 'd1', itemId: 't1' };
    const rows = propertyRows(doc, sel, lookups);
    const formatting = rows.filter((row) => Number(row.section) === 7 || row.group === 'Formatting');
    expect(formatting.map((row) => String(row.id)).slice(0, 4)).toEqual(['4', 'static-valign', '55', '56']);
  });
});
