#!/usr/bin/env node

// generate_diagram.js
// Converts JSON table definition into SVG database diagram

// CHANGELOG
// 2.0.0 - 2025-11-17 - Implemented before/after comparison with automatic highlighting
// 1.1.0 - 2025-09-30 - Added metadata, starting with 'Tables included' to output
// 1.0.0 - 2025-09-28 - Initial creation for database diagram generation

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// ============================================================================
// THEME DEFAULTS
// ============================================================================

/**
 * Font settings for different elements
 * Easy to change fonts across the entire diagram
 */
const FONTS = {
    header: 'Cairo, Arial, sans-serif',    // Title font (bold, larger)
    row: 'Cairo, Arial, sans-serif'        // Row text font
};

// ============================================================================
// PREPROCESSING FUNCTIONS
// ============================================================================

/**
 * Remove duplicate objects from JSON data based on object_id
 * @param {Array} objects - Array of objects to deduplicate
 * @returns {Array} - Array with duplicates removed
 */
function removeDuplicateObjects(objects) {
    const seen = new Set();
    return objects.filter(obj => {
        if (seen.has(obj.object_id)) {
            return false;
        }
        seen.add(obj.object_id);
        return true;
    });
}

/**
 * Preprocess JSON data: clean, validate, and remove duplicates
 * @param {Array|Object} data - Raw JSON data
 * @returns {Array} - Processed array of objects
 */
function preprocessJSONData(data) {
    // Ensure we have an array
    const objects = Array.isArray(data) ? data : [data];

    // Remove duplicates
    const deduplicated = removeDuplicateObjects(objects);

    return deduplicated;
}

// ============================================================================
// HIGHLIGHTING FUNCTIONS
// ============================================================================

/**
 * Add highlight flags to "after" JSON based on comparison with "before"
 * @param {Object} beforeData - {diagram: [...]} or just [...]
 * @param {Object} afterData - {diagram: [...]} or just [...]
 * @returns {Array} - Array of table objects with highlight flags added
 */
function addHighlightFlags(beforeData, afterData) {
    // Normalize inputs
    const beforeDiagram = Array.isArray(beforeData) ? beforeData : (beforeData.diagram || []);
    const afterDiagram = Array.isArray(afterData) ? afterData : (afterData.diagram || []);
    
    // Filter to only table objects
    const beforeTables = beforeDiagram.filter(obj => obj.object_type === 'table');
    const afterTables = afterDiagram.filter(obj => obj.object_type === 'table');
    
    console.error(`Comparing ${beforeTables.length} before tables with ${afterTables.length} after tables`);
    
    // Create lookup map of before tables
    const beforeTableMap = new Map();
    beforeTables.forEach(table => {
        beforeTableMap.set(table.object_id, table);
    });
    
    // Process each table in after state
    afterTables.forEach(afterTable => {
        const beforeTable = beforeTableMap.get(afterTable.object_id);
        
        if (!beforeTable) {
            // NEW TABLE - highlight table and all columns
            console.error(`  New table: ${afterTable.object_id}`);
            afterTable.highlight = true;
            
            afterTable.table.forEach(col => {
                // Preserve existing highlight if present
                if (col.highlight !== true) {
                    col.highlight = true;
                }
            });
        } else {
            // EXISTING TABLE - check each column
            const beforeColMap = new Map();
            beforeTable.table.forEach(col => {
                beforeColMap.set(col.name, col);
            });
            
            afterTable.table.forEach(afterCol => {
                // Preserve existing highlight flag if already true
                if (afterCol.highlight === true) {
                    console.error(`  Preserving existing highlight: ${afterTable.object_id}.${afterCol.name}`);
                    return;
                }
                
                const beforeCol = beforeColMap.get(afterCol.name);
                
                if (!beforeCol) {
                    // NEW COLUMN
                    console.error(`  New column: ${afterTable.object_id}.${afterCol.name}`);
                    afterCol.highlight = true;
                } else {
                    // Check for modifications
                    const modifications = [];
                    
                    if (beforeCol.datatype !== afterCol.datatype) {
                        modifications.push('datatype');
                    }
                    if (beforeCol.nullable !== afterCol.nullable) {
                        modifications.push('nullable');
                    }
                    if (beforeCol.primary_key !== afterCol.primary_key) {
                        modifications.push('primary_key');
                    }
                    if (beforeCol.unique !== afterCol.unique) {
                        modifications.push('unique');
                    }
                    if (beforeCol.lookup !== afterCol.lookup) {
                        modifications.push('lookup');
                    }
                    if (beforeCol.standard !== afterCol.standard) {
                        modifications.push('standard');
                    }
                    
                    if (modifications.length > 0) {
                        console.error(`  Modified column: ${afterTable.object_id}.${afterCol.name} (${modifications.join(', ')})`);
                        afterCol.highlight = true;
                    }
                }
            });
        }
    });
    
    return afterTables;
}

// ============================================================================
// RENDERING FUNCTIONS
// ============================================================================

/**
 * Render table border rectangle
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} width - Table width
 * @param {number} height - Table height
 * @param {number} scale - Scale factor (affects stroke width)
 * @returns {string} - SVG rectangle element
 */
