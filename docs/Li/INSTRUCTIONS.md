# Lithium Project Instructions

## Overview

Lithium is a modular single-page application (SPA) built with modern web technologies. It features a login system, main menu, and multiple sub-modules for different functionalities. This document provides comprehensive guidance for AI models to be immediately productive with the Lithium codebase.

## Tech Stack

- **JavaScript**: ES6 modules (no bundling required, uses native browser support)
- **Build Tool**: Vite (for development server and asset handling)
- **UI Framework**: Bootstrap 5 (for responsive design and components)
- **Architecture**: Modular SPA with dynamic module loading
- **Authentication**: JWT-based (currently placeholder implementation)
- **PWA**: Progressive Web App with service worker and offline capabilities

## Project Structure

```directory
/docs/Li/                   # Project documentation
├── README.md               # Main HTML TOC
├── TESTING.md              # Info on tests with Mocha and nyc
├── BUILD.md                # What building the project involves
└── INSTRUCTIONS.md         # This file
/elements/003-lithium/      # Project source
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
│   ├── network/           # Network utilities
│   ├── router/            # Routing utilities
│   ├── storage/           # Storage utilities
│   ├── utils/             # Utility functions
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

## Module System

### Structure

- Each module has a `.js` and `.html` file in its own directory
- Modules are loaded dynamically using ES6 `import()` with Vite's asset handling
- Each module exports a default class with an `init()` function that receives the app instance

### Loading Process

1. App checks for JWT in localStorage
2. If valid JWT: Load main menu module
3. If no JWT: Load login module
4. Modules inject their HTML into containers and manage their own DOM
5. Navigation switches between modules by hiding/showing containers

### Module Class Structure

```javascript
class ModuleName {
  constructor(app, container) {
    this.app = app;          // Reference to main app instance
    this.container = container; // DOM container for this module
    this.logger = app.logger; // Access to logging system
  }

  async init() {
    // Load HTML template
    this.container.innerHTML = htmlTemplate;
    
    // Set up event listeners
    this.setupEventListeners();
  }

  setupEventListeners() {
    // Attach DOM event handlers here
  }

  destroy() {
    // Clean up resources if needed
  }
}

export default ModuleName;
```

## Key Files and Components

### `index.html`

- Minimal HTML with Bootstrap CSS/JS and empty `#app` div
- Includes PWA manifest and service worker registration
- Loads all third-party libraries via CDN
- Main app script loaded as ES module

### `src/app.js`

Contains `LithiumApp` class with:

- JWT authentication check and management
- Module loading system with dynamic imports
- Logger and network services (placeholder)
- App initialization and lifecycle management
- PWA event handlers (install prompts, online/offline detection)

### `vite.config.js`

- Base path configuration for production
- Build options with source maps and minification
- Development server on port 3000
- ES module support and modern browser targeting

## Coding Guidelines

### JavaScript Standards

- **ES6 Modules**: Use `import`/`export` syntax
- **Async/Await**: Preferred for asynchronous operations
- **DOM Manipulation**: Use modern APIs (`querySelector`, `addEventListener`)
- **Event Handling**: Attach events in module `init()` functions
- **Error Handling**: Use try/catch with meaningful error messages

### Code Organization

- **Styling**: Use Bootstrap classes, custom styles in `lithium.css`
- **Comments**: Extensive comments for future development planning
- **File Structure**: Keep related HTML and JS files together in module directories
- **Naming**: Use descriptive names for functions and variables

### Best Practices

- **Modularity**: Keep modules self-contained and focused
- **Reusability**: Create utility functions in `src/utils/` for shared logic
- **Performance**: Use lazy loading for heavy dependencies
- **Accessibility**: Follow WCAG guidelines for UI components
- **Security**: Sanitize user input, validate data

## Authentication Flow

1. **App Initialization**: `LithiumApp` checks for JWT in localStorage
2. **JWT Validation**: If valid JWT exists, load main menu
3. **Login Process**: If no valid JWT, load login module
4. **Login Success**: Store fake JWT, transition to main menu
5. **Main Menu**: Sidebar with buttons to load sub-modules
6. **Logout**: Clear JWT from localStorage, return to login

### JWT Management Methods

```javascript
// Get JWT from localStorage
app.getJWT()

// Set JWT in localStorage
app.setJWT(token)

// Clear JWT from localStorage
app.clearJWT()

// Validate JWT (placeholder implementation)
app.isValidJWT(token)
```

## Module Development

### Creating New Modules

1. **Create Directory**: Add new directory in `src/modules/`
2. **Add Files**: Create `module-name.js` and `module-name.html`
3. **Export Class**: Export default class with `init()` method
4. **HTML Template**: Use template string or load via Vite
5. **Services**: Use `app.logger` and `app.network` for services
6. **Navigation**: Add button to main menu if needed

### Module Lifecycle

1. **Construction**: Module class instantiated with app reference
2. **Initialization**: `init()` called after DOM injection
3. **Event Setup**: Event listeners attached in `init()`
4. **Active Use**: Module handles user interactions
5. **Cleanup**: `destroy()` called when module unloaded

### Current Modules

- **login**: Username/password form, dummy authentication
- **main**: Sidebar menu with navigation buttons
- **dashboard**: App info and status display
- **source-editor**: Placeholder for code editing
- **queries**: Placeholder for database queries
- **lookups**: Placeholder for data reference

## Testing

### Manual Testing

- Run development server: `npm run dev`
- Open browser at <http://localhost:3000>
- Check console for errors
- Test module switching and authentication flow
- Verify PWA installation and offline functionality

### Automated Testing

