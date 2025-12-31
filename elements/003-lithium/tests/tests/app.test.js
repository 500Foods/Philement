// Lithium App Tests
import { expect } from 'chai';
import { LithiumApp } from '../../src/app.js';

describe('LithiumApp', () => {
  let app;

  beforeEach(() => {
    app = new LithiumApp();
  });

  describe('Initialization', () => {
    it('should create an instance with correct properties', () => {
      expect(app).to.be.an.instanceof(LithiumApp);
      expect(app.version).to.equal('1.0.0');
      expect(app.name).to.equal('Lithium PWA');
    });

    it('should have logger, router, storage, and network modules', () => {
      expect(app.logger).to.exist;
      expect(app.router).to.exist;
      expect(app.storage).to.exist;
      expect(app.network).to.exist;
    });
  });

  describe('getState()', () => {
    it('should return the current app state', () => {
      const state = app.getState();
      
      expect(state).to.be.an('object');
      expect(state.version).to.equal('1.0.0');
      expect(state.name).to.equal('Lithium PWA');
      expect(state.timestamp).to.be.a('string');
    });
  });

  describe('Module Integration', () => {
    it('should have working logger module', () => {
      expect(app.logger).to.have.property('info');
      expect(app.logger).to.have.property('error');
      expect(app.logger).to.have.property('success');
    });

    it('should have working router module', () => {
      expect(app.router).to.have.property('addRoute');
      expect(app.router).to.have.property('navigate');
      expect(app.router).to.have.property('start');
    });
  });
});