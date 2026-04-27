/**
 * Copy Manager HTML Templates
 * 
 * Vite only bundles JS/CSS files. HTML templates are fetched at runtime via fetch()
 * and must exist in public/src/managers/{name}/ to be available in production.
 * 
 * This script automatically copies all HTML templates from src/managers/ to public/src/managers/
 * so they are included in the deployment.
 */

import { readdirSync, copyFileSync, mkdirSync, existsSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const ROOT_DIR = join(__dirname, '..');
const SRC_MANAGERS_DIR = join(ROOT_DIR, 'src', 'managers');
const PUBLIC_MANAGERS_DIR = join(ROOT_DIR, 'public', 'src', 'managers');

// Get all manager directories in src/managers/
const managers = readdirSync(SRC_MANAGERS_DIR, { withFileTypes: true })
  .filter(dirent => dirent.isDirectory())
  .map(dirent => dirent.name);

console.log('Copying manager HTML templates...\n');

let copiedCount = 0;
let skippedCount = 0;

for (const manager of managers) {
  const srcDir = join(SRC_MANAGERS_DIR, manager);
  const destDir = join(PUBLIC_MANAGERS_DIR, manager);
  
  // Find HTML and CSS files in the manager directory (recursively)
  const files = [];
  function findTemplateFiles(dir) {
    const items = readdirSync(dir, { withFileTypes: true });
    for (const item of items) {
      if (item.isDirectory()) {
        findTemplateFiles(join(dir, item.name));
      } else if (item.name.endsWith('.html') || item.name.endsWith('.css')) {
        files.push(join(dir.substring(srcDir.length + 1), item.name));
      }
    }
  }
  findTemplateFiles(srcDir);
  
  if (files.length === 0) {
    console.log(`  ${manager}/: No HTML templates found (skipped)`);
    skippedCount++;
    continue;
  }
  
  // Create destination directory if it doesn't exist
  if (!existsSync(destDir)) {
    mkdirSync(destDir, { recursive: true });
  }
  
  // Copy each HTML file
  for (const file of files) {
    const srcPath = join(srcDir, file);
    const destPath = join(destDir, file);

    // Create destination directory if it doesn't exist
    const destDirPath = dirname(destPath);
    if (!existsSync(destDirPath)) {
      mkdirSync(destDirPath, { recursive: true });
    }

    copyFileSync(srcPath, destPath);
    console.log(`  ${manager}/${file}`);
    copiedCount++;
  }
}

console.log(`\nDone: ${copiedCount} template(s) copied, ${skippedCount} manager(s) skipped (no HTML files)`);
