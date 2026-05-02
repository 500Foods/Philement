#!/usr/bin/env node
/**
 * generate-icon-registry.js
 *
 * Scans the Lithium codebase for Font Awesome icon markup in JS and HTML files.
 * Extracts full <fa ...></fa> tags and outputs to:
 *   - config/icons-dev.txt (one <fa> tag per line)
 *
 * Run via: node scripts/generate-icon-registry.js
 * Also invoked automatically by: npm run build / npm run deploy
 */

import fs from 'fs';
import path from 'path';

const PROJECT_ROOT = path.resolve(import.meta.dirname, '..');
const SRC_DIR = path.join(PROJECT_ROOT, 'src');
const CONFIG_DIR = path.join(PROJECT_ROOT, 'config');
const DIST_CONFIG_DIR = process.env.LITHIUM_DEPLOY
  ? path.join(process.env.LITHIUM_DEPLOY, 'config')
  : path.join(PROJECT_ROOT, 'dist', 'config');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function walkDir(dir, extFilter) {
  const results = [];
  if (!fs.existsSync(dir)) return results;
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      if (['node_modules', 'dist', 'coverage', 'build', '.git'].includes(entry.name) || entry.name.startsWith('.tmp-')) continue;
      results.push(...walkDir(fullPath, extFilter));
    } else if (extFilter.includes(path.extname(entry.name).toLowerCase())) {
      results.push(fullPath);
    }
  }
  return results;
}

// Extract icon markup from content
function extractIconMarkup(text) {
  const icons = new Set();

  // Pattern 1: <fa ...>...</fa> or <fa ... />
  const faTagRegex = /<fa\s+[^>]+>\s*<\/\s*fa\s*>/gi;
  let match;
  while ((match = faTagRegex.exec(text)) !== null) {
    icons.add(match[0].trim());
  }

  // Also capture self-closing <fa ... />
  const faSelfClosingRegex = /<fa\s+[^>]+\s*\/>/gi;
  while ((match = faSelfClosingRegex.exec(text)) !== null) {
    icons.add(match[0].trim());
  }

  // Pattern 2: <i class="fa-..."></i> or <i class="fa-..." />
  const iClassRegex = /<i\s+class=["'][^"']*fa-[^"']*["']\s*><\/i>/gi;
  while ((match = iClassRegex.exec(text)) !== null) {
    icons.add(match[0].trim());
  }

  // Self-closing <i class="fa-..." />
  const iClassSelfClosingRegex = /<i\s+class=["'][^"']*fa-[^"']*["']\s*\/>/gi;
  while ((match = iClassSelfClosingRegex.exec(text)) !== null) {
    icons.add(match[0].trim());
  }

  return [...icons].sort();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

function main() {
  console.log('Lithium Icon Registry Generator');
  console.log('==================================\n');

  const jsFiles = walkDir(SRC_DIR, ['.js']);
  const htmlFiles = walkDir(SRC_DIR, ['.html']);

  // Also check index.html at root
  const rootHtml = path.join(PROJECT_ROOT, 'index.html');
  if (fs.existsSync(rootHtml)) htmlFiles.push(rootHtml);

  console.log(`Scanning ${jsFiles.length} JS files and ${htmlFiles.length} HTML files...\n`);

  const allIcons = new Set();

  // Scan JS files
  for (const file of jsFiles) {
    try {
      const content = fs.readFileSync(file, 'utf8');
      extractIconMarkup(content).forEach(icon => allIcons.add(icon));
    } catch (e) {
      // Skip files that can't be read
    }
  }

  // Scan HTML files
  for (const file of htmlFiles) {
    try {
      const content = fs.readFileSync(file, 'utf8');
      extractIconMarkup(content).forEach(icon => allIcons.add(icon));
    } catch (e) {
      // Skip files that can't be read
    }
  }

  const iconList = [...allIcons].filter(Boolean).filter(icon => {
    if (icon.includes('${')) return false;
    if (icon.includes('opt.')) return false;
    if (icon.includes('config.')) return false;
    return true;
  }).sort();

  console.log(`Found ${iconList.length} unique icon elements:\n`);
  iconList.forEach(icon => console.log(`  ${icon}`));
  console.log('');

  // Write to config/icons-dev.txt (source config)
  const devIconsPath = path.join(CONFIG_DIR, 'icons-dev.txt');
  fs.writeFileSync(devIconsPath, iconList.join('\n') + '\n', 'utf8');
  console.log(`Written to: ${devIconsPath}`);

  // Also write to dist/config/icons-dev.txt (build output)
  fs.mkdirSync(DIST_CONFIG_DIR, { recursive: true });
  const distIconsPath = path.join(DIST_CONFIG_DIR, 'icons-dev.txt');
  fs.writeFileSync(distIconsPath, iconList.join('\n') + '\n', 'utf8');
  console.log(`Written to: ${distIconsPath}`);

  console.log('\nDone!');
}

main();