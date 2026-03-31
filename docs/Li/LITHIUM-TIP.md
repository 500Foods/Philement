# Lithium Tooltip System

Advanced tooltip system powered by [Floating UI](https://floating-ui.com/) for intelligent positioning, animations, and rich content.

---

## Installation

```bash
npm install @floating-ui/dom
```

---

## Architecture

The tooltip system consists of three layers:

| Layer | File | Purpose |
|-------|------|---------|
| **Core** | `src/core/tooltip.js` | FloatingUI integration, positioning engine |
| **Styles** | `src/styles/tooltip.css` | CSS variables, themes, arrows, animations |
| **API** | `src/core/tooltip-api.js` | High-level helper functions |

---

## Core Module

`src/core/tooltip.js` — Low-level FloatingUI wrapper.

```javascript
import { computePosition, flip, shift, offset, arrow, autoUpdate } from '@floating-ui/dom';

const TOOLTIP_DEFAULTS = {
  placement: 'top',
  offset: 8,
  arrow: true,
  theme: 'default',        // default | success | warning | danger | info | crimson
  trigger: 'hover',        // hover | click | manual
  delay: { show: 200, hide: 100 },
  maxWidth: 320,
  interactive: false,      // allow mouse interaction inside tooltip
  autoDirection: true,     // flip placement when near viewport edge
};

export class Tooltip {
  constructor(triggerEl, content, options = {}) {
    this.trigger = triggerEl;
    this.content = content;
    this.options = { ...TOOLTIP_DEFAULTS, ...options };
    this.tooltipEl = null;
    this.arrowEl = null;
    this.cleanup = null;
    this.showTimeout = null;
    this.hideTimeout = null;
    this.isVisible = false;
  }

  init() {
    this.createTooltipElement();
    this.bindEvents();
    return this;
  }

  createTooltipElement() {
    this.tooltipEl = document.createElement('div');
    this.tooltipEl.className = `li-tooltip li-tooltip--${this.options.theme}`;
    this.tooltipEl.setAttribute('role', 'tooltip');
    this.tooltipEl.setAttribute('aria-hidden', 'true');

    // Content container
    const contentEl = document.createElement('div');
    contentEl.className = 'li-tooltip__content';
    contentEl.innerHTML = this.content;
    this.tooltipEl.appendChild(contentEl);

    // Arrow
    if (this.options.arrow) {
      this.arrowEl = document.createElement('div');
      this.arrowEl.className = 'li-tooltip__arrow';
      this.tooltipEl.appendChild(this.arrowEl);
    }

    document.body.appendChild(this.tooltipEl);
  }

  bindEvents() {
    if (this.options.trigger === 'hover') {
      this.trigger.addEventListener('mouseenter', () => this.scheduleShow());
      this.trigger.addEventListener('mouseleave', () => this.scheduleHide());
      this.trigger.addEventListener('focus', () => this.scheduleShow());
      this.trigger.addEventListener('blur', () => this.scheduleHide());
    } else if (this.options.trigger === 'click') {
      this.trigger.addEventListener('click', (e) => {
        e.stopPropagation();
        this.toggle();
      });
      document.addEventListener('click', (e) => {
        if (!this.tooltipEl.contains(e.target) && !this.trigger.contains(e.target)) {
          this.hide();
        }
      });
    }

    if (this.options.interactive) {
      this.tooltipEl.addEventListener('mouseenter', () => clearTimeout(this.hideTimeout));
      this.tooltipEl.addEventListener('mouseleave', () => this.scheduleHide());
    }
  }

  scheduleShow() {
    clearTimeout(this.hideTimeout);
    this.showTimeout = setTimeout(() => this.show(), this.options.delay.show);
  }

  scheduleHide() {
    clearTimeout(this.showTimeout);
    this.hideTimeout = setTimeout(() => this.hide(), this.options.delay.hide);
  }

  show() {
    if (this.isVisible) return;
    this.isVisible = true;
    this.tooltipEl.setAttribute('aria-hidden', 'false');
    this.tooltipEl.classList.add('li-tooltip--visible');

    this.cleanup = autoUpdate(this.trigger, this.tooltipEl, () => this.updatePosition());
    this.updatePosition();
  }

  hide() {
    if (!this.isVisible) return;
    this.isVisible = false;
    this.tooltipEl.setAttribute('aria-hidden', 'true');
    this.tooltipEl.classList.remove('li-tooltip--visible');

    if (this.cleanup) {
      this.cleanup();
      this.cleanup = null;
    }
  }

  toggle() {
    this.isVisible ? this.hide() : this.show();
  }

  async updatePosition() {
    const middleware = [
      offset(this.options.offset),
      flip({ fallbackAxisSideAssignment: 'start' }),
      shift({ padding: 8 }),
    ];

    if (this.options.arrow && this.arrowEl) {
      middleware.push(arrow({ element: this.arrowEl }));
    }

    const { x, y, placement, middlewareData } = await computePosition(
      this.trigger,
      this.tooltipEl,
      {
        placement: this.options.placement,
        strategy: 'fixed',
        middleware,
      }
    );

    Object.assign(this.tooltipEl.style, {
      left: `${x}px`,
      top: `${y}px`,
    });

    this.tooltipEl.setAttribute('data-placement', placement);

    // Position arrow
    if (this.options.arrow && this.arrowEl && middlewareData.arrow) {
      const { x: arrowX, y: arrowY } = middlewareData.arrow;
      const staticSide = {
        top: 'bottom',
        right: 'left',
        bottom: 'top',
        left: 'right',
      }[placement.split('-')[0]];

      Object.assign(this.arrowEl.style, {
        left: arrowX != null ? `${arrowX}px` : '',
        top: arrowY != null ? `${arrowY}px` : '',
        right: '',
        bottom: staticSide === 'top' ? '-4px' : '',
        [staticSide]: '-4px',
      });
    }
  }

  updateContent(newContent) {
    this.content = newContent;
    const contentEl = this.tooltipEl?.querySelector('.li-tooltip__content');
    if (contentEl) contentEl.innerHTML = newContent;
  }

  destroy() {
    this.hide();
    clearTimeout(this.showTimeout);
    clearTimeout(this.hideTimeout);
    this.tooltipEl?.remove();
    this.tooltipEl = null;
    this.arrowEl = null;
  }
}
```

---

## High-Level API

`src/core/tooltip-api.js` — Convenience functions for common patterns.

```javascript
import { Tooltip } from './tooltip.js';

const tooltipRegistry = new WeakMap();

/**
 * Attach a tooltip to an element.
 * Returns the Tooltip instance for manual control.
 */
export function tip(triggerEl, content, options = {}) {
  const existing = tooltipRegistry.get(triggerEl);
  if (existing) existing.destroy();

  const t = new Tooltip(triggerEl, content, options).init();
  tooltipRegistry.set(triggerEl, t);
  return t;
}

/**
 * Remove tooltip from element.
 */
export function untip(triggerEl) {
  const existing = tooltipRegistry.get(triggerEl);
  if (existing) {
    existing.destroy();
    tooltipRegistry.delete(triggerEl);
  }
}

/**
 * Initialize all elements with data-tooltip attribute.
 * Reads options from data-tip-* attributes.
 *
 *   <button data-tooltip="Save" data-tip-theme="success" data-tip-placement="bottom">
 */
export function initTooltips(root = document) {
  root.querySelectorAll('[data-tooltip]').forEach((el) => {
    const content = el.getAttribute('data-tooltip');
    if (!content) return;

    const options = {};
    const placement = el.getAttribute('data-tip-placement');
    const theme = el.getAttribute('data-tip-theme');
    const trigger = el.getAttribute('data-tip-trigger');
    const interactive = el.getAttribute('data-tip-interactive');

    if (placement) options.placement = placement;
    if (theme) options.theme = theme;
    if (trigger) options.trigger = trigger;
    if (interactive !== null) options.interactive = true;

    tip(el, content, options);
  });
}

/**
 * Bulk update all tooltips matching a selector.
 */
export function updateTooltipContent(selector, newContent) {
  document.querySelectorAll(selector).forEach((el) => {
    const t = tooltipRegistry.get(el);
    if (t) t.updateContent(newContent);
  });
}
```

---

## Themes

Tooltips support six built-in themes, controlled via the `theme` option or `data-tip-theme` attribute.

| Theme | Color | Use Case |
|-------|-------|----------|
| `default` | Neutral gray | General UI elements |
| `success` | Green | Confirmation, positive actions |
| `warning` | Amber | Caution, unsaved changes |
| `danger` | Red | Errors, destructive actions |
| `info` | Blue | Informational, help text |
| `crimson` | Crimson red | AI agent interactions, Crimson-specific |

### Theme CSS Variables

```css
:root {
  /* Default theme */
  --tip-bg: #2E2E2E;
  --tip-border: #3A3A3A;
  --tip-text: #E0E0E0;
  --tip-arrow: #2E2E2E;
  --tip-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);

  /* Success */
  --tip-success-bg: #1B3A2A;
  --tip-success-border: #2D6A4F;
  --tip-success-text: #95D5B2;
  --tip-success-arrow: #1B3A2A;

  /* Warning */
  --tip-warning-bg: #3D2E0A;
  --tip-warning-border: #B45309;
  --tip-warning-text: #FCD34D;
  --tip-warning-arrow: #3D2E0A;

  /* Danger */
  --tip-danger-bg: #3B1111;
  --tip-danger-border: #991B1B;
  --tip-danger-text: #FCA5A5;
  --tip-danger-arrow: #3B1111;

  /* Info */
  --tip-info-bg: #0F1F3A;
  --tip-info-border: #1E40AF;
  --tip-info-text: #93C5FD;
  --tip-info-arrow: #0F1F3A;

  /* Crimson */
  --tip-crimson-bg: #2A0A0F;
  --tip-crimson-border: #DC143C;
  --tip-crimson-text: #FDA4AF;
  --tip-crimson-arrow: #2A0A0F;
}
```

---

## CSS

`src/styles/tooltip.css`:

```css
/* ========================================
   Tooltip System — FloatingUI Powered
   ======================================== */

/* Base */
.li-tooltip {
  position: fixed;
  z-index: var(--z-tooltip, 500);
  max-width: 320px;
  padding: var(--space-1) var(--space-2);
  border-radius: var(--border-radius-sm, 4px);
  font-size: var(--font-size-xs, 12px);
  line-height: 1.4;
  white-space: normal;
  word-wrap: break-word;
  pointer-events: none;
  opacity: 0;
  transform: scale(0.96);
  transition: opacity 150ms ease, transform 150ms ease;
  will-change: transform, opacity;
}

.li-tooltip--visible {
  opacity: 1;
  transform: scale(1);
}

/* Content */
.li-tooltip__content {
  position: relative;
  z-index: 1;
}

/* Arrow */
.li-tooltip__arrow {
  position: absolute;
  width: 8px;
  height: 8px;
  transform: rotate(45deg);
  z-index: 0;
}

/* ========================================
   Themes
   ======================================== */

/* Default */
.li-tooltip--default {
  background-color: var(--tip-bg);
  border: 1px solid var(--tip-border);
  color: var(--tip-text);
  box-shadow: var(--tip-shadow);
}
.li-tooltip--default .li-tooltip__arrow {
  background-color: var(--tip-arrow);
  border: inherit;
}

/* Success */
.li-tooltip--success {
  background-color: var(--tip-success-bg);
  border: 1px solid var(--tip-success-border);
  color: var(--tip-success-text);
  box-shadow: var(--tip-shadow);
}
.li-tooltip--success .li-tooltip__arrow {
  background-color: var(--tip-success-arrow);
}

/* Warning */
.li-tooltip--warning {
  background-color: var(--tip-warning-bg);
  border: 1px solid var(--tip-warning-border);
  color: var(--tip-warning-text);
  box-shadow: var(--tip-shadow);
}
.li-tooltip--warning .li-tooltip__arrow {
  background-color: var(--tip-warning-arrow);
}

/* Danger */
.li-tooltip--danger {
  background-color: var(--tip-danger-bg);
  border: 1px solid var(--tip-danger-border);
  color: var(--tip-danger-text);
  box-shadow: var(--tip-shadow);
}
.li-tooltip--danger .li-tooltip__arrow {
  background-color: var(--tip-danger-arrow);
}

/* Info */
.li-tooltip--info {
  background-color: var(--tip-info-bg);
  border: 1px solid var(--tip-info-border);
  color: var(--tip-info-text);
  box-shadow: var(--tip-shadow);
}
.li-tooltip--info .li-tooltip__arrow {
  background-color: var(--tip-info-arrow);
}

/* Crimson */
.li-tooltip--crimson {
  background-color: var(--tip-crimson-bg);
  border: 1px solid var(--tip-crimson-border);
  color: var(--tip-crimson-text);
  box-shadow: var(--tip-shadow);
}
.li-tooltip--crimson .li-tooltip__arrow {
  background-color: var(--tip-crimson-arrow);
}
```

---

## Usage Patterns

### Basic (data-attribute, zero JS)

```html
<button data-tooltip="Save changes">Save</button>
```

Initialize once on page load:

```javascript
import { initTooltips } from '../core/tooltip-api.js';

initTooltips();
```

### Title attribute (automatic capture)

`initTooltips()` also captures `title` attributes. When a `title` is used, the attribute is removed to prevent the native browser tooltip from appearing. `data-tooltip` takes precedence if both are present.

```html
<!-- These all become FloatingUI tooltips automatically -->
<button title="Close">X</button>
<button title="Search" data-tip-theme="info">Search</button>
```

This means existing `title="..."` attributes across the codebase (sidebar buttons, menu items, toolbar buttons) work as fancy tooltips with zero changes. No need to convert them to `data-tooltip`.

### With theme and placement

```html
<button
  data-tooltip="Delete permanently"
  data-tip-theme="danger"
  data-tip-placement="bottom"
>
  <fa fa-trash></fa>
</button>
```

### Rich HTML content

```javascript
import { tip } from '../core/tooltip-api.js';

const el = document.getElementById('status-indicator');

tip(el, `
  <strong>Connected</strong><br>
  <span style="opacity: 0.7">Latency: 12ms</span>
`, {
  theme: 'success',
  placement: 'bottom-end',
  interactive: true,
});
```

### Programmatic control

```javascript
import { tip, untip } from '../core/tooltip-api.js';

const t = tip(buttonEl, 'Loading...', {
  trigger: 'manual',
  theme: 'info',
});

// Show
t.show();

// Update content
t.updateContent('Done!');

// Hide after delay
setTimeout(() => t.hide(), 2000);

// Remove entirely
untip(buttonEl);
```

### Click trigger (toggle)

```html
<input
  data-tooltip="Click to copy"
  data-tip-trigger="click"
  data-tip-theme="info"
>
```

### Interactive tooltip (stays open on hover)

```html
<button
  data-tooltip="<strong>Ctrl+S</strong> — Save<br><strong>Ctrl+Z</strong> — Undo"
  data-tip-interactive
  data-placement="right"
>
  Shortcuts
</button>
```

---

## Migration from CSS-only Tooltips

The old system used three `::after` pseudo-element patterns — all removed:

```css
/* OLD — all removed */
.btn[data-tooltip]::after { content: attr(data-tooltip); ... }
.sidebar-icon-btn[title]::after { content: attr(title); ... }
.tooltip::after { content: attr(data-tooltip); ... }
```

### What was done

1. Removed `::after` tooltip CSS from `layout.css` (4 rule blocks)
2. Removed `::after` tooltip CSS from `login.css` (`.login-alt-btn.tooltip`)
3. Removed `.tooltip` class rules from `components.css`
4. Removed sidebar/menu/zoom `::after` tooltip CSS from `main.css`
5. Removed `classList.add('tooltip')` calls from `login.js`
6. `initTooltips()` now captures both `data-tooltip` and `title` attributes
7. `title` attributes are removed after capture to prevent native browser tooltips

### Files changed

| File | Action |
|------|--------|
| `package.json` | Added `@floating-ui/dom` dependency |
| `src/core/tooltip.js` | Created — FloatingUI `Tooltip` class |
| `src/core/tooltip-api.js` | Created — `tip()`, `untip()`, `initTooltips()`, MutationObserver |
| `src/styles/tooltip.css` | Created — 6 themes, arrows, transitions |
| `src/app.js` | Import tooltip.css, initTooltips(), expose on window |
| `src/styles/layout.css` | Removed 4 old tooltip rule blocks |
| `src/styles/components.css` | Removed `.tooltip` class rules |
| `src/managers/login/login.css` | Removed tooltip rules |
| `src/managers/login/login.js` | Removed `classList.add('tooltip')` |
| `src/managers/main/main.css` | Removed sidebar/menu/zoom tooltip rules |

---

## Keyboard Accessibility

The tooltip system follows WAI-ARIA tooltip patterns:

- `role="tooltip"` on the tooltip container
- `aria-hidden` toggled on show/hide
- Trigger elements with `tabindex` show tooltips on `focus`, hide on `blur`
- Interactive tooltips do not trap focus (passive `pointer-events` only when non-interactive)

---

## Performance

- `autoUpdate` from FloatingUI only runs while the tooltip is visible — no idle listeners
- `will-change: transform, opacity` enables GPU compositing
- Transitions use 150ms — fast enough to feel instant, slow enough to be perceptible
- WeakMap registry prevents memory leaks — destroyed elements are garbage collected

---

## Related Documentation

- Library details: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- CSS architecture: [LITHIUM-CSS.md](LITHIUM-CSS.md)
- Coding standards: [LITHIUM-INS.md](LITHIUM-INS.md)
