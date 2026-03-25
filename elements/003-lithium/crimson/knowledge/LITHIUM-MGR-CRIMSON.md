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

## WebSocket Chat Integration

Crimson uses the app-wide WebSocket connection for real-time AI chat with streaming responses. The connection is established after login and maintained throughout the session.

### Architecture

```
┌─────────────────┐                    ┌─────────────────┐
│                 │    WebSocket       │                 │
│  Lithium Client │ ◄────────────────► │ Hydrogen Server │
│   (app-ws.js)   │    Port 7001       │  (websocket_*)  │
└─────────────────┘                    └─────────────────┘
         │
         │ Uses by:
         ▼
┌─────────────────┐     ┌─────────────────┐
│  Crimson Chat   │     │  Notifications  │
│  (crimson-ws.js)│     │    (future)     │
└─────────────────┘     └─────────────────┘
```

### Key Files

| File | Purpose |
|------|---------|
| `src/shared/app-ws.js` | App-wide WebSocket client with keepalive |
| `src/shared/crimson-ws.js` | Crimson chat wrapper using app-ws |
| `src/managers/crimson/crimson.js` | Chat UI with streaming display |
| `src/managers/main/main.js` | WebSocket initialization and status indicator |
| `crimson/PROMPT.md` | AI system prompt with delimiter format |

### Connection Lifecycle

1. **On login**: `MainManager.initWebSocket()` establishes the connection
2. **During session**: Connection stays alive with keepalive pings every 30 seconds
3. **Status indicator**: Sidebar header shows connection state (green/blue/red)
4. **On logout**: Connection is gracefully closed

### Connection Configuration

The WebSocket connection uses settings from `config/lithium.json`:

```json
{
  "server": {
    "websocket_url": "wss://lithium.philement.com/wss",
    "websocket_key": "ABCDEFGHIJKLMNOP",
    "websocket_protocol": "hydrogen"
  }
}
```

### Authentication Flow

1. **Connection auth**: Query parameter `?key=<websocket_key>` authenticates the WebSocket connection
2. **Chat auth**: JWT token (with `Bearer ` prefix) authenticates each chat message
3. **Database extraction**: JWT claims provide the database context for the AI

```javascript
// JWT is sent with "Bearer " prefix in chat payload
const payload = {
  engine: "Crimson",
  messages: [{ role: "user", content: message }],
  stream: true,
  jwt: `Bearer ${jwt}`,  // Note the Bearer prefix
  context: {              // Context packet for personalization
    user: { id, username, displayName, roles, preferences },
    session: { sessionId, loginTime, currentManager },
    permissions: { managers, features },
    currentView: { managerId, managerName },
    lithiumVersion, buildDate
  }
};
```

### Context Packet

The context packet is gathered automatically from the app state and JWT claims:

```javascript
// From crimson-ws.js: gatherContext()
{
  user: {
    id: claims.user_id,
    username: claims.username,
    displayName: claims.display_name || claims.username,
    roles: claims.roles || [],
    preferences: {
      theme: localStorage.getItem('lithium_theme'),
      language: navigator.language
    }
  },
  session: {
    sessionId: app.sessionId,
    loginTime: claims.iat * 1000,
    currentManager: app.currentManager?.id,
    recentActivity: []
  },
  permissions: {
    managers: claims.managers || [],
    features: claims.features || []
  },
  currentView: {
    managerId: app.currentManager?.id,
    managerName: app.currentManager?.name,
    activeTab: null,
    selectedRecord: null
  },
  lithiumVersion: app.version,
  buildDate: app.build
}
```

### Streaming Response Format

The AI responses use a delimiter-based format for structured data:

```
Conversation text streams here in plain text...
[LITHIUM-CRIMSON-JSON]
{
  "followUpQuestions": ["Question 1?", "Question 2?"],
  "suggestions": { ... },
  "metadata": { "confidence": 0.95, "category": "help" }
}
```

#### Reasoning Content

Models like Kimi K2.5 expose reasoning/thinking via a separate `reasoning_content` field in the streaming response:

- **Separate field**: `reasoning_content` arrives in the WebSocket chunk alongside `content`
- **Streaming**: Reasoning content streams in real-time to the reasoning panel
- **Display**: The reasoning panel must be toggled visible (person icon) to see content
- **Timing**: `reasoning_content` arrives before `content` (the model thinks first, then answers)

#### Delimiter Handling

The client handles the delimiter `[LITHIUM-CRIMSON-JSON]` which may be split across WebSocket chunks:

1. **Before delimiter**: Conversation text streams in real-time
2. **After delimiter**: JSON metadata is accumulated and parsed
3. **On completion**: Follow-up questions are rendered as clickable buttons

```javascript
// Delimiter detection handles partial matches across chunks
this.DELIMITER = '[LITHIUM-CRIMSON-JSON]';
this.seenDelimiter = false;
this.conversationBuffer = '';
this.metadataBuffer = '';
this.partialDelimiter = '';
```

### Response Structure

The JSON metadata after the delimiter supports:

| Field | Type | Purpose |
|-------|------|---------|
| `followUpQuestions` | `string[]` | 1-3 suggested follow-up questions |
| `suggestions.highlightButtons` | `object[]` | Highlight UI elements |
| `suggestions.suggestManagers` | `object[]` | Suggest navigating to a manager |
| `suggestions.offerTours` | `object[]` | Offer guided tours |
| `suggestions.openDocumentation` | `object` | Link to documentation |
| `metadata.confidence` | `number` | AI confidence (0-1) |
| `metadata.category` | `string` | Response category (help, navigation, etc.) |

### Follow-up Questions

When `followUpQuestions` are present, clickable buttons are rendered below the message:

