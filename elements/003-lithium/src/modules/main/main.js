// Main Menu Module for Lithium PWA
// Displays navigation menu and loads sub-modules

import htmlTemplate from './main.html?raw';

class MainModule {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.logger = app.logger;
    this.currentSubModule = null;
  }

  async init() {
    this.logger.info('Initializing main menu module');

    // Load HTML template
    this.container.innerHTML = htmlTemplate;

    // Initialize split.js for resizable panels
    this.initializeSplitter();

    // Set up event listeners
    this.setupEventListeners();

    this.logger.info('Main menu module initialized');
  }

  setupEventListeners() {
    // Menu buttons
    const buttons = this.container.querySelectorAll('.menu-button');
    buttons.forEach(button => {
      button.addEventListener('click', (e) => {
        const moduleName = e.target.closest('.menu-button').dataset.module;
        this.loadSubModule(moduleName);
      });
    });

    // Logout button
    const logoutButton = this.container.querySelector('#logout-button');
    if (logoutButton) {
      logoutButton.addEventListener('click', () => {
        this.handleLogout();
      });
    }
  }

  initializeSplitter() {
    const menuSidebar = this.container.querySelector('.menu-sidebar');
    const contentArea = this.container.querySelector('.content-area');

    this.logger.info('Initializing splitter with elements:', {
      menuSidebar: !!menuSidebar,
      contentArea: !!contentArea,
      Split: typeof window.Split,
      SplitAvailable: !!window.Split
    });

    if (menuSidebar && contentArea) {
      // Wait a bit for split.js to load
      setTimeout(() => {
        if (window.Split) {
          try {
            this.splitInstance = window.Split([menuSidebar, contentArea], {
              sizes: [20, 80], // Initial sizes in percentages
              minSize: [150, 300], // Minimum sizes in pixels
              maxSize: [500, Infinity], // Maximum sizes in pixels
              expandToMin: false,
              gutterSize: 12,
              cursor: 'col-resize',
              direction: 'horizontal',
              snapOffset: 0,
              dragInterval: 1,
              onDrag: () => {
                this.logger.info('Splitter dragged');
              }
            });
            this.logger.info('Splitter initialized successfully');
          } catch (error) {
            this.logger.error('Error initializing splitter:', error);
          }
        } else {
          this.logger.warn('Split.js library not loaded after timeout');
        }
      }, 100);
    } else {
      this.logger.warn('Splitter elements not found');
    }
  }

  async loadSubModule(moduleName) {
    this.logger.info(`Loading sub-module: ${moduleName}`);

    // Get the sub-module container
    const subContainer = this.container.querySelector('#sub-module-container');
    
    // Ensure the sub-container is positioned relatively for crossfade
    subContainer.style.position = 'relative';

    // Hide current sub-module with crossfade
    if (this.currentSubModule) {
      this.hideSubModule(this.currentSubModule);
      
      // Wait for the fade-out animation to complete before loading new module
      await new Promise(resolve => setTimeout(resolve, 300));
    }

    // Load the sub-module into the sub-container
    await this.app.loadModule(moduleName, subContainer);

    // Update active button
    this.updateActiveButton(moduleName);

    this.currentSubModule = moduleName;
  }

  hideSubModule(moduleName) {
    // Since sub-modules are loaded at the app level, we need to hide them via app
    this.app.hideModule(moduleName);
  }

  updateActiveButton(activeModule) {
    const buttons = this.container.querySelectorAll('.menu-button');
    buttons.forEach(button => {
      if (button.dataset.module === activeModule) {
        button.classList.add('active');
      } else {
        button.classList.remove('active');
      }
    });
  }

  handleLogout() {
    this.logger.info('User logging out');

    // Clear JWT
    this.app.clearJWT();

    // Load login module
    this.app.loadModule('login');
  }

  destroy() {
    // Clean up split instance
    if (this.splitInstance && typeof this.splitInstance.destroy === 'function') {
      this.splitInstance.destroy();
      this.splitInstance = null;
    }

    this.logger.info('Destroying main menu module');
  }
}

export default MainModule;