function renderTableBorder(x, y, width, height, scale = 1, fill = "white", stroke = "black") {
    const strokeWidth = 3 * scale; // Thinner but noticeable border - 1 * 3 * scale
    const radius = 27 * scale; // Lightly rounded corners * 3 * 3 (more visible)
    return `<rect x="${x}" y="${y}" width="${width}" height="${height}" fill="${fill}" stroke="${stroke}" stroke-width="${strokeWidth}" rx="${radius}" ry="${radius}"/>`;
}

/**
 * Render table header
 * @param {Object} tableDef - Table definition object
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} width - Header width
 * @param {number} height - Header height
 * @param {number} scale - Scale factor
 * @param {boolean} isDB2 - Whether this is a DB2 table (affects table name casing)
 * @returns {string} - SVG header elements
 */
function renderTableHeader(tableDef, x, y, width, height, scale = 1, isDB2 = false) {
    let tableName = tableDef.object_id.replace('table.', '');
    if (isDB2) {
        tableName = tableName.toUpperCase();
    }
    const strokeWidth = 6 * scale; // Keep title separator black and thicker * 3
    const radius = 27 * scale; // Radius for rounded corners * 3 * 3 (more visible)
    const fontSize = 48 * scale; // Reduced proportionally with table width (from 12pt * 3 * 1.33)
    const textOffsetX = 30 * scale; // Reduced to match iconMargin (10 * 3 * scale)
    const textOffsetY = 62.37 * scale; // Moved down 10% from center (56.7 * scale * 1.1)
    let svg = '';

    // Determine header color based on highlight flag
    const headerColor = (tableDef.highlight === true) ? "#ffeb3b" : "skyblue";

    // Create path with only top corners rounded
    const pathData = `M ${x + radius} ${y} L ${x + width - radius} ${y} Q ${x + width} ${y} ${x + width} ${y + radius} L ${x + width} ${y + height} L ${x} ${y + height} L ${x} ${y + radius} Q ${x} ${y} ${x + radius} ${y} Z`;

    svg += `<path d="${pathData}" fill="${headerColor}" stroke="transparent" stroke-width="${strokeWidth}"/>\n`; // Header background color with only top corners rounded
    svg += `    <text x="${x + textOffsetX}" y="${y + textOffsetY}" font-family="${FONTS.header}" font-size="${fontSize}" font-weight="bold">${tableName}</text>`;

    // Add object_ref if present - right-justified in navy color
    if (tableDef.object_ref) {
        const objectRefFontSize = 36 * scale; // Slightly smaller than main title
        const objectRefOffsetX = 30 * scale; // Same left margin as main title
        const objectRefX = x + width - objectRefOffsetX; // Right-aligned with margin
        svg += `    <text x="${objectRefX}" y="${y + textOffsetY}" font-family="${FONTS.header}" font-size="${objectRefFontSize}" font-weight="bold" fill="navy" text-anchor="end">${tableDef.object_ref}</text>`;
    }

    return svg;
}

/**
 * Render a single table row
 * @param {Object} column - Column definition
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} width - Row width
 * @param {number} height - Row height
 * @param {number} scale - Scale factor
 * @param {boolean} isFirstRow - Whether this is the first row
 * @param {boolean} isLastRow - Whether this is the last row
 * @param {boolean} isDB2 - Whether this is a DB2 table (affects column name casing)
 * @returns {string} - SVG row elements
 */
