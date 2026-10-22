/**
 * Course Manager access (CATCHUP L3, menu ID 34).
 *
 * QueryRef 155 resolves each JWT role_id to roles.name. Staff access
 * is a resolved name of staff or admin. A missing client, a failed
 * query, or an empty name list denies. There is no local id map.
 */

import { authQuery } from '../../shared/conduit.js';
import { getClaims } from '../../core/jwt.js';
import { parseRoleIds, isCourseManagerAuthorized } from '../../core/utils.js';
import { log, Subsystems, Status } from '../../core/log.js';

export const COURSE_MANAGER_ID = 34;
export const ROLE_NAME_QUERY_REF = 155;

/**
 * Drop Course Manager from grouped menu data when access is denied.
 * Returns the same array when access is allowed. Does not mutate it.
 * @param {Array} groups - Grouped menu from getMenu
 * @param {boolean} allowed - True when the JWT may see manager 34
 * @returns {Array}
 */
export function visibleMenuGroups(groups, allowed) {
  if (!Array.isArray(groups)) return [];
  if (allowed) return groups;
  return groups
    .map((group) => ({
      ...group,
      items: Array.isArray(group.items)
        ? group.items.filter((item) => item.managerId !== COURSE_MANAGER_ID)
        : [],
    }))
    .filter((group) => group.items.length > 0);
}

/**
 * Remove manager 34 from a permitted-id list when access is denied.
 * @param {number[]} permittedIds
 * @param {boolean} allowed
 * @returns {number[]}
 */
export function permittedManagersForMenu(permittedIds, allowed) {
  const ids = Array.isArray(permittedIds) ? permittedIds : [];
  if (allowed) return ids;
  return ids.filter((id) => id !== COURSE_MANAGER_ID);
}

function namesFromRows(rows) {
  if (!Array.isArray(rows)) return null;
  return rows
    .map((row) => row && row.name)
    .filter((name) => typeof name === 'string' && name.length > 0);
}

/**
 * Resolve whether the current JWT may open Course Manager.
 * @param {Object} api - API client with post(); omitted denies
 * @param {Object|null} claims - JWT claims; defaults to the stored token
 * @returns {Promise<{allowed: boolean, roleIds: number[], roleNames: string[]}>}
 */
export async function resolveCourseManagerAccess(api, claims = getClaims()) {
  const roleIds = parseRoleIds(claims && claims.roles);
  if (roleIds.length === 0) {
    log(Subsystems.MANAGER, Status.INFO, 'Course Manager: no role ids; access denied');
    return { allowed: false, roleIds, roleNames: [] };
  }
  if (!api) {
    log(Subsystems.MANAGER, Status.WARN,
      'Course Manager: no API client; role names were not resolved');
    return { allowed: false, roleIds, roleNames: [] };
  }

  let roleNames = [];
  try {
    const results = await Promise.all(
      roleIds.map((id) => authQuery(api, ROLE_NAME_QUERY_REF, {
        INTEGER: { ROLEID: id },
      }))
    );
    for (const rows of results) {
      const names = namesFromRows(rows);
      if (names === null) {
        throw new Error('QueryRef 155 returned a non-array');
      }
      roleNames = roleNames.concat(names);
    }
  } catch (err) {
    const message = err && err.message ? err.message : String(err);
    log(Subsystems.MANAGER, Status.WARN,
      `Course Manager: QueryRef 155 failed; access denied: ${message}`);
    return { allowed: false, roleIds, roleNames: [] };
  }

  const allowed = isCourseManagerAuthorized(roleIds, roleNames);
  log(Subsystems.MANAGER, Status.INFO,
    `Course Manager: role_ids=[${roleIds.join(',')}] names=[${roleNames.join(',')}] allowed=${allowed}`);
  return { allowed, roleIds, roleNames };
}
