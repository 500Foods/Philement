/**
 * Course Builder edit block and Intake move.
 *
 * The manage buttons insert, copy, edit, flag, annotate, and delete.
 * The receipt email is not sent from this screen. Intake leaves the
 * receipt mark alone. Suggested clears it so the next pass can send.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { authQuery } from '../../shared/conduit.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import {
  ANNOTATION_MAX,
  FLAG_QUERY_REF,
  stageMove,
  SUGGESTION_DELETE_QUERY_REF,
  SUGGESTION_DUPLICATE_QUERY_REF,
  SUGGESTION_INSERT_QUERY_REF,
  SUGGESTION_UPDATE_QUERY_REF,
  WORKING_TITLE_QUERY_REF,
  buildDuplicateParams,
  buildIdParams,
  buildInsertParams,
  buildUpdateParams,
  buildWorkingTitleParams,
  builderUserId,
  fillStageSelect,
  isFlagged,
  paintSubmission,
  readSubmissionEditor,
  receiptLabel,
  showSubmissionEditor,
  storedStage,
  submissionValues,
} from './course-suggestions.js';

/**
 * @param {CourseBuilderManager} manager
 */
export function installBuilderActions(manager) {
  const table = manager.suggestionTable;
  if (!table) return;
  table.handleAdd = () => beginAdd(manager);
  table.handleDuplicate = () => duplicateSuggestion(manager);
  table.handleEdit = () => beginEdit(manager);
  table.handleDelete = () => deleteSuggestion(manager);
  table.handleFlag = () => toggleFlag(manager);
  table.handleAnnotate = () => focusAnnotation(manager);
}

/**
 * @param {CourseBuilderManager} manager
 */
export function paintBuilderChrome(manager) {
  const active = manager._mode === 'add' || manager._selectedId != null;
  const stage = manager._mode === 'add' ? 'suggested' : storedStage(manager._row);
  fillStageSelect(manager.elements.stage, stage, active && manager._mode === 'view');
  const receipt = manager.elements.receipt;
  if (receipt) {
    receipt.textContent = receiptLabel(
      manager._mode === 'add' ? '' : receiptValue(manager._row),
      active,
    );
  }
  const flag = manager.elements.flag;
  if (flag) {
    flag.textContent = active && manager._mode !== 'add' && isFlagged(manager._row) ? 'Flagged' : '';
  }
  const note = manager.elements.annotation;
  if (!note) return;
  const stored = manager._mode === 'add' ? '' : annotationValue(manager._row);
  if (document.activeElement !== note) note.value = stored;
  note.disabled = !active || manager._mode === 'view';
  manager._loadedAnnotation = stored;
}

/**
 * @param {CourseBuilderManager} manager
 */
export async function beginAdd(manager) {
  if (!(await confirmLeave(manager))) return;
  manager._mode = 'add';
  manager._selectedId = null;
  manager._loadedTitle = '';
  manager.elements.title.value = '';
  manager.elements.title.disabled = true;
  showSubmissionEditor(manager.elements.form, emptySubmission());
  paintBuilderChrome(manager);
  const note = manager.elements.annotation;
  if (note) {
    note.value = '';
    note.disabled = false;
  }
  manager._syncDirty();
  manager.elements.form.querySelector('[data-edit="course_name"]')?.focus();
}

/**
 * @param {CourseBuilderManager} manager
 */
export function beginEdit(manager) {
  if (manager._mode === 'add') return;
  if (manager._selectedId == null) {
    toast.info('No Row Selected', { description: 'Select a suggestion to edit', duration: 3000 });
    return;
  }
  manager._mode = 'edit';
  showSubmissionEditor(manager.elements.form, submissionValues(manager._row));
  paintBuilderChrome(manager);
  manager._syncDirty();
  manager.elements.form.querySelector('[data-edit="course_name"]')?.focus();
}

/**
 * @param {CourseBuilderManager} manager
 */
export function focusAnnotation(manager) {
  if (manager._selectedId == null && manager._mode !== 'add') {
    toast.info('No Row Selected', { description: 'Select a suggestion to annotate', duration: 3000 });
    return;
  }
  const note = manager.elements.annotation;
  if (!note) return;
  note.disabled = false;
  note.focus();
}

/**
 * @param {CourseBuilderManager} manager
 */
export async function duplicateSuggestion(manager) {
  if (manager._mode === 'add' || manager._selectedId == null) {
    toast.info('No Row Selected', { description: 'Select a suggestion to duplicate', duration: 3000 });
    return;
  }
  if (!(await confirmLeave(manager))) return;
  const built = buildDuplicateParams(
    manager._selectedId,
    submissionValues(manager._row).course_name,
    builderUserId(manager.app),
  );
  if (!built.ok) {
    toast.error('Suggestion was not copied', { description: built.message });
    return;
  }
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Suggestion was not copied', { description: 'No server connection.' });
    return;
  }
  try {
    await authQuery(api, SUGGESTION_DUPLICATE_QUERY_REF, built.params);
    toast.success('Suggestion copied');
    await manager._reload(null);
    selectNamed(manager, built.courseName);
  } catch (error) {
    toast.error('Suggestion was not copied', {
      description: error.message,
      serverError: error.serverError,
    });
  }
}

