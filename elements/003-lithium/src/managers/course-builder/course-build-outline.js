/**
 * Course Builder Run tab — outline, materials, versions (CATCHUP Phase 22).
 *
 * Artifact bodies are untrusted (LLM output). Everything here builds
 * DOM with textContent only; no innerHTML, no HTML strings. An outline
 * body is parsed as JSON and rendered as a nested list of keys and
 * text values. A body that is not JSON renders as plain text.
 * "Open LMS" uses the configured Canvas base URL and the run's
 * canvas_course_id. No token ever reaches the browser.
 */

import { canvasCourseUrl } from '../course-manager/course-catalog.js';

export const OUTLINE_KIND = 'outline';
export const MATERIAL_KIND = 'material';
export const OUTLINE_MAX_DEPTH = 6;

/** Latest artifact (highest artifact_id) of one kind, or null. */
export function latestOfKind(artifacts, kind) {
  const list = versionsOfKind(artifacts, kind);
  return list.length ? list[0] : null;
}

/** All artifacts of one kind, newest (highest artifact_id) first. */
export function versionsOfKind(artifacts, kind) {
  if (!Array.isArray(artifacts)) return [];
  return artifacts
    .filter((a) => a && a.kind === kind)
    .sort((a, b) => Number(b.artifact_id) - Number(a.artifact_id));
}

/**
 * Materials: latest version per module_index, ordered by module_index.
 * @returns {Array<object & {versions:number}>}
 */
export function latestMaterials(artifacts) {
  const byIndex = new Map();
  for (const art of versionsOfKind(artifacts, MATERIAL_KIND)) {
    const key = art.module_index ?? '';
    const seen = byIndex.get(key);
    if (seen) seen.versions += 1;
    else byIndex.set(key, { ...art, versions: 1 });
  }
  return [...byIndex.values()].sort((a, b) => Number(a.module_index ?? 0) - Number(b.module_index ?? 0));
}

/** Parse an outline body. { ok, value } or { ok:false, text }. */
export function parseOutline(body) {
  if (body && typeof body === 'object') return { ok: true, value: body };
  const text = body === null || body === undefined ? '' : String(body);
  try {
    return { ok: true, value: JSON.parse(text) };
  } catch {
    return { ok: false, text };
  }
}

function node(tag, className, text) {
  const el = document.createElement(tag);
  if (className) el.className = className;
  if (text !== undefined && text !== null) el.textContent = String(text);
  return el;
}

function renderValue(value, depth) {
  if (value === null || typeof value !== 'object') {
    return node('span', 'builder-outline-text', value === null ? '' : value);
  }
  if (depth >= OUTLINE_MAX_DEPTH) return node('span', 'builder-outline-text', '…');
  const list = node('ul', 'builder-outline-list');
  const entries = Array.isArray(value) ? value.map((v, i) => [i + 1, v]) : Object.entries(value);
  for (const [key, child] of entries) {
    const item = node('li', 'builder-outline-item');
    item.append(node('strong', 'builder-outline-key', Array.isArray(value) ? `${key}.` : key), ' ');
    item.append(renderValue(child, depth + 1));
    list.append(item);
  }
  return list;
}

/**
 * Sanitized outline element. Text nodes only; markup in the body is
 * shown, never parsed.
 */
export function renderOutline(body) {
  const wrap = node('div', 'builder-outline');
  const parsed = parseOutline(body);
  if (!parsed.ok) {
    wrap.append(node('pre', 'builder-outline-raw', parsed.text));
    return wrap;
  }
  wrap.append(renderValue(parsed.value, 0));
  return wrap;
}

/** Teacher link for the run's Canvas course, or '' when none. */
export function lmsUrlFor(run) {
  return canvasCourseUrl(run && run.canvas_course_id);
}

/**
 * Paints outline, materials, and versions into host.
 * @param {HTMLElement} host
 * @param {object} run
 * @param {Array<object>} artifacts
 */
export function paintOutlineSection(host, run, artifacts) {
  const url = lmsUrlFor(run);
  if (url) {
    const link = node('a', 'btn btn-sm btn-secondary builder-run-lms', 'Open LMS');
    link.href = url;
    link.target = '_blank';
    link.rel = 'noopener noreferrer';
    host.append(link);
  }

  const outline = latestOfKind(artifacts, OUTLINE_KIND);
  host.append(node('h4', 'builder-run-sub', 'Outline'));
  if (outline) host.append(renderOutline(outline.body));
  else host.append(node('p', 'builder-run-empty', 'No outline yet.'));

  const materials = latestMaterials(artifacts);
  host.append(node('h4', 'builder-run-sub', `Materials (${materials.length})`));
  const matList = node('ul', 'builder-run-list');
  for (const mat of materials) {
    matList.append(node('li', 'builder-run-item',
      `Material ${mat.module_index ?? '—'} — artifact ${mat.artifact_id}`
      + `${mat.versions > 1 ? ` (${mat.versions} versions)` : ''}`));
  }
  host.append(matList);

  const versions = versionsOfKind(artifacts, OUTLINE_KIND);
  host.append(node('h4', 'builder-run-sub', `Outline versions (${versions.length})`));
  const verList = node('ul', 'builder-run-list');
  versions.forEach((v, i) => {
    verList.append(node('li', 'builder-run-item',
      `v${versions.length - i} — artifact ${v.artifact_id} — ${v.created_at || ''}`
      + `${v.model_id ? ` — ${v.model_id}` : ''}`));
  });
  host.append(verList);
}
