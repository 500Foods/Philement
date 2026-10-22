/**
 * Scripting Manager — Lua format.
 *
 * Ctrl+P formats the Lua editor with StyLua defaults: tabs, indent
 * width 4, column width 120, Unix line endings. A fresh Config is
 * required for every call. Parse failures throw strings.
 */

/**
 * Turn a StyLua failure into one line. The wasm build throws a string,
 * not an Error, so `error.message` is empty on that path.
 *
 * @param {unknown} error
 * @returns {string}
 */
export function formatErrorMessage(error) {
  let text = '';
  if (typeof error === 'string') {
    text = error;
  } else if (error && typeof error.message === 'string') {
    text = error.message;
  } else if (error != null) {
    text = String(error);
  }
  text = text.replace(/\s+/g, ' ').trim();
  if (!text || text === '[object Object]') return 'Unknown formatting error';
  return text;
}

/**
 * Format Lua source. Empty input does not call the formatter.
 *
 * @param {string} source
 * @param {(source: string) => string | Promise<string>} formatFn
 * @returns {Promise<{ ok: true, empty: boolean, changed: boolean, text: string } | { ok: false, message: string }>}
 */
export async function formatLuaSource(source, formatFn) {
  if (typeof source !== 'string' || source.trim() === '') {
    return {
      ok: true,
      empty: true,
      changed: false,
      text: typeof source === 'string' ? source : '',
    };
  }
  if (typeof formatFn !== 'function') {
    return { ok: false, message: 'Formatter is unavailable' };
  }
  try {
    const text = await formatFn(source);
    if (typeof text !== 'string') {
      return { ok: false, message: 'Formatter returned no text' };
    }
    return { ok: true, empty: false, changed: text !== source, text };
  } catch (error) {
    return { ok: false, message: formatErrorMessage(error) };
  }
}

/**
 * Load the browser StyLua build and initialize its wasm once.
 * Tests pass their own loader into `formatWithStyLua`.
 *
 * @returns {Promise<{
 *   Config: { new(): object },
 *   formatCode: Function,
 *   OutputVerification: { Full: number },
 * }>}
 */
export async function loadStyLua() {
  const stylua = await import('@johnnymorganz/stylua/web');
  await stylua.default();
  return stylua;
}

/**
 * Format with StyLua defaults. `formatCode` consumes the Config.
 *
 * @param {string} source
 * @param {() => Promise<{
 *   Config: { new(): object },
 *   formatCode: Function,
 *   OutputVerification: { Full: number },
 * }>} [load]
 * @returns {Promise<string>}
 */
export async function formatWithStyLua(source, load = loadStyLua) {
  const stylua = await load();
  const config = stylua.Config.new();
  return stylua.formatCode(source, config, undefined, stylua.OutputVerification.Full);
}