function renderTableRow(column, x, y, width, height, scale = 1, isFirstRow = false, isLastRow = false, isDB2 = false) {
    const strokeWidth = 1.5 * scale; // Thinner lines * 3
    const fontSizeName = 40 * scale; // Reduced proportionally with table width (from 10pt * 3 * 1.33)
    const fontSizeType = 40 * scale; // 10 * 3 * scale * 1.33
    const textOffsetX = 30 * scale; // Reduced to match iconMargin (10 * 3 * scale)
    const textOffsetY = 59.4 * scale; // Moved down 10% (54 * scale * 1.1)
    const textEndOffset = 10 * scale;

    let svg = '';

    // Row background with conditional stroke (gray lines between rows, but not on first or last row)
    const strokeColor = (!isFirstRow && !isLastRow) ? "#cccccc" : "transparent";
    const backgroundColor = column.highlight ? "yellow" : "transparent";
    svg += `<rect x="${x}" y="${y}" width="${width}" height="${height}" fill="${backgroundColor}" stroke="${strokeColor}" stroke-width="${strokeWidth}"/>\n`;

    // Column name
    let colText = column.name;
    if (isDB2) {
        colText = colText.toUpperCase();
    }

    // Determine font color and weight based on column attributes
    let fontColor = "black"; // default
    let fontWeight = "normal"; // default
    if (column.primary_key) {
        fontColor = "navy";
        fontWeight = "bold";
    } else if (column.standard) {
        fontColor = "forestgreen";
    } else if (column.lookup) {
        fontColor = "maroon";
    }

    // Icon positioning and sizing (fixed spaces reserved) - scaled for reduced table width
    const iconMargin = 30 * scale; // Reduced a bit more to bring text closer to icons
    const iconSize = 48 * scale; // 12 * 3 * scale * 1.33 (proportional reduction)
    const iconY = y + (height - iconSize) / 2;

    // Left icon space (always reserved) - key for primary_key, lookup for lookup attribute
    const leftIconSpace = iconSize + (2 * iconMargin); // Icon + margins
    const leftIconX = x + iconMargin;

    let leftIconId = null;
    if (column.primary_key) {
        // Map font colors to icon IDs
        if (fontColor === "navy") {
            leftIconId = "key-icon-navy";
        } else if (fontColor === "forestgreen") {
            leftIconId = "key-icon-darkgreen";
        } else {
            leftIconId = "key-icon-gray";
        }
    } else if (column.unique) {
        leftIconId = "unique-icon-gray";
    } else if (column.lookup) {
        leftIconId = "lookup-icon-maroon";
    }

    // Right icon space (always reserved) - not-null icon
    const rightIconSpace = iconSize + (2 * iconMargin); // Icon + margins
    const rightIconX = x + width - iconMargin - iconSize;

    let rightIconId = null;
    if (column.nullable === false) {
        // Map font colors to icon IDs
        if (fontColor === "navy") {
            rightIconId = "not-null-icon-navy";
        } else if (fontColor === "forestgreen") {
            rightIconId = "not-null-icon-darkgreen";
        } else if (fontColor === "maroon") {
            rightIconId = "not-null-icon-maroon";
        } else {
            rightIconId = "not-null-icon-black";
        }
    }

    // Column name positioning (after left icon space)
    const columnNameX = x + iconMargin + iconSize + iconMargin;

    // Data type positioning (before right icon space)
    const dataTypeX = x + width - iconMargin - iconSize - iconMargin;

    // Render left icon (if applicable)
    if (leftIconId) {
        svg += `    <use href="#${leftIconId}" x="${leftIconX}" y="${iconY}" width="${iconSize}" height="${iconSize}" rx="${iconSize * 0.2}" ry="${iconSize * 0.2}"/>\n`;
    }

    // Render right icon (if applicable)
    if (rightIconId) {
        svg += `    <use href="#${rightIconId}" x="${rightIconX}" y="${iconY}" width="${iconSize}" height="${iconSize}" rx="${iconSize * 0.2}" ry="${iconSize * 0.2}"/>\n`;
    }

    // Column name (always positioned after left icon space)
    svg += `    <text x="${columnNameX}" y="${y + textOffsetY}" font-family="${FONTS.row}" font-size="${fontSizeName}" fill="${fontColor}" font-weight="${fontWeight}">${colText}</text>\n`;

   // Data type (always positioned before right icon space)
   svg += `    <text x="${dataTypeX}" y="${y + textOffsetY}" font-family="${FONTS.row}" font-size="${fontSizeType}" text-anchor="end" fill="${fontColor}" font-weight="${fontWeight}">${column.datatype}</text>`;

    return svg;
}

/**
 * Render complete table with shadow and margin
 * @param {Object} tableDef - Table definition object
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} scale - Scale factor for the table
 * @param {boolean} isDB2 - Whether this is a DB2 table (affects column name casing)
 * @returns {string} - Complete SVG table elements with shadow and margin
 */
function renderTable(tableDef, x, y, scale = 1, isDB2 = false) {
    const tableName = tableDef.object_id.replace('table.', '');
    const columns = tableDef.table;


    // Calculate dimensions (60mm table width * 3 for scaling)
    const headerHeight = 102.06 * scale; // 30 * 3 * scale * 3 * 0.7 * 0.8 * 0.75 * 0.9
    const rowHeight = 90 * scale; // 20 * 3 * scale * 1.5
    const colWidth = 850.5 * scale; // Reduced to allow more columns (3x base width)
    const tableWidth = colWidth;
    const tableHeight = headerHeight + (columns.length * rowHeight);

    // Add 2px margin on all sides
    const margin = 2 * scale;
    const tableX = margin;
    const tableY = margin;

    let svg = '';

    // Shadow filter definition
    const shadowId = `shadow-${tableName.replace(/[^a-zA-Z0-9]/g, '-')}`;
    svg += `<defs>\n`;
    svg += `    <filter id="${shadowId}" x="-20%" y="-20%" width="140%" height="140%">\n`;
    svg += `        <feDropShadow dx="2" dy="2" stdDeviation="1.5" flood-color="rgba(0,0,0,0.3)"/>\n`;
    svg += `    </filter>\n`;


    svg += `</defs>\n`;

    // Table group with shadow
    svg += `<g transform="translate(${x}, ${y})" filter="url(#${shadowId})">\n`;

    // White background rectangle (no border) - bottom layer
    svg += `    ${renderTableBorder(tableX, tableY, tableWidth, tableHeight, scale, "white", "transparent")}\n`;

    // Table header (rendered second)
    svg += `    ${renderTableHeader(tableDef, tableX, tableY, tableWidth, headerHeight, scale, isDB2)}\n`;

    // Column rows
    columns.forEach((col, index) => {
        const rowY = tableY + headerHeight + (index * rowHeight);
        const isFirstRow = index === 0;
        const isLastRow = index === columns.length - 1;
        svg += `    ${renderTableRow(col, tableX, rowY, tableWidth, rowHeight, scale, isFirstRow, isLastRow, isDB2)}\n`;
    });

    // Border-only rectangle (no background) - top layer
    svg += `    ${renderTableBorder(tableX, tableY, tableWidth, tableHeight, scale, "transparent", "silver")}\n`;

    svg += `</g>\n`;

    return svg;
}

