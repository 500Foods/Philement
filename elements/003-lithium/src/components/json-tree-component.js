/**
 * JsonTree.js Component Wrapper
 * 
 * A lightweight wrapper around json-tree.js for use in Lithium managers.
 * Provides a simpler API that matches our use case better than vanilla-jsoneditor.
 * 
 * JsonTree.js uses the DOM element's `id` attribute as its internal instance key.
 * The `render()` method returns the library object (for chaining), NOT an ID.
 * After rendering, the element's `.id` is the key for all subsequent API calls
 * (setJson, getJson, destroy, etc.).
 * 
 * @see https://github.com/williamtroup/JsonTree.js
 */

// Module-level cache for the JsonTree library
let JsonTreeLib = null;

/**
 * Ensure the JsonTree library is loaded and cached.
 * @returns {Promise<Object>} The $jsontree global reference
 */
async function ensureLibrary() {
  if (!JsonTreeLib) {
    await import('json-tree.js');

    if (typeof window !== 'undefined' && window.$jsontree) {
      JsonTreeLib = window.$jsontree;
    } else {
      throw new Error('JsonTree library not found on window.$jsontree');
    }
  }
  return JsonTreeLib;
}

/**
 * Initialize a JsonTree.js instance
 * @param {Object} options - Configuration options
 * @param {HTMLElement} options.target - The DOM element to render the tree into
 * @param {Object} options.data - The JSON data to display
 * @param {boolean} options.readOnly - Whether the tree is read-only (default: true)
 * @param {Function} options.onJsonEdit - Callback when data is edited in the tree
 * @returns {Promise<string>} The element ID used as the instance key
 */
export async function initJsonTree(options) {
  const { target, data = {}, readOnly = true, onJsonEdit = null } = options;

  if (!target) {
    throw new Error('initJsonTree: target element is required');
  }

  const lib = await ensureLibrary();

  // Clear target and set up container
  target.innerHTML = '';
  target.classList.add('json-tree-container');

  // Build the render options
  const renderOptions = {
    data: data,
    allowEditing: !readOnly,
    showStringQuotes: true,
    showCommas: true,
    showArrayIndexBrackets: true,
    showValueColors: true,
  };

  // Wire up the onJsonEdit event for change detection
  if (onJsonEdit) {
    renderOptions.events = {
      onJsonEdit: onJsonEdit,
    };
  }

  // render() returns the library object (for chaining), NOT an ID.
  // After rendering, the library uses target.id as the instance key.
  // If target didn't have an id, the library assigns a random UUID.
  lib.render(target, renderOptions);

  // The element's id attribute is now the instance key for all API calls
  const instanceId = target.id;

  // Store the ID on the element for later reference
  target._jsonTreeId = instanceId;

  return instanceId;
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
 * Set data on a JsonTree.js instance (replaces all data and re-renders)
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
 * Update binding options on a JsonTree.js instance without destroying it.
 * Preserves the current data and view state while updating options like
 * allowEditing, showStringQuotes, etc.
 * 
 * @param {HTMLElement} target - The DOM element containing the tree
 * @param {Object} newOptions - Options to update (e.g., { allowEditing: true })
 */
export function updateJsonTreeOptions(target, newOptions) {
  if (!target || !target._jsonTreeId || !JsonTreeLib) {
    return;
  }

  try {
    JsonTreeLib.updateBindingOptions(target._jsonTreeId, newOptions);
  } catch (e) {
    console.error('Failed to update JsonTree options:', e);
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
  } catch (e) {
    console.error('Failed to destroy JsonTree:', e);
  }

  target._jsonTreeId = null;
  target.innerHTML = '';
}
