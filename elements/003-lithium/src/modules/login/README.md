# Login Module

## Overview

The Login Module handles user authentication for the Lithium PWA. It provides a username/password form and manages the authentication flow.

## CHANGELOG

2025-12-31 - Initially created.

## Module Structure

```files
login/
├── login.js        # Main module JavaScript class
├── login.html      # HTML template for login form
└── README.md       # This file
```

## Functionality

### Current Features

- Username/password form with validation
- Placeholder authentication (accepts any non-empty credentials)
- JWT token generation and storage
- Error handling and user feedback
- Transition to main menu on successful login

### Future Enhancements

- Real authentication backend integration
- Password recovery functionality
- Remember me option
- Social login integration
- Multi-factor authentication

## Class Structure

```javascript
class LoginModule {
  constructor(app, container) {
    this.app = app;          // Reference to main app instance
    this.container = container; // DOM container for this module
    this.logger = app.logger; // Access to logging system
  }

  async init() {
    // Load HTML template and set up event listeners
  }

  setupEventListeners() {
    // Attach form submission and button click handlers
  }

  handleLogin() {
    // Validate credentials and generate JWT
  }

  showError(message) {
    // Display error messages to user
  }

  destroy() {
    // Clean up resources
  }
}
```

## Integration Points

### App Services Used

- `app.logger` - For logging information and errors
- `app.setJWT(token)` - To store authentication token
- `app.loadModule('main')` - To transition to main menu

### Events Handled

- Form submission
- Login button click

## Development Guidelines

### Adding New Features

1. Add new UI elements to `login.html`
2. Implement event handlers in `setupEventListeners()`
3. Add business logic to appropriate methods
4. Update this README with new functionality

### Testing

- Test with valid credentials (should succeed)
- Test with empty credentials (should show error)
- Test JWT storage in localStorage
- Test transition to main menu

### Styling

- Use Bootstrap classes for form elements
- Follow existing styling patterns
- Add custom styles to main CSS file if needed

## Dependencies

- Bootstrap 5 (for form styling and components)
- Font Awesome (for icons)
- Main app services (logger, JWT management)

## Best Practices

- Keep authentication logic separate from UI
- Provide clear error messages
- Sanitize user input
- Handle edge cases gracefully
- Log important events for debugging