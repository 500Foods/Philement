// Lithium PWA - Main Application Module
// This is the source version that will be built and minified for production

// Import styles
import './lithium.css';

// Import utility modules
import { Logger } from './utils/logger.js';
import { Router } from './router/router.js';
import { Storage } from './storage/storage.js';
import { Network } from './network/network.js';

// Main Application Class
class LithiumApp {
  constructor() {
    this.version = '1.0.0';
    this.name = 'Lithium PWA';
    this.logger = new Logger('LithiumApp');
    this.router = new Router();
    this.storage = new Storage();
    this.network = new Network();
    
    this.logger.info(`${this.name} v${this.version} initializing...`);
  }

  async init() {
    try {
      // Initialize all modules
      await this.initModules();
      
      // Set up event listeners
      this.setupEventListeners();
      
      // Start the router
      this.router.start();
      
      this.logger.success('Application initialized successfully');
      return true;
    } catch (error) {
      this.logger.error('Failed to initialize application:', error);
      return false;
    }
  }

  async initModules() {
    // Initialize storage
    await this.storage.init();
    
    // Initialize network
    await this.network.init();
    
    // Initialize router
    this.router.init();
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

  // Utility method to get current app state
  getState() {
    return {
      version: this.version,
      name: this.name,
      online: this.network ? this.network.isOnline : navigator.onLine,
      timestamp: new Date().toISOString()
    };
  }
}

// Export the main app class
export { LithiumApp };