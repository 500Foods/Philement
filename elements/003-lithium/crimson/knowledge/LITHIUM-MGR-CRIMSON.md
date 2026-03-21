# Crimson AI Agent Manager

The Crimson AI Agent Manager provides a popup chat interface for "Crimson" — Lithium's AI assistant for helping users navigate and use the application effectively.

**Location:** `src/managers/crimson/`

---

## Overview

| Aspect | Details |
|--------|---------|
| **Type** | Singleton popup manager (not a traditional slot-based manager) |
| **Purpose** | AI-powered user assistance via chat interface |
| **Activation** | Global keyboard shortcut (`Ctrl+Shift+C`) or toolbar button |
| **Key Files** | `crimson.js`, `crimson.css` |

Crimson is named after the crimson flame color when lithium burns. The agent was created to help users navigate the growing complexity of Lithium as the application expands with more managers and features.

---

## UI Layout

```diagram
┌──────────────────────────────────────────────────────────────┐
│  [🤖 Chat with Crimson]                    [✕ Close (ESC)]  │  ← Draggable header
├──────────────────────────────────────────────────────────────┤
│                                                              │
│                    ┌───────────┐                             │
│                    │   🤖      │                             │
│                    └───────────┘                             │
│                   Hello, [Username]!                         │  ← Welcome message
│                How can I help you today?                     │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ 🤖  I'm here to help! What would you like to know?   │   │  ← Chat messages
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ 👤  How do I create a new query?                    │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────┐  ┌────────┐ │
│  │ Type your message...                       │  │   ➤    │ │  ← Input area
│  └────────────────────────────────────────────┘  └────────┘ │
└──────────────────────────────────────────────────────────────┘
     ↖ Resize handle (drag any corner)
```

### Popup Features

- **Draggable header** — Click and drag to reposition anywhere on screen
- **Resizable corners** — Drag any of the four corner handles to resize
- **Overlay backdrop** — Semi-transparent overlay behind the popup (click to close)
- **Centered initially** — Opens at 70% viewport size, centered
- **Constrained to viewport** — Cannot be dragged completely off-screen

---

## Architecture

### Singleton Pattern

Crimson uses a singleton pattern to maintain a single instance across the application:

```javascript
// Singleton instance tracking
let crimsonInstance = null;

export function getCrimson() {
  if (!crimsonInstance) {
    crimsonInstance = new CrimsonManager();
  }
  return crimsonInstance;
}
```

### Class Structure

```javascript
class CrimsonManager {
  constructor() {
    // DOM element references
    this.popup = null;
    this.overlay = null;
    this.conversation = null;
    this.input = null;
    this.sendBtn = null;
    
    // State tracking
    this.isVisible = false;
    this.isDragging = false;
    this.isResizing = false;
    this.messages = [];
    
    // Drag/resize tracking
    this.dragStartX = 0;
    this.dragStartY = 0;
    // ... more tracking variables
    
    this.init();
  }
  
  init() { /* Create DOM elements */ }
  show(options) { /* Display popup */ }
  hide() { /* Hide popup */ }
  
  // Interaction handlers
  handleDragStart(e) { /* ... */ }
  handleResizeStart(e, corner) { /* ... */ }
  handleSend() { /* Send message */ }
  addMessage(sender, text) { /* Add to conversation */ }
}
```

### DOM Creation

Unlike traditional managers that use HTML templates, Crimson creates its DOM elements dynamically in JavaScript:

```javascript
init() {
  if (this.popup) return; // Already initialized
  
  // Create overlay
  this.overlay = document.createElement('div');
  this.overlay.className = 'crimson-overlay';
  document.body.appendChild(this.overlay);
  
  // Create popup with innerHTML template
  this.popup = document.createElement('div');
  this.popup.className = 'crimson-popup';
  this.popup.innerHTML = `...template...`;
  document.body.appendChild(this.popup);
  
  // Cache element references and attach listeners
  // ...
}
```

---

## Integration Points

### Global Keyboard Shortcut

```javascript
export function initCrimsonShortcut() {
  document.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'C') {
      // Don't trigger when typing in inputs
      const isTyping = activeElement?.tagName === 'INPUT' ||
                       activeElement?.tagName === 'TEXTAREA';
      if (!isTyping) {
        e.preventDefault();
        toggleCrimson();
      }
    }
  });
}
```

