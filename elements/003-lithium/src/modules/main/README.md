# Main Module

## Overview

The Main Module serves as the primary navigation hub for the Lithium PWA. It provides a sidebar menu for accessing all sub-modules and handles user logout functionality.

## CHANGELOG

2025-12-31 - Initially created.

## Module Structure

```files
main/
├── main.js         # Main module JavaScript class
├── main.html       # HTML template for main menu
└── README.md       # This file
```

## Functionality

### Current Features

- Sidebar navigation menu with module buttons
- Dynamic sub-module loading
- Active button highlighting
- Logout functionality
- Sub-module container management

### Future Enhancements

- User profile section
- Recent activity tracking
- Customizable menu layout
- Module permissions and access control
- Breadcrumbs navigation

## Class Structure

```javascript
class MainModule {
  constructor(app, container) {
    this.app = app;          // Reference to main app instance
    this.container = container; // DOM container for this module
    this.logger = app.logger; // Access to logging system
    this.currentSubModule = null; // Track active sub-module
  }

  async init() {
    // Load HTML template and set up event listeners
  }

  setupEventListeners() {
    // Attach menu button click handlers
    // Attach logout button click handler
  }

  async loadSubModule(moduleName) {
    // Hide current sub-module
    // Load new sub-module into sub-container
    // Update active button state
  }

  hideSubModule(moduleName) {
    // Hide specified sub-module
  }

  updateActiveButton(activeModule) {
    // Update button styling to show active module
  }

  handleLogout() {
    // Clear JWT and return to login
  }

  destroy() {
    // Clean up resources
  }
}
```

## Integration Points

### App Services Used

- `app.logger` - For logging information and errors
- `app.loadModule(moduleName, targetContainer)` - To load sub-modules
- `app.hideModule(moduleName)` - To hide sub-modules
- `app.clearJWT()` - To clear authentication on logout
- `app.loadModule('login')` - To return to login screen

### Events Handled

- Menu button clicks
- Logout button click

## Development Guidelines

### Adding New Menu Items

1. Add button to `main.html` with appropriate `data-module` attribute
2. Add button styling and layout
3. Test navigation to the target module

### Adding Sub-Module Support

1. Create new module in `src/modules/`
2. Add button to main menu with matching `data-module` value
3. Ensure module can be loaded into sub-container

### Testing

- Test navigation between all modules
- Test active button highlighting
- Test logout functionality
- Test sub-module loading and unloading

### Styling

- Use Bootstrap classes for menu layout
- Follow existing styling patterns for buttons
- Ensure responsive design for different screen sizes

## Dependencies

- Bootstrap 5 (for menu styling and components)
- Font Awesome (for icons)
- Main app services (logger, module loading)

## Best Practices

- Keep navigation logic separate from UI
- Provide clear visual feedback for active module
- Handle module loading errors gracefully
- Log navigation events for debugging
- Ensure smooth transitions between modules