/**
 * Render layout region rectangle (visible for debugging)
 * @param {number} x - X position (5mm from border)
 * @param {number} y - Y position (5mm from border)
 * @param {number} width - Layout region width
 * @param {number} height - Layout region height
 * @returns {string} - SVG rectangle element with clipping group
 */
function renderLayoutRegion(x, y, width, height) {
    // Create a clipPath for the layout region
    const clipPathId = "layout-region-clip";
    const radius = 42.51; // Match template's 5mm rounding * 3
    let svg = `<defs>\n`;
    svg += `    <clipPath id="${clipPathId}">\n`;
    svg += `        <rect x="${x}" y="${y}" width="${width}" height="${height}" rx="${radius}" ry="${radius}"/>\n`;
    svg += `    </clipPath>\n`;
    svg += `</defs>\n`;

    // Render the visible layout region rectangle with solid orange fill and no border
    // svg += `<rect x="${x}" y="${y}" width="${width}" height="${height}" fill="#ffa500" rx="${radius}" ry="${radius}"/>\n`;

    return svg;
}

// ============================================================================
// CORE DIAGRAM GENERATION
// ============================================================================

/**
 * Calculate table dimensions
 * @param {Object} tableDef - Table definition object
 * @returns {Object} - {width, height} of the table
 */
function calculateTableDimensions(tableDef) {
    const columns = tableDef.table;
    const headerHeight = 102.06; // 30 * 3 * 3 * 0.7 * 0.8 * 0.75 * 0.9 for 3x scaling + tripled height - 30% - 20% - 25% - 10%
    const rowHeight = 90; // 20 * 3 * 1.5 for 3x scaling + 50% increase (more proportional to reduced width)
    const colWidth = 850.5; // Reduced from 1326.78 to 850.5 to allow more columns (back to 3x base width)
    const tableWidth = colWidth;
    const tableHeight = headerHeight + (columns.length * rowHeight);
    return { width: tableWidth, height: tableHeight };
}

/**
 * Calculate table positions with post-processing for consistent vertical spacing
 * @param {Array} tables - Array of table definitions
 * @param {number} layoutX - Layout region X position (5mm within border)
 * @param {number} layoutY - Layout region Y position (5mm within border)
 * @param {number} layoutWidth - Layout region width
 * @param {number} layoutHeight - Layout region height
 * @param {number} margin - Margin between tables (in px)
 * @returns {Object} - {positions: Array of {x, y}, scale: scaling factor}
 */
