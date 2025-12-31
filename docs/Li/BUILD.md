# Lithium Build System

## Overview

Lithium uses a modern build system based on Vite with comprehensive
optimization for production deployments. The build process transforms ES6
modules, processes CSS, minifies all assets, and prepares the application for
deployment.

## Table of Contents

- [**Build Process**](#-build-process) - How the build system works
- [**Build Commands**](#-build-commands) - Available npm scripts
- [**Development Workflow**](#-development-workflow) - Development vs production builds
- [**Build Output**](#-build-output) - What gets generated
- [**Optimization Features**](#-optimization-features) - Build optimization
- [**Deployment**](#-deployment) - How to deploy the built application
- [**Troubleshooting**](#-troubleshooting) - Common build issues

## 🏗 Build Process

### Architecture

```text
Source Code (src/) → Vite Build System → dist/ (Production Ready)
     ↓                    ↓                    ↓
  ES6 Modules      Bundling & Minification   Static Files
  CSS Files        Tree Shaking             Cache Busting
  Static Assets    Code Splitting           Optimized Assets
```

### Build Pipeline

1. **Source Processing**: ES6 modules are bundled and transpiled
2. **CSS Processing**: CSS is processed, minified, and bundled
3. **Asset Optimization**: Images, fonts, and other assets are optimized
4. **HTML Minification**: `index.html` is minified and optimized
5. **Service Worker**: `service-worker.js` is minified
6. **Cache Busting**: Assets get hashed filenames for cache invalidation

## 🚀 Build Commands

### Development Commands

```bash
# Start development server with hot reload
npm run dev

# Start development server on specific port
npm run dev -- --port 3001

# Build development version (no minification)
npm run build
```

### Production Commands

```bash
# Build optimized production version
npm run build:prod

# Preview production build locally
npm run preview

# Clean build artifacts
npm run clean
```

### Testing Commands

```bash
# Run tests
npm test

# Run tests with coverage
npm run test:coverage

# Run all tests (including build verification)
npm run test:all
```

## 🔄 Development Workflow

### Development Mode

```bash
npm run dev
```

**Features:**

- Hot Module Replacement (HMR)
- Source maps for debugging
- Fast rebuilds
- Development server on `http://localhost:3000`
- No minification (readable code)

### Production Mode

```bash
npm run build:prod
```

**Features:**

- Full minification and optimization
- Code splitting and tree shaking
- Asset optimization
- Cache busting with hashed filenames
- Production-ready static files

## 📦 Build Output

### Directory Structure

```text
dist/
├── index.html              # Minified HTML (9.1K)
├── manifest.json           # PWA manifest (2.2K)
├── service-worker.js       # Minified service worker (2.9K)
├── favicon.ico             # Favicon (0K)
└── assets/
    ├── index-[hash].js     # Bundled JS (1.16K → 0.52K gzipped)
    ├── index-[hash].css    # Bundled CSS (5.93K → 1.69K gzipped)
    ├── vendor-[hash].js    # Vendor chunks (0.05K)
    └── [other assets]      # Fonts, images, icons
```

### File Size Optimization

| File | Development | Production | Savings |
| ------ | ------------- | ------------ | --------- |
| `index.html` | 11.59K | 9.1K | 22% |
| `app.js` | ~100K | 1.16K | 99% |
| `lithium.css` | ~6K | 5.93K* | Bundled |
| Total | ~117K | ~18K | 85% |

*CSS size includes all styles; gzipped compression provides additional 70% savings.

## ⚡ Optimization Features

### JavaScript Optimization

- **Terser Minification**: Advanced compression and mangling
- **Tree Shaking**: Removes unused code
- **Code Splitting**: Splits code into optimized chunks
- **ES6 Transpilation**: Converts modern JS for browser compatibility

### CSS Optimization

- **PostCSS Processing**: Autoprefixer, CSS variables
- **Minification**: Removes whitespace and comments
- **Bundling**: Combines all CSS into single file
- **Source Maps**: For debugging in development

### HTML Optimization

- **html-minifier-terser**: Removes comments and whitespace
- **Inline Minification**: Minifies inline CSS and JavaScript
- **Attribute Optimization**: Removes redundant attributes
- **URL Minification**: Optimizes URLs

### Asset Optimization

- **Font Loading**: Self-hosted fonts with preload hints
- **Image Optimization**: Compression and format optimization
- **Cache Busting**: Hashed filenames for long-term caching
- **Gzip Compression**: Server-side compression ready

## 🚀 Deployment

### Automated Deployment

```bash
# Build and deploy (configured in package.json)
npm run deploy
```

### Manual Deployment

```bash
# 1. Build the application
npm run build:prod

# 2. Verify build output
ls -la dist/

# 3. Deploy to web server
rsync -avz dist/ user@server:/var/www/lithium/

# Or using SCP
scp -r dist/* user@server:/var/www/lithium/
```

### Server Configuration

#### Nginx Configuration

```nginx
server {
    listen 80;
    server_name your-domain.com;
    root /var/www/lithium;
    index index.html;

    # Enable gzip compression
    gzip on;
    gzip_types text/css application/javascript text/javascript application/json;

    # Cache static assets
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }

    # Handle SPA routing
    location / {
        try_files $uri $uri/ /index.html;
    }

    # Security headers for PWA
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;
}
```

#### Apache Configuration

```apache
<VirtualHost *:80>
    ServerName your-domain.com
    DocumentRoot /var/www/lithium

    # Enable compression
    <IfModule mod_deflate.c>
        AddOutputFilterByType DEFLATE text/plain
        AddOutputFilterByType DEFLATE text/html
        AddOutputFilterByType DEFLATE text/xml
        AddOutputFilterByType DEFLATE text/css
        AddOutputFilterByType DEFLATE application/xml
        AddOutputFilterByType DEFLATE application/xhtml+xml
        AddOutputFilterByType DEFLATE application/rss+xml
        AddOutputFilterByType DEFLATE application/javascript
        AddOutputFilterByType DEFLATE application/x-javascript
    </IfModule>

    # Cache headers
    <FilesMatch "\.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$">
        Header set Cache-Control "max-age=31536000, public"
    </FilesMatch>

    # SPA fallback
    <Directory /var/www/lithium>
        FallbackResource /index.html
    </Directory>
</VirtualHost>
```

### CDN Deployment

For CDN deployment, upload the `dist/` contents to your CDN provider:

```bash
# Upload to AWS S3
aws s3 sync dist/ s3://your-bucket-name --delete

# Upload to Cloudflare R2
rclone sync dist/ r2:bucket-name
```

## 🔧 Troubleshooting

### Build Fails

**Issue:** `vite: command not found`

```bash
# Solution: Install dependencies
npm install
```

**Issue:** ESLint errors blocking build

```bash
# Fix linting issues
npm run lint:fix

# Or skip linting for build
npm run build -- --no-eslint
```

### Large Bundle Size

**Check bundle analyzer:**

```bash
npm install --save-dev rollup-plugin-visualizer
npm run build -- --mode analyze
```

**Common solutions:**

- Check for unused imports
- Use dynamic imports for code splitting
- Optimize images and assets
- Remove unused dependencies

### Development Server Issues

**Port already in use:**

```bash
# Use different port
npm run dev -- --port 3001
```

**CORS issues:**

```bash
# Configure Vite proxy in vite.config.js
export default defineConfig({
  server: {
    proxy: {
      '/api': 'http://localhost:3001'
    }
  }
})
```

### Production Issues

**Service worker not registering:**

- Ensure `service-worker.js` is in the root of `dist/`
- Check that the PWA is served over HTTPS
- Verify service worker scope in browser dev tools

**Assets not loading:**

- Check that asset paths are correct in `dist/`
- Verify that the web server serves static files correctly
- Check browser network tab for 404 errors

### Performance Issues

**Slow builds:**

- Use `npm run build` for development (no minification)
- Only use `npm run build:prod` for final builds
- Consider using a faster machine or CI/CD

**Large initial load:**

- Implement code splitting
- Use lazy loading for routes
- Optimize images and fonts
- Enable gzip/brotli compression on server

## 📊 Build Metrics

### Performance Budget

- **First Contentful Paint**: < 1.5s
- **Largest Contentful Paint**: < 2.5s
- **First Input Delay**: < 100ms
- **Cumulative Layout Shift**: < 0.1

### Bundle Analysis

```bash
# Analyze bundle size
npm install --save-dev webpack-bundle-analyzer
npx vite-bundle-analyzer dist
```

### Lighthouse Scores

Target scores for production:

- **Performance**: > 90
- **Accessibility**: > 90
- **Best Practices**: > 90
- **SEO**: > 90
- **PWA**: > 90

## 🎯 Best Practices

### Pre-deployment Checklist

- [ ] Run `npm run build:prod` successfully
- [ ] Test `npm run preview` locally
- [ ] Verify all routes work in production build
- [ ] Check PWA installation works
- [ ] Test offline functionality
- [ ] Validate with Lighthouse
- [ ] Check console for errors

### Continuous Integration

```yaml
# .github/workflows/deploy.yml
name: Deploy
on:
  push:
    branches: [ main ]
jobs:
  build-and-deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: 18
      - run: npm ci
      - run: npm run build:prod
      - run: npm run test:coverage
      - name: Deploy to server
        run: rsync -avz dist/ user@server:/var/www/lithium/
```

## 📝 License

MIT License - Free to use, modify, and distribute.

## 🎉 Build Ready

The Lithium build system provides enterprise-grade optimization for modern web
applications. Follow this guide to ensure your deployments are fast, reliable,
and production-ready.
