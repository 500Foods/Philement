/**
 * Main Menu Manager
 * 
 * Handles the main application layout: sidebar, header bar, workspace.
 * Loads permitted managers into the workspace on demand.
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims } from '../../core/jwt.js';
import { getPermittedManagers, canAccessManager } from '../../core/permissions.js';

/**
 * Main Menu Manager Class
 */
export default class MainManager {
  constructor(app, container, permittedManagers) {
    this.app = app;
    this.container = container;
    this.permittedManagers = permittedManagers || [];
    this.elements = {};
    this.currentManagerId = null;
    this.user = null;

    // Manager registry with icons
    this.managerIcons = {
      1: { icon: 'fa-palette', name: 'Style Manager' },
      2: { icon: 'fa-user-cog', name: 'Profile Manager' },
      3: { icon: 'fa-chart-line', name: 'Dashboard' },
      4: { icon: 'fa-list', name: 'Lookups' },
      5: { icon: 'fa-search', name: 'Queries' },
    };
  }

  /**
   * Initialize the main manager
   */
  async init() {
    console.log('[MainManager] Initializing...');
    
    try {
      await this.render();
      this.setupEventListeners();
      this.loadUserInfo();
      this.buildSidebar();
      this.show();

      // Load first permitted manager by default
      if (this.permittedManagers.length > 0) {
        await this.loadManager(this.permittedManagers[0]);
      }
    } catch (error) {
      console.error('[MainManager] Initialization error:', error);
    }
  }