function calculateAutoFitTablePositions(tables, layoutX = 42.51, layoutY = 42.51, layoutWidth = 2434.98, layoutHeight = 1894.98, margin = 90) {
    console.error(`\n=== DEBUG: calculateAutoFitTablePositions called with ${tables.length} tables ===`);

    if (tables.length === 0) {
        return { positions: [], scale: 1 };
    }

    // Calculate table dimensions (all tables have same dimensions)
    const tableDim = calculateTableDimensions(tables[0]);
    const tableWidth = tableDim.width;
    const tableHeight = tableDim.height;

    console.error(`Table dimensions: ${tableWidth} x ${tableHeight}`);

    // Calculate aspect ratio of actual layout region
    const layoutAspectRatio = layoutWidth / layoutHeight;

    // Try different numbers of columns to find optimal layout
    let bestLayout = { positions: [], scale: 0, virtualWidth: 0, virtualHeight: 0 };

    for (let columns = 1; columns <= Math.min(tables.length, 10); columns++) { // Try up to 10 columns
        // Calculate virtual layout dimensions with same aspect ratio
        const virtualHeight = Math.sqrt((tables.length * tableWidth * tableHeight) / layoutAspectRatio);
        const virtualWidth = virtualHeight * layoutAspectRatio;

        // Calculate how many rows we need
        const rows = Math.ceil(tables.length / columns);

        // Adjust virtual dimensions to fit all tables (using 2mm vertical spacing)
        const actualVirtualWidth = columns * tableWidth + (columns - 1) * margin;
        const verticalMarginInPt = 51.03; // 2mm for vertical spacing * 3 * 3
        const actualVirtualHeight = rows * tableHeight + (rows - 1) * verticalMarginInPt;

        // Scale factor to fit virtual layout into actual layout region (with 2mm margin)
        const marginInPt = 17.01; // 2mm ≈ 5.67pt * 3
        const availableWidth = layoutWidth - (2 * marginInPt);
        const availableHeight = layoutHeight - (2 * marginInPt);
        const scaleX = availableWidth / actualVirtualWidth;
        const scaleY = availableHeight / actualVirtualHeight;
        const scale = Math.min(scaleX, scaleY, 1);

        // If this layout fits better (higher scale), use it
        if (scale > bestLayout.scale) {
            // Create grid layout with consistent vertical spacing within columns
            const positions = [];
            const scaledTableWidth = tableWidth * scale;
            const scaledTableHeight = tableHeight * scale;
            const scaledMargin = margin * scale; // Horizontal spacing between columns
            const verticalSpacing = 51.03 * scale; // 2mm in scaled units * 3 * 3

            // First pass: Create initial grid layout (preserve top table positions)
            for (let i = 0; i < tables.length; i++) {
                const col = i % columns;
                const row = Math.floor(i / columns);

                const virtualX = col * (scaledTableWidth + scaledMargin);

                // For the FIRST table in each column (row 0), use minimal Y offset to preserve position
                // For other tables, use a much larger initial spacing that we'll fix in the second pass
                let virtualY;
                if (row === 0) {
                    virtualY = 0; // First table in each column stays near the top
                } else {
                    virtualY = row * (scaledTableHeight + (51.03 * scale)) * 3; // Spread others out initially
                }

                // Transform to actual layout region coordinates (with 2mm margin)
                const actualX = layoutX + marginInPt + virtualX;
                const actualY = layoutY + marginInPt + virtualY;

                positions.push({ x: actualX, y: actualY });

                // DEBUG: Show table placement
                const tableName = tables[i].object_id.replace('table.', '');
                console.error(`Table ${i}: ${tableName} -> Column ${col}, Row ${row}, Position (${actualX.toFixed(2)}, ${actualY.toFixed(2)})`);
            }

            // DEBUG: Show column structure
            console.error(`\n=== DEBUG: Column Structure ===`);
            for (let col = 0; col < columns; col++) {
                console.error(`Column ${col}:`);
                for (let i = 0; i < tables.length; i++) {
                    if (i % columns === col) {
                        const tableName = tables[i].object_id.replace('table.', '');
                        console.error(`  - Table: ${tableName} (index ${i})`);
                    }
                }
            }

            // Second pass: Fix vertical spacing within each column, keeping top table in place
            console.error(`\n=== DEBUG: Second Pass - Fixing Vertical Spacing ===`);

            for (let col = 0; col < columns; col++) {
                const tablesInColumn = [];

                // Find all tables in this column
                for (let i = 0; i < tables.length; i++) {
                    if (i % columns === col) {
                        tablesInColumn.push(i);
                    }
                }

                if (tablesInColumn.length <= 1) continue;

                console.error(`\nColumn ${col} (${tablesInColumn.length} tables):`);

                // Keep the first table in this column where it is
                const firstTableIndex = tablesInColumn[0];
                const firstTableY = positions[firstTableIndex].y;
                const firstTableName = tables[firstTableIndex].object_id.replace('table.', '');
                const firstTableHeight = calculateTableDimensions(tables[firstTableIndex]).height * scale;
                console.error(`  First table (H=${firstTableHeight.toFixed(2)}): ${firstTableName} at Y=${firstTableY.toFixed(2)} (keeping in place)`);

                // Position subsequent tables with consistent 2mm spacing below the first
                let previousTableY = firstTableY;
                let previousTableHeight = firstTableHeight;

                for (let j = 1; j < tablesInColumn.length; j++) {
                    const tableIndex = tablesInColumn[j];
                    const tableName = tables[tableIndex].object_id.replace('table.', '');
                    const tableHeight = calculateTableDimensions(tables[tableIndex]).height * scale;

                    // Calculate where this table SHOULD start (previous table's Y + previous table's height + margin)
                    const expectedY = previousTableY + previousTableHeight + verticalSpacing;

                    console.error(`  Table ${j} (H=${tableHeight.toFixed(2)}): ${tableName} -> moving from Y=${positions[tableIndex].y.toFixed(2)} to Y=${expectedY.toFixed(2)} (prev Y=${previousTableY.toFixed(2)} + prev H=${previousTableHeight.toFixed(2)} + margin=${verticalSpacing.toFixed(2)})`);

                    positions[tableIndex].y = expectedY;

                    // Update for next iteration
                    previousTableY = expectedY;
                    previousTableHeight = tableHeight;
                }

                // Third pass: Center the entire column vertically within the layout region
                if (tablesInColumn.length > 0) {
                    const lastTableIndex = tablesInColumn[tablesInColumn.length - 1];
                    const lastTableY = positions[lastTableIndex].y;
                    const lastTableHeight = calculateTableDimensions(tables[lastTableIndex]).height * scale;
                    const columnTotalHeight = (lastTableY + lastTableHeight) - firstTableY;

                    // Available height in layout region (subtracting margins)
                    const availableHeight = layoutHeight - (2 * 17.01); // 2mm margins on top and bottom * 3
                    const centeringOffset = (availableHeight - columnTotalHeight) / 2;

                    if (centeringOffset > 0) {
                        console.error(`  Third pass: Centering column - total height=${columnTotalHeight.toFixed(2)}, available=${availableHeight.toFixed(2)}, offset=${centeringOffset.toFixed(2)}`);

                        // Apply centering offset to all tables in this column
                        for (let j = 0; j < tablesInColumn.length; j++) {
                            const tableIndex = tablesInColumn[j];
                            const tableName = tables[tableIndex].object_id.replace('table.', '');
                            const oldY = positions[tableIndex].y;

                            positions[tableIndex].y += centeringOffset;

                            console.error(`    ${tableName}: Y=${oldY.toFixed(2)} -> Y=${positions[tableIndex].y.toFixed(2)}`);
                        }
                    }
                }
            }

            bestLayout = {
                positions,
                scale,
                virtualWidth: actualVirtualWidth * scale,
                virtualHeight: actualVirtualHeight * scale
            };
        }
    }

    return bestLayout;
}

