#!/usr/bin/env node

/**
 * Generate Brotli sidecar files for deployable text assets.
 *
 * Only selected file types are compressed:
 *   - .css
 *   - .css.map
 *   - .html
 *   - .json
 *   - .svg
 *   - .js
 *   - .js.map
 *
 * Binary assets such as PNG files are intentionally ignored.
 *
 * Sidecar files are written as: original-filename.ext.br
 * Existing .br files are regenerated only when the source file is newer.
 */

import {
  existsSync,
  readdirSync,
  readFileSync,
  statSync,
  writeFileSync,
} from 'fs';
import { dirname, extname, join, resolve } from 'path';
import { fileURLToPath } from 'url';
import { brotliCompressSync, constants as zlibConstants } from 'zlib';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = resolve(__dirname, '..');

const targetDirArg = process.argv[2];
const targetDir = targetDirArg
  ? resolve(targetDirArg)
  : resolve(process.env.LITHIUM_ROOT || projectRoot, 'dist');

const ALLOWED_EXTENSIONS = new Set(['.css', '.html', '.json', '.svg', '.js']);

function isEligibleFile(filePath) {
  const lowerPath = filePath.toLowerCase();

  if (lowerPath.endsWith('.js.map') || lowerPath.endsWith('.css.map')) {
    return true;
  }

  return ALLOWED_EXTENSIONS.has(extname(lowerPath));
}

function formatBytes(bytes) {
  if (bytes < 1024) {
    return `${bytes} B`;
  }

  const units = ['KB', 'MB', 'GB', 'TB'];
  let value = bytes;
  let unitIndex = -1;

  do {
    value /= 1024;
    unitIndex += 1;
  } while (value >= 1024 && unitIndex < units.length - 1);

  return `${value.toFixed(value >= 10 ? 1 : 2)} ${units[unitIndex]}`;
}

function walkFiles(dirPath) {
  const entries = readdirSync(dirPath, { withFileTypes: true });
  const files = [];

  for (const entry of entries) {
    const fullPath = join(dirPath, entry.name);

    if (entry.isDirectory()) {
      files.push(...walkFiles(fullPath));
      continue;
    }

    if (!entry.isFile()) {
      continue;
    }

    files.push(fullPath);
  }

  return files;
}

if (!existsSync(targetDir)) {
  console.error(`[brotli] Target directory does not exist: ${targetDir}`);
  process.exit(1);
}

// Timing: Record start time
const startTime = Date.now();
const startTimeFmt = new Date().toISOString();
console.log(`[brotli] ==================================================`);
console.log(`[brotli] Brotli Compression Started: ${startTimeFmt}`);
console.log(`[brotli] Target directory: ${targetDir}`);
console.log(`[brotli] ==================================================`);

const allFiles = walkFiles(targetDir);
const candidateFiles = allFiles.filter((filePath) => {
  if (filePath.endsWith('.br')) {
    return false;
  }

  return isEligibleFile(filePath);
});

let createdCount = 0;
let updatedCount = 0;
let skippedCount = 0;
let totalSourceBytes = 0;
let totalBrotliBytes = 0;

for (const sourcePath of candidateFiles) {
  const brotliPath = `${sourcePath}.br`;
  const sourceStats = statSync(sourcePath);
  const brotliExists = existsSync(brotliPath);

  totalSourceBytes += sourceStats.size;

  if (brotliExists) {
    const brotliStats = statSync(brotliPath);

    if (brotliStats.mtimeMs >= sourceStats.mtimeMs) {
      skippedCount++;
      totalBrotliBytes += brotliStats.size;
      continue;
    }
  }

  const sourceBuffer = readFileSync(sourcePath);
  const compressedBuffer = brotliCompressSync(sourceBuffer, {
    params: {
      [zlibConstants.BROTLI_PARAM_MODE]: zlibConstants.BROTLI_MODE_TEXT,
      [zlibConstants.BROTLI_PARAM_QUALITY]: zlibConstants.BROTLI_MAX_QUALITY,
      [zlibConstants.BROTLI_PARAM_SIZE_HINT]: sourceBuffer.length,
    },
  });

  writeFileSync(brotliPath, compressedBuffer);
  totalBrotliBytes += compressedBuffer.length;

  if (brotliExists) {
    updatedCount++;
  } else {
    createdCount++;
  }
}

const bytesSaved = Math.max(totalSourceBytes - totalBrotliBytes, 0);
const savingsPercent = totalSourceBytes > 0
  ? ((bytesSaved / totalSourceBytes) * 100).toFixed(2)
  : '0.00';

// Timing: Calculate elapsed time
const endTime = Date.now();
const endTimeFmt = new Date().toISOString();
const elapsedMs = endTime - startTime;
const elapsedSec = (elapsedMs / 1000).toFixed(2);

console.log(`[brotli] ==================================================`);
console.log(`[brotli] Brotli Compression Complete: ${endTimeFmt}`);
console.log(`[brotli] Elapsed time: ${elapsedSec}s (${elapsedMs}ms)`);
console.log(`[brotli] --------------------------------------------------`);
console.log(`[brotli] Eligible files: ${candidateFiles.length}`);
console.log(`[brotli] Created: ${createdCount}`);
console.log(`[brotli] Updated: ${updatedCount}`);
console.log(`[brotli] Skipped (up-to-date): ${skippedCount}`);
console.log(`[brotli] --------------------------------------------------`);
console.log(`[brotli] Size before: ${formatBytes(totalSourceBytes)} (${totalSourceBytes} bytes)`);
console.log(`[brotli] Size after : ${formatBytes(totalBrotliBytes)} (${totalBrotliBytes} bytes)`);
console.log(`[brotli] Savings    : ${formatBytes(bytesSaved)} (${bytesSaved} bytes, ${savingsPercent}%)`);
console.log(`[brotli] ==================================================`);
