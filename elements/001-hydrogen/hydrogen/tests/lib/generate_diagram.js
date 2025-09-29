#!/usr/bin/env node

// generate_diagram.js
// Converts JSON table definition into SVG database diagram

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Template will be provided in JSON data

// Function to generate SVG for a table
function generateTableSVG(tableDef, x, y) {
    const tableName = tableDef.object_id.replace('table.', '');
    const columns = tableDef.table;

    // Calculate dimensions
    const headerHeight = 30;
    const rowHeight = 20;
    const colWidth = 200;
    const tableWidth = colWidth;
    const tableHeight = headerHeight + (columns.length * rowHeight);

    let svg = '';

    // Table background
    svg += `<rect x="${x}" y="${y}" width="${tableWidth}" height="${tableHeight}" fill="white" stroke="black" stroke-width="2"/>`;

    // Table header
    svg += `<rect x="${x}" y="${y}" width="${tableWidth}" height="${headerHeight}" fill="#e1f5fe" stroke="black" stroke-width="1"/>`;
    svg += `<text x="${x + 10}" y="${y + 20}" font-family="Arial" font-size="14" font-weight="bold">${tableName}</text>`;

    // Column rows
    columns.forEach((col, index) => {
        const rowY = y + headerHeight + (index * rowHeight);

        // Column background
        svg += `<rect x="${x}" y="${rowY}" width="${tableWidth}" height="${rowHeight}" fill="white" stroke="black" stroke-width="1"/>`;

        // Column name
        let colText = col.name;
        if (col.primary_key) colText += ' (PK)';
        if (col.unique && !col.primary_key) colText += ' (UQ)';

        svg += `<text x="${x + 10}" y="${rowY + 15}" font-family="Arial" font-size="12">${colText}</text>`;

        // Data type (right-aligned)
        svg += `<text x="${x + tableWidth - 10}" y="${rowY + 15}" font-family="Arial" font-size="10" text-anchor="end">${col.datatype}</text>`;
    });

    return svg;
}

// Function to clean and validate JSON input
function cleanJSONInput(jsonInput) {
    // The JSON should already be properly cleaned by the awk script
    // Just trim whitespace and return
    return jsonInput.trim();
}

// Main function
function generateDiagram(jsonInput) {
    try {
        // Clean the JSON input (should already be properly formatted by awk)
        const cleanedJSON = cleanJSONInput(jsonInput);
        const data = JSON.parse(cleanedJSON);

        // Ensure we have an array
        const objects = Array.isArray(data) ? data : [data];

        // Find the template object
        const templateObj = objects.find(obj => obj.object_type === 'template');
        if (!templateObj || !templateObj.object_value) {
            throw new Error('Template object not found in JSON data');
        }
        const template = templateObj.object_value;

        // Find all table objects
        const tables = objects.filter(obj => obj.object_type === 'table');

        let svgContent = '';

        // Position tables in a simple layout
        tables.forEach((tableDef, index) => {
            const x = 50 + (index % 2) * 350; // Two columns
            const y = 50 + Math.floor(index / 2) * 250;
            svgContent += generateTableSVG(tableDef, x, y);
        });

        // Replace placeholder in template
        const finalSVG = template.replace('<!-- ERD SVG content goes here -->', svgContent);

        return finalSVG;
    } catch (error) {
        if (error instanceof SyntaxError && error.message.includes('JSON')) {
            console.error('JSON parsing error:', error.message);
            console.error('Position:', error.message.match(/position (\d+)/) ?
                error.message.match(/position (\d+)/)[1] : 'unknown');
            console.error('Please check the JSON formatting in the input data.');
        } else {
            console.error('Error generating diagram:', error.message);
        }
        process.exit(1);
    }
}

// CLI interface
if (import.meta.url === `file://${process.argv[1]}`) {
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

export { generateDiagram };