/**
 * Generate SVG content for all tables with auto-fitting layout
 * @param {Array} tables - Array of table definitions
 * @param {boolean} isDB2 - Whether this is a DB2 database engine
 * @returns {string} - SVG content for layout region and all tables
 */
function generateTablesSVG(tables, isDB2 = false) {
    // Layout region dimensions (5mm inset from page edge, like border is 2mm inset)
    // Updated for 3x larger template (2520x1980)
    const layoutX = 42.51;  // 14.17 * 3
    const layoutY = 42.51;  // 14.17 * 3
    const layoutWidth = 2434.98;  // 811.66 * 3
    const layoutHeight = 1894.98;  // 631.66 * 3

    // Calculate auto-fitting positions
    const layout = calculateAutoFitTablePositions(tables, layoutX, layoutY, layoutWidth, layoutHeight, 90); // 30 * 3 for tripled margins

    let svg = '\n<!-- Layout Region -->\n';

    // Add layout region rectangle (visible for debugging) - this includes the clipPath
    svg += renderLayoutRegion(layoutX, layoutY, layoutWidth, layoutHeight);

    // Add global definitions (fonts and icons) once for the entire diagram
    svg += `<defs>\n`;

    // Icon definitions - defined once for the entire diagram
    svg += `    <!-- Icon Definitions (Font Awesome Free v7.0.1) -->\n`;
    svg += `    <!-- Not-null icons -->\n`;
    svg += `    <symbol id="not-null-icon-black" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M320 64C324.6 64 329.2 65 333.4 66.9L521.8 146.8C543.8 156.1 560.2 177.8 560.1 204C559.6 303.2 518.8 484.7 346.5 567.2C329.8 575.2 310.4 575.2 293.7 567.2C121.3 484.7 80.6 303.2 80.1 204C80 177.8 96.4 156.1 118.4 146.8L306.7 66.9C310.9 65 315.4 64 320 64z" fill="gray"/>\n`;
    svg += `    </symbol>\n`;
    svg += `    <symbol id="not-null-icon-darkgreen" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M320 64C324.6 64 329.2 65 333.4 66.9L521.8 146.8C543.8 156.1 560.2 177.8 560.1 204C559.6 303.2 518.8 484.7 346.5 567.2C329.8 575.2 310.4 575.2 293.7 567.2C121.3 484.7 80.6 303.2 80.1 204C80 177.8 96.4 156.1 118.4 146.8L306.7 66.9C310.9 65 315.4 64 320 64z" fill="darkgreen"/>\n`;
    svg += `    </symbol>\n`;
    svg += `    <symbol id="not-null-icon-navy" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M320 64C324.6 64 329.2 65 333.4 66.9L521.8 146.8C543.8 156.1 560.2 177.8 560.1 204C559.6 303.2 518.8 484.7 346.5 567.2C329.8 575.2 310.4 575.2 293.7 567.2C121.3 484.7 80.6 303.2 80.1 204C80 177.8 96.4 156.1 118.4 146.8L306.7 66.9C310.9 65 315.4 64 320 64z" fill="navy"/>\n`;
    svg += `    </symbol>\n`;
    svg += `    <symbol id="not-null-icon-maroon" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M320 64C324.6 64 329.2 65 333.4 66.9L521.8 146.8C543.8 156.1 560.2 177.8 560.1 204C559.6 303.2 518.8 484.7 346.5 567.2C329.8 575.2 310.4 575.2 293.7 567.2C121.3 484.7 80.6 303.2 80.1 204C80 177.8 96.4 156.1 118.4 146.8L306.7 66.9C310.9 65 315.4 64 320 64z" fill="maroon"/>\n`;
    svg += `    </symbol>\n`;

    svg += `    <!-- Key icons for primary keys -->\n`;
    svg += `    <symbol id="key-icon-navy" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M400 416C497.2 416 576 337.2 576 240C576 142.8 497.2 64 400 64C302.8 64 224 142.8 224 240C224 258.7 226.9 276.8 232.3 293.7L71 455C66.5 459.5 64 465.6 64 472L64 552C64 565.3 74.7 576 88 576L168 576C181.3 576 192 565.3 192 552L192 512L232 512C245.3 512 256 501.3 256 488L256 448L296 448C302.4 448 308.5 445.5 313 441L346.3 407.7C363.2 413.1 381.3 416 400 416zM440 160C462.1 160 480 177.9 480 200C480 222.1 462.1 240 440 240C417.9 240 400 222.1 400 200C400 177.9 417.9 160 440 160z" fill="navy"/>\n`;
    svg += `    </symbol>\n`;
    svg += `    <symbol id="key-icon-darkgreen" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M400 416C497.2 416 576 337.2 576 240C576 142.8 497.2 64 400 64C302.8 64 224 142.8 224 240C224 258.7 226.9 276.8 232.3 293.7L71 455C66.5 459.5 64 465.6 64 472L64 552C64 565.3 74.7 576 88 576L168 576C181.3 576 192 565.3 192 552L192 512L232 512C245.3 512 256 501.3 256 488L256 448L296 448C302.4 448 308.5 445.5 313 441L346.3 407.7C363.2 413.1 381.3 416 400 416zM440 160C462.1 160 480 177.9 480 200C480 222.1 462.1 240 440 240C417.9 240 400 222.1 400 200C400 177.9 417.9 160 440 160z" fill="darkgreen"/>\n`;
    svg += `    </symbol>\n`;
    svg += `    <symbol id="key-icon-gray" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M400 416C497.2 416 576 337.2 576 240C576 142.8 497.2 64 400 64C302.8 64 224 142.8 224 240C224 258.7 226.9 276.8 232.3 293.7L71 455C66.5 459.5 64 465.6 64 472L64 552C64 565.3 74.7 576 88 576L168 576C181.3 576 192 565.3 192 552L192 512L232 512C245.3 512 256 501.3 256 488L256 448L296 448C302.4 448 308.5 445.5 313 441L346.3 407.7C363.2 413.1 381.3 416 400 416zM440 160C462.1 160 480 177.9 480 200C480 222.1 462.1 240 440 240C417.9 240 400 222.1 400 200C400 177.9 417.9 160 440 160z" fill="gray"/>\n`;
    svg += `    </symbol>\n`;

    svg += `    <!-- Lookup icons -->\n`;
    svg += `    <symbol id="lookup-icon-gray" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M96.5 160L96.5 309.5C96.5 326.5 103.2 342.8 115.2 354.8L307.2 546.8C332.2 571.8 372.7 571.8 397.7 546.8L547.2 397.3C572.2 372.3 572.2 331.8 547.2 306.8L355.2 114.8C343.2 102.7 327 96 310 96L160.5 96C125.2 96 96.5 124.7 96.5 160zM208.5 176C226.2 176 240.5 190.3 240.5 208C240.5 225.7 226.2 240 208.5 240C190.8 240 176.5 225.7 176.5 208C176.5 190.3 190.8 176 208.5 176z" fill="gray"/>\n`;
    svg += `    </symbol>\n`;
    svg += `    <symbol id="lookup-icon-maroon" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M96.5 160L96.5 309.5C96.5 326.5 103.2 342.8 115.2 354.8L307.2 546.8C332.2 571.8 372.7 571.8 397.7 546.8L547.2 397.3C572.2 372.3 572.2 331.8 547.2 306.8L355.2 114.8C343.2 102.7 327 96 310 96L160.5 96C125.2 96 96.5 124.7 96.5 160zM208.5 176C226.2 176 240.5 190.3 240.5 208C240.5 225.7 226.2 240 208.5 240C190.8 240 176.5 225.7 176.5 208C176.5 190.3 190.8 176 208.5 176z" fill="maroon"/>\n`;
    svg += `    </symbol>\n`;

    svg += `    <!-- Unique icons -->\n`;
    svg += `    <symbol id="unique-icon-gray" viewBox="0 0 640 640">\n`;
    svg += `        <path d="M512 320C512 214 426 128 320 128C214 128 128 214 128 320C128 426 214 512 320 512C426 512 512 426 512 320zM64 320C64 178.6 178.6 64 320 64C461.4 64 576 178.6 576 320C576 461.4 461.4 576 320 576C178.6 576 64 461.4 64 320zM320 400C364.2 400 400 364.2 400 320C400 275.8 364.2 240 320 240C275.8 240 240 275.8 240 320C240 364.2 275.8 400 320 400zM320 176C399.5 176 464 240.5 464 320C464 399.5 399.5 464 320 464C240.5 464 176 399.5 176 320C176 240.5 240.5 176 320 176zM288 320C288 302.3 302.3 288 320 288C337.7 288 352 302.3 352 320C352 337.7 337.7 352 320 352C302.3 352 288 337.7 288 320z" fill="gray"/>\n`;
    svg += `    </symbol>\n`;
    svg += `</defs>\n`;

    // Add tables at calculated positions with scaling, clipped to layout region
    svg += `<!-- Table Content (clipped to layout region) -->\n`;
    svg += `<g clip-path="url(#layout-region-clip)">\n`;
    tables.forEach((tableDef, index) => {
        const pos = layout.positions[index];
        if (pos) {
            const tableName = tableDef.object_id.replace('table.', '');
            console.error(`SVG: Placing ${tableName} at (${pos.x.toFixed(2)}, ${pos.y.toFixed(2)})`);
            svg += `    <!-- ${tableDef.object_id} at (${pos.x.toFixed(2)}, ${pos.y.toFixed(2)}) -->\n`;
            svg += renderTable(tableDef, pos.x, pos.y, layout.scale, isDB2);
        }
    });
    svg += `</g>\n`;

    return svg;
}

