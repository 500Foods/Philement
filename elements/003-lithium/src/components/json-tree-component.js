/**
 * JsonTree.js Component Wrapper
 * 
 * A lightweight wrapper around json-tree.js for use in Lithium managers.
 * Provides a simpler API that matches our use case better than vanilla-jsoneditor.
 * 
 * @see https://github.com/williamtroup/JsonTree.js
 */

// Module-level cache for the JsonTree library
let JsonTreeLib = null;

/**
 * Initialize a JsonTree.js instance
 * @param {Object} options - Configuration options
 * @param {HTMLElement} options.target - The DOM element to render the tree into
 * @param {Object} options.data - The JSON data to display
 * @param {boolean} options.readOnly - Whether the tree is read-only (default: true)
 * @param {Function} options.onChange - Callback when data changes (receives updated data)
 * @returns {Promise<string>} The ID of the created instance
 */
export async function initJsonTree(options) {
  const { target, data = {}, readOnly = true, onChange = null } = options;

  if (!target) {
    throw new Error('initJsonTree: target element is required');
  }

  // Dynamic import json-tree.js
  if (!JsonTreeLib) {
    await import('json-tree.js');
    
    if (typeof window !== 'undefined' && window.$jsontree) {
      JsonTreeLib = window.$jsontree;
    } else {
      throw new Error('JsonTree library not found on window.$jsontree');
    }
  }

  // Clear target and set up container
  target.innerHTML = '';
  target.classList.add('json-tree-container');

  // Render the tree
  const id = JsonTreeLib.render(target, {
    data: data,
    allowEditing: !readOnly,
    showStringQuotes: true,
    showCommas: true,
    showArrayIndexBrackets: true,
    showValueColors: true,
    onDataChange: onChange,
  });

  // Store the ID for later reference
  target._jsonTreeId = id;

  return id;
}

/**
 * Get data from a JsonTree.js instance
 * @param {HTMLElement} target - The DOM element containing the tree
 * @returns {Object|null} The current JSON data
 */
export function getJsonTreeData(target) {
  if (!target || !target._jsonTreeId || !JsonTreeLib) {
    return null;
  }
  
  try {
    return JsonTreeLib.getJson(target._jsonTreeId);
  } catch (e) {
    console.error('Failed to get JsonTree data:', e);
    return null;
  }
}

/**
 * Set data on a JsonTree.js instance
 * @param {HTMLElement} target - The DOM element containing the tree
 * @param {Object} data - The new JSON data
 */
export function setJsonTreeData(target, data) {
  if (!target || !target._jsonTreeId || !JsonTreeLib) {
    return;
  }
  
  try {
    JsonTreeLib.setJson(target._jsonTreeId, data);
  } catch (e) {
    console.error('Failed to set JsonTree data:', e);
  }
}

/**
 * Destroy a JsonTree.js instance
 * @param {HTMLElement} target - The DOM element containing the tree
 */
export function destroyJsonTree(target) {
  if (!target || !target._jsonTreeId || !JsonTreeLib) {
    return;
  }
  
  try {
    JsonTreeLib.destroy(target._jsonTreeId);
    target._jsonTreeId = null;
    target._jsonTreeData = null;
    target.innerHTML = '';
  } catch (e) {
    console.error('Failed to destroy JsonTree:', e);
  }
}
