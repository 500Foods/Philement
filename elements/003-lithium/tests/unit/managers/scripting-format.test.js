import { describe, it, expect, vi } from 'vitest';
import * as stylua from '@johnnymorganz/stylua';
import { toast } from '../../../src/shared/toast.js';
import { EditorManager } from '../../../src/managers/scripting/scripting-editors.js';
import {
  formatErrorMessage,
  formatLuaSource,
  formatWithStyLua,
} from '../../../src/managers/scripting/scripting-format.js';

function nodeStyLua() {
  return Promise.resolve(stylua);
}

function fakeEditor(text) {
  let current = text;
  const calls = [];
  return {
    calls,
    state: {
      doc: {
        toString: () => current,
        get length() { return current.length; },
      },
    },
    dispatch(change) {
      calls.push(change);
      current = change.changes.insert;
    },
  };
}

describe('formatLuaSource', () => {
  it('formats with an injected function and skips an unchanged buffer', async () => {
    const changed = await formatLuaSource('local x=1', async () => 'local x = 1\n');
    expect(changed).toEqual({
      ok: true,
      empty: false,
      changed: true,
      text: 'local x = 1\n',
    });
    const same = await formatLuaSource('local x = 1\n', async () => 'local x = 1\n');
    expect(same.changed).toBe(false);
  });

  it('does not call the formatter for empty source', async () => {
    const formatFn = vi.fn();
    expect(await formatLuaSource('  \n', formatFn)).toEqual({
      ok: true,
      empty: true,
      changed: false,
      text: '  \n',
    });
    expect(formatFn).not.toHaveBeenCalled();
  });

  it('keeps a string parse error and does not throw', async () => {
    const result = await formatLuaSource('function (', async () => {
      throw 'error parsing: unexpected token `(`';
    });
    expect(result).toEqual({
      ok: false,
      message: 'error parsing: unexpected token `(`',
    });
    expect(formatErrorMessage(new Error('boom'))).toBe('boom');
    expect(formatErrorMessage({})).toBe('Unknown formatting error');
  });
});

describe('formatWithStyLua', () => {
  it('formats ordinary Lua with the installed package', async () => {
    const text = await formatWithStyLua('local x=1', nodeStyLua);
    expect(text).toBe('local x = 1\n');
  });

  it('returns a parse message for Lua 5.5 global', async () => {
    const result = await formatLuaSource(
      'global x = 1',
      (source) => formatWithStyLua(source, nodeStyLua),
    );
    expect(result.ok).toBe(false);
    expect(result.message).toMatch(/error parsing: unexpected token `x`/);
  });
});

describe('EditorManager.handlePrettify', () => {
  it('enters edit mode and replaces the document', async () => {
    const row = { group_name: 'Api', script_name: 'Echo' };
    const enterEditMode = vi.fn(async () => {});
    const manager = {
      scriptTable: {
        isEditing: false,
        table: { getSelectedRows: () => [row] },
        enterEditMode,
      },
    };
    const editors = new EditorManager(manager);
    editors.luaEditor = fakeEditor('local function hi( name )\nreturn name\nend');
    await editors.handlePrettify(async () => 'local function hi(name)\n\treturn name\nend\n');
    expect(enterEditMode).toHaveBeenCalledWith(row);
    expect(editors.luaEditor.calls).toHaveLength(1);
    expect(editors.luaEditor.calls[0].changes).toEqual({
      from: 0,
      to: 'local function hi( name )\nreturn name\nend'.length,
      insert: 'local function hi(name)\n\treturn name\nend\n',
    });
  });

  it('leaves the buffer unchanged when StyLua throws a string', async () => {
    const errorSpy = vi.spyOn(toast, 'error').mockImplementation(() => 'toast-id');
    const editors = new EditorManager({ scriptTable: { isEditing: true } });
    editors.luaEditor = fakeEditor('function (');
    await editors.handlePrettify(async () => {
      throw 'error parsing: unexpected token `(`';
    });
    expect(editors.luaEditor.calls).toHaveLength(0);
    expect(editors.luaEditor.state.doc.toString()).toBe('function (');
    expect(errorSpy).toHaveBeenCalledWith('Lua Format Failed', expect.objectContaining({
      description: 'error parsing: unexpected token `(`',
      subsystem: 'Scripting',
    }));
    errorSpy.mockRestore();
  });

  it('asks for a selection when the row is not in edit mode', async () => {
    const warningSpy = vi.spyOn(toast, 'warning').mockImplementation(() => 'toast-id');
    const formatFn = vi.fn(async (source) => source);
    const editors = new EditorManager({
      scriptTable: {
        isEditing: false,
        table: { getSelectedRows: () => [] },
        enterEditMode: vi.fn(),
      },
    });
    editors.luaEditor = fakeEditor('local x=1');
    await editors.handlePrettify(formatFn);
    expect(formatFn).not.toHaveBeenCalled();
    expect(editors.luaEditor.calls).toHaveLength(0);
    expect(warningSpy).toHaveBeenCalledWith(
      'Select a script before formatting',
      expect.objectContaining({ subsystem: 'Scripting' }),
    );
    warningSpy.mockRestore();
  });

  it('does not dispatch when the text is already formatted', async () => {
    const editors = new EditorManager({});
    editors.luaEditor = fakeEditor('local x = 1\n');
    await editors.handlePrettify(async (source) => source);
    expect(editors.luaEditor.calls).toHaveLength(0);
  });
});
