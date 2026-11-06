/**
 * Paint one PDF page or thumbnail.
 * The canvas bitmap is the viewport times the device pixel ratio.
 * The text layer stays in CSS pixels and uses the same viewport.
 * Flips are CSS on the sheet, so this module does not mirror the paint.
 * The visible canvas keeps its current bitmap until the new one is
 * finished. pdf.js would otherwise clear it to white first.
 */

/** No white matte. A PDF that paints its own paper still does. */
const CANVAS_CLEAR = 'rgba(0,0,0,0)';

import {
  getDocument,
  GlobalWorkerOptions,
  PasswordResponses,
  TextLayer,
} from 'pdfjs-dist';
import { marqueeSource, PDF_WORKER_URL } from './pdf-view.js';

GlobalWorkerOptions.workerSrc = PDF_WORKER_URL;

export { getDocument, PasswordResponses };

/**
 * @param {unknown} error
 * @returns {boolean}
 */
export function renderCancelled(error) {
  const name = error && typeof error === 'object' ? error.name : '';
  return name === 'RenderingCancelledException' || name === 'AbortException';
}

/**
 * @param {{ renderTask?: { cancel: Function, promise?: Promise<unknown> }|null, textLayer?: { cancel: Function }|null, textEl?: HTMLElement|null, ready?: boolean, key?: string }} shell
 */
export function releasePaint(shell) {
  const task = shell.renderTask;
  shell.renderTask = null;
  if (task) {
    task.cancel();
    task.promise?.catch(() => {});
  }
  if (shell.textLayer) {
    shell.textLayer.cancel();
    shell.textLayer = null;
  }
  if (shell.textEl) shell.textEl.replaceChildren();
  shell.ready = false;
  shell.key = '';
}

/**
 * @param {object} shell
 * @param {object} viewport
 * @param {() => boolean} alive
 * @returns {Promise<boolean>}
 */
/**
 * Render off screen. The caller copies the bitmap across when it finishes.
 * @param {object} proxy
 * @param {object} viewport
 * @returns {{ canvas: HTMLCanvasElement, task: { promise: Promise<void>, cancel: Function } }}
 */
function openBitmap(proxy, viewport) {
  const ratio = window.devicePixelRatio || 1;
  const canvas = document.createElement('canvas');
  canvas.width = Math.max(1, Math.floor(viewport.width * ratio));
  canvas.height = Math.max(1, Math.floor(viewport.height * ratio));
  const task = proxy.render({
    canvas,
    viewport,
    background: CANVAS_CLEAR,
    transform: ratio === 1 ? undefined : [ratio, 0, 0, ratio, 0, 0],
  });
  return { canvas, task };
}

/**
 * Replace the visible bitmap in one turn, so the canvas is not cleared
 * and left blank while the browser paints.
 * @param {HTMLCanvasElement} target
 * @param {HTMLCanvasElement} source
 */
function commitBitmap(target, source) {
  target.width = source.width;
  target.height = source.height;
  target.getContext('2d')?.drawImage(source, 0, 0);
}

export async function paintPage(shell, viewport, alive) {
  releasePaint(shell);
  const painted = openBitmap(shell.proxy, viewport);
  shell.renderTask = painted.task;
  try {
    await painted.task.promise;
  } catch (error) {
    shell.renderTask = null;
    if (renderCancelled(error) || !alive()) return false;
    throw error;
  }
  shell.renderTask = null;
  if (!alive()) return false;
  commitBitmap(shell.canvas, painted.canvas);
  if (!shell.textContent) shell.textContent = await shell.proxy.getTextContent();
  if (!alive()) return false;
  const items = (shell.textContent.items || []).filter((item) => typeof item.str === 'string');
  shell.textItems = items;
  shell.textEl.replaceChildren();
  const layer = new TextLayer({
    textContentSource: { items, styles: shell.textContent.styles || {} },
    container: shell.textEl,
    viewport,
  });
  shell.textLayer = layer;
  try {
    await layer.render();
  } catch (error) {
    if (renderCancelled(error) || !alive()) return false;
    throw error;
  }
  if (!alive()) return false;
  shell.ready = true;
  return true;
}

/**
 * @param {object} thumb
 * @param {object} viewport
 * @param {() => boolean} alive
 * @returns {Promise<boolean>}
 */
export async function paintThumb(thumb, viewport, alive) {
  const task = thumb.renderTask;
  thumb.renderTask = null;
  if (task) {
    task.cancel();
    task.promise?.catch(() => {});
  }
  const painted = openBitmap(thumb.proxy, viewport);
  thumb.renderTask = painted.task;
  try {
    await painted.task.promise;
  } catch (error) {
    thumb.renderTask = null;
    if (renderCancelled(error) || !alive()) return false;
    throw error;
  }
  thumb.renderTask = null;
  if (!alive()) return false;
  commitBitmap(thumb.canvas, painted.canvas);
  thumb.ready = true;
  return true;
}

/**
 * PNG of a bitmap region, flipped the way the sheet is flipped.
 * @param {HTMLCanvasElement} canvas
 * @param {{ x: number, y: number, w: number, h: number }} local CSS pixels
 * @param {number} cssW
 * @param {number} cssH
 * @param {boolean} flipH
 * @param {boolean} flipV
 * @returns {Promise<Blob|null>}
 */
export function marqueeBlob(canvas, local, cssW, cssH, flipH, flipV) {
  if (!canvas || !(canvas.width > 0) || !(local.w > 0) || !(local.h > 0)) {
    return Promise.resolve(null);
  }
  const src = marqueeSource(local.x, local.y, local.w, local.h, canvas.width, canvas.height, cssW, cssH);
  const out = document.createElement('canvas');
  out.width = src.w;
  out.height = src.h;
  const ctx = out.getContext('2d');
  if (!ctx || typeof out.toBlob !== 'function') return Promise.resolve(null);
  ctx.translate(flipH ? src.w : 0, flipV ? src.h : 0);
  ctx.scale(flipH ? -1 : 1, flipV ? -1 : 1);
  ctx.drawImage(canvas, src.x, src.y, src.w, src.h, 0, 0, src.w, src.h);
  return new Promise((resolve) => {
    out.toBlob((blob) => resolve(blob), 'image/png');
  });
}
