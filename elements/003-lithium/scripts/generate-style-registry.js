#!/usr/bin/env node
/**
 * generate-style-registry.js
 *
 * Scans the Lithium codebase for:
 *   1. CSS custom properties (variables) used in CSS, JS, and HTML files
 *   2. CSS class selectors/rules from CSS files
 *
 * Outputs:
 *   - config/lithium-style-vars.json
 *   - config/lithium-style-classes.json
 *
 * Run via: node scripts/generate-style-registry.js
 * Also invoked automatically by: npm run build / npm run deploy
 */

import fs from 'fs';
import path from 'path';

const PROJECT_ROOT = path.resolve(import.meta.dirname, '..');
const SRC_DIR = path.join(PROJECT_ROOT, 'src');
const DIST_CONFIG_DIR = path.join(PROJECT_ROOT, 'dist', 'config');

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

// ---------------------------------------------------------------------------
// 1. CSS Variable Extraction
// ---------------------------------------------------------------------------

function extractCSSVars(text) {
  const varRegex = /(?:^|[^a-zA-Z0-9_-])(--(?:[a-zA-Z_][a-zA-Z0-9_-]*))/g;
  const vars = new Set();
  let match;
  while ((match = varRegex.exec(text)) !== null) {
    const varName = match[1];
    if (varName.length > 2 && !varName.startsWith('---')) {
      vars.add(varName);
    }
  }
  return [...vars];
}

function countVarOccurrences(text, varName) {
  const escapedName = varName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const regex = new RegExp(`(?:^|[^a-zA-Z0-9_-])${escapedName}(?:[^a-zA-Z0-9_-]|$)`, 'g');
  let count = 0;
  regex.lastIndex = 0;
  let match;
  while ((match = regex.exec(text)) !== null) {
    count++;
    if (match.index === regex.lastIndex) regex.lastIndex++;
  }
  return count;
}

// ---------------------------------------------------------------------------
// 2. CSS Class Name Extraction from JS and HTML
// ---------------------------------------------------------------------------

/**
 * Extract all CSS class names from JavaScript source code.
 * Handles: classList.add/remove/toggle/contains, className, querySelector/closest strings,
 * and className = '...' assignments.
 */
