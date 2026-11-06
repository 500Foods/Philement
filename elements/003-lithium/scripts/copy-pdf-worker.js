#!/usr/bin/env node
/**
 * Copy the pdf.js worker to a stable path.
 * The file is renamed to .js on the way out. Hydrogen labels .js as
 * JavaScript and does not label .mjs, so a module worker under the
 * library's own name never starts. Vite does not hash this file.
 *
 * Usage: node scripts/copy-pdf-worker.js [destDir]
 * destDir defaults to public/. The file lands at
 * <destDir>/assets/pdf.worker.min.js
 */

import { copyFileSync, mkdirSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const destRoot = resolve(process.argv[2] || resolve(root, 'public'));
const source = require.resolve('pdfjs-dist/build/pdf.worker.min.mjs');
const dest = resolve(destRoot, 'assets', 'pdf.worker.min.js');

mkdirSync(dirname(dest), { recursive: true });
copyFileSync(source, dest);
console.log(`[pdf-worker] ${dest}`);
