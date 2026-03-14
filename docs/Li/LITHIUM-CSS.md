# Lithium CSS Architecture

Lithium uses a custom CSS architecture based on CSS custom properties (variables) with a dark-mode-first approach. No UI frameworks — just pure vanilla CSS.

**Location:** `src/styles/`

---

## Overview

| File | Purpose |
|------|---------|
| [`base.css`](elements/003-lithium/src/styles/base.css) | CSS reset, variables, dark theme defaults |
| [`layout.css`](elements/003-lithium/src/styles/layout.css) | Layout components (sidebar, header, etc.) |
| [`components.css`](elements/003-lithium/src/styles/components.css) | Reusable UI components |
| [`transitions.css`](elements/003-lithium/src/styles/transitions.css) | Animation and transition utilities |

---

## CSS Variables System

### Design Tokens

The entire theming system is built on CSS custom properties defined in `base.css`:

```css
:root {
  /* Colors - Dark theme defaults */
  --bg-primary: #1A1A1A;
  --bg-secondary: #242424;
  --bg-tertiary: #2E2E2E;
  --bg-input: #363636;
  --bg-hover: #3A3A3A;
  --bg-active: #454545;
  
  --text-primary: #E0E0E0;
  --text-secondary: #B0B0B0;
  --text-muted: #707070;
  --text-disabled: #505050;
  
  --border-color: #3A3A3A;
  --accent-primary: #5C6BC0;
  --accent-danger: #EF5350;
  --accent-success: #66BB6A;
  
  /* Typography */
  --font-family: 'Vanadium Sans', ...;
  --font-mono: 'Vanadium Mono', ...;
  
  /* Spacing */
  --space-1: 2px;
  --space-2: 4px;
  --space-3: 6px;
  --space-4: 8px;
  --space-5: 16px;
  --space-6: 24px;
  --space-7: 32px;
  --space-8: 48px;
  
  /* Transitions - configurable */
  --transition-duration: 500ms;
  --transition-fast: calc(var(--transition-duration) * 0.3);
}
```

### Theming

Themes work by overriding these CSS variables. When a theme is applied:

```javascript
// In Style Manager
const style = document.createElement('style');
style.textContent = `:root { --bg-primary: #ffffff; --text-primary: #000000; }`;
document.head.appendChild(style);
```

---

## CSS Architecture Principles

### 1. Dark Mode First

Dark theme is the default. All colors are designed for dark backgrounds first.

```css
/* Good - explicitly dark */
background-color: var(--bg-primary);

/* Avoid - hardcoded values */
background-color: #1A1A1A;
```

### 2. Semantic Naming

Use semantic variable names, not color names:

```css
/* Good */
color: var(--text-primary);
background: var(--bg-secondary);

/* Avoid */
color: #E0E0E0;
background: #242424;
```

### 3. Consistent Spacing

Use spacing variables instead of arbitrary values:

```css
/* Good */
padding: var(--space-4);

/* Avoid */
padding: 8px;
```

---

## Layout System

### Main Layout

```css
/* Standard layout structure */
#main-layout {
  display: flex;
  height: 100vh;
  overflow: hidden;
}

.main-sidebar {
  width: 260px;
  flex-shrink: 0;
}

.manager-area {
  flex: 1;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}
```

### Manager Slots

Each manager gets a slot with header/workspace/footer:

```css
.manager-slot {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.manager-slot-header {
  flex-shrink: 0;
}

.manager-slot-workspace {
  flex: 1;
  overflow: auto;
}

.manager-slot-footer {
  flex-shrink: 0;
}
```

---

## Component Patterns

### Buttons

```css
.btn {
  padding: var(--space-3) var(--space-5);
  border-radius: var(--border-radius-md);
  background: var(--bg-tertiary);
  color: var(--text-primary);
  border: var(--border-standard);
  cursor: pointer;
}

.btn:hover {
  background: var(--bg-hover);
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
```

### Inputs

```css
.input {
  padding: var(--space-3);
  background: var(--bg-input);
  border: var(--border-standard);
  border-radius: var(--border-radius-md);
  color: var(--text-primary);
}

.input:focus {
  border-color: var(--accent-primary);
  outline: none;
}
```

### Panels

```css
.panel {
  background: var(--bg-secondary);
  border: var(--border-standard);
  border-radius: var(--border-radius-lg);
  padding: var(--space-5);
}
```

---

## Transitions

### Configurable Duration

All transitions use the `--transition-duration` CSS variable:

```css
.fade-enter {
  opacity: 0;
}

.fade-enter-active {
  opacity: 1;
  transition: opacity var(--transition-duration) ease;
}
```

### Standard Transitions

```css
/* Fast - for hover states */
transition: background-color var(--transition-fast);

/* Standard - for most interactions */
transition: all var(--transition-std);

/* Slow - for page transitions */
transition: all var(--transition-slow);
```

### Reduced Motion

Respect user preferences:

```css
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    transition-duration: 0.01ms !important;
  }
}
```

---

## Scrollbar Styling

### WebKit (Chrome, Safari, Edge)

```css
::-webkit-scrollbar {
  width: var(--scrollbar-size);
  height: var(--scrollbar-size);
}