function extractJSClasses(jsText) {
  const classes = new Set();
  let match;

  // 1. classList operations: classList.add('cls', 'cls2'), classList.remove('cls'), etc.
  const classListRegex = /classList\.(?:add|remove|toggle|contains)\(\s*(['"`])([^'"`]+)\1/g;
  while ((match = classListRegex.exec(jsText)) !== null) {
    match[2].split(/\s*,\s*|\s+/).forEach(c => {
      c = c.trim();
      if (c && /^[\w-]+$/.test(c)) classes.add(c);
    });
  }

  // 2. className = '...' (single class or space-separated)
  const classNameRegex = /\.className\s*=\s*(['"`])([^'"`]+)\1/g;
  while ((match = classNameRegex.exec(jsText)) !== null) {
    match[2].split(/\s+/).forEach(c => {
      if (c && /^[\w-]+$/.test(c)) classes.add(c);
    });
  }

  // 3. querySelector / querySelectorAll / closest with string literal arguments
  const qsRegex = /(?:querySelector(?:All)?|closest)\(\s*(['"`])([^'"`]+)\1/g;
  while ((match = qsRegex.exec(jsText)) !== null) {
    // Split on commas for compound selectors like '.foo, .bar'
    match[2].split(',').forEach(part => {
      const classMatches = part.match(/\.([\w-]+)/g);
      if (classMatches) {
        classMatches.forEach(c => {
          const name = c.slice(1);
          if (name && /^[\w-]+$/.test(name)) classes.add(name);
        });
      }
    });
  }

  // 4. setAttribute('class', '...')
  const setAttrRegex = /setAttribute\(\s*(['"`])class\1\s*,\s*(['"`])([^'"`]+)\2/g;
  while ((match = setAttrRegex.exec(jsText)) !== null) {
    match[3].split(/\s+/).forEach(c => {
      if (c && /^[\w-]+$/.test(c)) classes.add(c);
    });
  }

  // 5. Raw class="..." strings in JS (inline HTML templates)
  const rawClassRegex = /class\s*=\s*["']([^"']+)["']/g;
  while ((match = rawClassRegex.exec(jsText)) !== null) {
    match[1].split(/\s+/).forEach(c => {
      // Only include if it looks like a real class name (no ${}, no dots, no slashes)
      if (c && /^[\w-]+$/.test(c) && !c.startsWith('$')) classes.add(c);
    });
  }

  return classes;
}

/**
 * Extract all CSS class names from HTML content.
 * Handles: class="foo bar", class="foo ${bar}"
 */
function extractHTMLClasses(htmlText) {
  const classes = new Set();
  const classAttrRegex = /class\s*=\s*"([^"]+)"/g;
  let match;
  while ((match = classAttrRegex.exec(htmlText)) !== null) {
    match[1].split(/\s+/).forEach(c => {
      if (c && !c.startsWith('$') && !c.startsWith('{') && /^[\w-]+$/.test(c)) classes.add(c);
    });
  }
  return classes;
}

/**
 * Filter out false-positive class names that aren't real CSS classes.
 */
function filterClassNames(classes) {
  const filtered = new Set();
  for (const cls of classes) {
    // Skip single-char or empty
    if (cls.length < 2) continue;
    // Skip names that are clearly not CSS classes
    if (/^(cssPrefix|managerId|fa-\w+|fas?|far|fab|fal)$/.test(cls)) continue;
    // Skip Font Awesome size/modifier classes that are icon-related
    if (/^(fa-(?:xs|sm|lg|xl|1x|2x|3x|4x|5x|6x|7x|8x|9x|10x|2xs|1xs|2xl|3xl|4xl|5xl))$/.test(cls)) continue;
    // Skip things starting/ending with dash
    if (/^[-]|[-]$/.test(cls)) continue;
    // Skip purely numeric
    if (/^\d+$/.test(cls)) continue;
    // Skip template variable fragments
    if (cls.startsWith('$') || cls.startsWith('{')) continue;
    filtered.add(cls);
  }
  return filtered;
}

// ---------------------------------------------------------------------------
// 3. CSS Class Selector Extraction (from CSS files)
// ---------------------------------------------------------------------------

/**
 * Strip all comments from CSS text.
 */
function stripComments(text) {
  return text.replace(/\/\*[\s\S]*?\*\//g, '');
}

/**
 * Flatten @layer blocks by removing the @layer keyword and wrapping braces,
 * so all rules are at the top level for easier parsing.
 */
function flattenLayers(text) {
  // Handle both "@layer name {" and "@layer name," multi-layer declarations
  // First pass: remove @layer block wrappers, keeping content
  let result = text;
  let maxIterations = 50; // safety
  while (maxIterations-- > 0) {
    // Match @layer keyword followed by a name (or comma-separated names) and opening brace
    const layerMatch = result.match(/@layer\s+[\w.-]+(?:\s*,\s*[\w.-]+)*\s*\{/);
    if (!layerMatch) break;

    // Remove the "@layer name {" opening but keep the content
    const start = layerMatch.index;
    const afterOpen = start + layerMatch[0].length;

    // Find the matching closing brace
    let depth = 1;
    let i = afterOpen;
    while (i < result.length && depth > 0) {
      if (result[i] === '{') depth++;
      else if (result[i] === '}') depth--;
      i++;
    }

    // Replace @layer name { ...content... } with just the content
    const content = result.substring(afterOpen, i - 1);
    result = result.substring(0, start) + content + result.substring(i);
  }

  // Also handle "@layer {" (anonymous layers)
  maxIterations = 50;
  while (maxIterations-- > 0) {
    const anonMatch = result.match(/@layer\s*\{/);
    if (!anonMatch) break;

    const start = anonMatch.index;
    const afterOpen = start + anonMatch[0].length;
    let depth = 1;
    let i = afterOpen;
    while (i < result.length && depth > 0) {
      if (result[i] === '{') depth++;
      else if (result[i] === '}') depth--;
      i++;
    }

    const content = result.substring(afterOpen, i - 1);
    result = result.substring(0, start) + content + result.substring(i);
  }

  return result;
}

/**
 * Parse CSS text and extract all selectors that contain at least one class reference.
 * Returns selectors as full compound selector strings (handles comma-separated).
 */
function extractClassSelectors(cssText) {
  // Step 1: Strip comments
  let text = stripComments(cssText);

  // Step 2: Flatten @layer blocks
  text = flattenLayers(text);

  // Step 3: Remove @media, @supports, @keyframes blocks (keep content for nested selectors)
  // For @keyframes, remove entirely (they don't contain class selectors)
  text = text.replace(/@keyframes\s+[\w-]+\s*\{[^}]*\}/g, '');
  text = text.replace(/@keyframes\s+[\w-]+\s*\{[\s\S]*?\}(?:\s*\})?/g, '');

  // For @media/@supports/@container, unwrap them to keep nested rules
  let maxIter = 50;
  while (maxIter-- > 0) {
    const atMatch = text.match(/@(?:media|supports|container)\s+[^{]+\{/);
    if (!atMatch) break;
    const start = atMatch.index;
    const afterOpen = start + atMatch[0].length;
    let depth = 1;
    let i = afterOpen;
    while (i < text.length && depth > 0) {
      if (text[i] === '{') depth++;
      else if (text[i] === '}') depth--;
      i++;
    }
    const content = text.substring(afterOpen, i - 1);
    text = text.substring(0, start) + content + text.substring(i);
  }

  // Step 4: Parse selectors and rules
  const selectors = new Set();
  const len = text.length;
  let i = 0;

  while (i < len) {
    // Skip whitespace
    while (i < len && /\s/.test(text[i])) i++;
    if (i >= len) break;

    // Skip remaining @-rules (font-face, charset, etc.)
    if (text[i] === '@') {
      while (i < len && text[i] !== '{' && text[i] !== ';' && text[i] !== '\n') i++;
      if (i < len && text[i] === '{') {
        let depth = 1;
        i++;
        while (i < len && depth > 0) {
          if (text[i] === '{') depth++;
          else if (text[i] === '}') depth--;
          i++;
        }
      } else {
        i++;
      }
      continue;
    }

    // Collect everything up to the opening brace -- that's a selector block
    let selectorStart = i;
    while (i < len && text[i] !== '{' && text[i] !== '\n') i++;

    if (i < len && text[i] === '{') {
      let selectorText = text.substring(selectorStart, i).trim();

      if (selectorText && selectorText.includes('.')) {
        // Split comma-separated selectors
        const parts = selectorText.split(',');
        for (let part of parts) {
          part = part.trim();
          if (!part) continue;
          // Must contain at least one class reference
          if (!part.includes('.')) continue;
          // Skip pure hex color false positives
          if (/^#[0-9a-fA-F]{3,8}/.test(part)) continue;
          // Skip if starts with @ (shouldn't happen after cleanup, but safety)
          if (part.startsWith('@')) continue;
          selectors.add(part);
        }
      }

      // Skip the block content
      let depth = 1;
      i++;
      while (i < len && depth > 0) {
        if (text[i] === '{') depth++;
        else if (text[i] === '}') depth--;
        i++;
      }
    } else {
      i++;
    }
  }

  return [...selectors];
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

function main() {
  console.log('Lithium Style Registry Generator');
  console.log('=================================\n');

  const cssFiles = walkDir(SRC_DIR, ['.css']);
  const jsFiles = walkDir(SRC_DIR, ['.js']);
  const htmlFiles = walkDir(SRC_DIR, ['.html']);

  const rootHtml = path.join(PROJECT_ROOT, 'index.html');
  if (fs.existsSync(rootHtml)) htmlFiles.push(rootHtml);

  console.log(`Found ${cssFiles.length} CSS files, ${jsFiles.length} JS files, ${htmlFiles.length} HTML files\n`);

  const cssContents = cssFiles.map(f => ({ file: f, content: fs.readFileSync(f, 'utf8') }));
  const jsContents = jsFiles.map(f => ({ file: f, content: fs.readFileSync(f, 'utf8') }));
  const htmlContents = htmlFiles.map(f => ({ file: f, content: fs.readFileSync(f, 'utf8') }));

  // =========================================================================
  // PHASE 1: CSS Variables
  // =========================================================================
  console.log('Phase 1: Extracting CSS variables...');

  const allVarNames = new Set();

  for (const { content } of cssContents) extractCSSVars(content).forEach(v => allVarNames.add(v));
  for (const { content } of jsContents) extractCSSVars(content).forEach(v => allVarNames.add(v));
  for (const { content } of htmlContents) extractCSSVars(content).forEach(v => allVarNames.add(v));

  console.log(`  Found ${allVarNames.size} unique CSS variable names\n`);

  const varResults = [];
  for (const varName of [...allVarNames].sort()) {
    let cssCount = 0, jsCount = 0, htmlCount = 0;

    for (const { content } of cssContents) cssCount += countVarOccurrences(content, varName);
    for (const { content } of jsContents) jsCount += countVarOccurrences(content, varName);
    for (const { content } of htmlContents) htmlCount += countVarOccurrences(content, varName);

    varResults.push({ name: varName, css: cssCount, js: jsCount, html: htmlCount, total: cssCount + jsCount + htmlCount });
  }

  varResults.sort((a, b) => a.name.localeCompare(b.name));

  const varsJson = {
    _meta: {
      description: 'CSS custom properties (variables) used across the Lithium codebase',
      generated: new Date().toISOString(),
      sources: {
        css: `${cssFiles.length} files in src/**/*.css`,
        js: `${jsFiles.length} files in src/**/*.js`,
        html: `${htmlFiles.length} files in src/**/*.html + index.html`
      },
      total_variables: varResults.length,
      total_occurrences: varResults.reduce((sum, v) => sum + v.total, 0)
    },
    variables: varResults
  };

  fs.mkdirSync(DIST_CONFIG_DIR, { recursive: true });
  const varsOutputPath = path.join(DIST_CONFIG_DIR, 'lithium-style-vars.json');
  fs.writeFileSync(varsOutputPath, JSON.stringify(varsJson, null, 2), 'utf8');
  console.log(`  Written to: ${varsOutputPath}`);
  console.log(`  Total variables: ${varResults.length}`);
  console.log(`  Total occurrences: ${varsJson._meta.total_occurrences}\n`);

  // =========================================================================
  // PHASE 2: CSS Class Selectors / Names
  // =========================================================================
  console.log('Phase 2: Extracting CSS class selectors and names...');

  // --- 2a. Collect selectors from CSS files ---
  const classSelectorMap = new Map();

  for (const { file, content } of cssContents) {
    const selectors = extractClassSelectors(content);
    for (const sel of selectors) {
      if (!classSelectorMap.has(sel)) {
        classSelectorMap.set(sel, { cssCount: 0, jsCount: 0, htmlCount: 0, cssFiles: [], jsFiles: [], htmlFiles: [] });
      }
      const entry = classSelectorMap.get(sel);
      entry.cssCount++;
      const relPath = path.relative(PROJECT_ROOT, file);
      if (!entry.cssFiles.includes(relPath)) entry.cssFiles.push(relPath);
    }
  }

  // --- 2b. Collect individual class names from JS files ---
  const jsClassMap = new Map(); // className -> Set of files
  for (const { file, content } of jsContents) {
    const classes = filterClassNames(extractJSClasses(content));
    const relPath = path.relative(PROJECT_ROOT, file);
    for (const cls of classes) {
      if (!jsClassMap.has(cls)) jsClassMap.set(cls, new Set());
      jsClassMap.get(cls).add(relPath);
    }
  }

  // --- 2c. Collect individual class names from HTML files ---
  const htmlClassMap = new Map();
  for (const { file, content } of htmlContents) {
    const classes = filterClassNames(extractHTMLClasses(content));
    const relPath = path.relative(PROJECT_ROOT, file);
    for (const cls of classes) {
      if (!htmlClassMap.has(cls)) htmlClassMap.set(cls, new Set());
      htmlClassMap.get(cls).add(relPath);
    }
  }

  // --- 2d. For every class name found in JS/HTML, count occurrences in each file ---
  function countClassInFile(text, className) {
    // Count times className appears as a CSS class reference (after a dot, in a string, or in class attr)
    const escaped = className.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    let count = 0;
    // classList.add/remove/toggle/contains('className')
    const classListRe = new RegExp(`classList\\.(?:add|remove|toggle|contains)\\s*\\(\\s*['"\`"][^'"\`"]*\\b${escaped}\\b`, 'g');
    count += (text.match(classListRe) || []).length;
    // querySelector/querySelectorAll/closest('.className') or ('.other .className')
    const qsRe = new RegExp(`(?:querySelector(?:All)?|closest)\\s*\\(\\s*['"\`"][^'"\`"]*\\.${escaped}\\b`, 'g');
    count += (text.match(qsRe) || []).length;
    // className = 'className'
    const cnRe = new RegExp(`\\.className\\s*=\\s*['"\`"][^'"\`"]*\\b${escaped}\\b`, 'g');
    count += (text.match(cnRe) || []).length;
    // class="className"
    const classAttrRe = new RegExp(`class\\s*=\\s*["'][^"']*\\b${escaped}\\b`, 'g');
    count += (text.match(classAttrRe) || []).length;
    // setAttribute('class', 'className')
    const saRe = new RegExp(`setAttribute\\s*\\(\\s*['"\`"]class['"\`"]\\s*,\\s*['"\`"][^'"\`"]*\\b${escaped}\\b`, 'g');
    count += (text.match(saRe) || []).length;
    return count;
  }

  // Add JS references to existing CSS selectors, and create new entries for classes only in JS/HTML
  for (const [cls, files] of jsClassMap) {
    // Find which CSS selectors reference this class
    let matchedSelector = false;
    for (const sel of classSelectorMap.keys()) {
      if (sel.includes(`.${cls}`)) {
        const entry = classSelectorMap.get(sel);
        for (const f of files) {
          const count = countClassInFile(jsContents.find(jc => path.relative(PROJECT_ROOT, jc.file) === f)?.content || '', cls);
          entry.jsCount += count;
          if (!entry.jsFiles.includes(f)) entry.jsFiles.push(f);
        }
        matchedSelector = true;
      }
    }
    // Even if no CSS selector exists, add it as an entry for tracking
    if (!matchedSelector) {
      const key = `.${cls}`;
      if (!classSelectorMap.has(key)) {
        classSelectorMap.set(key, { cssCount: 0, jsCount: 0, htmlCount: 0, cssFiles: [], jsFiles: [], htmlFiles: [] });
      }
      const entry = classSelectorMap.get(key);
      for (const f of files) {
        const count = countClassInFile(jsContents.find(jc => path.relative(PROJECT_ROOT, jc.file) === f)?.content || '', cls);
        entry.jsCount += count;
        if (!entry.jsFiles.includes(f)) entry.jsFiles.push(f);
      }
    }
  }

  for (const [cls, files] of htmlClassMap) {
    let matchedSelector = false;
    for (const sel of classSelectorMap.keys()) {
      if (sel.includes(`.${cls}`)) {
        const entry = classSelectorMap.get(sel);
        for (const f of files) {
          const count = countClassInFile(htmlContents.find(hc => path.relative(PROJECT_ROOT, hc.file) === f)?.content || '', cls);
          entry.htmlCount += count;
          if (!entry.htmlFiles.includes(f)) entry.htmlFiles.push(f);
        }
        matchedSelector = true;
      }
    }
    if (!matchedSelector) {
      const key = `.${cls}`;
      if (!classSelectorMap.has(key)) {
        classSelectorMap.set(key, { cssCount: 0, jsCount: 0, htmlCount: 0, cssFiles: [], jsFiles: [], htmlFiles: [] });
      }
      const entry = classSelectorMap.get(key);
      for (const f of files) {
        const count = countClassInFile(htmlContents.find(hc => path.relative(PROJECT_ROOT, hc.file) === f)?.content || '', cls);
        entry.htmlCount += count;
        if (!entry.htmlFiles.includes(f)) entry.htmlFiles.push(f);
      }
    }
  }

  const classResults = [...classSelectorMap.entries()]
    .map(([selector, data]) => ({
      selector,
      css: data.cssCount,
      js: data.jsCount,
      html: data.htmlCount,
      total: data.cssCount + data.jsCount + data.htmlCount,
      files: {
        css: data.cssFiles.sort(),
        js: data.jsFiles.sort(),
        html: data.htmlFiles.sort()
      }
    }))
    .sort((a, b) => a.selector.localeCompare(b.selector));

  const classesJson = {
    _meta: {
      description: 'CSS class selectors/names used across the Lithium codebase',
      generated: new Date().toISOString(),
      sources: {
        css: `${cssFiles.length} files in src/**/*.css`,
        js: `${jsFiles.length} files in src/**/*.js`,
        html: `${htmlFiles.length} files in src/**/*.html + index.html`
      },
      total_selectors: classResults.length,
      total_occurrences: classResults.reduce((sum, s) => sum + s.total, 0)
    },
    selectors: classResults
  };

  const classesOutputPath = path.join(DIST_CONFIG_DIR, 'lithium-style-classes.json');
  fs.writeFileSync(classesOutputPath, JSON.stringify(classesJson, null, 2), 'utf8');
  console.log(`  Written to: ${classesOutputPath}`);
  console.log(`  Total selectors: ${classResults.length}`);
  console.log(`  Total occurrences: ${classesJson._meta.total_occurrences}`);
  const cssOnly = classResults.filter(s => s.css > 0 && s.js === 0 && s.html === 0).length;
  const jsOnly = classResults.filter(s => s.css === 0 && s.js > 0).length;
  const htmlOnly = classResults.filter(s => s.css === 0 && s.html > 0).length;
  const multi = classResults.filter(s => (s.css > 0 ? 1 : 0) + (s.js > 0 ? 1 : 0) + (s.html > 0 ? 1 : 0) >= 2).length;
  console.log(`  Breakdown: ${cssOnly} CSS-only, ${jsOnly} JS-only, ${htmlOnly} HTML-only, ${multi} multi-source\n`);

  console.log('Done!');
}

main();
