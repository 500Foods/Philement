/**
 * Lua stream highlighter. A long bracket used to call slice on
 * stream.match, which is the method, and row changes threw.
 */

import { describe, it, expect } from 'vitest';
import { EditorState, EditorView, lua } from '../../src/core/codemirror.js';
import { ensureSyntaxTree } from '@codemirror/language';
import { setEditorContentNoHistory } from '../../src/core/codemirror-setup.js';

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
});
