import { describe, it, expect } from 'vitest';
import { shapeDrawing, SEEDED_PATHS, defaultShapeFields, resolveSide, setShapeCatalog, shapeAttributeKeys, shapeKindOptions, shapeSvg, withShapeSections } from '../../src/managers/reports/reports-shape.js';

function box(width, height) {
  return { left: '0 in', top: '0 in', width, height };
}

function shapeItem(kind, extra = {}) {
  return { id: 's', typeKey: 25, kind, box: box('1 in', '0.6 in'), ...extra };
}

describe('debug line positions', () => {
  it('shows line positions', () => {
    const top = shapeDrawing(shapeItem('line', { side: 'top', box: box('1 in', '0.25 in') }));
    const bottom = shapeDrawing(shapeItem('line', { side: 'bottom', box: box('1 in', '0.25 in') }));
    console.log('Top line (0.25in):', top.d);
    console.log('Bottom line (0.25in):', bottom.d);
    
    const top2 = shapeDrawing(shapeItem('line', { side: 'top', box: box('1 in', '0.5 in') }));
    const bottom2 = shapeDrawing(shapeItem('line', { side: 'bottom', box: box('1 in', '0.5 in') }));
    console.log('Top line (0.5in):', top2.d);
    console.log('Bottom line (0.5in):', bottom2.d);
    
    // Check the Y coordinates
    const topY = Number(top.d.match(/M 0 ([\d.]+)/)[1]);
    const bottomY = Number(bottom.d.match(/M 0 ([\d.]+)/)[1]);
    console.log('Top Y:', topY, 'Bottom Y:', bottomY, 'Height:', 24);
    
    const topY2 = Number(top2.d.match(/M 0 ([\d.]+)/)[1]);
    const bottomY2 = Number(bottom2.d.match(/M 0 ([\d.]+)/)[1]);
    console.log('Top Y (0.5in):', topY2, 'Bottom Y (0.5in):', bottomY2, 'Height:', 48);
  });
});
