import { describe, it, expect } from 'vitest';
import { buildScriptSummaryHtml, scriptPreviewLua } from '../../../src/managers/scripting/scripting-preview.js';
import { sanitizeHtml } from '../../../src/core/utils.js';

describe('scriptPreviewLua', () => {
  it('uses the editor buffer, including a blank one', () => {
    expect(scriptPreviewLua({ editorText: 'local x = 1', pending: 'old', code: 'row' })).toBe('local x = 1');
    expect(scriptPreviewLua({ editorText: '', pending: 'old', code: 'row' })).toBe('');
  });

  it('falls back to the pending load, then the row', () => {
    expect(scriptPreviewLua({ editorText: null, pending: 'pending', code: 'row' })).toBe('pending');
    expect(scriptPreviewLua({ pending: '', code: 'row' })).toBe('');
    expect(scriptPreviewLua({ code: 'row' })).toBe('row');
    expect(scriptPreviewLua({})).toBe('');
  });
});

describe('buildScriptSummaryHtml', () => {
  it('is empty when the summary is blank', async () => {
    const parse = () => {
      throw new Error('parse should not run');
    };
    expect(await buildScriptSummaryHtml(null, { parse })).toEqual({ empty: true, html: '' });
    expect(await buildScriptSummaryHtml('   ', { parse })).toEqual({ empty: true, html: '' });
  });

  it('sanitizes the parsed summary', async () => {
    const built = await buildScriptSummaryHtml('# Hello\n\n<script>alert(1)</script>', {
      parse: async () => '<h1>Hello</h1><script>alert(1)</script><p>Keep</p>',
      sanitize: sanitizeHtml,
    });
    expect(built.empty).toBe(false);
    expect(built.html).toContain('Hello');
    expect(built.html).toContain('<p>Keep</p>');
    expect(built.html.toLowerCase()).not.toContain('<script');
  });

  it('reports a parse failure without html', async () => {
    const built = await buildScriptSummaryHtml('summary', {
      parse: async () => {
        throw new Error('marked down');
      },
    });
    expect(built.failed).toBe(true);
    expect(built.message).toBe('marked down');
    expect(built.html).toBe('');
  });
});