::-webkit-scrollbar-track {
  background: var(--scrollbar-track);
}

::-webkit-scrollbar-thumb {
  background: var(--scrollbar-thumb);
  border-radius: var(--border-radius-full);
}
```

### Firefox

```css
* {
  scrollbar-width: auto;
  scrollbar-color: var(--scrollbar-thumb) var(--scrollbar-track);
}
```

---

## Fonts

### Local-First Strategy

```css
@font-face {
  font-family: 'Vanadium Sans';
  src: local('Vanadium Sans'),
       url('/assets/fonts/VanadiumSans-SemiExtended.woff2') format('woff2');
  font-display: swap;
}
```

### Font Stack

```css
--font-family: 'Vanadium Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', ...;
--font-mono: 'Vanadium Mono', 'Courier New', Courier, monospace;
```

---

## Z-Index Scale

Avoid arbitrary z-index values:

```css
:root {
  --z-dropdown: 100;
  --z-sticky: 200;
  --z-modal: 300;
  --z-popover: 400;
  --z-tooltip: 500;
}
```

---

## Manager Styles

Each manager can have its own CSS file:

| Manager | Styles |
|---------|---------|
| Login | `src/managers/login/login.css` |
| Main | `src/managers/main/main.css` |
| Style Manager | `src/managers/style-manager/style-manager.css` |
| Session Log | `src/managers/session-log/session-log.css` |
| Profile | `src/managers/profile-manager/profile-manager.css` |

These are imported in the manager's JavaScript:

```javascript
import './login.css';
```

---

## Loading States

### Basic Spinner

A simple circular spinner using CSS custom properties:

```css
.spinner {
  display: inline-block;
  width: 20px;
  height: 20px;
  border: 2px solid var(--border-color);
  border-top-color: var(--accent-primary);
  border-radius: 50%;
  animation: spin 1s linear infinite;
}
```

### Fancy Spinner

A multi-shadow circular spinner with theme colors. Uses CSS custom properties for easy theming and sizing.

**Important:** You must specify both the base `spinner-fancy` class AND a size class for the spinner to work correctly.

**Size variants:**

```html
<!-- Small (8px) -->
<div class="spinner-fancy spinner-fancy-sm"></div>

<!-- Medium (11px) -->
<div class="spinner-fancy spinner-fancy-md"></div>

<!-- Large (16px) -->
<div class="spinner-fancy spinner-fancy-lg"></div>
```

**Note:** The base `spinner-fancy` class alone does not include the animation keyframes. Always use one of the size classes (`-sm`, `-md`, or `-lg`) to ensure the spinner animates properly.

**Custom colors:**
Override the theme colors using CSS custom properties:

```css
.custom-spinner {
  --spinner-fancy-color-1: #FF5722;  /* Center color */
  --spinner-fancy-color-2: #FFC107; /* Secondary shadow */
  --spinner-fancy-color-3: #4CAF50; /* Tertiary shadow */
}
```

**With overlay:**

```html
<div class="spinner-overlay">
  <div class="spinner-fancy"></div>
</div>
```

### Skeleton Loading

For content placeholder animations:

```css
.skeleton {
  background: linear-gradient(
    90deg,
    var(--bg-tertiary) 25%,
    var(--bg-hover) 50%,
    var(--bg-tertiary) 75%
  );
  background-size: 200% 100%;
  animation: shimmer 1.5s infinite;
  border-radius: var(--border-radius-md);
}
```

---

## Best Practices

1. **Use CSS variables** for all colors, spacing, and timing
2. **Avoid hardcoded values** except in `base.css` definitions
3. **Mobile-first** responsive design
4. **Reduced motion** accessibility
5. **Semantic class names** (e.g., `.btn-primary`, not `.blue-button`)
6. **Component isolation** - each manager's styles should be scoped

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager system
- [LITHIUM-LIB.md](LITHIUM-LIB.md) - Libraries
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
