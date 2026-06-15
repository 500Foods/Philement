import './index-CEuz7QhE.js';

/**
 * Lithium Toolbar
 *
 * Standardized toolbar component for managers.
 * Provides consistent toolbar styling with collapsible text support.
 */


// ===== Static Toolbar Functions =====

/**
 * Calculate the collapse breakpoint for a toolbar.
 * @param {HTMLElement} container - Toolbar container element
 * @returns {number} Breakpoint width in pixels
 */
function calculateToolbarBreakpoint(container) {
  const wasCollapsed = container.classList.contains('collapsed-text');
  if (wasCollapsed) {
    container.classList.remove('collapsed-text');
  }

  let buttonsWidth = 0;
  const children = Array.from(container.children);
  const gap = parseFloat(getComputedStyle(container).gap) || 1;
  let numButtons = 0;

  for (const child of children) {
    if (child.classList.contains('lithium-toolbar-placeholder')) continue;
    const rect = child.getBoundingClientRect();
    buttonsWidth += rect.width;
    numButtons++;
  }

  if (wasCollapsed) {
    container.classList.add('collapsed-text');
  }

  const totalGapWidth = (numButtons > 0 ? numButtons - 1 : 0) * gap;
  return Math.ceil(buttonsWidth + totalGapWidth + 100);
}

/**
 * Initialize all static toolbars on the page
 * @param {string} selector - CSS selector for toolbar containers (default: '.lithium-toolbar')
 */
function initToolbars(selector = '.lithium-toolbar') {
  const toolbars = document.querySelectorAll(selector);
  toolbars.forEach((container) => {
    if (container.dataset.toolbarInitialized) return;
    container.dataset.toolbarInitialized = 'true';

    let placeholder = container.querySelector('.lithium-toolbar-placeholder');

    if (!placeholder) {
      placeholder = document.createElement('div');
      placeholder.className = 'lithium-toolbar-placeholder';
      container.appendChild(placeholder);
    }

    container.dataset.toolbarBreakpoint = calculateToolbarBreakpoint(container);

    if (window.ResizeObserver) {
      const resizeObserver = new ResizeObserver((entries) => {
        for (const entry of entries) {
          let breakpoint = parseFloat(container.dataset.toolbarBreakpoint) || 0;
          if (breakpoint <= 0) {
            breakpoint = calculateToolbarBreakpoint(container);
            container.dataset.toolbarBreakpoint = breakpoint;
          }
          const width = entry.contentRect.width;
          container.classList.toggle('collapsed-text', width < breakpoint);
        }
      });
      resizeObserver.observe(container);
    }
  });
}

export { initToolbars as i };
