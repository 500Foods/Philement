/**
 * Virtual-column pad and trim, and a single registration beside a compartment.
 */

import { describe, it, expect } from 'vitest';
import { EditorState, EditorView } from '../../src/core/codemirror.js';
import { virtualColumns } from '../../src/core/cm6-virtual-columns.js';
import {
  buildEditorExtensions,
  createVirtualColumnsCompartment,
} from '../../src/core/codemirror-setup.js';

function press(view, key) {
  const event = new window.KeyboardEvent('keydown', {
    key,
    bubbles: true,
    cancelable: true,
  });
  view.contentDOM.dispatchEvent(event);
}

describe('virtual columns', () => {
  it('pads a short line to the column and trims those spaces on leave', async () => {
    const parent = document.createElement('div');
    document.body.appendChild(parent);
    const view = new EditorView({
      parent,
      state: EditorState.create({
        doc: 'abcdef\nxy\nz',
        selection: { anchor: 6 },
        extensions: [virtualColumns()],
      }),
    });

    try {
      press(view, 'ArrowDown');
      expect(view.state.doc.line(2).text).toBe('xy    ');
      expect(view.state.selection.main.head).toBe(view.state.doc.line(2).to);

      press(view, 'ArrowDown');
      await Promise.resolve();
      expect(view.state.doc.line(2).text).toBe('xy');
      expect(view.state.doc.line(3).text).toBe('z     ');
    } finally {
      view.destroy();
      parent.remove();
    }
  });

  it('registers the extension once when a compartment is provided', () => {
    const compartment = createVirtualColumnsCompartment();
    const extensions = buildEditorExtensions({
      language: 'lua',
      virtualColumnsCompartment: compartment,
      virtualColumns: true,
      lineNumbersEnabled: false,
    });
    expect(() => EditorState.create({ doc: 'ab\nc', extensions })).not.toThrow();
  });
});
