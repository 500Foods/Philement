# Lithium Coding Instructions

## ⚠️ MANDATORY READING

**All developers must read and follow these instructions religiously when working on the Lithium project. Failure to adhere to these guidelines will result in code review rejections.**

---

## Core Principles

### 1. No Fallback Code for Primary Features

**DO NOT** create fallback implementations that duplicate functionality of what you're implementing.

❌ **Wrong:**

```javascript
// Creating a textarea fallback when JsonTree fails
try {
  const tree = new JsonTree(container, options);
} catch (e) {
  // Fallback to textarea - DON'T DO THIS
  container.innerHTML = '<textarea>...</textarea>';
}
```

✅ **Correct:**

```javascript
// Make the primary implementation robust
const tree = new JsonTree(container, {
  data: sanitizedData,
  onError: handleError gracefully
});
```

**Rationale:** Fallback code adds confusion, increases bundle size, and creates maintenance burden. Instead, make the primary implementation handle edge cases properly.

---

### 2. File Size Limits

**ALL source files must remain under 1,000 lines of code.**

When a file approaches this limit:

- Split into logical modules
- Extract reusable components
- Move utility functions to shared modules

See [REFACTORING_PLAN.md](../../elements/003-lithium/docs/REFACTORING_PLAN.md) for examples of how to split large files.

**Rationale:** Large files create extra cost and complexity in an age of AI coding. Models tend to not deal very well with large files, often corrupting them in the process of making changes.

---

### 3. ES6 Module Architecture

Strive for **reusable and testable** code modules. We're using ES6 for a reason!

**Guidelines:**

- One class per file (generally)
- Pure functions where possible
- Clear exports/imports
- Dependency injection for testability

✅ **Good Example:**

```javascript
// auth-service.js
export class AuthService {
  constructor(apiClient) {
    this.api = apiClient;
  }
  
  async login(credentials) {
    // Testable - apiClient can be mocked
  }
}

// usage
import { AuthService } from './services/auth-service.js';
const auth = new AuthService(api);
```

**Rationale:** There are a great many parts of our code that are reused over and over again. We need to do the work to implement them once rather than having the same thing littered all over the place with different variations and fixes depending on where you look.

---

### 4. CSS-First Styling

**Prefer CSS classes instead of inline JavaScript style injection ALWAYS.**

❌ **Never do this:**

```javascript
element.style.backgroundColor = 'var(--bg-primary)';
element.style.padding = '16px';
element.style.borderRadius = '8px';
```

✅ **Do this instead:**

```javascript
element.classList.add('panel-container');
```

```css
/* In your CSS file */
.panel-container {
  background-color: var(--bg-primary);
  padding: var(--space-4);
  border-radius: var(--border-radius-lg);
}
```

**There should be no CSS in JS.** Dynamic values should use CSS custom properties (variables) that can be updated via classes.

**Rationale:** We have (or will have) a Style Manager for theming. This relies largely on having CSS be accessible to us, which doesn't work very well when inline styles are used.
This is because inline styles typically take precedence over CSS classes, making them harder to override and maintain. And there's no reason to use inline styles in the first place.

---

### 5. Testable Code Structure

Aim for testable code using helper functions to achieve higher coverage numbers.

**Strategies:**

- Extract business logic into pure functions
- Use dependency injection for external services
- Avoid tight coupling to DOM or global state
- Export functions for unit testing

✅ **Testable Pattern:**

```javascript
// Pure function - easily tested
export function validateQueryParams(params) {
  if (!params.queryRef) return { valid: false, error: 'Missing queryRef' };
  if (typeof params.queryRef !== 'number') return { valid: false, error: 'Invalid type' };
  return { valid: true };
}

// Component uses the function
class QueryManager {
  async loadData(params) {
    const validation = validateQueryParams(params);
    if (!validation.valid) throw new Error(validation.error);
    // ...
  }
}
```

---

### 6. CSS Reuse

Try to reuse existing CSS where feasible - not everything needs a new CSS file.

**Before creating new styles, check:**

- `src/styles/base.css` - Base styles and variables
- `src/styles/components.css` - Shared components
- `src/styles/layout.css` - Layout utilities
- Other manager CSS files for similar patterns

**Use existing CSS variables:**

```css
/* Available variables */
--bg-primary, --bg-secondary, --bg-tertiary
--text-primary, --text-secondary, --text-muted
--space-1 through --space-8
--border-radius-sm, --border-radius-md, --border-radius-lg
```

---

## Code Review Checklist

Before submitting code for review, verify:

- [ ] No file exceeds 1,000 lines
- [ ] No fallback implementations for primary features
- [ ] No inline styles (use CSS classes)
- [ ] ES6 modules are properly structured
- [ ] Functions are extractable and testable
- [ ] Existing CSS is reused where possible
- [ ] New CSS follows naming conventions

---

## Anti-Patterns to Avoid

### The Fallback Trap

Creating fallback UIs when libraries fail. Instead, fix the library integration or handle errors gracefully at the application level.

### The God Class

A single class that does everything. Split into focused classes with single responsibilities.

### Style Injection

Using `element.style.property = value` for anything other than dynamic positioning (e.g., drag handles).

### Copy-Paste Development

Duplicating code across files. Extract to shared utilities.

---

## Related Documentation

- [LITHIUM-TOC.md](LITHIUM-TOC.md) - Documentation index
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
- [LITHIUM-TST.md](LITHIUM-TST.md) - Testing guidelines
- [LITHIUM-CSS.md](LITHIUM-CSS.md) - CSS architecture
- [REFACTORING_PLAN.md](../../elements/003-lithium/docs/REFACTORING_PLAN.md) - File splitting guide

---

**Last Updated:** March 2026  
**Applies To:** All Lithium development work
