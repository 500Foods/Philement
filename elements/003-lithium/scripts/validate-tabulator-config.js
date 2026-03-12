/**
 * Validate Tabulator Configuration Files
 *
 * Validates all JSON config files under config/tabulator/ against their
 * respective JSON schemas (coltypes-schema.json and tabledef-schema.json).
 *
 * Run:  node scripts/validate-tabulator-config.js
 * npm:  npm run validate:tabulator
 *
 * Exit codes:
 *   0 — all files valid
 *   1 — one or more validation errors found
 */

import Ajv from 'ajv';
import addFormats from 'ajv-formats';
import { readFileSync, readdirSync, statSync } from 'fs';
import { join, dirname, relative, basename } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const ROOT_DIR = join(__dirname, '..');
const CONFIG_DIR = join(ROOT_DIR, 'config', 'tabulator');

// ── Helpers ────────────────────────────────────────────────────────────────

/**
 * Load and parse a JSON file. Exits on parse failure.
 */
function loadJSON(filePath) {
  try {
    const raw = readFileSync(filePath, 'utf-8');
    return JSON.parse(raw);
  } catch (err) {
    console.error(`  ✗ Failed to parse ${relative(ROOT_DIR, filePath)}: ${err.message}`);
    return null;
  }
}

/**
 * Recursively find all .json files under a directory, excluding *-schema.json.
 */
function findDataFiles(dir) {
  const results = [];
  let entries;
  try {
    entries = readdirSync(dir, { withFileTypes: true });
  } catch {
    return results;
  }
  for (const entry of entries) {
    const fullPath = join(dir, entry.name);
    if (entry.isDirectory()) {
      results.push(...findDataFiles(fullPath));
    } else if (
      entry.isFile() &&
      entry.name.endsWith('.json') &&
      !entry.name.endsWith('-schema.json')
    ) {
      results.push(fullPath);
    }
  }
  return results;
}

/**
 * Format a single Ajv error for display.
 */
function formatError(err) {
  const path = err.instancePath || '(root)';
  const msg = err.message || 'unknown error';
  const params = err.params ? ` ${JSON.stringify(err.params)}` : '';
  return `    → ${path}: ${msg}${params}`;
}

// ── Cross-reference check ──────────────────────────────────────────────────

/**
 * Verify that every coltype referenced in a table definition exists in coltypes.json.
 * Returns an array of error strings (empty = no errors).
 */
function checkColtypeReferences(tableDefPath, tableDef, coltypes) {
  const errors = [];
  if (!tableDef.columns || typeof tableDef.columns !== 'object') return errors;
  if (!coltypes) return errors;

  const validTypes = Object.keys(coltypes);

  for (const [fieldName, colDef] of Object.entries(tableDef.columns)) {
    if (colDef.coltype && !validTypes.includes(colDef.coltype)) {
      errors.push(
        `    → columns.${fieldName}.coltype: unknown coltype "${colDef.coltype}" ` +
        `(valid: ${validTypes.join(', ')})`
      );
    }
  }
  return errors;
}

// ── Main ───────────────────────────────────────────────────────────────────

console.log('Validating Tabulator configuration files...\n');

// 1. Load schemas
const coltypesSchemaPath = join(CONFIG_DIR, 'coltypes-schema.json');
const tabledefSchemaPath = join(CONFIG_DIR, 'tabledef-schema.json');

const coltypesSchema = loadJSON(coltypesSchemaPath);
const tabledefSchema = loadJSON(tabledefSchemaPath);

if (!coltypesSchema || !tabledefSchema) {
  console.error('\n✗ Could not load schema file(s). Aborting.');
  process.exit(1);
}

// 2. Compile validators
const ajv = new Ajv({ allErrors: true, strict: false });
addFormats(ajv);

let validateColtypes;
let validateTabledef;
try {
  validateColtypes = ajv.compile(coltypesSchema);
  validateTabledef = ajv.compile(tabledefSchema);
} catch (err) {
  console.error(`✗ Schema compilation error: ${err.message}`);
  process.exit(1);
}

let totalFiles = 0;
let passCount = 0;
let failCount = 0;

// 3. Validate coltypes.json
const coltypesPath = join(CONFIG_DIR, 'coltypes.json');
const coltypesData = loadJSON(coltypesPath);
totalFiles++;

if (coltypesData) {
  const valid = validateColtypes(coltypesData);
  if (valid) {
    const typeCount = Object.keys(coltypesData.coltypes || {}).length;
    console.log(`  ✓ coltypes.json — ${typeCount} column type(s) defined`);
    passCount++;
  } else {
    console.error(`  ✗ coltypes.json — validation errors:`);
    for (const err of validateColtypes.errors) {
      console.error(formatError(err));
    }
    failCount++;
  }
} else {
  failCount++;
}

// 4. Validate all table definition files
const dataFiles = findDataFiles(CONFIG_DIR).filter(
  (f) => f !== coltypesPath
);

for (const filePath of dataFiles) {
  const relPath = relative(CONFIG_DIR, filePath);
  totalFiles++;

  const data = loadJSON(filePath);
  if (!data) {
    failCount++;
    continue;
  }

  const valid = validateTabledef(data);
  if (valid) {
    const colCount = Object.keys(data.columns || {}).length;
    console.log(`  ✓ ${relPath} — ${colCount} column(s) defined for "${data.title || data.table}"`);
    passCount++;

    // Cross-reference: check all coltype values exist
    if (coltypesData?.coltypes) {
      const refErrors = checkColtypeReferences(filePath, data, coltypesData.coltypes);
      if (refErrors.length > 0) {
        console.error(`  ⚠ ${relPath} — coltype reference warning(s):`);
        for (const re of refErrors) {
          console.error(re);
        }
        // Warnings don't bump failCount — the file is structurally valid
      }
    }
  } else {
    console.error(`  ✗ ${relPath} — validation errors:`);
    for (const err of validateTabledef.errors) {
      console.error(formatError(err));
    }
    failCount++;
  }
}

// 5. Summary
console.log(`\n${totalFiles} file(s) checked: ${passCount} passed, ${failCount} failed`);

if (failCount > 0) {
  console.error('\n✗ Tabulator config validation FAILED');
  process.exit(1);
} else {
  console.log('\n✓ All Tabulator config files are valid');
  process.exit(0);
}
