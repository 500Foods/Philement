/**
 * Lua stream highlighter. A long bracket used to call slice on
 * stream.match, which is the method, and row changes threw.
 */

import { describe, it, expect } from 'vitest';
import { EditorState, EditorView, foldGutter, lua, luaFoldRanges } from '../../src/core/codemirror.js';
import { ensureSyntaxTree, foldState } from '@codemirror/language';
import { foldAllInEditor, setEditorContentNoHistory } from '../../src/core/codemirror-setup.js';

function tokenNames(doc) {
  const state = EditorState.create({
    doc,
    extensions: [lua()],
  });
  const tree = ensureSyntaxTree(state, doc.length, 5000);
  expect(tree).toBeTruthy();
  const names = [];
  tree.iterate({
    enter(node) {
      if (node.name) names.push(node.name);
    },
  });
  return names;
}

function paint(doc) {
  const parent = document.createElement('div');
  document.body.appendChild(parent);
  const view = new EditorView({
    parent,
    state: EditorState.create({
      doc: '',
      extensions: [lua()],
    }),
  });
  try {
    setEditorContentNoHistory(view, doc);
    ensureSyntaxTree(view.state, doc.length, 5000);
    expect(view.state.doc.toString()).toBe(doc);
  } finally {
    view.destroy();
    parent.remove();
  }
}

describe('lua highlighter', () => {
  it('paints a long string without throwing', () => {
    const doc = 'local sql = [[select id from orders]]';
    expect(() => tokenNames(doc)).not.toThrow();
    expect(tokenNames(doc)).toContain('string');
  });

  it('paints an equals-level long string and keeps the following keyword', () => {
    const names = tokenNames('local sql = [=[select ]] from t]=]\nlocal done = true');
    expect(names).toContain('string');
    expect(names).toContain('keyword');
  });

  it('keeps a multi-line long string open across a blank line', () => {
    const names = tokenNames('local body = [[\nline\n\nstill]]\nreturn body');
    expect(names.filter(name => name === 'string').length).toBeGreaterThan(0);
    expect(names).toContain('keyword');
  });

  it('paints a long comment, including --[=[', () => {
    expect(tokenNames('--[[\nstill a comment\n]]\nlocal x = 1')).toContain('comment');
    expect(tokenNames('--[=[ ]=]\nlocal x = 1')).toContain('comment');
  });

  it('replaces editor text the way a row change does', () => {
    expect(() => paint('local q = [[Account.Orders]]')).not.toThrow();
    expect(() => paint('local q = [=[Enroll\n]=]')).not.toThrow();
    expect(() => paint('local function echo(value)\n  return value\nend')).not.toThrow();
  });

  it('tints H. and H.name, and leaves a bare H as a name', () => {
    expect(tokenNames('local url = H.http')).toContain('namespace');
    expect(tokenNames('H.')).toContain('namespace');
    expect(tokenNames('H.http')).not.toContain('variableName');

    const bare = tokenNames('local name = H');
    expect(bare).toContain('variableName');
    expect(bare).not.toContain('namespace');
    expect(tokenNames('local other = notH.http')).not.toContain('namespace');
  });

  it('does not tint H. inside a string or a comment', () => {
    expect(tokenNames('local s = "H.http"')).not.toContain('namespace');
    expect(tokenNames('local s = [[H.http]]')).not.toContain('namespace');
    expect(tokenNames('-- H.http\nlocal x = 1')).not.toContain('namespace');
  });
});

function lineBounds(source, lineNo) {
  const lines = source.split('\n');
  let from = 0;
  for (let i = 1; i < lineNo; i++) from += lines[i - 1].length + 1;
  return { from, to: from + lines[lineNo - 1].length };
}

function bodyFold(source, openLine, closeLine) {
  const open = lineBounds(source, openLine);
  const close = lineBounds(source, closeLine);
  return { lineFrom: open.from, from: open.to, to: close.from };
}

describe('lua statement folds', () => {
  it('folds function, if, do, and repeat, and keeps the header line', () => {
    const fn = 'function f()\n  return 1\nend';
    expect(luaFoldRanges(fn)).toEqual([bodyFold(fn, 1, 3)]);

    const conditional = 'if x then\n  return 1\nelse\n  return 2\nend';
    expect(luaFoldRanges(conditional)).toEqual([bodyFold(conditional, 1, 5)]);

    const block = 'do\n  local x = 1\nend';
    expect(luaFoldRanges(block)).toEqual([bodyFold(block, 1, 3)]);

    const loop = 'repeat\n  x = x + 1\nuntil x > 3';
    expect(luaFoldRanges(loop)).toEqual([bodyFold(loop, 1, 3)]);
  });

  it('treats the do of for and while as the block body, not a second fold', () => {
    const counted = 'for i = 1, 10 do\n  print(i)\nend';
    expect(luaFoldRanges(counted)).toEqual([bodyFold(counted, 1, 3)]);

    const spinning = 'while true do\n  break\nend';
    expect(luaFoldRanges(spinning)).toEqual([bodyFold(spinning, 1, 3)]);
  });

  it('folds a nested if inside a function', () => {
    const source = 'function f()\n  if x then\n    return 1\n  end\nend';
    expect(luaFoldRanges(source)).toEqual([
      bodyFold(source, 2, 4),
      bodyFold(source, 1, 5),
    ]);
  });

  it('does not fold a block that opens and closes on one line', () => {
    expect(luaFoldRanges('function f() return 1 end')).toEqual([]);
  });

  it('ignores keywords inside strings, long strings, and comments', () => {
    const trapped = 'local s = [[\nfunction ghost()\nend\n]]\nlocal x = 1';
    expect(luaFoldRanges(trapped)).toEqual([]);

    const source = 'function f()\n  local s = [[\nend\n]]\n  return 1\nend';
    expect(luaFoldRanges(source)).toEqual([bodyFold(source, 1, 6)]);

    const commented = 'for i = 1, 3 do\n  -- end\n  print(i)\nend';
    expect(luaFoldRanges(commented)).toEqual([bodyFold(commented, 1, 4)]);
  });

  it('does not fold parentheses or braces', () => {
    expect(luaFoldRanges('local t = {\n  a = 1\n}')).toEqual([]);
    expect(luaFoldRanges('local f = (\n  1\n)')).toEqual([]);
  });

  it('fold all collapses the function body in the editor', () => {
    const source = 'function f()\n  return 1\nend\n';
    const parent = document.createElement('div');
    document.body.appendChild(parent);
    const view = new EditorView({
      parent,
      state: EditorState.create({
        doc: source,
        extensions: [lua(), foldGutter()],
      }),
    });
    try {
      ensureSyntaxTree(view.state, source.length, 5000);
      foldAllInEditor(view);
      const found = [];
      view.state.field(foldState).between(0, view.state.doc.length, (from, to) => {
        found.push({ from, to });
      });
      const expected = bodyFold(source, 1, 3);
      expect(found).toContainEqual({ from: expected.from, to: expected.to });
    } finally {
      view.destroy();
      parent.remove();
    }
  });
});
