/**
 * Scripting save, duplicate, and delete payloads.
 *
 * QueryRefs 129 and 130 require INTEGER INVOKABLE. QueryRef 131 deletes
 * by GROUP_NAME and SCRIPT_NAME. Rename of an existing row is refused.
 */

import { authQuery } from '../../shared/conduit.js';
import { confirmDialog, promptDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';

export const SCRIPT_DETAIL_QUERY_REF = 87;
export const SCRIPT_INSERT_QUERY_REF = 129;
export const SCRIPT_UPDATE_QUERY_REF = 130;
export const SCRIPT_DELETE_QUERY_REF = 131;

function storedText(value) {
  return value == null ? '' : String(value);
}

function hasText(value) {
  return storedText(value).trim() !== '';
}

function scriptError(error, message) {
  const failure = new Error(message);
  failure.serverError = { error, message };
  return failure;
}

/**
 * @param {unknown} value
 * @returns {0|1}
 */
export function scriptInvokable(value) {
  return Number(value) === 1 ? 1 : 0;
}

/**
 * @param {object|null|undefined} app
 * @returns {number}
 */
export function scriptUserId(app) {
  const id = Number(app?.auth?.user?.id);
  return Number.isInteger(id) ? id : 0;
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function scriptRowLabel(row) {
  const groupName = storedText(row?.group_name).trim();
  const scriptName = storedText(row?.script_name).trim();
  if (groupName && scriptName) return `${groupName}.${scriptName}`;
  return 'this script';
}

/**
 * @param {object|null|undefined} row
 * @returns {{ title: string, message: string, confirmLabel: string, cancelLabel: string }}
 */
export function scriptDirtyPromptOptions(row) {
  return {
    title: 'Unsaved changes',
    message: `Save changes to ${scriptRowLabel(row)}?`,
    confirmLabel: 'Save',
    cancelLabel: 'Stay',
  };
}

/**
 * Yes saves and lets the row switch proceed. No stays on the dirty row.
 *
 * @param {object|null|undefined} row
 * @param {(options: object) => Promise<boolean>} confirm
 * @param {() => Promise<boolean>} save
 * @returns {Promise<boolean>}
 */
export async function confirmDirtyRowSwitch(row, confirm, save) {
  const accepted = await confirm(scriptDirtyPromptOptions(row));
  if (!accepted) return false;
  return save();
}

/**
 * @param {object} args
 * @param {object} args.row
 * @param {string} args.groupName
 * @param {string} args.scriptName
 * @param {string} [args.code]
 * @param {number} [args.userId]
 * @param {boolean} [args.clearRuns]
 * @returns {{ STRING: object, INTEGER: { USERID: number, INVOKABLE: 0|1 } }}
 */
export function buildScriptSaveParams({
  row,
  groupName,
  scriptName,
  code = '',
  userId = 0,
  clearRuns = false,
}) {
  return {
    STRING: {
      GROUP_NAME: groupName,
      SCRIPT_NAME: scriptName,
      SCRIPT_TYPE: row?.script_type ?? 1,
      SCHEDULE: row?.schedule || null,
      NEXT_RUN: clearRuns ? null : (row?.next_run || null),
      LAST_RUN_START: clearRuns ? null : (row?.last_run_start || null),
      LAST_RUN_END: clearRuns ? null : (row?.last_run_end || null),
      STATUS: row?.status ?? 1,
      CODE: code ?? '',
      SUMMARY: row?.summary || '',
    },
    INTEGER: {
      USERID: Number.isInteger(userId) ? userId : 0,
      INVOKABLE: scriptInvokable(row?.invokable),
    },
  };
}

/**
 * A persisted identity updates in place. Anything else inserts.
 * A changed group or name on a persisted row throws.
 *
 * @param {object} args
 * @param {object|null|undefined} args.original
 * @param {object|null|undefined} args.current
 * @param {string} [args.code]
 * @param {number} [args.userId]
 * @param {boolean} [args.persisted]
 */
export function resolveScriptSave({
  original,
  current,
  code = '',
  userId = 0,
  persisted = false,
}) {
  const origGroup = storedText(original?.group_name);
  const origName = storedText(original?.script_name);
  const curGroup = storedText(current?.group_name);
  const curName = storedText(current?.script_name);
  const hadIdentity = hasText(origGroup) && hasText(origName);
  const update = persisted && hadIdentity;

  if (update && (curGroup !== origGroup || curName !== origName)) {
    throw scriptError(
      'Rename Not Allowed',
      'Scripts are edited in place. Group and name cannot change.',
    );
  }

  const groupName = update ? origGroup : curGroup;
  const scriptName = update ? origName : curName;
  if (!hasText(groupName) || !hasText(scriptName)) {
    throw scriptError(
      'Group and name are required',
      'Enter a group and a script name before saving.',
    );
  }

  return {
    queryRef: update ? SCRIPT_UPDATE_QUERY_REF : SCRIPT_INSERT_QUERY_REF,
    isInsert: !update,
    rowId: `${groupName}::${scriptName}`,
    params: buildScriptSaveParams({
      row: current,
      groupName,
      scriptName,
      code,
      userId,
    }),
  };
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function scriptRowId(row) {
  return `${storedText(row?.group_name)}::${storedText(row?.script_name)}`;
}

/**
 * @param {object|null|undefined} row
 * @param {number} userId
 * @param {string} code
 */
export function buildScriptDuplicateParams(row, userId, code) {
  const groupName = storedText(row?.group_name);
  const scriptName = storedText(row?.script_name);
  if (!hasText(groupName) || !hasText(scriptName)) {
    throw scriptError(
      'Group and name are required',
      'Group and name are required to duplicate a script.',
    );
  }

  const newScriptName = `${scriptName} (Copy)`;
  return {
    queryRef: SCRIPT_INSERT_QUERY_REF,
    rowId: `${groupName}::${newScriptName}`,
    params: buildScriptSaveParams({
      row,
      groupName,
      scriptName: newScriptName,
      code: code || '',
      userId,
      clearRuns: true,
    }),
  };
}

/**
 * @param {object|null|undefined} row
 */
export function buildScriptDeleteParams(row) {
  const groupName = storedText(row?.group_name);
  const scriptName = storedText(row?.script_name);
  if (!hasText(groupName) || !hasText(scriptName)) {
    throw scriptError(
      'Group and name are required',
      'Group and name are required to delete a script.',
    );
  }

  return {
    queryRef: SCRIPT_DELETE_QUERY_REF,
    rowId: `${groupName}::${scriptName}`,
    params: {
      STRING: {
        GROUP_NAME: groupName,
        SCRIPT_NAME: scriptName,
      },
    },
  };
}

function configuredRef(configured, fallback) {
  return configured ?? fallback;
}

async function codeForDuplicate(manager, rowData, groupName, scriptName) {
  if (typeof rowData?.code === 'string' && rowData.code !== '') return rowData.code;

  const rowId = `${groupName}::${scriptName}`;
  if (manager._loadedDetailRowId === rowId) {
    const editorCode = manager.luaEditor?.state?.doc?.toString();
    if (typeof editorCode === 'string') return editorCode;
    if (typeof manager.currentScript?.code === 'string') return manager.currentScript.code;
  }

  const detailQueryRef = configuredRef(
    manager.scriptTable?.queryRefs?.detailQueryRef,
    SCRIPT_DETAIL_QUERY_REF,
  );
  const details = await authQuery(manager.app.api, detailQueryRef, {
    STRING: { GROUP_NAME: groupName, SCRIPT_NAME: scriptName },
  });
  return details?.[0]?.code || '';
}

export const scriptingSaveMethods = {
  _rememberPersistedScripts(rows) {
    this._persistedScriptIds = new Set((rows || []).map(row => scriptRowId(row)));
  },

  _isPersistedScript(row) {
    return this._persistedScriptIds?.has(scriptRowId(row)) === true;
  },

  _installScriptCrud() {
    const table = this.scriptTable;
    table.handleAdd = () => this._executeAdd();
    table.handleDelete = () => this._executeDelete();
    const saveThenSwitch = table.autoSaveBeforeRowChange.bind(table);
    table.autoSaveBeforeRowChange = () => confirmDirtyRowSwitch(
      table.getEditingRow?.()?.getData?.(),
      confirmDialog,
      saveThenSwitch,
    );
  },

  async _executeAdd() {
    const table = this.scriptTable;
    if (!table?.table || table.readonly || table.alwaysEditable) return;

    if (table.isEditing && table.isActuallyDirty?.()) {
      const saved = await table.autoSaveBeforeRowChange();
      if (!saved) return;
      await table.exitEditMode('save');
    } else if (table.isEditing) {
      await table.exitEditMode('row-change');
    }

    const groupName = await promptDialog({
      title: 'New script',
      message: 'Group',
      confirmLabel: 'Next',
    });
    if (!hasText(groupName)) return;

    const scriptName = await promptDialog({
      title: 'New script',
      message: 'Script name',
      confirmLabel: 'Create',
    });
    if (!hasText(scriptName)) return;

    const draft = {
      group_name: storedText(groupName).trim(),
      script_name: storedText(scriptName).trim(),
      script_type: 1,
      status: 1,
      invokable: 0,
      summary: '',
      code: '',
    };
    if (this._isPersistedScript(draft)) {
      toast.error('Create Failed', {
        serverError: {
          error: 'Script already exists',
          message: `${scriptRowLabel(draft)} is already in the list.`,
        },
        subsystem: 'Conduit',
        duration: 8000,
      });
      return;
    }

    const newRow = await table.table.addRow(draft, true);
    if (!newRow) return;

    this.currentScript = draft;
    this._loadedDetailRowId = scriptRowId(draft);
    this._pendingLuaContent = '';
    await this.switchTab(this._getActiveTabId());

    const selected = table.table.getSelectedRows?.() || [];
    if (selected.length > 0) table.table.deselectRow(selected);
    newRow.select();
    requestAnimationFrame(() => {
      newRow.scrollTo?.('center', false);
    });
    await table.enterEditMode(newRow);
  },

  async _executeSave(row) {
    const rowData = row.getData();
    const luaContent = this.luaEditor?.state?.doc?.toString() || '';
    const original = this.scriptTable?.originalRowData;
    const save = resolveScriptSave({
      original,
      current: rowData,
      code: luaContent || rowData.code || '',
      userId: scriptUserId(this.app),
      persisted: this._isPersistedScript(original),
    });
    const queryRef = configuredRef(
      save.isInsert
        ? this.scriptTable?.queryRefs?.insertQueryRef
        : this.scriptTable?.queryRefs?.updateQueryRef,
      save.queryRef,
    );

    if (!queryRef) {
      throw scriptError(
        'Save Not Configured',
        'Save not available: no query reference configured.',
      );
    }

    await authQuery(this.app.api, queryRef, save.params);

    if (save.isInsert) {
      this._persistedScriptIds?.add(save.rowId);
      this._loadedDetailRowId = save.rowId;
      this.scriptTable.saveSelectedRowId(save.rowId);
    }
  },

  async _executeDuplicate(rowData) {
    const groupName = storedText(rowData?.group_name);
    const scriptName = storedText(rowData?.script_name);
    if (!hasText(groupName) || !hasText(scriptName)) {
      throw scriptError(
        'Group and name are required',
        'Group and name are required to duplicate a script.',
      );
    }

    const code = await codeForDuplicate(this, rowData, groupName, scriptName);
    const duplicate = buildScriptDuplicateParams(rowData, scriptUserId(this.app), code);
    const insertQueryRef = configuredRef(
      this.scriptTable?.queryRefs?.insertQueryRef,
      duplicate.queryRef,
    );

    await authQuery(this.app.api, insertQueryRef, duplicate.params);

    this._loadedDetailRowId = duplicate.rowId;
    this.scriptTable.saveSelectedRowId(duplicate.rowId);
    await this.scriptTable.loadData();
    this._persistedScriptIds?.add(duplicate.rowId);
    return null;
  },

  async _executeDelete() {
    const table = this.scriptTable;
    if (!table?.table || table.readonly || table.alwaysEditable) return;

    const selected = table.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to delete', duration: 3000 });
      return;
    }

    const row = selected[0];
    const rowData = row.getData();
    let built;
    try {
      built = buildScriptDeleteParams(rowData);
    } catch (error) {
      toast.error('Delete Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
      return;
    }

    const confirmed = await confirmDialog({
      title: 'Delete script',
      message: `Delete "${scriptRowLabel(rowData)}"?\n\nThis action cannot be undone.`,
      confirmLabel: 'Delete',
      danger: true,
    });
    if (!confirmed) return;

    if (this._isPersistedScript(rowData)) {
      const deleteRef = configuredRef(table.queryRefs?.deleteQueryRef, built.queryRef);
      try {
        await authQuery(this.app.api, deleteRef, built.params);
      } catch (error) {
        toast.error('Delete Failed', {
          serverError: error.serverError,
          subsystem: 'Conduit',
          duration: 8000,
        });
        return;
      }
    }

    this._persistedScriptIds?.delete(built.rowId);

    const allRows = table.table.getRows('active');
    const currentIndex = allRows.findIndex(item => item === row);
    const nextRow = allRows[currentIndex + 1] || allRows[currentIndex - 1] || null;

    await table.exitEditMode?.('cancel');
    row.delete();

    if (this._loadedDetailRowId === built.rowId) {
      this._handleRowDeselected();
    }

    if (nextRow) {
      nextRow.select();
      requestAnimationFrame(() => {
        nextRow?.scrollTo?.('center', false);
      });
    }

    toast.success('Row Deleted', { duration: 3000 });
  },
};