### Manager Toolbar Button

```javascript
export function createCrimsonButton(tooltip = 'Chat with Crimson (Ctrl+Shift+C)') {
  const button = document.createElement('button');
  button.className = 'subpanel-header-btn subpanel-header-close crimson-btn';
  button.innerHTML = '<fa fa-robot></fa>';
  button.addEventListener('click', () => toggleCrimson());
  return button;
}
```

Add to any manager's toolbar:

```javascript
import { createCrimsonButton } from '../crimson/crimson.js';

// In manager init:
const crimsonBtn = createCrimsonButton();
headerGroup.insertBefore(crimsonBtn, closeBtn);
```

### Current Integrations

The Crimson button appears in:

- **Query Manager** toolbar — Next to font/prettify buttons
- **Lookups Manager** toolbar — In the right panel header

---

## Animation System

### Show/Hide Transitions

The popup uses CSS transitions for smooth appearance:

```css
.crimson-popup {
  opacity: 0;
  visibility: hidden;
  transform: scale(0.95);
  transition: opacity var(--transition-duration) ease,
              visibility var(--transition-duration) ease,
              transform var(--transition-duration) ease;
}

.crimson-popup.visible {
  opacity: 1;
  visibility: visible;
  transform: scale(1);
}
```

### Animation Timing

| Property | Duration | Easing |
|----------|----------|--------|
| Opacity | `--transition-duration` (350ms) | ease |
| Transform | `--transition-duration` (350ms) | ease |
| Visibility | `--transition-duration` (350ms) | ease |

### Important: First-Show Animation

When the popup is first created and shown, the browser may not animate the transition if the element doesn't have time to establish its initial state. The fix is to force a reflow before adding the `.visible` class:

```javascript
show(options = {}) {
  if (!this.popup) this.init();
  
  // Force reflow to ensure initial styles are applied
  void this.popup.offsetHeight;
  void this.overlay.offsetHeight;
  
  // Now add visible class for animation
  this.overlay.classList.add('visible');
  this.popup.classList.add('visible');
}
```

This ensures the browser processes the initial `opacity: 0` state before transitioning to `opacity: 1`.

---

## Drag and Resize System

### Drag Implementation

```javascript
handleDragStart(e) {
  this.isDragging = true;
  this.popup.classList.add('dragging');
  
  this.dragStartX = e.clientX;
  this.dragStartY = e.clientY;
  
  const rect = this.popup.getBoundingClientRect();
  this.popupStartX = rect.left;
  this.popupStartY = rect.top;
  
  document.addEventListener('mousemove', this.handleDragMove);
  document.addEventListener('mouseup', this.handleDragEnd);
}

handleDragMove(e) {
  const deltaX = e.clientX - this.dragStartX;
  const deltaY = e.clientY - this.dragStartY;
  
  let newX = this.popupStartX + deltaX;
  let newY = this.popupStartY + deltaY;
  
  // Constrain to viewport (minimum 50px visible)
  // ... constraint logic
  
  this.popup.style.left = `${newX}px`;
  this.popup.style.top = `${newY}px`;
}
```

### Resize Corners

Four resize handles support resizing from any corner:

| Corner | Class | Cursor | Behavior |
|--------|-------|--------|----------|
| Bottom-Right | `.crimson-resize-handle-br` | nwse-resize | Grows down-right |
| Bottom-Left | `.crimson-resize-handle-bl` | nesw-resize | Grows down-left, moves left |
| Top-Right | `.crimson-resize-handle-tr` | nesw-resize | Grows up-right, moves up |
| Top-Left | `.crimson-resize-handle-tl` | nwse-resize | Grows up-left, moves up-left |

---

## Chat Interface

### Message Structure

```javascript
addMessage(sender, text) {
  // Remove welcome message on first user message
  const welcome = this.conversation.querySelector('.crimson-welcome');
  if (welcome) welcome.remove();
  
  const messageEl = document.createElement('div');
  messageEl.className = `crimson-message crimson-message-${sender}`;
  messageEl.innerHTML = `
    <div class="crimson-message-avatar">
      <fa fa-${sender === 'agent' ? 'robot' : 'user'}></fa>
    </div>
    <div class="crimson-message-content">${escapedText}</div>
  `;
  
  this.conversation.appendChild(messageEl);
  this.conversation.scrollTop = this.conversation.scrollHeight;
}
```

