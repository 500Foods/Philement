#!/usr/bin/env node

/**
 * Prune old hashed deploy assets while preserving a rollback window.
 *
 * Keeps the newest N hashed versions of each top-level assets/*.js and assets/*.css
 * family, together with their related sidecars (.map and .br variants).
 *
 * Examples grouped together as one versioned family:
 *   - assets/index-abc123.js
 *   - assets/index-abc123.js.map
 *   - assets/index-abc123.js.br
 *   - assets/index-abc123.js.map.br
 */

import {
  existsSync,
  readdirSync,
  readFileSync,
  statSync,
  unlinkSync,
} from 'fs';
import { dirname, join, resolve } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = resolve(__dirname, '..');

const targetDirArg = process.argv[2];
const deployDir = targetDirArg
  ? resolve(targetDirArg)
  : resolve(process.env.LITHIUM_ROOT || projectRoot, 'dist');
const assetsDir = join(deployDir, 'assets');
const keepCountRaw = Number.parseInt(process.env.LITHIUM_DEPLOY_KEEP ?? '3', 10);
const keepCount = Number.isFinite(keepCountRaw) && keepCountRaw >= 1 ? keepCountRaw : 3;

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

function parseHashedAsset(fileName) {
  let normalized = fileName;
  let hasBrotli = false;
  let hasSourceMap = false;

  if (normalized.endsWith('.br')) {
    hasBrotli = true;
    normalized = normalized.slice(0, -3);
  }

  if (normalized.endsWith('.map')) {
    hasSourceMap = true;
    normalized = normalized.slice(0, -4);
  }

  const match = normalized.match(/^(?<stem>.+)-(?<hash>[A-Za-z0-9_-]{8,})\.(?<ext>js|css)$/);
  if (!match?.groups) {
    return null;
  }

  return {
    fileName,
    stem: match.groups.stem,
    hash: match.groups.hash,
    ext: match.groups.ext,
    family: `${match.groups.stem}.${match.groups.ext}`,
    hasBrotli,
    hasSourceMap,
  };
}

function collectReferencedAssets() {
  const referenced = new Set();

  for (const fileName of ['index.html', 'service-worker.js']) {
    const fullPath = join(deployDir, fileName);
    if (!existsSync(fullPath)) {
      continue;
    }

    const content = readFileSync(fullPath, 'utf8');
    const matches = content.match(/assets\/[^"'\s)]+/g) || [];
    for (const match of matches) {
      referenced.add(match.replace(/^assets\//, ''));
    }
  }

  return referenced;
}

if (!existsSync(deployDir)) {
  console.error(`[prune] Target directory does not exist: ${deployDir}`);
  process.exit(1);
}

if (!existsSync(assetsDir)) {
  console.log(`[prune] Assets directory not found, nothing to prune: ${assetsDir}`);
  process.exit(0);
}

const referencedAssets = collectReferencedAssets();
const groups = new Map();

for (const fileName of readdirSync(assetsDir)) {
  const parsed = parseHashedAsset(fileName);
  if (!parsed) {
    continue;
  }

  const filePath = join(assetsDir, fileName);
  const stats = statSync(filePath);
  const versionKey = `${parsed.family}::${parsed.hash}`;

  if (!groups.has(parsed.family)) {
    groups.set(parsed.family, new Map());
  }

  const familyVersions = groups.get(parsed.family);
  if (!familyVersions.has(versionKey)) {
    familyVersions.set(versionKey, {
      family: parsed.family,
      hash: parsed.hash,
      newestMtimeMs: 0,
      referenced: false,
      files: [],
    });
  }

  const version = familyVersions.get(versionKey);
  version.newestMtimeMs = Math.max(version.newestMtimeMs, stats.mtimeMs);
  version.referenced = version.referenced || referencedAssets.has(fileName);
  version.files.push({ fileName, filePath, size: stats.size });
}

let deletedFiles = 0;
let deletedBytes = 0;
let deletedVersions = 0;
let keptVersions = 0;

for (const familyVersions of groups.values()) {
  const sortedVersions = [...familyVersions.values()].sort((a, b) => {
    if (a.referenced !== b.referenced) {
      return a.referenced ? -1 : 1;
    }

    return b.newestMtimeMs - a.newestMtimeMs;
  });

  const keep = new Set(sortedVersions.slice(0, keepCount).map((version) => version.hash));

  for (const version of sortedVersions) {
    if (keep.has(version.hash)) {
      keptVersions += 1;
      continue;
    }

    deletedVersions += 1;

    for (const file of version.files) {
      unlinkSync(file.filePath);
      deletedFiles += 1;
      deletedBytes += file.size;
    }
  }
}

console.log(`[prune] Target directory: ${deployDir}`);
console.log(`[prune] Assets directory: ${assetsDir}`);
console.log(`[prune] Keep window per asset family: ${keepCount}`);
console.log(`[prune] Hashed asset families scanned: ${groups.size}`);
console.log(`[prune] Version groups kept: ${keptVersions}`);
console.log(`[prune] Version groups deleted: ${deletedVersions}`);
console.log(`[prune] Files deleted: ${deletedFiles}`);
console.log(`[prune] Space reclaimed: ${formatBytes(deletedBytes)} (${deletedBytes} bytes)`);
