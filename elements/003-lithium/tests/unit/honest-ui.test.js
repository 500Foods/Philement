/**
 * Phase 4 honest UI: dialogs, stub saves, and SunEditor bearer headers.
 */

import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import { confirmDialog, promptDialog } from '../../src/shared/confirm.js';
import { uploadAuthHeader, SunEditorInit } from '../../src/init/suneditor-init.js';
import { BaseSettingsPage, SimpleSettingsPage } from '../../src/managers/profile-manager/pages/settings-page-base.js';
import EmailPage from '../../src/managers/profile-manager/pages/email/page-email.js';
import LanguagePage from '../../src/managers/profile-manager/pages/language/page-language.js';
import DateFormatsPage from '../../src/managers/profile-manager/pages/date-formats/page-date-formats.js';
import PhotoPage from '../../src/managers/profile-manager/pages/photo/page-photo.js';
import LookupsManagerPage from '../../src/managers/profile-manager/pages/manager-23/page-manager-23.js';
import { SettingsPageRegistry } from '../../src/managers/profile-manager/pages/page-registry.js';
import StyleManager from '../../src/managers/style-manager/style-manager.js';

describe('uploadAuthHeader', () => {
  it('omits Authorization when the token is missing', () => {
    expect(uploadAuthHeader(null)).toEqual({});
    expect(uploadAuthHeader(undefined)).toEqual({});
    expect(uploadAuthHeader('')).toEqual({});
    expect(uploadAuthHeader(0)).toEqual({});
  });

  it('sends a real bearer when the token is a non-empty string', () => {
    expect(uploadAuthHeader('abc.def')).toEqual({ Authorization: 'Bearer abc.def' });
  });
});

describe('SunEditorInit headers', () => {
  afterEach(() => {
    localStorage.removeItem('lithium_jwt');
    delete globalThis.SUNEDITOR;
  });

  it('does not bake Bearer null when no JWT is stored', () => {
    localStorage.removeItem('lithium_jwt');
    const editor = new SunEditorInit();
    expect(editor.defaultOptions.imageUploadHeader).toEqual({});
    expect(editor.defaultOptions.videoUploadHeader).toEqual({});
    expect(JSON.stringify(editor.defaultOptions)).not.toContain('Bearer null');
  });

  it('reads the JWT at editor creation', () => {
    localStorage.removeItem('lithium_jwt');
    const editor = new SunEditorInit();
    localStorage.setItem('lithium_jwt', 'later-token');

    const host = document.createElement('div');
    host.id = 'sun-honest-ui';
    document.body.append(host);

    let seen = null;
    globalThis.SUNEDITOR = {
      create: (_element, options) => {
        seen = options;
        return { setContents() {} };
      },
    };

    editor.initEditor('sun-honest-ui', '');
    expect(seen.imageUploadHeader).toEqual({ Authorization: 'Bearer later-token' });
    expect(seen.videoUploadHeader).toEqual({ Authorization: 'Bearer later-token' });
    expect(JSON.stringify(seen)).not.toContain('Bearer null');
    host.remove();
  });
});

describe('confirm dialogs', () => {
  afterEach(() => {
    document.querySelectorAll('.lithium-confirm-backdrop').forEach((node) => node.remove());
  });

  it('confirms with a danger button and keeps the message as text', async () => {
    const pending = confirmDialog({
      title: 'Delete row',
      message: '<img src=x onerror=alert(1)>',
      confirmLabel: 'Delete',
      danger: true,
    });
    const dialog = document.querySelector('.lithium-confirm');
    expect(dialog.querySelector('img')).toBeNull();
    expect(dialog.querySelector('.lithium-confirm-message').textContent).toContain('<img');
    expect(dialog.querySelector('.btn-danger')).not.toBeNull();
    dialog.querySelector('.btn-danger').click();
    await expect(pending).resolves.toBe(true);
    expect(document.querySelector('.lithium-confirm')).toBeNull();
  });

  it('cancels on Escape and on the cancel button', async () => {
    const escaped = confirmDialog({ title: 'Reload', message: 'Reload the default?' });
    document.dispatchEvent(new window.KeyboardEvent('keydown', { key: 'Escape' }));
    await expect(escaped).resolves.toBe(false);

    const cancelled = confirmDialog({ title: 'Reload', message: 'Reload the default?' });
    expect(document.querySelector('.btn-danger')).toBeNull();
    document.querySelector('.btn-secondary').click();
    await expect(cancelled).resolves.toBe(false);
  });

  it('returns the prompt text, or null when cancelled', async () => {
    const pending = promptDialog({
      title: 'Save template',
      message: 'Enter template name',
      defaultValue: 'draft',
    });
    const input = document.querySelector('.lithium-confirm-input');
    expect(input.value).toBe('draft');
    input.value = '  Named  ';
    document.querySelector('.btn-primary').click();
    await expect(pending).resolves.toBe('  Named  ');

    const cancelled = promptDialog({ message: 'Enter template name' });
    document.querySelector('.btn-secondary').click();
    await expect(cancelled).resolves.toBeNull();
  });
});

