/**
 * Custom scrollbar plugin removes its DOM on destroy.
 */

import { describe, it, expect } from 'vitest';
import { EditorState, EditorView } from '../../src/core/codemirror.js';
import { customScrollbars } from '../../src/core/cm6-custom-scrollbars.js';

describe('custom scrollbars', () => {
  it('removes the tracks and the corner when the view is destroyed', () => {
    const parent = document.createElement('div');
    document.body.appendChild(parent);
    const view = new EditorView({
      parent,
      state: EditorState.create({
        doc: 'hello\n'.repeat(40),
        extensions: [customScrollbars()],
      }),
    });

    const scroller = view.scrollDOM;
    expect(scroller.querySelector('.cm-scroller-track-v')).toBeTruthy();
    expect(scroller.querySelector('.cm-scroller-track-h')).toBeTruthy();
    expect(scroller.querySelector('.cm-scroller-corner')).toBeTruthy();

    view.destroy();

    expect(scroller.querySelector('.cm-scroller-track-v')).toBeNull();
    expect(scroller.querySelector('.cm-scroller-track-h')).toBeNull();
    expect(scroller.querySelector('.cm-scroller-corner')).toBeNull();
    parent.remove();
  });
});
