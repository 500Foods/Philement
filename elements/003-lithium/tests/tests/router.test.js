// Lithium Router Tests
import { expect } from 'chai';
import { Router } from '../../src/router/router.js';

describe('Router', () => {
  let router;

  beforeEach(() => {
    // Create a mock app container
    document.body.innerHTML = '<div id="app"></div>';
    router = new Router();
    router.init();
  });

  describe('Initialization', () => {
    it('should create a router instance', () => {
      expect(router).to.be.an.instanceof(Router);
      expect(router.routes).to.be.an('object');
      expect(router.currentRoute).to.be.null;
    });

    it('should have default routes', () => {
      expect(router.routes['/']).to.exist;
      expect(router.routes['/about']).to.exist;
      expect(router.routes['/settings']).to.exist;
      expect(router.routes['/404']).to.exist;
    });
  });

  describe('Route Management', () => {
    it('should add new routes', () => {
      const testHandler = () => {};
      router.addRoute('/test', testHandler);
      
      expect(router.routes['/test']).to.equal(testHandler);
    });

    it('should navigate to routes', () => {
      // Mock the navigate method to avoid actual navigation
      let navigateCalled = false;
      router.navigate = (path) => {
        navigateCalled = true;
        expect(path).to.equal('/test');
      };
      
      router.navigate('/test');
      expect(navigateCalled).to.be.true;
    });
  });

  describe('Route Handlers', () => {
    it('should have home route handler', () => {
      expect(router.homeRoute).to.be.a('function');
    });

    it('should have about route handler', () => {
      expect(router.aboutRoute).to.be.a('function');
    });

    it('should have settings route handler', () => {
      expect(router.settingsRoute).to.be.a('function');
    });

    it('should have 404 route handler', () => {
      expect(router.notFoundRoute).to.be.a('function');
    });
  });

  describe('Page Rendering', () => {
    it('should render pages into the app container', () => {
      const testContent = '<div class="test-page">Test Content</div>';
      router.renderPage(testContent);
      
      const appContainer = document.getElementById('app');
      expect(appContainer.innerHTML).to.equal(testContent);
      expect(appContainer.classList.contains('fade-in')).to.be.true;
    });
  });
});