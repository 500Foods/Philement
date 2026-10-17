import { describe, it, expect, vi } from 'vitest';
import {
  SCRIPT_INSERT_QUERY_REF,
  SCRIPT_UPDATE_QUERY_REF,
  SCRIPT_DELETE_QUERY_REF,
  scriptInvokable,
  scriptUserId,
  scriptRowLabel,
  scriptDirtyPromptOptions,
  confirmDirtyRowSwitch,
  buildScriptSaveParams,
  resolveScriptSave,
  buildScriptDuplicateParams,
  buildScriptDeleteParams,
} from '../../../src/managers/scripting/scripting-save.js';

const existing = {
  group_name: 'Api',
  script_name: 'Echo',
  script_type: 2,
  schedule: '0 * * * *',
  next_run: '2026-10-17T00:00:00Z',
  last_run_start: '2026-10-16T00:00:00Z',
  last_run_end: '2026-10-16T00:00:01Z',
  status: 3,
  summary: 'Echoes its params',
  invokable: 1,
  code: 'return params',
};

describe('scriptInvokable', () => {
  it('sends 1 only for the number 1', () => {
    expect(scriptInvokable(1)).toBe(1);
    expect(scriptInvokable('1')).toBe(1);
  });

  it('sends 0 when the row has none', () => {
    expect(scriptInvokable(undefined)).toBe(0);
    expect(scriptInvokable(null)).toBe(0);
    expect(scriptInvokable(0)).toBe(0);
    expect(scriptInvokable('0')).toBe(0);
    expect(scriptInvokable(2)).toBe(0);
  });
});

describe('scriptUserId', () => {
  it('reads app.auth.user.id', () => {
    expect(scriptUserId({ auth: { user: { id: 42 } } })).toBe(42);
    expect(scriptUserId({ auth: { user: { id: '7' } } })).toBe(7);
  });

  it('uses 0 when the id is missing', () => {
    expect(scriptUserId(undefined)).toBe(0);
    expect(scriptUserId({ auth: {} })).toBe(0);
    expect(scriptUserId({ auth: { user: { id: null } } })).toBe(0);
  });
});

describe('resolveScriptSave', () => {
  it('updates a persisted row on QueryRef 130 with INTEGER INVOKABLE and USERID', () => {
    const save = resolveScriptSave({
      original: existing,
      current: { ...existing, summary: 'Edited' },
      code: 'return true',
      userId: 9,
      persisted: true,
    });

    expect(save.queryRef).toBe(SCRIPT_UPDATE_QUERY_REF);
    expect(save.isInsert).toBe(false);
    expect(save.params.INTEGER).toEqual({ USERID: 9, INVOKABLE: 1 });
    expect(save.params.STRING.GROUP_NAME).toBe('Api');
    expect(save.params.STRING.SCRIPT_NAME).toBe('Echo');
    expect(save.params.STRING.CODE).toBe('return true');
    expect(save.params.STRING.SUMMARY).toBe('Edited');
    expect(save.params.STRING.SCRIPT_TYPE).toBe(2);
    expect(save.params.STRING.STATUS).toBe(3);
    expect(save.params.INTEGER.SCRIPT_TYPE).toBeUndefined();
    expect(save.params.INTEGER.STATUS).toBeUndefined();
  });

  it('sends INVOKABLE 0 when the persisted row has none', () => {
    const save = resolveScriptSave({
      original: { ...existing, invokable: null },
      current: { ...existing, invokable: null },
      code: 'return 1',
      userId: 0,
      persisted: true,
    });

    expect(save.params.INTEGER.INVOKABLE).toBe(0);
    expect(save.params.INTEGER.USERID).toBe(0);
  });

  it('inserts a draft on QueryRef 129 even when the draft already has names', () => {
    const draft = {
      group_name: 'Staff',
      script_name: 'NewOne',
      invokable: 0,
    };
    const save = resolveScriptSave({
      original: draft,
      current: draft,
      code: '',
      userId: 4,
      persisted: false,
    });

    expect(save.queryRef).toBe(SCRIPT_INSERT_QUERY_REF);
    expect(save.isInsert).toBe(true);
    expect(save.rowId).toBe('Staff::NewOne');
    expect(save.params.INTEGER).toEqual({ USERID: 4, INVOKABLE: 0 });
    expect(save.params.STRING.GROUP_NAME).toBe('Staff');
    expect(save.params.STRING.SCRIPT_NAME).toBe('NewOne');
  });

  it('refuses a rename of a persisted row', () => {
    expect(() => resolveScriptSave({
      original: existing,
      current: { ...existing, script_name: 'Echo2' },
      code: 'return true',
      userId: 1,
      persisted: true,
    })).toThrow('Scripts are edited in place. Group and name cannot change.');
  });

  it('refuses a save with no group or name', () => {
    expect(() => resolveScriptSave({
      original: {},
      current: { group_name: 'Api', script_name: '   ' },
      persisted: false,
    })).toThrow('Enter a group and a script name before saving.');
  });
});