```css
.crimson-followups {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
  margin-top: var(--space-3);
}

.crimson-followup-btn {
  padding: var(--space-2) var(--space-3);
  background: var(--bg-secondary);
  border: 1px solid var(--accent-primary);
  border-radius: var(--border-radius-md);
  color: var(--accent-primary);
  cursor: pointer;
}
```

Clicking a follow-up question sends it as a new message.

### Connection State Management

The WebSocket connection is managed by the app-wide `app-ws.js` module. Crimson uses the shared connection through `crimson-ws.js`.

| State | Description | Status Indicator |
|-------|-------------|------------------|
| `DISCONNECTED` | No active connection | Red octagon |
| `CONNECTING` | Connection in progress | Orange (pulsing) |
| `CONNECTED` | Connection established | Green circle |
| `ERROR` | Connection failed | Red octagon |

### Keepalive

The connection uses a keepalive mechanism:
- **Ping interval**: 30 seconds
- **Pong timeout**: 10 seconds
- On timeout, the connection is automatically re-established

### Status Indicator

A visual indicator in the sidebar header shows the connection state:
- **Green circle** — Connected and idle
- **Blue flash** — Data sent (100ms flash)
- **Red octagon** — Disconnected or error

See [LITHIUM-WSS.md](LITHIUM-WSS.md) for full WebSocket documentation.

### Code Block Formatting

Conversation text supports markdown-like formatting:

```css
.crimson-code {
  display: block;
  margin: var(--space-2) 0;
  padding: var(--space-3);
  background: var(--bg-tertiary);
  border: var(--border-standard);
  border-radius: var(--border-radius-md);
  font-family: 'Consolas', 'Monaco', monospace;
}

.crimson-inline-code {
  display: inline;
  padding: 2px 6px;
  background: var(--bg-tertiary);
  border: 1px solid var(--border-color);
  border-radius: var(--border-radius-sm);
}
```

### Streaming Indicator

While content streams, a blinking cursor appears:

```css
.crimson-streaming .crimson-message-content::after {
  content: '';
  display: inline-block;
  width: 2px;
  height: 1em;
  background: var(--accent-primary);
  animation: crimson-blink 1s infinite;
}
```

### Known Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| JSON appears in conversation | Delimiter split across chunks | Handle partial delimiter matching |
| Send button stays disabled | Error in stream processing | Wrap handleStreamDone in try-finally |
| Connection not reestablished | Stale callbacks on singleton | Always update callbacks in initWebSocketClient |
| Key mismatch | Config key differs from server | Use consistent key: `ABCDEFGHIJKLMNOP` |
| Handlers lost after reset | cleanup() unregisters handlers | Re-register handlers in getCrimsonWS() |
| Non-streaming delimiter not parsed | Delimiter only parsed in streaming mode | Parse delimiter in handleStreamDone for non-streaming |

---

## UI Controls

### Debug Panel

The debug panel shows WebSocket message flow and can be toggled with animation.

```javascript
// Debug state is persisted in localStorage
localStorage.setItem('crimson_debug_mode', 'true/false');
```

| Control | Icon | Behavior |
|---------|------|----------|
| Debug toggle | `fa-bug` | Slides up/down with animation |
| Streaming toggle | `fa-water` / `fa-lines-leaning` | Enables/disables streaming mode |
| Reasoning toggle | `fa-person` / `fa-person-running` | Shows/hides reasoning panel |

### Resizable Panels

Both debug and reasoning panels are resizable via horizontal splitter bars:

```
┌──────────────────────────────────────┐
│ Reasoning Panel                      │
│ [reasoning content here]             │
├──────────────────────────────────────┤ ← Splitter (drag to resize)
│ Conversation Area                    │
│                                      │
├──────────────────────────────────────┤ ← Splitter (drag to resize)
│ Debug Panel                          │
│ [debug output here]                  │
└──────────────────────────────────────┘
```

- **Debug panel splitter**: At top of panel
- **Reasoning panel splitter**: At bottom of panel
- **Animation disabled during resize**: Instant response to mouse movement
- **State persisted**: Panel visibility remembered across sessions

### Persistent State

The following states are persisted in localStorage:

| Key | Purpose | Default |
|-----|---------|---------|
| `crimson_debug_mode` | Debug panel visibility | `false` |
| `crimson_streaming_enabled` | Streaming mode enabled | `true` |
| `crimson_reasoning_mode` | Reasoning panel visibility | `false` |

---

## WebSocket Handler Management

### Handler Lifecycle

Handlers are registered when `getCrimsonWS()` is called and must survive cleanup operations:

```javascript
export function getCrimsonWS(options = {}) {
  if (!instance) {
    instance = new CrimsonWebSocket(options);
  } else {
    // Always re-register handlers - they may have been unregistered by cleanup()
    instance._registerHandlers();
    // Update callbacks...
  }
  return instance;
}
```

### Message Flow Debugging

The debug panel shows all WebSocket message types:

```
[15:52:42.260] CHAT_DONE: id=crimson_123, content_length=147
[15:52:42.263] DONE: Stream complete
[15:52:58.670] CHAT_DONE: id=crimson_456, content_length=0
```

Debug message types:
- `SEND`: Message being sent
- `CHUNK`: Streaming chunk received
- `DONE`: Stream completion
- `ERROR`: Error messages
- `META`: Metadata parsing
- `DELIMITER`: Delimiter detection
- `WS_CALLBACK`: WebSocket callback invocation
- `WS_STATE`: Connection state

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
- [LITHIUM-WSS.md](LITHIUM-WSS.md) — WebSocket connection architecture
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

Last updated: March 24, 2026
