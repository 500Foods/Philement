# Lithium Deployment

This document describes the deployment process and anything special to know about deploying Lithium.

---

## Overview

Lithium deploys as a static SPA to any web server. It requires:

- HTTPS (for PWA features)
- Static file serving
- SPA fallback (redirect to index.html)

---

## Environment Variables

Set these before deploying:

| Variable | Purpose | Example |
|----------|---------|---------|
| `LITHIUM_ROOT` | Project source | `/mnt/extra/Projects/Philement/elements/003-lithium` |
| `LITHIUM_DEPLOY` | Web server root | `/fvl/tnt/t-philement/lithium` |
| `LITHIUM_DEPLOY_KEEP` | Rollback versions to keep | `3` |

---

## Deploy Commands

### Full Deploy

```bash
npm run deploy
```

This runs:

1. Test suite with coverage
2. Copy coverage to `public/`
3. Copy HTML templates to `public/`
4. Bump version number
5. Vite build to deploy directory
6. Copy runtime config (if not exists)
7. Minify HTML and service worker
8. Prune old hashed assets
9. Generate Brotli sidecars

### Manual Deploy

```bash
# Build
npm run build:prod

# Copy to server
rsync -avz dist/ user@server:/path/to/lithium/
```

---

## Deploy Flow Details

### 1. Test & Coverage

```bash
npm run test:coverage
npm run coverage:copy
```

### 2. Templates

```bash
npm run templates:copy
```

Copies manager HTML templates from `src/managers/*/` to `public/src/managers/*/`

### 3. Version Bump

```bash
npm run version:bump
```

- Increments build number in `version.json`
- Updates `CACHE_VERSION` in `service-worker.js`
- Copies to `public/`

### 4. Build

```bash
vite build --mode deploy
```

Builds directly to `$LITHIUM_DEPLOY` (does NOT empty directory)

### 5. Config

```bash
# Copies config only if not present
cp config/lithium.json $LITHIUM_DEPLOY/config/
```

### 6. Minify

```bash
# Minifies index.html and service-worker.js in-place
```

### 7. Prune

```bash
# Removes old hashed JS/CSS keeping only N versions
```

Default: keep 3 newest versions per asset family

### 8. Brotli

```bash
# Generates .br sidecars for text assets
```

Eligible: `.css`, `.html`, `.json`, `.svg`, `.js`, `.map` files

---

## Output Structure

```structure
$LITHIUM_DEPLOY/
├── index.html              # Minified
├── manifest.json           # PWA manifest
├── service-worker.js       # Minified
├── version.json            # Build number
├── config/
│   └── lithium.json        # Runtime config (preserved)
├── assets/
│   ├── index-[hash].js    # Bundled JS
│   ├── index-[hash].css   # Bundled CSS
│   ├── fonts/             # Vanadium fonts
│   └── images/            # Logos
├── src/managers/          # HTML templates
│   ├── login/login.html
│   ├── main/main.html
│   └── style-manager/style-manager.html
└── coverage/               # Test coverage
    └── index.html
```

---

## Runtime Config

`config/lithium.json` is fetched at runtime, NOT bundled.

**Key settings:**

```json
{
  "server": {
    "url": "https://hydrogen.example.com",
    "api_prefix": "/api"
  },
  "auth": {
    "api_key": "public-app-id",
    "default_database": "Demo_PG"
  },
  "app": {
    "name": "Lithium",
    "default_theme": "dark"
  }
}
```

---

## Service Worker

The service worker (`service-worker.js`) implements:

- **Cache-first** for static assets
- **Network-first** for `version.json`
- **Stale-while-revalidate** for API calls
- **Versioned caches** with automatic cleanup

### Cache Version

`CACHE_VERSION` in the service worker matches the build number. Changes on each deploy trigger cache cleanup.

---

## PWA Features

### Requirements

1. **HTTPS** — Required for service worker
2. **manifest.json** — App metadata
3. **Icons** — 192x192 and 512x512 PNG

### Manifest

```json
{
  "name": "Lithium",
  "short_name": "Lithium",
  "start_url": "/",
  "display": "standalone",
  "theme_color": "#0D1117",
  "background_color": "#0D1117",
  "icons": [...]
}
```

---

## Server Configuration

### Nginx

```nginx
server {
    listen 80;
    server_name lithium.example.com;
    root /var/www/lithium;
    index index.html;

    # Enable gzip
    gzip on;
    gzip_types text/css application/javascript text/javascript application/json;

    # Cache static assets
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }

    # SPA fallback
    location / {
        try_files $uri $uri/ /index.html;
    }
}
```

### Apache

```apache
<VirtualHost *:80>
    ServerName lithium.example.com
    DocumentRoot /var/www/lithium

    <Directory /var/www/lithium>
        FallbackResource /index.html
    </Directory>
</VirtualHost>
```

---

## Testing Deployment

### Local Preview

```bash
npm run preview
```

Serves the production build locally at `http://localhost:4173`

### Coverage Dashboard

After deploy with coverage:

```url
https://lithium.philement.com/coverage/
```

---

## Troubleshooting

### 404 on HTML Templates

Run `npm run templates:copy` — HTML must be in `public/src/managers/`

### Icons Not Loading

Check Font Awesome Kit URL in `index.html`

### Service Worker Not Updating

1. Check `version.json` is being fetched (network-first)
2. Ensure build number incremented
3. Clear browser cache or use Incognito

### Config Not Loading

Verify `config/lithium.json` exists in deploy directory

---

## Related Documentation

- Development: [LITHIUM-DEV.md](LITHIUM-DEV.md)
- Libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Managers: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- FAQ: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