/**
 * Core diagram generation function (works in both Node.js and browser)
 * @param {string} jsonInput - JSON string containing table definitions
 * @param {Object} options - Generation options
 * @returns {string} - Generated SVG diagram
 */
function generateDiagramCore(jsonInput, options = {}) {
    console.error(`\n=== Diagram Generation Started ===`);

    // Initialize metadata collection
    const metadata = {
        table_count: 0,
        highlighted_tables: 0,
        highlighted_columns: 0,
        processing_timestamp: new Date().toISOString()
    };

    try {
        // Parse input JSON
        const data = JSON.parse(jsonInput.trim());
        
        let processedDiagram;
        let templateObj;
        
        // Check if this is before/after comparison mode
        if (data.before && data.after) {
            console.error('Comparison mode detected');
            
            // Extract template from after state (it should have it)
            const afterDiagram = data.after.diagram || [];
            templateObj = afterDiagram.find(obj => obj.object_type === 'template');
            
            // Add highlight flags by comparing (returns only tables)
            const highlightedTables = addHighlightFlags(data.before, data.after);
            
            // Combine template with highlighted tables
            processedDiagram = templateObj ? [templateObj, ...highlightedTables] : highlightedTables;
            
            // Count highlights for metadata
            highlightedTables.forEach(table => {
                if (table.highlight === true) {
                    metadata.highlighted_tables++;
                }
                table.table.forEach(col => {
                    if (col.highlight === true) {
                        metadata.highlighted_columns++;
                    }
                });
            });
            
            console.error(`Highlighting: ${metadata.highlighted_tables} tables, ${metadata.highlighted_columns} columns`);
        } else {
            // Single state mode - use diagram as-is
            console.error('Single state mode (no comparison)');
            processedDiagram = Array.isArray(data) ? data : (data.diagram || []);
            templateObj = processedDiagram.find(obj => obj.object_type === 'template');
        }
        
        // Preprocess to remove duplicates
        processedDiagram = removeDuplicateObjects(processedDiagram);
        
        console.error(`Processing ${processedDiagram.length} objects`);
        
        // Find template object if we don't have it yet
        if (!templateObj) {
            templateObj = processedDiagram.find(obj => obj.object_type === 'template');
        }
        
        // If no template, we need a default (this shouldn't happen in normal flow)
        if (!templateObj || !templateObj.object_value) {
            console.error('Error: No template found in diagram data');
            throw new Error('Template object not found in JSON data');
        }
        
        const template = templateObj.object_value;
        
        // Find all table objects
        const tables = processedDiagram.filter(obj => obj.object_type === 'table');
        metadata.table_count = tables.length;
        
        console.error(`Found ${tables.length} tables to render`);
        
        // Output metadata to STDERR
        console.error(`METADATA: ${JSON.stringify(metadata)}`);
        
        // Determine if this is DB2 engine
        const isDB2 = options.engine === 'db2';
        
        // Generate SVG with auto-fitting layout
        const svgContent = generateTablesSVG(tables, isDB2);
        
        // Replace placeholder in template
        const finalSVG = template.replace('<!-- ERD SVG content goes here -->', svgContent);
        
        return finalSVG;
        
    } catch (error) {
        console.error(`Error: ${error.message}`);
        if (error instanceof SyntaxError && error.message.includes('JSON')) {
            const errorMsg = `JSON parsing error: ${error.message}`;
            const position = error.message.match(/position (\d+)/) ?
                error.message.match(/position (\d+)/)[1] : 'unknown';
            throw new Error(`${errorMsg} (position: ${position}). Please check the JSON formatting in the input data.`);
        } else {
            throw new Error(`Error generating diagram: ${error.message}`);
        }
    }
}

