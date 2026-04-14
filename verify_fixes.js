#!/usr/bin/env node
/**
 * Verification script for Lithium Table fixes
 * 
 * This script verifies that the fixes for selection persistence and button flashing are correctly applied
 */

const fs = require('fs');
const path = '/mnt/extra/Projects/Philement/elements/003-lithium/src/tables/lithium-table-base.js';

const content = fs.readFileSync(path, 'utf8');

const checks = {
  'autoSelectRow saves selection': /autoSelectRow\(targetId\) \{[^}]*saveSelectedRowId/.test(content),
  'selectDataRow saves selection': /selectDataRow\([^}]*saveSelectedRowId/.test(content),
  '_pendingSelection property initialized': /this\._pendingSelection = null/.test(content),
  '_pendingSelection used in autoSelectRow': /this\._pendingSelection/.test(content),
  '_inSelectionTransition set before setData': /this\._inSelectionTransition = true;[^}]*this\.table\.setData/.test(content),
  'rowSelectionChanged checks _inSelectionTransition': /rowSelectionChanged\([^}]*if \(this\._inSelectionTransition\)/.test(content),
  'setTimeout clears _inSelectionTransition': /setTimeout\([^}]*_inSelectionTransition = false/.test(content),
  'Pending selection processing after autoSelectRow': /autoSelectRow\(previouslySelectedId\)[^}]*if \(this\._pendingSelection\)/.test(content)
};

console.log('Verification Results:');
console.log('=' .repeat(50));

let allPassed = true;
for (const [check, result] of Object.entries(checks)) {
  const status = result ? '✓ PASS' : '✗ FAIL';
  console.log(`${status}: ${check}`);
  if (!result) allPassed = false;
}

console.log('=' .repeat(50));
console.log(allPassed ? 'All checks passed!' : 'Some checks failed!');

process.exit(allPassed ? 0 : 1);