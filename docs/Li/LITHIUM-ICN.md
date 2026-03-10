# Lithium Icon System

Lithium uses a custom icon system built on Font Awesome with dynamic lookup support. The system replaces custom `<fa>` tags with proper Font Awesome elements.

**Location:** [`src/core/icons.js`](elements/003-lithium/src/core/icons.js)

---

## Overview

| Aspect | Details |
|--------|---------|
| **Library** | Font Awesome 6 (Pro) |
| **Approach** | Custom `<fa>` tag replacement |
| **Configuration** | Via `lithium.json` |
| **Dynamic Icons** | Server-driven icon overrides |

---

## Basic Usage

### HTML Template

```html
<!-- Direct icon -->
<fa fa-palette></fa>

<!-- With modifiers -->
<fa fa-user fa-fw></fa>

<!-- Spinning -->
<fa fa-spinner fa-spin></fa>
```

### JavaScript

```javascript
import { processIcons, setIcon } from '../../core/icons.js';

// Process all <fa> elements in container
processIcons(document.body);

// Programmatically set icon
setIcon(element, 'palette', { classes: ['fa-fw'] });
```

---

## How It Works

### 1. Custom Element Processing

The system uses a MutationObserver to watch for new `<fa>` elements:

```javascript
// Input
<fa fa-palette></fa>

// Output
<i class="fas fa-palette"></i>
```

### 2. Configuration

Icons are configured in `config/lithium.json`:

```json
{
  "icons": {
    "set": "duotone",
    "weight": "100",
    "fallback": "solid"
  }
}
```

### 3. Lookup Integration

Icons can be dynamically overridden from server lookups:

```html
<!-- Use lookup icon -->
<fa Status></fa>

<!-- Lookup provides: <i class="fa fa-flag"></i> -->
```

---

## Icon Sets

The system supports all Font Awesome 6 Pro sets:

| Set | Prefix | Example Class |
|------|--------|---------------|
| Solid | `fas` | `fas fa-user` |
| Regular | `far` | `far fa-user` |
| Light | `fal` | `fal fa-user` |
| Thin | `fat` | `fat fa-user` |
| Duotone | `fad` | `fad fa-user` |
| Sharp Solid | `fass` | `fass fa-user` |
| Brands | `fab` | `fab fa-github` |

### Setting Icon Set

```javascript
// Via config
{
  "icons": {
    "set": "duotone",
    "weight": "100"
  }
}
```

---

## Preserved Classes

These classes are preserved from the original `<fa>` tag:

### Size Classes

```html
<fa fa-user fa-xs></fa>
<fa fa-user fa-sm></fa>
<fa fa-user fa-lg></fa>
<fa fa-user fa-2x></fa>
<!-- ... up to fa-10x -->
```

### Animation Classes

```html
<fa fa-spinner fa-spin></fa>
<fa fa-spinner fa-pulse></fa>
<fa fa-bell fa-beat></fa>
<fa fa-bounce fa-bounce></fa>
```

### Utility Classes

```html
<fa fa-fw></fa>        <!-- Fixed width -->
<fa fa-rotate-90></fa> <!-- Rotate 90° -->
<fa fa-flip-horizontal></fa>
<fa fa-inverse></fa>
```

---

## Programmatic Usage

### Creating Icons

```javascript
import { createIcon, setIcon } from '../../core/icons.js';

// Create new icon element
const icon = createIcon('palette', { 
  set: 'duotone',
  weight: '100',
  classes: ['fa-fw']
});
document.body.appendChild(icon);

// Set icon on existing element
setIcon(existingElement, 'user', { classes: ['fa-2x'] });
```

### Configuration

```javascript
import { getIconConfiguration } from '../../core/icons.js';

const config = getIconConfiguration();
// { set: 'duotone', weight: '100', prefix: 'fad', fallback: 'solid' }
```

---

## Icon Configuration

Icons are configured in `config/lithium.json`:

```json
{
  "icons": {
    "set": "duotone",
    "weight": "thin",
    "prefix": "fa-duotone fa-thin",
    "fallback": "solid"
  }
}
```

| Property | Type | Description |
|----------|------|-------------|
| `set` | string | Font Awesome set name |
| `weight` | string | Font weight for compatible sets |
| `prefix` | string | Full CSS prefix (overrides set/weight) |
| `fallback` | string | Fallback set if primary unavailable |

### Example Configurations

```json
// Duotone thin icons (current)
{ "icons": { "set": "duotone", "weight": "thin", "prefix": "fa-duotone fa-thin" } }

// Solid icons
{ "icons": { "set": "solid", "fallback": "regular" } }

// Sharp Solid
{ "icons": { "set": "sharp-solid", "weight": "100" } }
```

---

## Lookup Integration

### Server-Side Icons

QueryRef 054 provides server-driven icon overrides:

```javascript
// In lookups.js - icons category
{
  "icons": [
    { "lookup_value": "Status", "icon": "<i class='fa fa-fw fa-flag'></i>" },
    { "lookup_value": "Error", "icon": "<i class='fa fa-fw fa-xmark'></i>" }
  ]
}
```

### Usage

```html
<!-- Will use lookup icon if available -->
<fa Status></fa>
```

---

## Events

The icon system listens for events to refresh icon data:

| Event | Action |
|-------|--------|
| `lookups:loaded` | Refresh icon lookups |
| `lookups:icons:loaded` | Refresh icon lookups and reprocess |

```javascript
import { eventBus, Events } from '../../core/event-bus.js';

eventBus.on(Events.LOOKUPS_LOADED, () => {
  // Icons will automatically refresh
});
```

---

## Best Practices

### 1. Use Semantic Names

```html
<!-- Good -->
<fa fa-palette></fa>
<fa fa-user-cog></fa>

<!-- Avoid -->
<fa fa-p></fa>
<fa fa-uc"></fa>
```

### 2. Prefer CSS Classes Over Attributes

```html
<!-- Good -->
<fa fa-spinner fa-spin></fa>

<!-- Works but less clear -->
<fa spinner spin></fa>
```

### 3. Use Fixed Width for Alignment

```html
<!-- Aligned icons -->
<fa fa-user fa-fw></fa>
<fa fa-cog fa-fw></fa>
```

### 4. Accessibly

```html
<!-- With title for accessibility -->
<fa fa-palette title="Open theme settings"></fa>

<!-- Or aria-label -->
<fa fa-search aria-label="Search"></fa>
```

---

## Testing

### Manual Testing

```javascript
import { processIcons } from '../../core/icons.js';

// Process any new icons after dynamic content
processIcons(container);

// Check icon configuration
const config = getIconConfiguration();
console.log('Icon set:', config.set);
```

---

## Related Documentation

- [LITHIUM-OTH.md](LITHIUM-OTH.md) - Other utilities
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) - Login Manager (uses icons extensively)
- [LITHIUM-LIB.md](LITHIUM-LIB.md) - Libraries
