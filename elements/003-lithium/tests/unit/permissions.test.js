/**
 * Permissions Unit Tests
 */

import { describe, it, expect } from 'vitest';
import {
  canAccessManager,
  hasFeature,
  getPermittedManagers,
  getFeaturesForManager,
  parsePermissions,
} from '../../src/core/permissions.js';

describe('Permissions', () => {
  describe('canAccessManager', () => {
    it('should allow access when no punchcard provided (fallback)', () => {
      expect(canAccessManager(1)).toBe(true);
      expect(canAccessManager(99)).toBe(true);
    });

    it('should deny access when punchcard has no managers array', () => {
      const punchcard = { features: {} };
      expect(canAccessManager(1, punchcard)).toBe(false);
    });

    it('should allow access when manager is in punchcard', () => {
      const punchcard = { managers: [1, 2, 3] };
      expect(canAccessManager(1, punchcard)).toBe(true);
      expect(canAccessManager(2, punchcard)).toBe(true);
    });

    it('should deny access when manager is not in punchcard', () => {
      const punchcard = { managers: [1, 2] };
      expect(canAccessManager(3, punchcard)).toBe(false);
      expect(canAccessManager(99, punchcard)).toBe(false);
    });

    it('should handle string manager IDs', () => {
      const punchcard = { managers: [1, 2, 3] };
      expect(canAccessManager('1', punchcard)).toBe(true);
      expect(canAccessManager('99', punchcard)).toBe(false);
    });
  });

  describe('hasFeature', () => {
    it('should allow feature when no punchcard provided (fallback)', () => {
      expect(hasFeature(1, 1)).toBe(true);
      expect(hasFeature(99, 99)).toBe(true);
    });

    it('should deny feature when manager is not accessible', () => {
      const punchcard = {
        managers: [1],
        features: { '1': [1, 2, 3] },
      };
      expect(hasFeature(2, 1, punchcard)).toBe(false);
    });

    it('should deny feature when features object is missing', () => {
      const punchcard = { managers: [1] };
      expect(hasFeature(1, 1, punchcard)).toBe(false);
    });

    it('should deny feature when manager has no features', () => {
      const punchcard = {
        managers: [1],
        features: { '2': [1, 2] },
      };
      expect(hasFeature(1, 1, punchcard)).toBe(false);
    });

    it('should allow feature when in manager features list', () => {
      const punchcard = {
        managers: [1, 2],
        features: {
          '1': [1, 2, 3],
          '2': [1],
        },
      };
      expect(hasFeature(1, 1, punchcard)).toBe(true);
      expect(hasFeature(1, 3, punchcard)).toBe(true);
      expect(hasFeature(2, 1, punchcard)).toBe(true);
    });

    it('should deny feature when not in manager features list', () => {
      const punchcard = {
        managers: [1],
        features: { '1': [1, 2] },
      };
      expect(hasFeature(1, 3, punchcard)).toBe(false);
      expect(hasFeature(1, 99, punchcard)).toBe(false);
    });

    it('should handle string feature IDs', () => {
      const punchcard = {
        managers: [1],
        features: { '1': [1, 2, 3] },
      };
      expect(hasFeature(1, '1', punchcard)).toBe(true);
      expect(hasFeature(1, '99', punchcard)).toBe(false);
    });
  });

  describe('getPermittedManagers', () => {
    it('should return default managers when no punchcard provided', () => {
      const managers = getPermittedManagers();
      // Profile Manager (ID 2) is a utility manager, not in the menu
      expect(managers).toEqual([1, 3, 4, 5]);
    });

    it('should return managers from punchcard', () => {
      const punchcard = { managers: [10, 20, 30] };
      const managers = getPermittedManagers(punchcard);
      expect(managers).toEqual([10, 20, 30]);
    });

    it('should return empty array when punchcard has no managers', () => {
      const punchcard = { features: {} };
      const managers = getPermittedManagers(punchcard);
      expect(managers).toEqual([]);
    });
  });

  describe('getFeaturesForManager', () => {
    it('should return all features when no punchcard provided', () => {
      const features = getFeaturesForManager(1);
      expect(features).toEqual([1, 2, 3, 4, 5]);
    });

    it('should return features for manager from punchcard', () => {
      const punchcard = {
        managers: [1],
        features: { '1': [10, 20, 30] },
      };
      const features = getFeaturesForManager(1, punchcard);
      expect(features).toEqual([10, 20, 30]);
    });

    it('should return empty array when manager has no features', () => {
      const punchcard = {
        managers: [1],
        features: { '2': [1, 2] },
      };
      const features = getFeaturesForManager(1, punchcard);
      expect(features).toEqual([]);
    });

    it('should return empty array when features object is missing', () => {
      const punchcard = { managers: [1] };
      const features = getFeaturesForManager(1, punchcard);
      expect(features).toEqual([]);
    });
  });

  describe('parsePermissions', () => {
    it('should parse permissions from claims with punchcard', () => {
      const claims = {
        user_id: 42,
        punchcard: {
          managers: [1, 2],
          features: { '1': [1, 2] },
        },
      };
      
      const perms = parsePermissions(claims);
      
      expect(perms.hasPunchcard).toBe(true);
      expect(perms.managers).toEqual([1, 2]);
      expect(perms.features).toEqual({ '1': [1, 2] });
    });

    it('should return defaults when claims have no punchcard', () => {
      const claims = { user_id: 42 };
      
      const perms = parsePermissions(claims);
      
      expect(perms.hasPunchcard).toBe(false);
      // Profile Manager (ID 2) is a utility manager, not in the menu
      expect(perms.managers).toEqual([1, 3, 4, 5]);
      expect(perms.features).toEqual({});
    });

    it('should return defaults when claims are null', () => {
      const perms = parsePermissions(null);
      
      expect(perms.hasPunchcard).toBe(false);
      // Profile Manager (ID 2) is a utility manager, not in the menu
      expect(perms.managers).toEqual([1, 3, 4, 5]);
    });

    it('should handle punchcard with empty arrays', () => {
      const claims = {
        punchcard: {
          managers: [],
          features: {},
        },
      };
      
      const perms = parsePermissions(claims);
      
      expect(perms.hasPunchcard).toBe(true);
      expect(perms.managers).toEqual([]);
      expect(perms.features).toEqual({});
    });
  });
});