/**
 * @param {CourseBuilderManager} manager
 */
export async function deleteSuggestion(manager) {
  if (manager._mode === 'add' || manager._selectedId == null) {
    toast.info('No Row Selected', { description: 'Select a suggestion to delete', duration: 3000 });
    return;
  }
  const title = manager.elements.title?.value || 'this suggestion';
  const confirmed = await confirmDialog({
    title: 'Delete suggestion',
    message: `Delete "${title}"? This cannot be undone.`,
    confirmLabel: 'Delete',
    danger: true,
  });
  if (!confirmed) return;
  const built = buildIdParams(manager._selectedId, builderUserId(manager.app));
  if (!built.ok) return;
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Suggestion was not deleted', { description: 'No server connection.' });
    return;
  }
  try {
    await authQuery(api, SUGGESTION_DELETE_QUERY_REF, built.params);
    toast.success('Suggestion deleted');
    manager._row = null;
    manager._selectedId = null;
    await manager._reload(null);
    manager._applyRow(null);
  } catch (error) {
    toast.error('Suggestion was not deleted', {
      description: error.message,
      serverError: error.serverError,
    });
  }
}

/**
 * @param {CourseBuilderManager} manager
 */
export async function toggleFlag(manager) {
  if (manager._mode === 'add' || manager._selectedId == null) {
    toast.info('No Row Selected', { description: 'Select a suggestion to flag', duration: 3000 });
    return;
  }
  const built = buildIdParams(manager._selectedId, builderUserId(manager.app));
  if (!built.ok) return;
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Flag was not saved', { description: 'No server connection.' });
    return;
  }
  const id = manager._selectedId;
  try {
    await authQuery(api, FLAG_QUERY_REF, built.params);
    await manager._reload(id);
  } catch (error) {
    toast.error('Flag was not saved', {
      description: error.message,
      serverError: error.serverError,
    });
  }
}

/**
 * @param {CourseBuilderManager} manager
 */
export async function saveBuilder(manager) {
  if (manager._busy) return;
  if (manager._mode === 'add') {
    await saveInsert(manager);
    return;
  }
  if (manager._mode === 'edit' || annotationDirty(manager)) {
    await saveUpdate(manager);
    return;
  }
  await saveTitle(manager);
}

/**
 * @param {CourseBuilderManager} manager
 */
export function cancelBuilder(manager) {
  if (manager._mode === 'add' || manager._mode === 'edit') {
    manager._applyRow(manager._row);
    return;
  }
  if (manager.elements.title) manager.elements.title.value = manager._loadedTitle;
  if (manager.elements.annotation) manager.elements.annotation.value = manager._loadedAnnotation || '';
  manager._syncDirty();
}

/**
 * @param {CourseBuilderManager} manager
 */
export async function onStageChange(manager) {
  const select = manager.elements.stage;
  if (!select || manager._mode !== 'view' || manager._selectedId == null) return;
  const previous = select.dataset.stage || 'suggested';
  const move = stageMove(previous, select.value);
  if (!move) {
    select.value = previous;
    return;
  }
  const confirmed = await confirmDialog({
    title: move.title,
    message: move.message,
    confirmLabel: move.confirmLabel,
  });
  if (!confirmed) {
    select.value = previous;
    return;
  }
  const built = buildIdParams(manager._selectedId, builderUserId(manager.app));
  if (!built.ok) {
    select.value = previous;
    return;
  }
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Stage was not changed', { description: 'No server connection.' });
    select.value = previous;
    return;
  }
  const id = manager._selectedId;
  try {
    await authQuery(api, move.queryRef, built.params);
    toast.success(move.success);
    await manager._reload(id);
  } catch (error) {
    select.value = previous;
    toast.error('Stage was not changed', {
      description: error.message,
      serverError: error.serverError,
    });
  }
}

/**
 * @param {CourseBuilderManager} manager
 * @returns {boolean}
 */
export function builderDirty(manager) {
  if (manager._mode === 'add') {
    const values = readSubmissionEditor(manager.elements.form);
    return String(values.course_name || '').trim() !== '' || String(values.summary || '').trim() !== '';
  }
  const title = String(manager.elements.title?.value || '').trim();
  const titleDirty = manager._selectedId != null && title !== manager._loadedTitle;
  return titleDirty || annotationDirty(manager) || editorDirty(manager);
}

/**
 * @param {CourseBuilderManager} manager
 * @returns {boolean}
 */
function annotationDirty(manager) {
  if (manager._selectedId == null || !manager.elements.annotation) return false;
  if (manager.elements.annotation.disabled && manager._mode === 'view') return false;
  const current = String(manager.elements.annotation.value || '').slice(0, ANNOTATION_MAX);
  return current !== String(manager._loadedAnnotation || '');
}

/**
 * @param {CourseBuilderManager} manager
 * @returns {boolean}
 */
function editorDirty(manager) {
  if (manager._mode !== 'edit') return false;
  const current = readSubmissionEditor(manager.elements.form);
  const loaded = submissionValues(manager._row);
  return Object.keys(loaded).some((key) => current[key] !== loaded[key]);
}

