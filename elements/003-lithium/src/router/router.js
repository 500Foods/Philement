// Lithium Router - SPA Routing Module

class Router {
  constructor() {
    this.routes = {};
    this.currentRoute = null;
    this.appContainer = null;
  }

  init() {
    // Initialize the router
    this.appContainer = document.getElementById('app');
    
    // Set up default routes
    this.addRoute('/', this.homeRoute.bind(this));
    this.addRoute('/about', this.aboutRoute.bind(this));
    this.addRoute('/settings', this.settingsRoute.bind(this));
    this.addRoute('/404', this.notFoundRoute.bind(this));
  }

  start() {
    // Handle navigation
    window.addEventListener('popstate', () => this.handleNavigation());
    window.addEventListener('click', (e) => this.handleLinkClick(e));
    
    // Initialize the router
    this.handleNavigation();
  }

  addRoute(path, handler) {
    this.routes[path] = handler;
  }

  handleNavigation() {
    const path = window.location.pathname;
    const routeHandler = this.routes[path] || this.routes['/404'];
    
    if (routeHandler) {
      this.currentRoute = path;
      routeHandler();
    }
  }

  handleLinkClick(e) {
    if (e.target.tagName === 'A' && e.target.href.startsWith(window.location.origin)) {
      e.preventDefault();
      const path = new URL(e.target.href).pathname;
      window.history.pushState({}, '', path);
      this.handleNavigation();
    }
  }

  navigate(path) {
    window.history.pushState({}, '', path);
    this.handleNavigation();
  }

  // Route handlers
  homeRoute() {
    this.renderPage('<div class="page home-page"><h2>Home</h2><p>Welcome to Lithium PWA!</p></div>');
  }

  aboutRoute() {
    this.renderPage('<div class="page about-page"><h2>About</h2><p>About Lithium PWA</p></div>');
  }

  settingsRoute() {
    this.renderPage('<div class="page settings-page"><h2>Settings</h2><p>App Settings</p></div>');
  }

  notFoundRoute() {
    this.renderPage('<div class="page not-found-page"><h2>404</h2><p>Page Not Found</p></div>');
  }

  renderPage(content) {
    if (this.appContainer) {
      this.appContainer.innerHTML = content;
      this.appContainer.classList.add('fade-in');
    }
  }
}

export { Router };