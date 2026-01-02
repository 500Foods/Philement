# Lithium Project Instructions

## Overview

Lithium is a modular single-page application (SPA) built with modern web technologies. This document provides comprehensive guidance for AI models to be immediately productive with the Lithium codebase.

## Tech Stack

- **JavaScript**: ES6 modules (no bundling required, uses native browser support)
- **Build Tool**: Vite (for development server and asset handling)
- **UI Framework**: Bootstrap 5 (for responsive design and components)
- **Architecture**: Modular SPA with dynamic module loading
- **Authentication**: JWT-based (currently placeholder implementation)
- **PWA**: Progressive Web App with service worker and offline capabilities

## Project Structure

```directory
/elements/003-lithium/
├── index.html              # Main HTML entry point
├── package.json            # Project dependencies and scripts
├── vite.config.js          # Vite build configuration
├── public/                 # Static assets
│   ├── manifest.json       # PWA manifest
│   ├── service-worker.js   # PWA service worker
│   └── assets/             # Images, fonts, etc.
├── src/
│   ├── app.js             # Main application logic and module loader
│   ├── lithium.css        # Global styles
│   ├── core/              # Core system components
│   │   ├── logger/        # Logging utilities
│   │   │   └── logger.js
│   │   ├── network/       # Network utilities
│   │   │   └── network.js
│   │   ├── router/        # Routing utilities
│   │   │   └── router.js
│   │   └── storage/       # Storage utilities
│   │       └── storage.js
│   ├── init/              # Initialization modules and CSS
│   │   ├── tabulator-simple.css  # Simple table styling
│   │   ├── tabulator-full.css   # Advanced table styling
│   │   ├── jsoneditor.css       # JSON editor styling
│   │   ├── tabulator-init.js    # Tabulator initialization
│   │   ├── jsoneditor-init.js   # JSONEditor initialization
│   │   ├── suneditor-init.js    # SunEditor initialization
│   │   ├── flatpickr-init.js    # Flatpickr initialization
│   │   ├── codemirror-init.js   # CodeMirror initialization
│   │   ├── prism-init.js        # Prism.js initialization
│   │   └── highlight-init.js    # Highlight.js initialization
│   ├── editors/            # Editor component folders
│   │   ├── table-simple/    # Simple table editor
│   │   ├── table-full/      # Advanced table editor
│   │   └── json/            # JSON editor
│   └── modules/           # All application modules
│       ├── login/         # Login module
│       │   ├── login.js
│       │   └── login.html
│       ├── main/          # Main menu module
│       │   ├── main.js
│       │   └── main.html
│       ├── dashboard/     # Dashboard module
│       ├── source-editor/ # Source editor module
│       ├── queries/       # Queries module
│       └── lookups/       # Lookups module
└── tests/                  # Test suite
```

## Development Setup

### Prerequisites

- Node.js >= 18.0.0
- npm >= 9.0.0
- Modern browser (Chrome, Firefox, Edge, Safari)

### Installation

```bash
cd elements/003-lithium
npm install
```

### Start Development Server

```bash
cd elements/003-lithium
npm run dev
```

This starts Vite dev server on <http://localhost:3000>

### Available Scripts

- `npm run dev`: Start development server with hot reloading
- `npm run build`: Build production version
- `npm run build:prod`: Production build with minification
- `npm run preview`: Preview production build locally
- `npm run serve`: Serve production build on port 3000
- `npm test`: Run test suite
- `npm run lint`: Run ESLint
- `npm run format`: Format code with Prettier
- `npm run clean`: Clean build artifacts

## Key Architectural Decisions

### 1. Core vs Init Separation

**Core Components** (`src/core/`):

- Essential system utilities (logger, network, router, storage)
- Low-level functionality used throughout the application
- Minimal dependencies, maximum reusability

**Initialization Modules** (`src/init/`):

- Third-party library initialization and configuration
- CSS files for editor components
- Higher-level abstractions for complex libraries
- Singleton pattern for easy access

### 2. Font Strategy

**Default Fonts:**

- **Proportional**: Vanadium Sans (primary), system fonts (fallback)
- **Monospace**: Vanadium Mono (primary), Courier New (fallback)

**CSS Variables:**

```css
:root {
  --font-family: 'Vanadium Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
  --font-mono: 'Vanadium Mono', 'Courier New', Courier, monospace;
}
```

### 3. CSS Organization

**Rationale for Init Folder CSS:**

- CSS files are co-located with their JavaScript initialization modules
- Single load point for all initialization-related assets
- Better organization than scattering CSS across different folders
- Easier to manage dependencies and build processes