/**
 * @param {CourseBuilderManager} manager
 * @returns {Promise<boolean>}
 */
async function confirmLeave(manager) {
  if (!builderDirty(manager)) return true;
  return confirmDialog({
    title: 'Unsaved changes',
    message: 'Discard the unsaved suggestion changes?',
    confirmLabel: 'Discard',
    cancelLabel: 'Stay',
  });
}

/**
 * @param {CourseBuilderManager} manager
 */
async function saveTitle(manager) {
  const built = buildWorkingTitleParams(
    manager._selectedId,
    manager.elements.title.value,
    builderUserId(manager.app),
  );
  if (!built.ok) {
    toast.error('Working title was not saved', { description: built.message });
    return;
  }
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Working title was not saved', { description: 'No server connection.' });
    return;
  }
  const courseName = submissionValues(manager._row).course_name;
  manager._busy = true;
  manager._syncDirty();
  try {
    await authQuery(api, WORKING_TITLE_QUERY_REF, built.params);
    manager._loadedTitle = built.title;
    manager.elements.title.value = built.title;
    toast.success('Working title saved');
    await manager._reload(manager._selectedId);
    const shown = submissionValues(manager._row).course_name;
    if (shown !== courseName) {
      log(Subsystems.MANAGER, Status.WARN, 'Course Builder save changed the submitted course name');
    }
  } catch (error) {
    toast.error('Working title was not saved', {
      description: error.message,
      serverError: error.serverError,
    });
  } finally {
    manager._busy = false;
    manager._syncDirty();
  }
}

/**
 * @param {CourseBuilderManager} manager
 */
async function saveInsert(manager) {
  const built = buildInsertParams(
    readSubmissionEditor(manager.elements.form),
    builderUserId(manager.app),
  );
  if (!built.ok) {
    toast.error('Suggestion was not saved', { description: built.message });
    return;
  }
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Suggestion was not saved', { description: 'No server connection.' });
    return;
  }
  manager._busy = true;
  manager._syncDirty();
  try {
    await authQuery(api, SUGGESTION_INSERT_QUERY_REF, built.params);
    toast.success('Suggestion saved');
    manager._mode = 'view';
    await manager._reload(null);
    selectNamed(manager, built.courseName);
  } catch (error) {
    toast.error('Suggestion was not saved', {
      description: error.message,
      serverError: error.serverError,
    });
  } finally {
    manager._busy = false;
    manager._syncDirty();
  }
}

/**
 * @param {CourseBuilderManager} manager
 */
async function saveUpdate(manager) {
  const values = manager._mode === 'edit'
    ? readSubmissionEditor(manager.elements.form)
    : submissionValues(manager._row);
  const built = buildUpdateParams(
    manager._selectedId,
    values,
    manager.elements.title.value,
    manager.elements.annotation?.value,
    builderUserId(manager.app),
  );
  if (!built.ok) {
    toast.error('Suggestion was not saved', { description: built.message });
    return;
  }
  const api = manager.app && manager.app.api;
  if (!api) {
    toast.error('Suggestion was not saved', { description: 'No server connection.' });
    return;
  }
  const id = manager._selectedId;
  manager._busy = true;
  manager._syncDirty();
  try {
    await authQuery(api, SUGGESTION_UPDATE_QUERY_REF, built.params);
    toast.success('Suggestion saved');
    manager._mode = 'view';
    await manager._reload(id);
  } catch (error) {
    toast.error('Suggestion was not saved', {
      description: error.message,
      serverError: error.serverError,
    });
  } finally {
    manager._busy = false;
    manager._syncDirty();
  }
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
function receiptValue(row) {
  if (!row) return '';
  const value = row.receipt_mail ?? row.RECEIPT_MAIL;
  return value == null ? '' : String(value);
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
function annotationValue(row) {
  if (!row) return '';
  const value = row.annotation ?? row.ANNOTATION;
  return value == null ? '' : String(value);
}

/**
 * @returns {Object<string, string>}
 */
function emptySubmission() {
  return {
    course_name: '',
    summary: '',
    detail: '',
    intended_audience: '',
    additional_features: '',
    submitter_name: '',
    submitter_email: '',
  };
}

/**
 * @param {CourseBuilderManager} manager
 * @param {string} courseName
 */
function selectNamed(manager, courseName) {
  const table = manager.suggestionTable?.table;
  if (!table || typeof table.getRows !== 'function') return;
  let best = 0;
  table.getRows().forEach((row) => {
    const data = row.getData?.() || {};
    if (String(data.course_name || '') !== courseName) return;
    const id = Number(data.suggestion_id) || 0;
    if (id >= best) best = id;
  });
  if (best > 0) manager._reselect(best);
}

/**
 * Restore the read-only submission fields after a mode change.
 * paintSubmission is used by the manager. This import keeps the
 * editor swap next to the save path.
 * @param {CourseBuilderManager} manager
 */
export function restoreSubmission(manager) {
  paintSubmission(manager.elements.form, manager._mode === 'add' ? null : manager._row, manager._sanitize);
}
