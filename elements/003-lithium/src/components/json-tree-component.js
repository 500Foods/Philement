/**
 * JSON Editor Component (CodeMirror-based)
 *
 * A CodeMirror 6 wrapper for JSON editing that replaces the previous
 * JsonTree.js implementation. Uses @codemirror/lang-json for syntax
 * highlighting and follows the same API as the old component for
 * drop-in compatibility.
 *
 * The component stores the CodeMirror EditorView on the target element's
 * `_cmView` property. The data is stored as a JSON string on `_cmData`
 * and kept in sync via the EditorView's doc.
 *
 * readOnly state is managed via a CodeMirror Compartment so it can be
 * toggled at runtime with dispatch({ effects: readOnlyCompartment.reconfigure(...) }).
 */

// Module-level reference to Compartment (resolved after first init)
let CompartmentRef = null;

/**
 * Initialize a CodeMirror JSON editor on the target element.
 * @param {Object} options - Configuration options
 * @param {HTMLElement} options.target - The DOM element to render into
 * @param {Object} options.data - The JSON data to display
 * @param {boolean} options.readOnly - Whether the editor is read-only (default: true)
 * @param {Function} options.onJsonEdit - Callback when data is edited
 * @returns {Promise<string>} A synthetic instance key (element id or 'cm-editor')
 */
export async function initJsonTree(options) {
  const { target, data = {}, readOnly = true, onJsonEdit = null } = options;

  if (!target) {
    throw new Error('initJsonTree: target element is required');
  }

  // Clean up any existing editor
  if (target._cmView) {
    destroyJsonTree(target);
  }

  const jsonStr = formatJson(data);

  const [
    { EditorState, Compartment },
    { EditorView, keymap },
    { defaultKeymap, history, historyKeymap },
    { json },
    { oneDark },
  ] = await Promise.all([
    import('@codemirror/state'),
    import('@codemirror/view'),
    import('@codemirror/commands'),
    import('@codemirror/lang-json'),
    import('@codemirror/theme-one-dark'),
  ]);

  // Cache the Compartment constructor for use by updateJsonTreeOptions
  CompartmentRef = Compartment;

  const readOnlyCompartment = new Compartment();

  const extensions = [
    json(),
    oneDark,
    history(),
    keymap.of([...defaultKeymap, ...historyKeymap]),
    EditorView.theme({
      '&': { height: '100%', fontSize: '13px' },
      '.cm-scroller': { overflow: 'auto', fontFamily: 'var(--font-mono, monospace)' },
      '.cm-content': { caretColor: 'var(--accent-primary, #007acc)' },
    }),
    readOnlyCompartment.of(EditorState.readOnly.of(readOnly)),
  ];

  if (onJsonEdit) {
    extensions.push(
      EditorView.updateListener.of((update) => {
        if (update.docChanged) {
          onJsonEdit();
        }
      })
    );
  }

  const state = EditorState.create({
    doc: jsonStr,
    extensions,
  });

  target.innerHTML = '';
  target.classList.add('json-tree-container');

  const view = new EditorView({
    state,
    parent: target,
  });

  // Store references on the target element
  target._cmView = view;
  target._cmData = jsonStr;
  target._cmReadOnlyCompartment = readOnlyCompartment;
  target._cmReadOnly = readOnly;

  if (readOnly) {
    target.classList.add('jsoneditor-readonly');
  }

  return target.id || 'cm-editor';
}

/**
 * Get data from a CodeMirror JSON editor.
 * @param {HTMLElement} target - The DOM element containing the editor
 * @returns {Object|null} The current JSON data as an object
 */
export function getJsonTreeData(target) {
  if (!target || !target._cmView) {
    return null;
  }

  try {
    const text = target._cmView.state.doc.toString();
    return JSON.parse(text);
  } catch (e) {
    console.error('Failed to parse JSON from editor:', e);
    return null;
  }
}

/**
 * Set data on a CodeMirror JSON editor (replaces all content).
 * @param {HTMLElement} target - The DOM element containing the editor
 * @param {Object} data - The new JSON data
 */
export function setJsonTreeData(target, data) {
  if (!target || !target._cmView) {
    return;
  }

  try {
    const jsonStr = formatJson(data);
    const view = target._cmView;
    view.dispatch({
      changes: { from: 0, to: view.state.doc.length, insert: jsonStr },
    });
    target._cmData = jsonStr;
  } catch (e) {
    console.error('Failed to set JSON editor data:', e);
  }
}

/**
 * Update editor options (e.g., read-only state) without destroying the editor.
 * Uses the Compartment stored on the target element to reconfigure readOnly.
 *
 * @param {HTMLElement} target - The DOM element containing the editor
 * @param {Object} newOptions - Options to update (e.g., { allowEditing: true })
 */
export function updateJsonTreeOptions(target, newOptions) {
  if (!target || !target._cmView) {
    return;
  }

  try {
    if (typeof newOptions.allowEditing === 'boolean') {
      // In the JsonTree API, allowEditing=true means editable (not readOnly)
      const isReadOnly = !newOptions.allowEditing;
      const view = target._cmView;
      const compartment = target._cmReadOnlyCompartment;

      if (!compartment) {
        console.error('updateJsonTreeOptions: no compartment found on target');
        return;
      }

      view.dispatch({
        effects: compartment.reconfigure(
          CompartmentRef ? CompartmentRef.of(isReadOnly) : isReadOnly
        ),
      });

      target._cmReadOnly = isReadOnly;
      target.classList.toggle('jsoneditor-readonly', isReadOnly);
    }
  } catch (e) {
    console.error('Failed to update JSON editor options:', e);
  }
}

/**
 * Destroy a CodeMirror JSON editor and clean up resources.
 * @param {HTMLElement} target - The DOM element containing the editor
 */
export function destroyJsonTree(target) {
  if (!target) {
    return;
  }

  if (target._cmView) {
    target._cmView.destroy();
    target._cmView = null;
  }

  target._cmData = null;
  target._cmReadOnly = null;
  target._cmReadOnlyCompartment = null;
  target.classList.remove('json-tree-container', 'jsoneditor-readonly');
  target.innerHTML = '';
}

/**
 * Format an object as a pretty-printed JSON string.
 * @param {Object} data - The data to format
 * @returns {string} Formatted JSON string
 */
function formatJson(data) {
  if (typeof data === 'string') {
    try {
      const parsed = JSON.parse(data);
      return JSON.stringify(parsed, null, 2);
    } catch {
      return data;
    }
  }
  return JSON.stringify(data, null, 2);
}