### 4. Editor Architecture

**Pattern:** Each editor has:

- JavaScript initialization module (`*-init.js`)
- CSS styling file (`*.css`)
- Dedicated folder in `src/editors/` for future expansion
- Singleton instance for global access
- Comprehensive API for programmatic control

## Build Process Considerations

### Vite Configuration

The current `vite.config.js` handles:

- ES6 module support with native browser imports
- CSS processing with source maps
- Production minification with Terser
- Modern browser targeting

### CSS Handling

**Development:**

- Individual CSS files loaded via `<link>` tags in index.html
- CSS source maps enabled for debugging
- Hot module replacement for styles

**Production:**

- Vite automatically processes and optimizes CSS
- CSS files are extracted and minified
- Proper caching headers applied
- CSS imports are resolved correctly

### CSS Import Notes

The `@import` statement in `tabulator-full.css`:

```css
@import '/src/init/tabulator-simple.css';
```

This works because:

1. Vite processes CSS imports during build
2. The path is relative to the project root
3. Vite resolves and bundles the imported CSS
4. Production build creates proper dependency graph

### Build Optimization Recommendations

1. **CSS Bundling:** Consider using Vite's CSS code splitting for large projects
2. **Critical CSS:** Extract critical CSS for above-the-fold content
3. **PurgeCSS:** Add PurgeCSS to remove unused styles in production
4. **Font Loading:** Consider preloading Vanadium fonts for better performance

## Initialization Module Usage

### Pattern

Each initialization module follows this pattern:

```javascript
// Singleton class
class EditorInit {
  constructor() {
    this.instances = new Map(); // Track instances
    this.defaultOptions = { /* sensible defaults */ };
  }

  // Main initialization method
  initEditor(elementId, content, options = {}) { /* ... */ }

  // Utility methods
  getEditor(elementId) { /* ... */ }
  destroyEditor(elementId) { /* ... */ }
  setDefaultOptions(options) { /* ... */ }
}

// Export singleton instance
export const editorInit = new EditorInit();
```

### Usage Example

```javascript
import { tabulatorInit } from '/src/init/tabulator-init.js';

// Initialize a table
tabulatorInit.initTable('my-table', columns, data);

// Get table instance
const table = tabulatorInit.getTable('my-table');

// Clean up
tabulatorInit.destroyTable('my-table');
```

## FontAwesome Integration

### SunEditor Configuration

FontAwesome is enabled by default in SunEditor:

```javascript
font: ['Vanadium Sans', 'Arial', 'Times New Roman', 'Courier New', 'Georgia', 'Verdana', 'FontAwesome'],
fontAwesome: true
```

### Configuration Method

```javascript
sunEditorInit.configureFontAwesome('editor1', true, ['fa-user', 'fa-envelope', 'fa-phone']);
```

## Future Development Notes

### Planned Enhancements

1. **Dynamic CSS Loading:** Load CSS files only when needed
2. **Theme System:** Create a comprehensive theming system
3. **CSS-in-JS:** Consider CSS-in-JS for component-specific styles
4. **Performance Optimization:** Lazy load non-critical CSS

### Technical Debt

1. **CSS Import Paths:** Consider using `@/` alias for cleaner imports
2. **CSS Variables:** Expand CSS variable usage for better theming
3. **Responsive Design:** Review mobile responsiveness across all editors
4. **Accessibility:** Ensure all editors meet WCAG standards

### Lessons Learned

1. **Organization Matters:** Co-locating related assets improves maintainability
2. **Consistent Patterns:** Singleton pattern works well for initialization modules
3. **Font Strategy:** Default fonts should be practical and widely available
4. **Build Process:** Vite handles CSS imports well, but documentation is key

## Troubleshooting

### CSS Not Loading

1. Check file paths in index.html
2. Verify Vite dev server is running
3. Ensure CSS files exist in correct locations
4. Check browser console for 404 errors

### CSS Import Issues

1. Verify import paths are correct
2. Check Vite build output for CSS processing
3. Ensure no circular dependencies
4. Clear Vite cache if needed

### Font Issues

1. Verify font files exist in `/public/assets/fonts/`
2. Check font-face declarations in CSS
3. Ensure proper font licensing
4. Test fallback fonts

## Additional Resources

- **Vite Documentation**: <https://vitejs.dev/>
- **CSS Modules**: <https://github.com/css-modules/css-modules>
- **Font Loading**: <https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face>
- **CSS Variables**: <https://developer.mozilla.org/en-US/docs/Web/CSS/Using_CSS_custom_properties>