describe('profile stub saves', () => {
  beforeEach(() => {
    document.body.innerHTML = '';
  });

  it('refuses a base save, stays dirty, and disables Save', async () => {
    const page = new BaseSettingsPage({ index: -1 });
    const container = document.createElement('div');
    container.innerHTML = '<button id="page-save">Save</button><button type="button">Cancel</button>';
    document.body.append(container);
    await page.init(container, {});
    expect(page.canSave()).toBe(false);
    expect(container.querySelector('#page-save').disabled).toBe(true);
    expect(container.querySelector('button[type="button"]').disabled).toBe(false);
    page.setDirty(true);
    const result = await page.save();
    expect(result.success).toBe(false);
    expect(page.isDirty()).toBe(true);
  });

  it('inherits the refusal on simple and email pages', async () => {
    const simple = new SimpleSettingsPage({ index: -2 });
    expect(simple.canSave()).toBe(false);
    simple.setDirty(true);
    const simpleResult = await simple.save();
    expect(simpleResult.success).toBe(false);
    expect(simple.isDirty()).toBe(true);

    const email = new EmailPage();
    expect(email.canSave()).toBe(false);
    email.setDirty(true);
    const emailResult = await email.save();
    expect(emailResult.success).toBe(false);
    expect(email.isDirty()).toBe(true);
  });

  it('keeps canSave on pages that already persist', () => {
    expect(new LanguagePage().canSave()).toBe(true);
    expect(new DateFormatsPage().canSave()).toBe(true);
    expect(new PhotoPage().canSave()).toBe(true);
    expect(new LookupsManagerPage().canSave()).toBe(true);
  });

  it('records a stub failure from saveAll and does not clear dirty', async () => {
    const registry = new SettingsPageRegistry();
    const stub = new BaseSettingsPage({ index: -4 });
    stub.setDirty(true);
    registry._handlers.set(-4, stub);
    const results = await registry.saveAll();
    expect(results).toEqual([
      expect.objectContaining({ index: -4, success: false }),
    ]);
    expect(stub.isDirty()).toBe(true);
  });
});

describe('style preview actions', () => {
  it('disables empty Apply and Copy, and writes editor text into the preview style', () => {
    const manager = new StyleManager(null, document.createElement('div'));
    const apply = document.createElement('button');
    const copy = document.createElement('button');
    const undo = document.createElement('button');
    const redo = document.createElement('button');
    manager.elements = { applyBtn: apply, copyCssBtn: copy, undoBtn: undo, redoBtn: redo };
    manager.updateUndoRedoButtons();
    manager.refreshStyleActionButtons();
    expect(undo.disabled).toBe(true);
    expect(redo.disabled).toBe(true);
    expect(apply.disabled).toBe(true);
    expect(copy.disabled).toBe(true);

    const preview = document.createElement('style');
    preview.id = 'style-manager-preview';
    manager.previewStyleEl = preview;
    manager.cssEditor = { state: { doc: { toString: () => '   ' } } };
    manager.applyStyleChanges();
    expect(preview.textContent).toBe('');

    manager.cssEditor = { state: { doc: { toString: () => '.a { color: red; }' } } };
    manager.applyStyleChanges();
    expect(preview.textContent).toBe('.a { color: red; }');
    manager.refreshStyleActionButtons();
    expect(apply.disabled).toBe(false);
    expect(copy.disabled).toBe(false);
  });

  it('copies the editor text and skips an empty editor', async () => {
    const manager = new StyleManager(null, document.createElement('div'));
    const writeText = vi.fn().mockResolvedValue(undefined);
    vi.spyOn(navigator.clipboard, 'writeText').mockImplementation(writeText);
    manager.cssEditor = { state: { doc: { toString: () => '' } } };
    manager.copyCssToClipboard();
    expect(writeText).not.toHaveBeenCalled();

    manager.cssEditor = { state: { doc: { toString: () => '.b { margin: 0; }' } } };
    manager.copyCssToClipboard();
    await Promise.resolve();
    expect(writeText).toHaveBeenCalledWith('.b { margin: 0; }');
  });
});
