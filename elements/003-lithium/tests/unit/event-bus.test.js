/**
 * Event Bus Unit Tests
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { eventBus, Events } from '../../src/core/event-bus.js';

describe('EventBus', () => {
  beforeEach(() => {
    // Remove all listeners after each test
    eventBus.removeEventListener(Events.AUTH_LOGIN, () => {});
    eventBus.removeEventListener(Events.AUTH_LOGOUT, () => {});
  });

  describe('emit and on', () => {
    it('should emit and receive events', () => {
      const handler = vi.fn();
      eventBus.on(Events.AUTH_LOGIN, handler);
      
      eventBus.emit(Events.AUTH_LOGIN, { userId: 42 });
      
      expect(handler).toHaveBeenCalledTimes(1);
      expect(handler.mock.calls[0][0].detail).toEqual({ userId: 42 });
    });

    it('should support multiple listeners for the same event', () => {
      const handler1 = vi.fn();
      const handler2 = vi.fn();
      
      eventBus.on(Events.AUTH_LOGIN, handler1);
      eventBus.on(Events.AUTH_LOGIN, handler2);
      
      eventBus.emit(Events.AUTH_LOGIN, { userId: 42 });
      
      expect(handler1).toHaveBeenCalledTimes(1);
      expect(handler2).toHaveBeenCalledTimes(1);
    });

    it('should emit events without detail', () => {
      const handler = vi.fn();
      eventBus.on(Events.AUTH_LOGOUT, handler);
      
      eventBus.emit(Events.AUTH_LOGOUT);
      
      expect(handler).toHaveBeenCalledTimes(1);
      expect(handler.mock.calls[0][0].detail).toBeNull();
    });
  });

  describe('off', () => {
    it('should remove event listeners', () => {
      const handler = vi.fn();
      eventBus.on(Events.AUTH_LOGIN, handler);
      eventBus.off(Events.AUTH_LOGIN, handler);
      
      eventBus.emit(Events.AUTH_LOGIN, { userId: 42 });
      
      expect(handler).not.toHaveBeenCalled();
    });
  });

  describe('once', () => {
    it('should only trigger once', () => {
      const handler = vi.fn();
      eventBus.once(Events.AUTH_LOGIN, handler);
      
      eventBus.emit(Events.AUTH_LOGIN, { userId: 1 });
      eventBus.emit(Events.AUTH_LOGIN, { userId: 2 });
      
      expect(handler).toHaveBeenCalledTimes(1);
      expect(handler.mock.calls[0][0].detail).toEqual({ userId: 1 });
    });
  });

  describe('Events constants', () => {
    it('should have all standard event names defined', () => {
      expect(Events.AUTH_LOGIN).toBe('auth:login');
      expect(Events.AUTH_LOGOUT).toBe('auth:logout');
      expect(Events.AUTH_EXPIRED).toBe('auth:expired');
      expect(Events.AUTH_RENEWED).toBe('auth:renewed');
      expect(Events.PERMISSIONS_UPDATED).toBe('permissions:updated');
      expect(Events.MANAGER_LOADED).toBe('manager:loaded');
      expect(Events.MANAGER_SWITCHED).toBe('manager:switched');
      expect(Events.THEME_CHANGED).toBe('theme:changed');
      expect(Events.LOCALE_CHANGED).toBe('locale:changed');
      expect(Events.NETWORK_ONLINE).toBe('network:online');
      expect(Events.NETWORK_OFFLINE).toBe('network:offline');
    });
  });
});