### Input Handling

| Key Combination | Action |
|-----------------|--------|
| `Enter` | Send message |
| `Shift+Enter` | Insert newline |

```javascript
handleInputKeydown(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    this.handleSend();
  }
}
```

### Auto-Resize Input

The input textarea automatically grows with content:

```javascript
autoResizeInput() {
  this.input.style.height = 'auto';
  const newHeight = Math.min(150, Math.max(44, this.input.scrollHeight));
  this.input.style.height = `${newHeight}px`;
}
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+C` | Toggle Crimson popup (global) |
| `ESC` | Close popup (when open) |
| `Enter` | Send message |
| `Shift+Enter` | New line in input |

---

## CSS Architecture

### Key Classes

| Class | Purpose |
|-------|---------|
| `.crimson-overlay` | Backdrop overlay |
| `.crimson-popup` | Main popup container |
| `.crimson-header` | Draggable header area |
| `.crimson-conversation` | Message scroll area |
| `.crimson-input-area` | Input + send button |
| `.crimson-message` | Individual message bubble |
| `.crimson-message-user` | User message styling |
| `.crimson-message-agent` | Agent message styling |
| `.crimson-resize-handle-*` | Corner resize handles |

### State Classes

| Class | Applied When |
|-------|--------------|
| `.visible` | Popup is shown |
| `.dragging` | Currently being dragged |
| `.resizing` | Currently being resized |

### Color Theme

The crimson color (`#DC143C`) is used for the robot icon to match the agent's name:

```css
.crimson-header-primary svg,
.crimson-message-agent .crimson-message-avatar {
  color: #DC143C; /* Crimson color for the robot icon */
}
```

---

## Future AI Integration

The current implementation includes a placeholder response system:

```javascript
simulateCrimsonResponse() {
  // Show typing indicator
  const indicator = this.addTypingIndicator();
  
  // Simulate response delay
  setTimeout(() => {
    this.removeTypingIndicator();
    
    // Placeholder responses
    const responses = [
      "I'm here to help! What would you like to know?",
      // ... more responses
    ];
    
    this.addMessage('agent', randomResponse);
  }, responseTime);
}
```

### Planned Integration

- **DigitalOcean GradientAI** model with RAG content
- **Tool calling** for actions like "click this button" or "run this report"
- **Training data** from `elements/003-lithium/crimson/` directory:
  - `ABOUT.md` — Agent description
  - `LITHIUM.md` — UI reference
  - `TOURS.md` — Built-in tours (future)
  - `CANVAS.md` — LMS course content
  - `SCHEMA.md` — Database schema
  - `FACTS.md` — Environment facts

---

## Training Data Location

Crimson-specific training documents are in:

```directory
elements/003-lithium/crimson/
├── ABOUT.md      # Crimson description and background
├── LITHIUM.md    # UI reference with DOM selectors
├── PROMPT.md     # System prompts for AI
├── TOURS.md      # Shepherd.js tours (future)
├── CANVAS.md     # LMS courses content (future)
├── SCHEMA.md     # Database schema (future)
└── FACTS.md      # Environment facts (future)
```

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Example integration
- [LITHIUM-MGR-MAIN.md](LITHIUM-MGR-MAIN.md) — Main manager (global shortcuts)
- `elements/003-lithium/crimson/ABOUT.md` — Agent background

---

## Implementation Notes

### No Template File

Unlike other managers, Crimson doesn't use a separate HTML template file. All markup is created dynamically in JavaScript. This keeps the implementation self-contained and allows for singleton pattern usage.

### Body-Appended Elements

The popup and overlay are appended directly to `document.body`, not to a manager slot. This allows Crimson to be accessed from anywhere in the application regardless of which manager is currently active.

### Event Delegation

Event listeners are attached directly to elements during initialization. No event delegation is used since the popup structure is fixed after creation.

---

Last updated: March 2026
