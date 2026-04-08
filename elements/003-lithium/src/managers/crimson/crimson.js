/**
 * Crimson AI Agent Manager
 *
 * A popup chat window for the AI Agent "Crimson".
 * Features:
 * - Draggable header
 * - Resizable (4 corner handles)
 * - Centered initially at 70% viewport
 * - Chat interface with conversation pane and input
 * - ESC to close, Ctrl+Shift+C to open
 * - WebSocket streaming for real-time AI responses
 * - Citations with LithiumTable popup
 * - Debug and reasoning panels
 * - Input history navigation
 *
 * @module managers/crimson
 */

// Import all modules to extend CrimsonManager class
// These imports add methods to the CrimsonManager prototype
import './crimson-core.js';
import './crimson-events.js';
import './crimson-ui.js';
import './crimson-chat.js';
import './crimson-citations.js';

// Import CSS
import './crimson.css';

// Re-export public API from core module
export {
  CrimsonManager,
  getCrimson,
  showCrimson,
  hideCrimson,
  toggleCrimson,
  createCrimsonButton,
  initCrimsonShortcut,
} from './crimson-core.js';
