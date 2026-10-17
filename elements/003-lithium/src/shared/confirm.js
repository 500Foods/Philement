/**
 * Promise confirm and prompt dialogs.
 * Native alert, confirm, and prompt stay out of the hot paths.
 */

let activeClose = null;

function closeActive() {
  if (activeClose) {
    activeClose();
  }
}

function openDialog({
  title,
  message,
  mode,
  defaultValue = '',
  confirmLabel,
  cancelLabel,
  danger = false,
}) {
  closeActive();

  return new Promise((resolve) => {
    const backdrop = document.createElement('div');
    backdrop.className = 'lithium-confirm-backdrop';

    const dialog = document.createElement('div');
    dialog.className = 'lithium-confirm';
    dialog.setAttribute('role', 'dialog');
    dialog.setAttribute('aria-modal', 'true');

    const titleEl = document.createElement('h2');
    titleEl.className = 'lithium-confirm-title';
    titleEl.textContent = title || (mode === 'prompt' ? 'Enter a value' : 'Confirm');

    const messageEl = document.createElement('p');
    messageEl.className = 'lithium-confirm-message';
    messageEl.textContent = message || '';

    let input = null;
    if (mode === 'prompt') {
      input = document.createElement('input');
      input.type = 'text';
      input.className = 'lithium-confirm-input';
      input.value = defaultValue;
    }

    const actions = document.createElement('div');
    actions.className = 'lithium-confirm-actions';

    const cancelBtn = document.createElement('button');
    cancelBtn.type = 'button';
    cancelBtn.className = 'btn btn-secondary';
    cancelBtn.textContent = cancelLabel || 'Cancel';

    const confirmBtn = document.createElement('button');
    confirmBtn.type = 'button';
    confirmBtn.className = danger ? 'btn btn-danger' : 'btn btn-primary';
    confirmBtn.textContent = confirmLabel || (mode === 'prompt' ? 'Save' : 'Confirm');

    actions.append(cancelBtn, confirmBtn);
    dialog.append(titleEl, messageEl);
    if (input) dialog.append(input);
    dialog.append(actions);
    backdrop.append(dialog);
    document.body.append(backdrop);

    const previousFocus = document.activeElement;
    let settled = false;

    const finish = (value) => {
      if (settled) return;
      settled = true;
      document.removeEventListener('keydown', onKey);
      document.removeEventListener('close-all-popups', onCloseAll);
      backdrop.remove();
      if (activeClose === cancel) activeClose = null;
      if (previousFocus && typeof previousFocus.focus === 'function') {
        previousFocus.focus();
      }
      resolve(value);
    };

    const accept = () => {
      if (mode === 'prompt') {
        finish(input.value);
        return;
      }
      finish(true);
    };

    const cancel = () => {
      finish(mode === 'prompt' ? null : false);
    };

    const onKey = (event) => {
      if (event.key === 'Escape') {
        event.preventDefault();
        cancel();
      } else if (event.key === 'Enter') {
        event.preventDefault();
        accept();
      }
    };

    const onCloseAll = () => cancel();

    activeClose = cancel;
    cancelBtn.addEventListener('click', cancel);
    confirmBtn.addEventListener('click', accept);
    backdrop.addEventListener('click', (event) => {
      if (event.target === backdrop) cancel();
    });
    document.dispatchEvent(new CustomEvent('close-all-popups'));
    document.addEventListener('keydown', onKey);
    document.addEventListener('close-all-popups', onCloseAll);

    if (input) {
      input.focus();
      input.select();
    } else {
      confirmBtn.focus();
    }
  });
}

/**
 * @param {Object} options
 * @param {string} [options.title]
 * @param {string} [options.message]
 * @param {string} [options.confirmLabel]
 * @param {string} [options.cancelLabel]
 * @param {boolean} [options.danger]
 * @returns {Promise<boolean>}
 */
export function confirmDialog(options = {}) {
  return openDialog({ ...options, mode: 'confirm' });
}

/**
 * @param {Object} options
 * @param {string} [options.title]
 * @param {string} [options.message]
 * @param {string} [options.defaultValue]
 * @param {string} [options.confirmLabel]
 * @param {string} [options.cancelLabel]
 * @returns {Promise<string|null>} The entered text, or null when cancelled
 */
export function promptDialog(options = {}) {
  return openDialog({ ...options, mode: 'prompt' });
}