  /**
   * Render the main layout
   */
  async render() {
    try {
      const response = await fetch('/src/managers/main/main.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[MainManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      layout: this.container.querySelector('#main-layout'),
      sidebar: this.container.querySelector('#sidebar'),
      sidebarMenu: this.container.querySelector('#sidebar-menu'),
      sidebarToggle: this.container.querySelector('#sidebar-toggle'),
      sidebarOverlay: this.container.querySelector('#sidebar-overlay'),
      mobileMenuBtn: this.container.querySelector('#mobile-menu-btn'),
      headerTitle: this.container.querySelector('#header-title'),
      workspace: this.container.querySelector('#workspace'),
      logoutBtn: this.container.querySelector('#logout-btn'),
      profileBtn: this.container.querySelector('#profile-btn'),
      profileAvatar: this.container.querySelector('#profile-avatar'),
      profileName: this.container.querySelector('#profile-name'),
      profileRole: this.container.querySelector('#profile-role'),
      themeBtn: this.container.querySelector('#theme-btn'),
      offlineIndicator: this.container.querySelector('#offline-indicator'),
    };
  }

  /**
   * Render a fallback layout if template fails to load
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="main-layout">
        <aside class="main-sidebar" id="sidebar">
          <div class="sidebar-menu" id="sidebar-menu"></div>
          <div class="sidebar-footer">
            <button class="logout-btn" id="logout-btn">Logout</button>
          </div>
        </aside>
        <div class="main-content">
          <header class="main-header">
            <h2 class="header-title" id="header-title">Dashboard</h2>
          </header>
          <main class="workspace" id="workspace"></main>
        </div>
      </div>
    `;
  }

  /**
   * Set up event listeners
   */
  setupEventListeners() {
    // Logout button
    this.elements.logoutBtn?.addEventListener('click', () => {
      eventBus.emit(Events.AUTH_LOGOUT);
    });

    // Sidebar toggle (collapse/expand)
    this.elements.sidebarToggle?.addEventListener('click', () => {
      this.toggleSidebar();
    });

    // Mobile menu button
    this.elements.mobileMenuBtn?.addEventListener('click', () => {
      this.openMobileSidebar();
    });

    // Sidebar overlay (close on click)
    this.elements.sidebarOverlay?.addEventListener('click', () => {
      this.closeMobileSidebar();
    });

    // Theme button (stub)
    this.elements.themeBtn?.addEventListener('click', () => {
      console.log('[MainManager] Theme toggle clicked (stub)');
    });

    // Profile button (stub)
    this.elements.profileBtn?.addEventListener('click', () => {
      console.log('[MainManager] Profile button clicked (stub)');
      // In the future, this could open a dropdown or navigate to profile
    });

    // Network status events
    eventBus.on(Events.NETWORK_ONLINE, () => {
      this.elements.offlineIndicator?.classList.remove('visible');
    });

    eventBus.on(Events.NETWORK_OFFLINE, () => {
      this.elements.offlineIndicator?.classList.add('visible');
    });
  }

  /**
   * Load user info from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (claims) {
      this.user = {
        id: claims.user_id,
        username: claims.username || 'User',
        email: claims.email,
        roles: claims.roles || [],
      };

      // Update profile display
      if (this.elements.profileName) {
        this.elements.profileName.textContent = this.user.username;
      }
      if (this.elements.profileAvatar) {
        this.elements.profileAvatar.textContent = this.user.username.charAt(0).toUpperCase();
      }
      if (this.elements.profileRole) {
        const role = Array.isArray(this.user.roles) && this.user.roles.length > 0
          ? this.user.roles[0]
          : 'User';
        this.elements.profileRole.textContent = role;
      }
    }
  }

  /**
   * Build sidebar menu from permitted managers
   */
  buildSidebar() {
    const menu = this.elements.sidebarMenu;
    if (!menu) return;

    menu.innerHTML = '';

    this.permittedManagers.forEach((managerId) => {
      if (!canAccessManager(managerId)) return;

      const managerInfo = this.managerIcons[managerId] || { icon: 'fa-cube', name: `Manager ${managerId}` };

      const button = document.createElement('div');
      button.className = 'menu-item';
      button.dataset.managerId = managerId;
      button.innerHTML = `
        <i class="fas ${managerInfo.icon}"></i>
        <span>${managerInfo.name}</span>
      `;

      button.addEventListener('click', () => {
        this.loadManager(managerId);
        this.closeMobileSidebar();
      });

      menu.appendChild(button);
    });
  }

  /**
   * Load a manager into the workspace
   */
  async loadManager(managerId) {
    if (this.currentManagerId === managerId) return;

    // Update active state in sidebar
    this.updateActiveMenuItem(managerId);

    // Update header title
    const managerInfo = this.managerIcons[managerId];
    if (managerInfo && this.elements.headerTitle) {
      this.elements.headerTitle.textContent = managerInfo.name;
    }

    // Delegate to app to load the manager
    await this.app.loadManager(managerId);
    this.currentManagerId = managerId;
  }

  /**
   * Update active state in sidebar menu
   */
  updateActiveMenuItem(managerId) {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    items?.forEach((item) => {
      if (parseInt(item.dataset.managerId, 10) === managerId) {
        item.classList.add('active');
      } else {
        item.classList.remove('active');
      }
    });
  }

  /**
   * Toggle sidebar collapse (desktop)
   */
  toggleSidebar() {
    this.elements.sidebar?.classList.toggle('collapsed');

    // Update toggle icon
    const icon = this.elements.sidebarToggle?.querySelector('i');
    if (icon) {
      const isCollapsed = this.elements.sidebar?.classList.contains('collapsed');
      icon.className = isCollapsed ? 'fas fa-chevron-right' : 'fas fa-chevron-left';
    }
  }

  /**
   * Open mobile sidebar
   */
  openMobileSidebar() {
    this.elements.sidebar?.classList.add('open');
    this.elements.sidebarOverlay?.classList.add('visible');
  }

  /**
   * Close mobile sidebar
   */
  closeMobileSidebar() {
    this.elements.sidebar?.classList.remove('open');
    this.elements.sidebarOverlay?.classList.remove('visible');
  }

  /**
   * Show the main layout
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.layout?.classList.add('visible');
    });
  }

  /**
   * Teardown the main manager
   */
  teardown() {
    // Clean up event listeners if needed
    this.elements = {};
  }
}