describe('buildScriptSaveParams', () => {
  it('keeps run timestamps on a normal save', () => {
    const params = buildScriptSaveParams({
      row: existing,
      groupName: 'Api',
      scriptName: 'Echo',
      code: 'return params',
      userId: 1,
    });

    expect(params.STRING.NEXT_RUN).toBe(existing.next_run);
    expect(params.STRING.LAST_RUN_START).toBe(existing.last_run_start);
    expect(params.STRING.SCHEDULE).toBe(existing.schedule);
  });
});

describe('buildScriptDuplicateParams', () => {
  it('inserts Group.Name (Copy) with INTEGER INVOKABLE and cleared runs', () => {
    const duplicate = buildScriptDuplicateParams(existing, 11, 'return params');

    expect(duplicate.queryRef).toBe(SCRIPT_INSERT_QUERY_REF);
    expect(duplicate.rowId).toBe('Api::Echo (Copy)');
    expect(duplicate.params.STRING.GROUP_NAME).toBe('Api');
    expect(duplicate.params.STRING.SCRIPT_NAME).toBe('Echo (Copy)');
    expect(duplicate.params.STRING.CODE).toBe('return params');
    expect(duplicate.params.STRING.NEXT_RUN).toBeNull();
    expect(duplicate.params.STRING.LAST_RUN_START).toBeNull();
    expect(duplicate.params.STRING.LAST_RUN_END).toBeNull();
    expect(duplicate.params.STRING.SCHEDULE).toBe(existing.schedule);
    expect(duplicate.params.INTEGER).toEqual({ USERID: 11, INVOKABLE: 1 });
  });

  it('copies INVOKABLE 0 when the source has none', () => {
    const duplicate = buildScriptDuplicateParams({ ...existing, invokable: undefined }, 1, '');
    expect(duplicate.params.INTEGER.INVOKABLE).toBe(0);
  });

  it('rejects a row with no name', () => {
    expect(() => buildScriptDuplicateParams({ group_name: 'Api' }, 1, '')).toThrow(
      'Group and name are required to duplicate a script.',
    );
  });
});

describe('buildScriptDeleteParams', () => {
  it('sends STRING group and name to QueryRef 131 and no INTEGER id', () => {
    const built = buildScriptDeleteParams(existing);

    expect(built.queryRef).toBe(SCRIPT_DELETE_QUERY_REF);
    expect(built.rowId).toBe('Api::Echo');
    expect(built.params).toEqual({
      STRING: { GROUP_NAME: 'Api', SCRIPT_NAME: 'Echo' },
    });
    expect(built.params.INTEGER).toBeUndefined();
  });

  it('rejects a row with no name', () => {
    expect(() => buildScriptDeleteParams({ script_name: 'Echo' })).toThrow(
      'Group and name are required to delete a script.',
    );
  });
});

describe('confirmDirtyRowSwitch', () => {
  it('names the script on the save-or-stay prompt', () => {
    expect(scriptRowLabel(existing)).toBe('Api.Echo');
    expect(scriptDirtyPromptOptions(existing)).toEqual({
      title: 'Unsaved changes',
      message: 'Save changes to Api.Echo?',
      confirmLabel: 'Save',
      cancelLabel: 'Stay',
    });
  });

  it('stays without saving when the prompt is declined', async () => {
    const save = vi.fn();
    const confirm = vi.fn().mockResolvedValue(false);

    await expect(confirmDirtyRowSwitch(existing, confirm, save)).resolves.toBe(false);
    expect(save).not.toHaveBeenCalled();
    expect(confirm).toHaveBeenCalledWith(scriptDirtyPromptOptions(existing));
  });

  it('saves when the prompt is accepted', async () => {
    const save = vi.fn().mockResolvedValue(true);
    const confirm = vi.fn().mockResolvedValue(true);

    await expect(confirmDirtyRowSwitch({ group_name: '', script_name: '' }, confirm, save)).resolves.toBe(true);
    expect(save).toHaveBeenCalledOnce();
    expect(confirm.mock.calls[0][0].message).toBe('Save changes to this script?');
  });
});
