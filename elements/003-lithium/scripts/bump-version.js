#!/usr/bin/env node

/**
 * Bump Version Script
 * 
 * Increments the build number in version.json and updates the timestamp.
 * The build number starts at 1000 and increments by 1 on each deploy.
 * The version string format is "0.1.<build>".
 * 
 * Also:
 * - Copies version.json to public/ so it's available at runtime
 * - Updates the service worker's CACHE_VERSION to the new build number
 *   (triggers cache invalidation on deploy)
 * 
 * Usage: node scripts/bump-version.js
 */

import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'fs';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = resolve(__dirname, '..');

const versionFile = resolve(projectRoot, 'version.json');
const serviceWorkerFile = resolve(projectRoot, 'public', 'service-worker.js');

// Read current version
let versionData;
try {
  versionData = JSON.parse(readFileSync(versionFile, 'utf-8'));
} catch (error) {
  console.error('[bump-version] Could not read version.json:', error.message);
  process.exit(1);
}

// Increment build number
const oldBuild = versionData.build;
versionData.build = oldBuild + 1;

// Update timestamp with ISO format
const now = new Date();
versionData.timestamp = now.toISOString();

// Update version string: "1.1.<build>"
versionData.version = `1.1.${versionData.build}`;

// Write updated version.json back to project root
writeFileSync(versionFile, JSON.stringify(versionData, null, 2) + '\n', 'utf-8');

// Copy version.json to public/ so it's available at runtime via fetch
const publicDir = resolve(projectRoot, 'public');
if (!existsSync(publicDir)) {
  mkdirSync(publicDir, { recursive: true });
}
writeFileSync(resolve(publicDir, 'version.json'), JSON.stringify(versionData, null, 2) + '\n', 'utf-8');

// Update service worker CACHE_VERSION to bust caches on deploy
try {
  let swContent = readFileSync(serviceWorkerFile, 'utf-8');
  swContent = swContent.replace(
    /const CACHE_VERSION = \d+;/,
    `const CACHE_VERSION = ${versionData.build};`
  );
  writeFileSync(serviceWorkerFile, swContent, 'utf-8');
  console.log(`[bump-version] Service worker CACHE_VERSION → ${versionData.build}`);
} catch (error) {
  console.warn('[bump-version] Could not update service worker:', error.message);
}

console.log(`[bump-version] Build ${oldBuild} → ${versionData.build} (${versionData.version})`);
console.log(`[bump-version] Timestamp: ${versionData.timestamp}`);
