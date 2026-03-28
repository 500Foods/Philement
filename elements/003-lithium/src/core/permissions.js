/**
 * Permissions - Punchcard-based permission system
 * 
 * Handles manager access and feature permissions based on
 * JWT punchcard claim. Falls back to allow-all if no punchcard present.
 */

import { getClaims } from './jwt.js';

/**
 * Get punchcard from current JWT
 * @returns {Object|null} Punchcard object or null
 */
function getPunchcard() {
  // In a real implementation, this would get the token from storage
  // For now, we accept a token parameter in the functions
  return null;
}

/**
 * Check if user can access a manager
 * @param {number|string} managerId - Manager ID
 * @param {Object} punchcard - Punchcard from JWT (optional, falls back to all access)
 * @returns {boolean} True if access allowed
 */
export function canAccessManager(managerId, punchcard = null) {
  // If no punchcard provided, try to get from current JWT
  if (!punchcard) {
    // Fallback: allow all managers if no punchcard
    return true;
  }

  // If punchcard exists but has no managers array, deny all
  if (!punchcard.managers || !Array.isArray(punchcard.managers)) {
    return false;
  }

  // Check if manager ID is in the allowed list
  return punchcard.managers.includes(Number(managerId));
}

/**
 * Check if user has a specific feature for a manager
 * @param {number|string} managerId - Manager ID
 * @param {number|string} featureId - Feature ID
 * @param {Object} punchcard - Punchcard from JWT (optional)
 * @returns {boolean} True if feature allowed
 */
export function hasFeature(managerId, featureId, punchcard = null) {
  // If no punchcard provided, fallback: allow all features
  if (!punchcard) {
    return true;
  }

  // Check manager access first
  if (!canAccessManager(managerId, punchcard)) {
    return false;
  }

  // If no features defined, allow none
  if (!punchcard.features || typeof punchcard.features !== 'object') {
    return false;
  }

  // Get features for this manager
  const managerFeatures = punchcard.features[String(managerId)];
  if (!Array.isArray(managerFeatures)) {
    return false;
  }

  return managerFeatures.includes(Number(featureId));
}

/**
 * Get list of permitted manager IDs
 * @param {Object} punchcard - Punchcard from JWT (optional)
 * @returns {Array} Array of manager IDs
 */
export function getPermittedManagers(punchcard = null) {
  // Fallback: return all default managers if no punchcard
  if (!punchcard) {
    // Return default manager IDs from lithium.json (007-032)
    // Excludes Group 0: System (001-006) which are hidden from main menu
    return [7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32];
  }

  return punchcard.managers || [];
}

/**
 * Get features for a specific manager
 * @param {number|string} managerId - Manager ID
 * @param {Object} punchcard - Punchcard from JWT (optional)
 * @returns {Array} Array of feature IDs
 */
export function getFeaturesForManager(managerId, punchcard = null) {
  // Fallback: return all features if no punchcard
  if (!punchcard) {
    return [1, 2, 3, 4, 5];
  }

  if (!punchcard.features || typeof punchcard.features !== 'object') {
    return [];
  }

  return punchcard.features[String(managerId)] || [];
}

/**
 * Parse permissions from JWT claims
 * @param {Object} claims - JWT claims object
 * @returns {Object} Parsed permissions
 */
export function parsePermissions(claims) {
  if (!claims || !claims.punchcard) {
    return {
      hasPunchcard: false,
      // Default manager IDs from lithium.json (007-032)
      // Excludes Group 0: System (001-006) which are hidden from main menu
      managers: [7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32],
      features: {},
    };
  }

  return {
    hasPunchcard: true,
    managers: claims.punchcard.managers || [],
    features: claims.punchcard.features || {},
  };
}

// Default export
export default {
  canAccessManager,
  hasFeature,
  getPermittedManagers,
  getFeaturesForManager,
  parsePermissions,
};
