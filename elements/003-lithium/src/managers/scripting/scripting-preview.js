/**
 * Scripting preview: sanitized summary markdown, then the Lua source.
 * The Lua is plain text. HTML is sanitized before display.
 */

import { sanitizeHtml } from '../../core/utils.js';

/**
 * @param {unknown} summary
 * @param {{ parse?: (markdown: string) => Promise<string>|string, sanitize?: (html: string) => string }} [deps]
 * @returns {Promise<{ empty: true, html: '' } | { empty: false, failed?: boolean, message?: string, html: string }>}
 */
export async function buildScriptSummaryHtml(summary, deps = {}) {
  const text = summary == null ? '' : String(summary);
  if (text.trim() === '') {
    return { empty: true, html: '' };
  }

  const parse = deps.parse || parseMarkdown;
  const sanitize = deps.sanitize || sanitizeHtml;

  try {
    const raw = await parse(text);
    const html = sanitize(typeof raw === 'string' ? raw : '');
    return { empty: false, html };
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    return { empty: false, failed: true, message, html: '' };
  }
}

/**
 * Editor text wins, including a blank buffer. Otherwise the pending
 * load, then the row's code.
 * @param {{ editorText?: string | null, pending?: string | null, code?: unknown }} [sources]
 * @returns {string}
 */
export function scriptPreviewLua(sources = {}) {
  if (typeof sources.editorText === 'string') return sources.editorText;
  if (typeof sources.pending === 'string') return sources.pending;
  if (sources.code == null) return '';
  return String(sources.code);
}

async function parseMarkdown(markdown) {
  const { marked } = await import('marked');
  return marked.parse(markdown);
}
