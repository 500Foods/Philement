// Login Module for Lithium PWA
// Handles user authentication with placeholder credentials

import htmlTemplate from './login.html?raw';

class LoginModule {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.logger = app.logger;
  }

  async init() {
    this.logger.info('Initializing login module');

    // Load HTML template
    this.container.innerHTML = htmlTemplate;

    // Set up event listeners
    this.setupEventListeners();

    this.logger.info('Login module initialized');
  }

  setupEventListeners() {
    const loginForm = this.container.querySelector('#login-form');
    const loginButton = this.container.querySelector('#login-button');

    if (loginForm) {
      loginForm.addEventListener('submit', (e) => {
        e.preventDefault();
        this.handleLogin();
      });
    }

    if (loginButton) {
      loginButton.addEventListener('click', () => {
        this.handleLogin();
      });
    }
  }

  handleLogin() {
    const username = this.container.querySelector('#username').value;
    const password = this.container.querySelector('#password').value;

    this.logger.info(`Login attempt for user: ${username}`);

    // Placeholder authentication - accept any non-empty credentials
    if (username.trim() && password.trim()) {
      // Simulate successful login
      const fakeJWT = 'fake-jwt-token-' + Date.now();
      this.app.setJWT(fakeJWT);

      this.logger.success('Login successful, loading main menu');

      // Load main menu module
      this.app.loadModule('main');
    } else {
      // Show error
      this.showError('Please enter both username and password');
    }
  }

  showError(message) {
    let errorDiv = this.container.querySelector('.error-message');
    if (!errorDiv) {
      errorDiv = document.createElement('div');
      errorDiv.className = 'error-message alert alert-danger mt-3';
      this.container.querySelector('.login-form').appendChild(errorDiv);
    }
    errorDiv.textContent = message;
    errorDiv.style.display = 'block';
  }

  destroy() {
    // Clean up if needed
    this.logger.info('Destroying login module');
  }
}

export default LoginModule;