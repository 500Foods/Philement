// Lithium PWA - Main Application Module
// This is the source version that will be built and minified for production

// Import styles
import './lithium.css';

// Main Application Class
class LithiumApp {
  constructor() {
    this.version = '1.0.0';
    this.name = 'Lithium PWA';

    // Module management
    this.loadedModules = new Map(); // moduleName -> { container, instance }
    this.currentModule = null;

    // Core services
    this.logger = {
      info: (msg) => console.log(`[INFO] ${msg}`),
      error: (msg, err) => console.error(`[ERROR] ${msg}`, err),
      success: (msg) => console.log(`[SUCCESS] ${msg}`)
    };

    this.network = {
      isOnline: navigator.onLine,
      setOnlineStatus: (status) => { this.network.isOnline = status; }
    };

    console.log(`${this.name} v${this.version} initializing...`);
  }

  async init() {
    try {
      // Check authentication and load initial module
      await this.checkAuthAndLoadModule();

      console.log('Application initialized successfully');
      return true;
    } catch (error) {
      console.error('Failed to initialize application:', error);
      return false;
    }
  }

  setupEventListeners() {
    // Online/offline detection
    window.addEventListener('online', () => this.handleOnlineStatus(true));
    window.addEventListener('offline', () => this.handleOnlineStatus(false));
    
    // PWA installation
    window.addEventListener('beforeinstallprompt', (e) => this.handleInstallPrompt(e));
    window.addEventListener('appinstalled', () => this.handleAppInstalled());
  }

  handleOnlineStatus(isOnline) {
    this.network.setOnlineStatus(isOnline);
    this.logger.info(`Network status: ${isOnline ? 'Online' : 'Offline'}`);
  }

  handleInstallPrompt(e) {
    e.preventDefault();
    this.deferredPrompt = e;
    this.logger.info('PWA install prompt available');
    
    // Show install button in UI
    this.showInstallButton();
  }

  handleAppInstalled() {
    this.logger.info('PWA was installed');
    this.deferredPrompt = null;
  }

  showInstallButton() {
    // This would be implemented in the UI module
    this.logger.info('Install button should be shown in UI');
  }

  // Check authentication and load initial module
  async checkAuthAndLoadModule() {
    const jwt = this.getJWT();
    if (this.isValidJWT(jwt)) {
      this.logger.info('Valid JWT found, loading main menu');
      await this.loadModule('main');
    } else {
      this.logger.info('No valid JWT, loading login');
      await this.loadModule('login');
    }
  }

  // JWT utility methods
  getJWT() {
    return localStorage.getItem('lithium_jwt');
  }

  setJWT(token) {
    localStorage.setItem('lithium_jwt', token);
  }

  clearJWT() {
    localStorage.removeItem('lithium_jwt');
  }

  isValidJWT(token) {
    // TODO: Implement proper JWT validation (decode, check expiry, etc.)
    // For now, just check if it exists and is not empty
    return token && token.length > 0;
  }

  // Module loading system
  async loadModule(moduleName, targetContainer = null) {
    try {
      this.logger.info(`Loading module: ${moduleName}`);

      // For main modules (login, main), hide current if exists
      if (!targetContainer && this.currentModule) {
        this.hideModule(this.currentModule);
      }

      // Check if module is already loaded
      if (this.loadedModules.has(moduleName)) {
        this.showModule(moduleName);
        if (!targetContainer) {
          this.currentModule = moduleName;
        }
        return;
      }

      // Load module CSS if it exists
      try {
        const cssPath = `/src/modules/${moduleName}/${moduleName}.css`;
        await import(/* @vite-ignore */ cssPath);
        this.logger.info(`Loaded CSS for module: ${moduleName}`);
      } catch (cssError) {
        // Module doesn't have specific CSS, that's fine
        this.logger.info(`No specific CSS found for module: ${moduleName}`);
      }

      // Load module dynamically
      const modulePath = `/src/modules/${moduleName}/${moduleName}.js`;
      const module = await import(/* @vite-ignore */ modulePath);

      // Create container div
      const container = document.createElement('div');
      container.id = `module-${moduleName}`;
      container.className = 'module-container';
      container.style.display = 'none';

      // Determine where to append the container
      let parentContainer;
      if (targetContainer) {
        parentContainer = targetContainer;
      } else {
        parentContainer = document.getElementById('app');
        if (!parentContainer) {
          throw new Error('App container not found');
        }
      }

      parentContainer.appendChild(container);

      // Initialize module
      const moduleInstance = new module.default(this, container);
      await moduleInstance.init();

      // Store loaded module
      this.loadedModules.set(moduleName, { container, instance: moduleInstance });

      // Show the module
      this.showModule(moduleName);
      if (!targetContainer) {
        this.currentModule = moduleName;
      }

      this.logger.success(`Module ${moduleName} loaded successfully`);
    } catch (error) {
      this.logger.error(`Failed to load module ${moduleName}:`, error);
    }
  }

  showModule(moduleName) {
    const moduleData = this.loadedModules.get(moduleName);
    if (moduleData) {
      // Remove any existing crossfade classes
      moduleData.container.classList.remove('module-crossfade-out');
      
      // Add crossfade-in class for animation
      moduleData.container.classList.add('module-crossfade-in');
      moduleData.container.style.display = 'block';
      
      // Remove the animation class after animation completes
      setTimeout(() => {
        moduleData.container.classList.remove('module-crossfade-in');
      }, 300);
    }
  }

  hideModule(moduleName) {
    const moduleData = this.loadedModules.get(moduleName);
    if (moduleData) {
      // Add crossfade-out class for animation
      moduleData.container.classList.add('module-crossfade-out');
      
      // Hide the module after animation completes
      setTimeout(() => {
        moduleData.container.style.display = 'none';
        moduleData.container.classList.remove('module-crossfade-out');
      }, 300);
    }
  }

  // Utility method to get current app state
  getState() {
    return {
      version: this.version,
      name: this.name,
      online: this.network ? this.network.isOnline : navigator.onLine,
      currentModule: this.currentModule,
      timestamp: new Date().toISOString()
    };
  }
}

// Export the main app class
export { LithiumApp };

// Initialize the app when the DOM is ready
document.addEventListener('DOMContentLoaded', async () => {
  const app = new LithiumApp();
  await app.init();
});