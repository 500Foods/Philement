/**
 * Filter Editor Module
 *
 * Header filter editor creation and management.
 *
 * @module tables/filters/filter-editor
 */

/**
 * Create a header filter editor function that captures cssPrefix in closure.
 * Returns a function suitable for use as Tabulator's headerFilter option.
 * Tabulator calls this with `this.table.modules.edit` as context, so we
 * must not depend on `this` context - capture everything in the closure.
 * @param {string} cssPrefix - CSS class prefix
 * @returns {Function} Filter editor function
 */
export function createFilterEditorFunction(cssPrefix) {
  return function(cell, onRendered, success, cancel, editorParams) {
    const input = document.createElement('input');
    input.type = 'text';
    input.placeholder = editorParams?.placeholder || 'filter...';
    input.className = `${cssPrefix}-header-filter-input`;

    let clearBtn = null;

    const updateClearBtn = () => {
      if (clearBtn) {
        clearBtn.style.display = input.value ? 'flex' : 'none';
      }
    };

    input.addEventListener('input', () => {
      updateClearBtn();
      success(input.value);
    });

    input.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        input.value = '';
        updateClearBtn();
        success('');
        e.stopPropagation();
      }
    });

    onRendered(() => {
      const parent = input.parentElement;
      if (!parent) return;
      parent.style.position = 'relative';

      clearBtn = document.createElement('span');
      clearBtn.className = `${cssPrefix}-header-filter-clear`;
      clearBtn.innerHTML = '&times;';
      clearBtn.title = 'Clear filter';
      clearBtn.style.display = input.value ? 'flex' : 'none';

      clearBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        input.value = '';
        clearBtn.style.display = 'none';
        success('');
      });

      parent.appendChild(clearBtn);
    });

    return input;
  };
}