- Run test suite: `npm test`
- Check test coverage: `npm run test:coverage`
- Run linter: `npm run lint`
- Fix linting issues: `npm run lint:fix`
- Format code: `npm run format`

## Build and Deployment

### Production Build

```bash
npm run build:prod
```

This creates optimized production build in `dist/` directory with:

- Minified JavaScript and CSS
- Compressed HTML
- Optimized assets
- Source maps for debugging

### Deployment

```bash
npm run deploy
```

Deploys production build to configured server (edit package.json for your setup)

## PWA Features

### Service Worker

- Automatically registered in `index.html`
- Handles offline caching and asset management
- Provides offline-first experience

### Manifest

- Defined in `public/manifest.json`
- Configures app name, icons, theme colors
- Enables install prompts and home screen icons

### Offline Support

- Assets cached by service worker
- Fallback strategies for network failures
- Online/offline detection with UI feedback

## Development Tools

### Third-Party Libraries

- **Bootstrap 5**: UI components and responsive design
- **Font Awesome**: Icon library
- **SheetJS**: Excel file processing
- **Interact.js**: Drag and drop interactions
- **SimpleBar**: Custom scrollbars
- **Tabulator**: Interactive tables
- **Luxon**: Date/time handling
- **Prism.js**: Code syntax highlighting
- **Highlight.js**: Alternative syntax highlighting
- **KaTeX**: Math rendering
- **CodeMirror**: Code editor
- **SunEditor**: Rich text editor

### Development Utilities

- **ESLint**: JavaScript linting
- **Prettier**: Code formatting
- **HTML Minifier**: HTML compression
- **Terser**: JavaScript minification
- **Vite**: Build tool and development server

## AI-Specific Information

### For AI Models Working on Lithium

1. **Code Patterns**: Follow existing module structure and naming conventions
2. **File Organization**: Keep related files together in module directories
3. **API Usage**: Use `app.logger` for logging, `app.network` for network operations
4. **Error Handling**: Implement robust error handling with user feedback
5. **Documentation**: Add comments explaining complex logic and future plans
6. **Tool Integration**: When using development tools (search, edit, run commands), ensure changes align with project structure and coding standards
7. **Incremental Development**: Make small, testable changes and verify functionality before proceeding to complex features
8. **Dependency Management**: Check `package.json` for existing dependencies before suggesting new ones; prefer lightweight, well-maintained packages
9. **Security Awareness**: Implement input validation, avoid XSS vulnerabilities, and follow secure coding practices for authentication
10. **Performance Considerations**: Optimize DOM manipulation, use efficient event handling, and consider lazy loading for module assets

### Collaboration Guidelines for AI Partners

To maximize effectiveness as a development partner:

- **Proactive Analysis**: Before making changes, analyze the codebase using search tools to understand existing patterns and dependencies
- **Clear Communication**: Explain proposed changes, including rationale and potential impacts
- **Iterative Refinement**: Be prepared to iterate on implementations based on feedback and testing results
- **Quality Assurance**: Run tests and linting after changes; suggest improvements to test coverage
- **Documentation Updates**: Update relevant documentation (READMEs, comments) when making structural changes
- **Version Control**: Commit changes with descriptive messages; suggest branching strategies for feature development

### Common Development Tasks

**Adding New Module**:

1. Create module directory in `src/modules/`
2. Add JS class and HTML template
3. Export default class with `init()` method
4. Add navigation button to main menu
5. Update module documentation in the module's README.md

**Extending Existing Module**:

1. Locate module in `src/modules/`
2. Review existing code and README.md for context
3. Add new functionality to JS class
4. Update HTML template if needed
5. Test in development environment
6. Update module documentation

**Adding Utility Function**:

1. Create new file in `src/utils/`
2. Export reusable function with JSDoc comments
3. Import and use in modules as needed
4. Add unit tests if applicable

**Integrating Third-Party Libraries**:

1. Check if library is already included in `package.json` or CDN links
2. If adding new dependency, ensure it supports ES6 modules and modern browsers
3. Update import statements and verify compatibility with Vite build process
4. Test integration thoroughly, especially PWA functionality

## Future Development Notes

### Planned Enhancements

- Replace placeholder JWT with real authentication
- Implement actual functionality in placeholder modules
- Add routing if needed (currently all client-side)
- Consider state management for complex interactions
- Add unit tests when functionality expands

### Technical Debt

- JWT validation needs proper implementation
- Module error handling could be enhanced
- Testing coverage needs expansion
- Documentation for new modules required

### Performance Considerations

- Optimize module loading for large applications
- Implement code splitting for better caching
- Consider lazy loading for non-critical modules
- Monitor bundle size and optimize dependencies

## Troubleshooting

### Common Issues

**Module Loading Failures**:

- Check module path in `app.loadModule()`
- Verify module exports default class
- Ensure HTML template is properly loaded

**Authentication Problems**:

- Check JWT storage and retrieval
- Verify JWT validation logic
- Test login/logout flow

**Build Errors**:

- Run `npm install` to ensure dependencies
- Check Vite configuration
- Verify ES module syntax

**PWA Issues**:

- Check service worker registration
- Verify manifest configuration
- Test offline functionality

## Additional Resources

- **Bootstrap Documentation**: <https://getbootstrap.com/docs/5.3/>
- **Vite Documentation**: <https://vitejs.dev/>
- **ES6 Modules Guide**: <https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Modules>
- **PWA Guide**: <https://web.dev/progressive-web-apps/>
- **JWT Specification**: <https://jwt.io/>

## Getting Help

For questions or issues:

- Check existing documentation in `docs/`
- Review similar modules for patterns
- Consult Vite and Bootstrap documentation
- Ask specific questions about implementation details