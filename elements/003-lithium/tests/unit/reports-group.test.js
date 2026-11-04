import { describe, expect, it } from 'vitest';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { addBandInOrder, canRemoveBand, addGroup, createBlank, exportDefinition, groupPartner, hasGroup, removeBand } from '../../src/managers/reports/model/document.js';
import { applyProperty, attributeKeys, propertyRows } from '../../src/managers/reports/report-properties.js';

const fixture = JSON.parse(readFileSync(resolve(process.cwd(), 'tests/fixtures/reporting/grouped.definition.json'), 'utf8'));

describe('Phase 16 one group level', () => {
  it('adds header before detail and footer after, 0.25 in each', () => {
    const doc = addGroup(createBlank());
    const types = doc.bands.map((band) => band.typeKey);
    const d = types.indexOf(15);
    expect(types[d - 1]).toBe(14);
    expect(types[d + 1]).toBe(16);
    const gh = doc.bands[d - 1];
    const gf = doc.bands[d + 1];
    expect(gh.height).toBe('0.25 in');
    expect(gf.height).toBe('0.25 in');
    expect(gh.field).toBe('');
    expect(Object.hasOwn(gf, 'field')).toBe(false);
  });

  it('nests a second group level: gh1 gh2 detail gf2 gf1', () => {
    const doc = addGroup(addGroup(createBlank()));
    expect(hasGroup(doc)).toBe(true);
    expect(doc.bands.map((band) => band.id)).toEqual(['ph1', 'gh1', 'gh2', 'd1', 'gf2', 'gf1', 'pf1']);
    expect(groupPartner(doc, 'gh1').id).toBe('gf1');
    expect(groupPartner(doc, 'gf2').id).toBe('gh2');
  });

  it('deleting either half of a group removes both', () => {
    const doc = addGroup(addGroup(createBlank()));
    expect(removeBand(doc, 'gf1').bands.map((band) => band.id)).toEqual(['ph1', 'gh2', 'd1', 'gf2', 'pf1']);
    expect(removeBand(doc, 'gh2').bands.map((band) => band.id)).toEqual(['ph1', 'gh1', 'd1', 'gf1', 'pf1']);
    expect(removeBand(doc, 'ph1').bands.map((band) => band.id)).toEqual(['gh1', 'gh2', 'd1', 'gf2', 'gf1', 'pf1']);
  });

  it('bands palette inserts in canvas order with short ids', () => {
    let doc = createBlank();
    for (const t of [18, 12, 13, 15, 17, 14]) doc = addBandInOrder(doc, t);
    expect(doc.bands.map((band) => band.id)).toEqual(['rh1', 'ph1', 'ph2', 'gh1', 'd1', 'd2', 'gf1', 'pf1', 'pf2', 'rf1']);
  });

  it('sets the field on the header only, and export matches fixture keys', () => {
    let doc = addGroup(createBlank());
    const gh = doc.bands.find((band) => band.typeKey === 14);
    const gf = doc.bands.find((band) => band.typeKey === 16);
    expect(attributeKeys(doc, { band: true, bandId: gh.id }, {})).toContain(7);
    expect(attributeKeys(doc, { band: true, bandId: gf.id }, {})).not.toContain(7);
    doc = applyProperty(doc, { band: true, bandId: gh.id }, '7', 'region');
    expect(() => applyProperty(doc, { band: true, bandId: gf.id }, '7', 'x')).toThrow();
    const out = exportDefinition(doc);
    const outGh = out.bands.find((band) => band.typeKey === 14);
    const outGf = out.bands.find((band) => band.typeKey === 16);
    const fxGh = fixture.bands.find((band) => band.typeKey === 14);
    const fxGf = fixture.bands.find((band) => band.typeKey === 16);
    expect(Object.keys(outGh).sort()).toEqual(Object.keys(fxGh).sort());
    expect(Object.keys(outGf).sort()).toEqual(Object.keys(fxGf).sort());
    expect(outGh.field).toBe('region');
    const rows = propertyRows(doc, { band: true, bandId: gh.id }, { sampleColumns: ['region'] });
    expect(rows.find((row) => row.key === 7).value).toBe('region');
  });
  it('keeps the last detail band', () => {
    const doc = createBlank();
    expect(doc.bands.map((band) => band.id)).toEqual(['ph1', 'd1', 'pf1']);
    expect(canRemoveBand(doc, 'd1')).toBe(false);
    expect(() => removeBand(doc, 'd1')).toThrow(/detail band/);
    const two = addBandInOrder(doc, 15);
    expect(canRemoveBand(two, 'd1')).toBe(true);
    expect(canRemoveBand(removeBand(two, 'd2'), 'd1')).toBe(false);
  });
});
