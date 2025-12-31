# Source Editor Module

## Overview

The Source Editor Module provides code editing functionality for the Lithium PWA. It's designed as a placeholder for future code editing capabilities.

## CHANGELOG

2025-12-31 - Initially created.

## Module Structure

```files
source-editor/
├── source-editor.js    # Main module JavaScript class
├── source-editor.html  # HTML template for editor interface
└── README.md           # This file
```

## Functionality

### Current Features

- Basic module structure with HTML template
- Placeholder for code editing interface
- Integration with main app services
- Standard module lifecycle methods

### Future Enhancements

- Full-featured code editor integration
- Syntax highlighting for multiple languages
- File management (open, save, new)
- Code formatting and linting
- Search and replace functionality
- Multiple editor tabs
- Collaboration features

## Class Structure

```javascript
class SourceEditorModule {
  constructor(app, container) {
    this.app = app;          // Reference to main app instance
    this.container = container; // DOM container for this module
    this.logger = app.logger; // Access to logging system
  }

  async init() {
    // Load HTML template
    // Initialize editor components
    // Set up event listeners
  }

  setupEventListeners() {
    // Attach editor UI event handlers
  }

  destroy() {
    // Clean up editor resources
  }
}
```

## Integration Points

### App Services Used

- `app.logger` - For logging information and errors

### Third-Party Libraries Available

- CodeMirror 6 (already loaded in index.html)
- Prism.js (syntax highlighting)
- Highlight.js (alternative syntax highlighting)
- SunEditor (rich text editing)

## Development Guidelines

### Editor Integration

1. Choose editor library (CodeMirror recommended)
2. Initialize editor in `init()` method
3. Set up language modes and themes
4. Add toolbar and UI controls
5. Implement file management

### Adding Features

1. Add UI elements to `source-editor.html`
2. Implement functionality in class methods
3. Add event handlers in `setupEventListeners()`
4. Test with various file types

### Testing

- Test editor initialization and rendering
- Test basic editing functionality
- Test syntax highlighting
- Test responsive design
- Test error handling

### Styling

- Use existing CSS framework for UI
- Follow application color scheme
- Ensure editor is responsive
- Provide clear visual feedback

## Dependencies

- Bootstrap 5 (for UI components)
- Font Awesome (for icons)
- Main app services (logger)
- CodeMirror 6 (for code editing)

## Best Practices

- Keep editor configuration modular
- Handle large files efficiently
- Provide clear user feedback
- Implement proper error handling
- Log editor events for debugging

## Future Implementation Notes

### Recommended Editor Setup

```javascript
import { basicSetup } from 'https://cdn.jsdelivr.net/npm/codemirror@6.0.1/basic-setup/+esm';
import { EditorView } from 'https://cdn.jsdelivr.net/npm/codemirror@6.0.1/view/+esm';
import { javascript } from 'https://cdn.jsdelivr.net/npm/codemirror@6.0.1/lang-javascript/+esm';

// Initialize editor
const editor = new EditorView({
  doc: "// Your code here",
  extensions: [basicSetup, javascript()],
  parent: this.container.querySelector('#editor-container')
});
```

### File Management Considerations

- Implement file open/save dialogs
- Add file type detection
- Handle encoding issues
- Provide file size limits
- Implement auto-save functionality