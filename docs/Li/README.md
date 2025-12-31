# Lithium - Progressive Web App Framework

## Overview

Lithium is a modern Progressive Web App (PWA) framework built with vanilla
JavaScript, featuring Single Page Application (SPA) routing, offline
capabilities, and comprehensive testing infrastructure. It serves as the
JavaScript counterpart to the Hydrogen C server, providing a complete web
application development stack.

## Table of Contents

### Getting Started

- [**Introduction**](#lithium---progressive-web-app-framework) - Overview and goals
- [**Features**](#-features) - Complete feature list
- [**Architecture**](#-architecture) - Technical architecture overview
- [**Quick Start**](#-quick-start) - Get up and running quickly
- [**Project Structure**](#-project-structure) - File structure overview

### Core Documentation

- [**PWA Features**](#-pwa-features) - Progressive Web App capabilities
- [**Testing Framework**](#-testing-framework) - Mocha + nyc testing setup
- [**Build System**](#-build-system) - Development and production builds
- [**Build Guide**](/docs/Li/BUILD.md) - Complete build process documentation
- [**Deployment**](#-deployment) - Deployment strategies and best practices

### Development Guides

- [**Development Workflow**](#-development-workflow) - Recommended workflow patterns
- [**Code Style**](#-code-style) - Coding standards and conventions
- [**Testing Guide**](/docs/Li/TESTING.md) - Comprehensive testing documentation

### Technical References

- [**API Reference**](#-api-reference) - JavaScript API documentation
- [**Configuration**](#configuration) - Configuration options and settings
- [**Module Documentation**](#-module-documentation) - Detailed module documentation
- [**Browser Support**](#-browser-support) - Supported browsers and polyfills

### Advanced Topics

- [**Service Worker**](#-service-worker) - Advanced service worker patterns
- [**Offline Strategies**](#-offline-strategies) - Offline data handling
- [**Internationalization**](#-internationalization) - Multi-language support
- [**Accessibility**](#-accessibility) - Accessibility best practices
- [**Performance Monitoring**](#-performance-monitoring) - Monitoring and analytics

## 🚀 Features

### PWA Capabilities

- **Offline Support**: Service worker caching for complete offline functionality
- **Installable**: Can be installed on home screen like native apps
- **Responsive Design**: Mobile-first approach with adaptive layouts
- **Dark Mode**: Automatic dark/light theme detection
- **Performance Optimized**: Caching, preloading, and lazy loading
- **Accessibility**: Reduced motion support and semantic HTML
- **Cross-Platform**: Works on all modern browsers and devices

### Technical Stack

- **Vanilla JavaScript**: No framework dependencies, pure ES6+ JavaScript
- **Modern CSS**: CSS variables, custom properties, and utility classes
- **PWA Standards**: Web App Manifest, Service Workers, Cache API
- **SPA Architecture**: Client-side routing with history API
- **Vanadium & Roboto Fonts**: Self-hosted fonts for better performance
- **Material Design**: Clean, modern UI components

### Testing Framework

- **Mocha**: JavaScript test framework with BDD/TDD support
- **Chai**: BDD/TDD assertion library with expressive syntax
- **NYC**: Istanbul code coverage tool with multiple reporters
- **Sinon**: Standalone test spies, stubs and mocks
- **Sinon-Chai**: Extends Chai with Sinon assertions

### Build System

- **Vite**: Modern build tool with fast HMR and optimized production builds
- **ESLint**: JavaScript linting with Airbnb style guide
- **Prettier**: Consistent code formatting
- **Terser**: Advanced JavaScript minification
- **html-minifier-terser**: HTML minification and optimization
- **Rollup**: Production bundling and code splitting

## 🏗 Architecture

### High-Level Architecture

```text
┌─────────────────────────────────────────────────┐
│                 Lithium PWA Application            │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌───────┐  │
│  │   Router    │    │  Storage    │    │Network│  │
│  └─────────────┘    └─────────────┘    └───────┘  │
│       ▲               ▲               ▲           │
│       │               │               │           │
│  ┌────┴─────┐   ┌─────┴─────┐   ┌─────┴─────┐   │
│  │  SPA      │   │ IndexedDB │   │  API     │   │
│  │ Routing   │   │  Storage  │   │ Clients  │   │
│  └────┬─────┘   └─────┬─────┘   └─────┬─────┘   │
│       │               │               │           │
│  ┌────┴───────────────┴───────────────┴─────┐  │
│  │                Core Application              │  │
│  └───────────────────────────────────────────┘  │
│                                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌───────┐  │
│  │  Logger     │    │  Utilities  │    │ Config │  │
│  └─────────────┘    └─────────────┘    └───────┘  │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Module Architecture

Lithium follows a modular architecture with clear separation of concerns:

1. **Core Module**: Main application orchestration
2. **Router Module**: SPA routing and navigation
3. **Storage Module**: Data persistence (IndexedDB, localStorage)
4. **Network Module**: API communication and offline handling
5. **Logger Module**: Comprehensive logging system
6. **Utility Modules**: Reusable utility functions

### Data Flow

```text
User Interaction → Router → Core → Modules → Storage/Network → Core → Router → UI
```

## 🚀 Quick Start

### Prerequisites

- Node.js 18+ (recommended LTS version)
- npm 9+
- Modern browser (Chrome 90+, Firefox 88+, Safari 14+)

### Installation

```bash
# Clone the repository (if not already in Philement)
git clone https://github.com/philement/lithium-pwa.git
cd lithium-pwa

# Install dependencies
npm install

# Install test dependencies
cd tests && npm install
cd ..
```

### Development

```bash
# Start development server
npm run dev

# Run tests
npm test

# Run tests with coverage
npm run test:coverage
```

### Production

```bash
# Build for production
npm run build:prod

# Preview production build
npm run preview

# Deploy to server
npm run deploy
```

## 📋 Project Structure

```text
elements/003-lithium/
├── public/                 # Static assets (copied to dist/)
│   ├── assets/             # Images, fonts, icons
│   ├── favicon.ico         # Favicon
│   ├── manifest.json       # PWA manifest
│   └── service-worker.js   # Service worker
├── src/                    # Source code (ES6 modules)
│   ├── app.js              # Main application entry point
│   ├── lithium.css         # Application styles (processed by Vite)
│   ├── router/             # SPA routing module
│   │   └── router.js       # Router implementation
│   ├── storage/            # Data storage module
│   │   └── storage.js      # IndexedDB & localStorage
│   ├── network/            # Network module
│   │   └── network.js      # API & offline handling
│   └── utils/              # Utility modules
│       └── logger.js       # Logging utility
├── tests/                  # Test suite
│   ├── package.json        # Test dependencies & config
│   ├── test-runner.js      # Test orchestration script
│   └── tests/              # Test files
│       ├── app.test.js     # Main app tests
│       ├── router.test.js  # Router tests
│       └── ...             # Additional test files
├── dist/                   # Production build output
│   ├── index.html          # Minified HTML
│   ├── assets/             # Bundled & optimized assets
│   │   ├── index-[hash].js     # Minified JS bundle
│   │   ├── index-[hash].css    # Minified CSS bundle
│   │   └── ...                  # Fonts, images, icons
│   ├── manifest.json       # PWA manifest
│   ├── service-worker.js   # Minified service worker
│   └── favicon.ico         # Favicon
├── index.html              # Development HTML entry point
├── package.json            # Project dependencies & scripts
├── vite.config.js          # Vite build configuration
└── README.md               # Brief project overview
```

## 🧪 Testing Framework

Lithium uses a comprehensive testing approach inspired by the Hydrogen project:

### Test Organization

```text
tests/
├── package.json            # Test dependencies and configuration
├── test-runner.js          # Test orchestration and reporting
└── tests/                  # Test files
    ├── app.test.js         # Main application tests
    ├── router.test.js      # Router module tests
    ├── storage.test.js     # Storage module tests
    ├── network.test.js     # Network module tests
    └── utils.test.js        # Utility function tests
```

### Test Configuration

**package.json** includes Mocha and NYC configuration:

```json
{
  "scripts": {
    "test": "mocha --recursive --exit",
    "test:watch": "mocha --watch --recursive",
    "test:coverage": "nyc --reporter=html --reporter=text mocha --recursive --exit"
  },
  "nyc": {
    "include": ["../src/**/*.js"],
    "exclude": ["**/tests/**", "**/dist/**"],
    "reporter": ["html", "text", "lcov"],
    "all": true,
    "check-coverage": true,
    "statements": 80,
    "branches": 80,
    "functions": 80,
    "lines": 80
  }
}
```

### Running Tests

```bash
# Run all tests
npm test

# Run tests with watch mode
npm run test:watch

# Run tests with coverage analysis
npm run test:coverage

# Generate coverage reports
npm run test:report
```

### Test Coverage

Lithium enforces minimum coverage thresholds:

- **Statements**: 80% minimum
- **Branches**: 80% minimum
- **Functions**: 80% minimum
- **Lines**: 80% minimum

Coverage reports are generated in:

- **HTML**: Interactive report in `tests/coverage/`
- **Text**: Console output
- **LCov**: For integration with other tools

For complete testing documentation, see [**TESTING.md**](/docs/Li/TESTING.md).

## 🏗 Build System

### Development Build

```bash
# Start development server with hot reload
npm run dev

# Build development version
npm run build
```

### Production Build

```bash
# Build optimized production version
npm run build:prod

# Preview production build locally
npm run preview

# Clean build artifacts
npm run clean
```

### Build Configuration

The build system uses Vite with comprehensive optimization:

- **Development**: Fast HMR, source maps, debug mode
- **Production**: Advanced minification, tree-shaking, code splitting
- **HTML Minification**: Removes comments, collapses whitespace
- **CSS Processing**: PostCSS, minification, and bundling
- **JavaScript**: Terser minification with compression and mangling
- **Asset Optimization**: Image compression, font optimization
- **Service Worker**: Minified for production deployment

**Production Build Process:**

1. Vite bundles and optimizes source code
2. HTML minifier processes `index.html`
3. Terser minifies `service-worker.js`
4. All assets copied with cache-busting hashes

## 📊 PWA Features

### Web App Manifest

```json
{
  "name": "Lithium PWA",
  "short_name": "Lithium",
  "description": "A modern Progressive Web App",
  "start_url": ".",
  "display": "standalone",
  "background_color": "#ffffff",
  "theme_color": "#3f51b5",
  "icons": [
    {"src": "assets/icons/icon-72x72.png", "sizes": "72x72"},
    {"src": "assets/icons/icon-192x192.png", "sizes": "192x192"},
    {"src": "assets/icons/icon-512x512.png", "sizes": "512x512"}
  ]
}
```

### Service Worker

The service worker provides:

- **Caching**: Cache-first strategy for static assets
- **Offline Support**: Full app functionality without internet
- **Background Sync**: Sync data when connection restored
- **Push Notifications**: Ready for notification support

### SPA Routing

Client-side routing with:

- **History API**: Browser history integration
- **Route Handlers**: Multiple page routes
- **Smooth Transitions**: Animated page changes
- **Error Handling**: 404 page for invalid routes

## 🔄 Development Workflow

### Feature Development

```bash
# Create feature branch
git checkout -b feature/new-feature

# Implement feature
# Add tests

# Run tests
npm test

# Commit
git add .
git commit -m "Add feature with tests"
```

### Bug Fixing

```bash
# Create bug fix branch
git checkout -b fix/bug-description

# Fix bug
# Add regression tests

# Verify fix
npm test

# Commit
git add .
git commit -m "Fix bug with tests"
```

### Code Review

```bash
# Run full test suite
npm run test:all

# Check code style
npm run lint

# Format code
npm run format

# Push for review
git push origin feature/new-feature
```

## 📝 Code Style

### JavaScript Standards

- **ESLint**: Airbnb JavaScript Style Guide
- **Prettier**: Consistent code formatting
- **EditorConfig**: Consistent editor settings

### CSS Standards

- **CSS Variables**: For theming and consistency
- **BEM Methodology**: Block-Element-Modifier naming
- **Mobile-First**: Responsive design approach

### HTML Standards

- **Semantic HTML**: Proper use of HTML5 elements
- **Accessibility**: ARIA attributes and best practices
- **Validation**: W3C compliant markup

## 🚀 Deployment

### Deployment Options

```bash
# Build and deploy
npm run deploy

# Manual deployment
npm run build:prod
rsync -avz dist/ user@server:/var/www/lithium/
```

### Server Configuration

- **HTTPS Required**: PWA features require HTTPS
- **Service Worker**: Configure proper headers
- **Caching**: Set appropriate cache headers
- **Compression**: Enable GZIP/Brotli compression

## 📚 Additional Documentation

- [**Build Guide**](/docs/Li/BUILD.md) - Complete build process and deployment
- [**Testing Guide**](/docs/Li/TESTING.md) - Comprehensive testing documentation
- [**API Reference**](#-api-reference) - JavaScript API documentation
- [**Configuration Guide**](#configuration) - Configuration options
- [**Performance Guide**](#-performance-optimization) - Optimization techniques

## 🔌 API Reference

### Core Application API

#### `LithiumApp`

Main application class that orchestrates all modules.

```javascript
import { LithiumApp } from './src/app.js';

const app = new LithiumApp();
await app.init();
```

**Methods:**

- `init()`: Initialize the application
- `destroy()`: Clean up resources

### Router Module

#### `Router`

Handles client-side routing and navigation.

```javascript
import { Router } from './src/router/router.js';

const router = new Router();
router.addRoute('/', () => console.log('Home'));
router.navigate('/page');
```

**Methods:**

- `addRoute(path, handler)`: Add a route handler
- `navigate(path)`: Navigate to a path
- `getCurrentPath()`: Get current route

### Storage Module

#### `Storage`

Provides data persistence using IndexedDB and localStorage.

```javascript
import { Storage } from './src/storage/storage.js';

const storage = new Storage();
await storage.set('key', 'value');
const data = await storage.get('key');
```

**Methods:**

- `set(key, value)`: Store data
- `get(key)`: Retrieve data
- `remove(key)`: Delete data
- `clear()`: Clear all data

### Network Module

#### `Network`

Handles API communication and offline functionality.

```javascript
import { Network } from './src/network/network.js';

const network = new Network();
const response = await network.fetch('/api/data');
```

**Methods:**

- `fetch(url, options)`: Make HTTP request
- `isOnline()`: Check connectivity status

### Logger Module

#### `Logger`

Comprehensive logging utility.

```javascript
import { Logger } from './src/utils/logger.js';

const logger = new Logger('ModuleName');
logger.info('Message');
logger.error('Error occurred');
```

**Methods:**

- `debug(message)`: Debug level logging
- `info(message)`: Info level logging
- `warn(message)`: Warning level logging
- `error(message)`: Error level logging

## Configuration

### Environment Variables

Lithium supports configuration through environment variables:

- `NODE_ENV`: Environment mode (`development`, `production`)
- `API_BASE_URL`: Base URL for API calls
- `LOG_LEVEL`: Logging level (`debug`, `info`, `warn`, `error`)

### Vite Configuration

Configuration is handled through `vite.config.js`:

```javascript
export default defineConfig({
  // Build configuration
  build: {
    outDir: 'dist',
    minify: true
  },
  // Development server
  server: {
    port: 3000,
    host: true
  }
});
```

### PWA Manifest

The web app manifest is configured in `public/manifest.json`:

```json
{
  "name": "Lithium PWA",
  "short_name": "Lithium",
  "start_url": "/",
  "display": "standalone",
  "theme_color": "#3f51b5"
}
```

## 📚 Module Documentation

### Core Module (`app.js`)

The main application orchestrator that initializes all other modules and
manages the application lifecycle.

**Responsibilities:**

- Initialize router, storage, network, and logger modules
- Handle application startup and shutdown
- Coordinate inter-module communication

### Router Module (`router/router.js`)

Implements client-side routing using the History API.

**Features:**

- Route registration and matching
- Browser history integration
- Programmatic navigation
- Route parameters and query strings

### Storage Module (`storage/storage.js`)

Provides persistent data storage with IndexedDB as primary storage and
localStorage as fallback.

**Features:**

- Asynchronous data operations
- Automatic fallback to localStorage
- Data serialization/deserialization
- Storage quota management

### Network Module (`network/network.js`)

Handles all network communication and offline functionality.

**Features:**

- Fetch API wrapper
- Offline detection
- Request queuing for offline scenarios
- Error handling and retries

### Logger Module (`utils/logger.js`)

Comprehensive logging system with multiple output targets.

**Features:**

- Multiple log levels
- Console and file output
- Log filtering and formatting
- Performance timing

## 🌐 Browser Support

Lithium supports all modern browsers:

### Supported Browsers

- **Chrome**: 90+
- **Firefox**: 88+
- **Safari**: 14+
- **Edge**: 90+

### Required Features

- ES6 Modules
- Async/Await
- Fetch API
- IndexedDB
- Service Workers
- Web App Manifest

### Polyfills

For older browsers, consider adding polyfills for:

- `core-js` for ES6+ features
- `whatwg-fetch` for Fetch API
- `indexeddbshim` for IndexedDB

## 🔧 Service Worker

### Service Worker Implementation

The service worker (`public/service-worker.js`) provides offline
functionality and caching.

**Features:**

- Cache-first strategy for static assets
- Runtime caching for API calls
- Background sync for offline actions
- Push notification support

### Cache Strategy

```javascript
// Cache static assets
workbox.routing.registerRoute(
  /\.(?:js|css|png|jpg|svg)$/,
  new workbox.strategies.CacheFirst()
);

// Network-first for API calls
workbox.routing.registerRoute(
  /\/api\//,
  new workbox.strategies.NetworkFirst()
);
```

## 📱 Offline Strategies

### Data Synchronization

Lithium implements several offline strategies:

1. **Optimistic Updates**: UI updates immediately, syncs in background
2. **Conflict Resolution**: Handles data conflicts on reconnection
3. **Queue Management**: Queues requests when offline

### Storage Strategy

- **IndexedDB**: Primary storage for structured data
- **localStorage**: Fallback for simple key-value data
- **Cache API**: Service worker caching for assets

## 🌍 Internationalization

### i18n Support

Lithium provides basic internationalization support:

```javascript
// Language files
const messages = {
  en: {
    welcome: 'Welcome',
    goodbye: 'Goodbye'
  },
  es: {
    welcome: 'Bienvenido',
    goodbye: 'Adiós'
  }
};

// Usage
const t = (key) => messages[currentLang][key];
console.log(t('welcome')); // 'Welcome' or 'Bienvenido'
```

### RTL Support

For right-to-left languages, add CSS:

```css
[dir="rtl"] {
  text-align: right;
}

[dir="rtl"] .component {
  /* RTL-specific styles */
}
```

## ♿ Accessibility

### Accessibility Features

Lithium follows WCAG 2.1 guidelines:

- **Semantic HTML**: Proper use of headings, landmarks, and ARIA
- **Keyboard Navigation**: Full keyboard accessibility
- **Screen Reader Support**: ARIA labels and descriptions
- **Color Contrast**: High contrast ratios
- **Focus Management**: Visible focus indicators

### ARIA Implementation

```html
<!-- Screen reader announcements -->
<div aria-live="polite" aria-atomic="true">
  Status updates appear here
</div>

<!-- Form accessibility -->
<label for="email">Email Address</label>
<input id="email" type="email" aria-describedby="email-help">
<div id="email-help">We'll never share your email.</div>
```

## 📈 Performance Monitoring

### Performance Metrics

Lithium tracks key performance metrics:

- **Core Web Vitals**: LCP, FID, CLS
- **Bundle Size**: JavaScript and CSS bundle sizes
- **Load Times**: Time to interactive
- **Cache Hit Rates**: Service worker cache effectiveness

### Monitoring Tools

```javascript
// Performance observer
const observer = new PerformanceObserver((list) => {
  for (const entry of list.getEntries()) {
    console.log(entry.name, entry.duration);
  }
});

observer.observe({ entryTypes: ['measure'] });

// Mark and measure
performance.mark('start');
performance.mark('end');
performance.measure('operation', 'start', 'end');
```

## ⚡ Performance Optimization

### Bundle Optimization

- **Code Splitting**: Dynamic imports for route-based splitting
- **Tree Shaking**: Remove unused code
- **Minification**: Terser for JavaScript, CSS minification
- **Compression**: Gzip/Brotli compression

### Runtime Optimization

- **Lazy Loading**: Components loaded on demand
- **Image Optimization**: Responsive images with WebP
- **Caching**: Aggressive caching strategies
- **Preloading**: Critical resources preloaded

### Memory Management

- **Event Listener Cleanup**: Proper cleanup to prevent leaks
- **Object Pooling**: Reuse objects to reduce GC pressure
- **Debouncing**: Limit frequent operations

## 🔒 Security

### Security Best Practices

Lithium implements several security measures:

- **HTTPS Only**: All communications over secure connections
- **CSP Headers**: Content Security Policy
- **Input Validation**: Sanitize all user inputs
- **XSS Protection**: Prevent cross-site scripting
- **CSRF Protection**: Token-based protection

### Secure Headers

```nginx
# Security headers
add_header X-Frame-Options "SAMEORIGIN";
add_header X-Content-Type-Options "nosniff";
add_header X-XSS-Protection "1; mode=block";
add_header Strict-Transport-Security "max-age=31536000";
```

## 📝 License

MIT License - Free to use, modify, and distribute.

## 🎉 Ready for Development

Lithium provides a solid foundation for building modern Progressive Web Apps
with comprehensive testing and development tooling.
