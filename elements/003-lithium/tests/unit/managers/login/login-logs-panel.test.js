/**
 * Login Logs Panel Tests
 *
 * Unit tests for the LogsPanel class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { LogsPanel } from '../../../../src/managers/login/login-logs-panel.js';

// Mock log module
vi.mock('../../../../src/core/log.js', () => ({
  getRawLog: vi.fn(() => [
    { ts: 1, subsystem: 'LOGIN', status: 'INFO', message: 'Test entry 1' },
    { ts: 2, subsystem: 'LOGIN', status: 'SUCCESS', message: 'Test entry 2' },
  ]),
}));

// Mock log-formatter
vi.mock('../../../../src/shared/log-formatter.js', () => ({
  formatLogText: vi.fn((entries) => entries.map(e => `[${e.subsystem}] ${e.message}`).join('\n')),
}));

// Mock scrollbar manager
vi.mock('../../../../src/core/scrollbar-manager.js', () => ({
  scrollbarManager: {
    destroy: vi.fn(),
  },
}));

describe('LogsPanel', () => {
  let logViewer;
  let elements;
  let mockEditorView;
  let mockEditorState;
  let mockBuildExtensions;
  let mockCreateRoCompartment;
  let codemirrorImports;
  let codemirrorSetupImports;

  beforeEach(() => {
    vi.clearAllMocks();

    logViewer = document.createElement('div');
    logViewer.id = 'log-viewer';
    document.body.appendChild(logViewer);

    elements = { logViewer };

    // Build a mock CodeMirror EditorView constructor with the API the panel uses.
    mockEditorView = vi.fn(function (config) {
      this.state = {
        doc: { length: (config?.state?.doc?.toString?.() || '').length },
      };
      this.dispatch = vi.fn((tx) => {
        // Simulate content replacement for repeated show() calls.
        if (tx?.changes?.insert !== undefined) {
          this.state.doc.length = tx.changes.insert.length;
        }
      });
      this.destroy = vi.fn();
      this._osbInstance = null;
    });

    mockEditorState = {
      create: vi.fn(({ doc }) => ({
        doc: { toString: () => doc, length: doc.length },
      })),
    };

    mockBuildExtensions = vi.fn(() => ['ext1', 'ext2']);
    mockCreateRoCompartment = vi.fn(() => ({ readOnly: true }));

    codemirrorImports = {
      EditorState: mockEditorState,
      EditorView: mockEditorView,
    };
    codemirrorSetupImports = {
      buildEditorExtensions: mockBuildExtensions,
      createReadOnlyCompartment: mockCreateRoCompartment,
    };
  });

  afterEach(() => {
    if (logViewer && logViewer.parentNode) {
      logViewer.parentNode.removeChild(logViewer);
    }
  });

  describe('constructor', () => {
    it('initializes with provided elements and no editor', () => {
      const panel = new LogsPanel({ elements });
      expect(panel.elements).toBe(elements);
      expect(panel._editorInstance).toBeNull();
    });

    it('accepts injected imports for testing', () => {
      const panel = new LogsPanel({
        elements,
        imports: { codemirror: codemirrorImports, codemirrorSetup: codemirrorSetupImports },
      });
      expect(panel._imports).toBeDefined();
      expect(panel._imports.codemirror).toBe(codemirrorImports);
    });

    it('handles missing options gracefully', () => {
      const panel = new LogsPanel();
      expect(panel.elements).toEqual({});
      expect(panel._imports).toBeNull();
    });
  });

  describe('show()', () => {
    it('warns and returns when logViewer is missing', async () => {
      const panel = new LogsPanel({ elements: {} });
      const warn = vi.spyOn(console, 'warn').mockImplementation(() => {});
      await panel.show();
      expect(warn).toHaveBeenCalledWith(expect.stringContaining('logViewer element not found'));
      expect(panel._editorInstance).toBeNull();
      warn.mockRestore();
    });

    it('initializes CodeMirror on first call with the formatted log text', async () => {
      const panel = new LogsPanel({
        elements,
        imports: { codemirror: codemirrorImports, codemirrorSetup: codemirrorSetupImports },
      });

      await panel.show();

      expect(mockBuildExtensions).toHaveBeenCalledWith(expect.objectContaining({
        language: 'log',
        readOnly: true,
        fontSize: 10,
      }));
      expect(mockEditorState.create).toHaveBeenCalledWith(expect.objectContaining({
        doc: expect.stringContaining('Test entry 1'),
      }));
      expect(mockEditorView).toHaveBeenCalled();
      expect(panel._editorInstance).not.toBeNull();
      // logViewer is cleared before parenting the editor.
      expect(logViewer.innerHTML).toBe('');
    });

    it('updates content via dispatch on subsequent calls (idempotent)', async () => {
      const panel = new LogsPanel({
        elements,
        imports: { codemirror: codemirrorImports, codemirrorSetup: codemirrorSetupImports },
      });

      await panel.show();
      const editor = panel._editorInstance;
      expect(mockEditorView).toHaveBeenCalledTimes(1);

      await panel.show();
      // Should NOT create a new editor instance.
      expect(mockEditorView).toHaveBeenCalledTimes(1);
      // Should dispatch a content replacement.
      expect(editor.dispatch).toHaveBeenCalledWith(expect.objectContaining({
        changes: expect.objectContaining({ from: 0 }),
      }));
    });

    it('falls back to <pre> rendering when CodeMirror init throws', async () => {
      // Drive failure by supplying an `imports.codemirror` that throws when
      // the destructuring assignment touches `EditorState`. This exercises the
      // catch branch without needing a real dynamic-import failure.
      const failingPanel = new LogsPanel({
        elements,
        imports: {
          codemirror: { get EditorState() { throw new Error('boom'); } },
          codemirrorSetup: codemirrorSetupImports,
        },
      });
      const warn = vi.spyOn(console, 'warn').mockImplementation(() => {});

      await failingPanel.show();

      expect(warn).toHaveBeenCalledWith(
        expect.stringContaining('CodeMirror failed to load'),
        expect.any(Error),
      );
      expect(logViewer.innerHTML).toContain('<pre class="log-content">');
      expect(logViewer.innerHTML).toContain('Test entry 1');
      expect(failingPanel._editorInstance).toBeNull();
      warn.mockRestore();
    });
  });

  describe('hide()', () => {
    it('is a no-op (does not destroy editor)', async () => {
      const panel = new LogsPanel({
        elements,
        imports: { codemirror: codemirrorImports, codemirrorSetup: codemirrorSetupImports },
      });
      await panel.show();
      const editor = panel._editorInstance;

      panel.hide();

      expect(editor.destroy).not.toHaveBeenCalled();
      expect(panel._editorInstance).toBe(editor);
    });
  });

  describe('destroy()', () => {
    it('does nothing when editor was never created', () => {
      const panel = new LogsPanel({ elements });
      expect(() => panel.destroy()).not.toThrow();
      expect(panel._editorInstance).toBeNull();
    });

    it('destroys the editor and clears the reference', async () => {
      const panel = new LogsPanel({
        elements,
        imports: { codemirror: codemirrorImports, codemirrorSetup: codemirrorSetupImports },
      });
      await panel.show();
      const editor = panel._editorInstance;

      panel.destroy();

      expect(editor.destroy).toHaveBeenCalledTimes(1);
      expect(panel._editorInstance).toBeNull();
    });

    it('cleans up an attached OverlayScrollbars instance before destroying the editor', async () => {
      const { scrollbarManager } = await import('../../../../src/core/scrollbar-manager.js');
      const panel = new LogsPanel({
        elements,
        imports: { codemirror: codemirrorImports, codemirrorSetup: codemirrorSetupImports },
      });
      await panel.show();
      const editor = panel._editorInstance;
      const fakeOsb = { id: 'osb-instance' };
      editor._osbInstance = fakeOsb;

      panel.destroy();

      expect(scrollbarManager.destroy).toHaveBeenCalledWith(fakeOsb);
      expect(editor.destroy).toHaveBeenCalled();
    });
  });
});
