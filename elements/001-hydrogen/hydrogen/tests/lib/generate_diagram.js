#!/usr/bin/env node

// generate_diagram.js
// Converts JSON table definition into SVG database diagram

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

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
// RENDERING FUNCTIONS
// ============================================================================

/**
 * Render table border rectangle
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} width - Table width
 * @param {number} height - Table height
 * @returns {string} - SVG rectangle element
 */
function renderTableBorder(x, y, width, height) {
    return `<rect x="${x}" y="${y}" width="${width}" height="${height}" fill="white" stroke="black" stroke-width="2"/>`;
}

/**
 * Render table header
 * @param {string} tableName - Name of the table
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} width - Header width
 * @param {number} height - Header height
 * @returns {string} - SVG header elements
 */
function renderTableHeader(tableName, x, y, width, height) {
    let svg = '';
    svg += `<rect x="${x}" y="${y}" width="${width}" height="${height}" fill="#e1f5fe" stroke="black" stroke-width="1"/>`;
    svg += `<text x="${x + 10}" y="${y + 20}" font-family="Arial" font-size="14" font-weight="bold">${tableName}</text>`;
    return svg;
}

/**
 * Render a single table row
 * @param {Object} column - Column definition
 * @param {number} x - X position
 * @param {number} y - Y position
 * @param {number} width - Row width
 * @param {number} height - Row height
 * @returns {string} - SVG row elements
 */
function renderTableRow(column, x, y, width, height) {
    let svg = '';

    // Row background
    svg += `<rect x="${x}" y="${y}" width="${width}" height="${height}" fill="white" stroke="black" stroke-width="1"/>`;

    // Column name with constraints
    let colText = column.name;
    if (column.primary_key) colText += ' (PK)';
    if (column.unique && !column.primary_key) colText += ' (UQ)';

    svg += `<text x="${x + 10}" y="${y + 15}" font-family="Arial" font-size="12">${colText}</text>`;

    // Data type (right-aligned)
    svg += `<text x="${x + width - 10}" y="${y + 15}" font-family="Arial" font-size="10" text-anchor="end">${column.datatype}</text>`;

    return svg;
}

/**
 * Render complete table
 * @param {Object} tableDef - Table definition object
 * @param {number} x - X position
 * @param {number} y - Y position
 * @returns {string} - Complete SVG table elements
 */
function renderTable(tableDef, x, y) {
    const tableName = tableDef.object_id.replace('table.', '');
    const columns = tableDef.table;

    // Calculate dimensions
    const headerHeight = 30;
    const rowHeight = 20;
    const colWidth = 200;
    const tableWidth = colWidth;
    const tableHeight = headerHeight + (columns.length * rowHeight);

    let svg = '';

    // Table border
    svg += renderTableBorder(x, y, tableWidth, tableHeight);

    // Table header
    svg += renderTableHeader(tableName, x, y, tableWidth, headerHeight);

    // Column rows
    columns.forEach((col, index) => {
        const rowY = y + headerHeight + (index * rowHeight);
        svg += renderTableRow(col, x, rowY, tableWidth, rowHeight);
    });

    return svg;
}

// ============================================================================
// CORE DIAGRAM GENERATION
// ============================================================================

/**
 * Calculate table positions in a grid layout
 * @param {Array} tables - Array of table definitions
 * @param {number} columns - Number of columns in layout
 * @param {number} startX - Starting X position
 * @param {number} startY - Starting Y position
 * @param {number} spacingX - Horizontal spacing between tables
 * @param {number} spacingY - Vertical spacing between tables
 * @returns {Array} - Array of {x, y} positions
 */
function calculateTablePositions(tables, columns = 2, startX = 50, startY = 50, spacingX = 350, spacingY = 250) {
    return tables.map((_, index) => ({
        x: startX + (index % columns) * spacingX,
        y: startY + Math.floor(index / columns) * spacingY
    }));
}

/**
 * Generate SVG content for all tables
 * @param {Array} tables - Array of table definitions
 * @param {Array} positions - Array of {x, y} positions
 * @returns {string} - SVG content for all tables
 */
function generateTablesSVG(tables, positions) {
    return tables.map((tableDef, index) =>
        renderTable(tableDef, positions[index].x, positions[index].y)
    ).join('');
}

/**
 * Core diagram generation function (works in both Node.js and browser)
 * @param {string} jsonInput - JSON string containing table definitions
 * @param {Object} options - Generation options
 * @returns {string} - Generated SVG diagram
 */
function generateDiagramCore(jsonInput, options = {}) {
    try {
        // Parse and preprocess JSON data
        const data = JSON.parse(jsonInput.trim());
        const objects = preprocessJSONData(data);

        // Find the template object
        const templateObj = objects.find(obj => obj.object_type === 'template');
        if (!templateObj || !templateObj.object_value) {
            throw new Error('Template object not found in JSON data');
        }
        const template = templateObj.object_value;

        // Find all table objects
        const tables = objects.filter(obj => obj.object_type === 'table');

        // Calculate positions and generate SVG
        const positions = calculateTablePositions(tables, options.columns, options.startX, options.startY, options.spacingX, options.spacingY);
        const svgContent = generateTablesSVG(tables, positions);

        // Replace placeholder in template
        const finalSVG = template.replace('<!-- ERD SVG content goes here -->', svgContent);

        return finalSVG;
    } catch (error) {
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

    if (args.length === 0) {
        // Read from stdin
        jsonContent = fs.readFileSync(0, 'utf8');
    } else {
        // Read from file (legacy support)
        const jsonFile = args[0];

        if (!fs.existsSync(jsonFile)) {
            console.error(`File not found: ${jsonFile}`);
            process.exit(1);
        }

        jsonContent = fs.readFileSync(jsonFile, 'utf8');
    }

    const svgOutput = generateDiagram(jsonContent);

    // Output to stdout
    console.log(svgOutput);
}

// ============================================================================
// EXPORTS (works in both Node.js and browser)
// ============================================================================

export {
    generateDiagram,
    generateDiagramCore,
    preprocessJSONData,
    removeDuplicateObjects,
    renderTable,
    renderTableBorder,
    renderTableHeader,
    renderTableRow,
    calculateTablePositions,
    generateTablesSVG
};