// ============================================================================
// NODE.JS SPECIFIC FUNCTIONS
// ============================================================================

/**
 * Node.js wrapper for generateDiagramCore with error handling
 * @param {string} jsonInput - JSON string containing table definitions
 * @param {Object} options - Generation options
 * @returns {string} - Generated SVG diagram
 */
function generateDiagram(jsonInput, options = {}) {
    try {
        return generateDiagramCore(jsonInput, options);
    } catch (error) {
        console.error(error.message);
        if (typeof process !== 'undefined') {
            process.exit(1);
        }
        throw error;
    }
}

// ============================================================================
// CLI INTERFACE (Node.js only)
// ============================================================================

// CLI interface
if (typeof process !== 'undefined' && import.meta.url === `file://${process.argv[1]}`) {
    // Check if we have file arguments or should read from stdin
    const args = process.argv.slice(2);

    let jsonContent;
    let engine = 'postgresql'; // default engine

    if (args.length === 0) {
        // Read from stdin
        jsonContent = fs.readFileSync(0, 'utf8');
    } else if (args.length === 1) {
        // Read from stdin, engine as first arg
        engine = args[0];
        jsonContent = fs.readFileSync(0, 'utf8');
    } else {
        // Read from file (legacy support), engine as first arg
        engine = args[0];
        const jsonFile = args[1];

        if (!fs.existsSync(jsonFile)) {
            console.error(`File not found: ${jsonFile}`);
            process.exit(1);
        }

        jsonContent = fs.readFileSync(jsonFile, 'utf8');
    }

    const svgOutput = generateDiagram(jsonContent, { engine });

    // Output to stdout
    console.log(svgOutput);
}

// ============================================================================
// EXPORTS (works in both Node.js and browser)
// ============================================================================

export {
    generateDiagram,
    generateDiagramCore,
    addHighlightFlags,
    preprocessJSONData,
    removeDuplicateObjects,
    renderTable,
    renderTableBorder,
    renderTableHeader,
    renderTableRow,
    calculateTableDimensions,
    calculateAutoFitTablePositions,
    renderLayoutRegion,
    generateTablesSVG
};

