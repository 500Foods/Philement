/**
 * Sidebar management for MainManager
 * Handles building the sidebar menu, menu items, and group management
 */

import { getMenu, buildManagerIconsRegistry, clearMenuCache } from '../../shared/menu.js';
import { canAccessManager } from '../../core/permissions.js';
import { processIcons, setIcon } from '../../core/icons.js';
import { initTooltips } from '../../core/tooltip-api.js';
import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Methods to be mixed into MainManager prototype
 */
export const sidebarMethods = {
  /**
   * Load menu data from server
   */
  async loadMenuData() {
    try {
      this.menuData = await getMenu(this.app.api);

      // Build manager icons registry from menu data
      this.managerIcons = buildManagerIconsRegistry(this.menuData);

      // Build flat list of accessible manager names for Crimson context.
      // This is the final filtered list — only managers that pass config,
      // permission, and visibility filters appear in the menu.
      this.accessibleManagerNames = Object.values(this.managerIcons)
        .map(info => info?.name)
        .filter(Boolean);

      // Update permitted managers from menu data
      const menuManagerIds = Object.keys(this.managerIcons).map(id => parseInt(id, 10));

      // Filter to only include managers that are in the menu and permitted
      this.permittedManagers = this.permittedManagers.filter(id => menuManagerIds.includes(id));

      // If no permitted managers from permission system, use all menu managers
      if (this.permittedManagers.length === 0) {
        this.permittedManagers = menuManagerIds;
      }
    } catch (error) {
      console.error('[MainManager] Failed to load menu data:', error);
      // Clear cache to force fresh fetch on next load
      clearMenuCache();
      // Fall back to static manager icons
      this.managerIcons = {
        1: { iconHtml: '<fa fa-palette></fa>', name: 'Style Manager' },
        2: { iconHtml: '<fa fa-user-cog></fa>', name: 'Profile Manager' },
        3: { iconHtml: '<fa fa-chart-line></fa>', name: 'Dashboard' },
        4: { iconHtml: '<fa fa-search></fa>', name: 'Queries' },
        5: { iconHtml: '<fa fa-list-check></fa>', name: 'Lookups' },
      };
      this.accessibleManagerNames = Object.values(this.managerIcons)
        .map(info => info?.name)
        .filter(Boolean);
    }
  },

  /**
   * Get the last-used manager key from localStorage.
   * @returns {string|null} Key in format 'manager:ID' or 'utility:KEY', or null
   */
  _getLastManagerKey() {
    try {
      return localStorage.getItem(LAST_MANAGER_KEY) || null;
    } catch (e) {
      return null;
    }
  },

  /**
   * Parse a last manager key into its components.
   * @param {string} key - The last manager key (e.g., 'manager:1' or 'utility:user-profile')
   * @returns {Object|null} Parsed type and id, or null if invalid
   * @private
   */
  _parseLastManagerKey(key) {
    const colonIdx = key.indexOf(':');
    if (colonIdx <= 0) return null;
    
    const type = key.substring(0, colonIdx);
    const id = key.substring(colonIdx + 1);
    return { type, id };
  },

  /**
   * Try to load a manager by ID.
   * @param {string} id - Manager ID string
   * @returns {boolean} True if manager was loaded successfully
   * @private
   */
  _tryLoadManager(id) {
    const managerId = parseInt(id, 10);
    if (isNaN(managerId)) return false;
    if (!this.permittedManagers.includes(managerId)) return false;
    
    this.loadManager(managerId);
    return true;
  },

  /**
   * Try to load a utility manager by key.
   * @param {string} key - Utility manager key
   * @returns {boolean} True if utility manager was loaded successfully
   * @private
   */
  _tryLoadUtility(key) {
    if (!this.app.utilityManagerRegistry[key]) return false;
    
    this.app.loadUtilityManager(key);
    return true;
  },

  /**
   * Load the initial manager after login.
   * Restores the last-used manager from localStorage if available and permitted.
   * Falls back to the user profile utility manager if no stored preference
   * or the stored manager is not in the permitted list.
   * 
   * On startup, all groups are collapsed except the one containing the active manager.
   */
  async _loadInitialManager() {
    const lastManagerKey = this._getLastManagerKey();
    let loadedManagerId = null;
    let loadedUtilityKey = null;

    if (lastManagerKey) {
      const parsed = this._parseLastManagerKey(lastManagerKey);
      
      if (parsed) {
        if (parsed.type === 'manager' && this._tryLoadManager(parsed.id)) {
          loadedManagerId = parsed.id;
        } else if (parsed.type === 'utility' && this._tryLoadUtility(parsed.id)) {
          loadedUtilityKey = parsed.id;
        }
      }
    }

    // If no manager was loaded, default to user profile
    if (!loadedManagerId && !loadedUtilityKey) {
      await this.app.loadUtilityManager('user-profile');
      loadedUtilityKey = 'user-profile';
    }

    // Collapse all groups, then expand only the group containing the active menu manager
    if (loadedManagerId) {
      // Menu manager loaded - collapse all, then expand its group
      this._collapseAllGroups();
      this._expandGroupForManager(loadedManagerId);
    } else {
      // Utility manager loaded (or default) - collapse all groups
      this._collapseAllGroups();
    }
  },

  /**
   * Build sidebar menu from menu data with collapsible groups
   */
  buildSidebar() {
    const menu = this.elements.sidebarMenu;
    if (!menu) return;

    menu.innerHTML = '';

    if (!this.menuData || this.menuData.length === 0) {
      // Fallback to static menu if no menu data
      this._buildStaticSidebar();
      return;
    }

    // Build from menu data with groups
    this.menuData.forEach((group) => {
      // Skip internal System/Internal groups (should not appear in menu)
      if (group.id === 0 || group.name === 'System' || group.name === 'Internal') return;

      // Filter items to only include permitted managers
      const permittedItems = group.items.filter(item => 
        this.permittedManagers.includes(item.managerId)
      );
      
      if (permittedItems.length === 0) return;

      const groupEl = document.createElement('div');
      groupEl.className = 'menu-group';
      groupEl.dataset.groupId = group.id;
      
      const isCollapsed = this.collapsedGroups.has(group.id);
      if (isCollapsed) {
        groupEl.classList.add('is-collapsed');
      }

      // Group header with toggle
      const headerEl = document.createElement('div');
      headerEl.className = 'menu-group-header';
      headerEl.innerHTML = `
        <span class="group-toggle-icon" style="transform: ${isCollapsed ? 'rotate(-90deg)' : 'rotate(0deg)'}">
          <fa fa-chevron-down></fa>
        </span>
        <span class="group-name">${group.name} (${permittedItems.length})</span>
      `;
      headerEl.addEventListener('click', () => this._toggleGroup(group.id));

      // Group items container
      const itemsEl = document.createElement('div');
      itemsEl.className = 'menu-group-items';
      if (isCollapsed || this.isSidebarCollapsed) {
        itemsEl.classList.add('collapsed');
      }

      // Build menu items
      permittedItems.forEach((item) => {
        const button = document.createElement('div');
        button.className = 'menu-item';
        button.dataset.managerId = item.managerId;
        const itemName = item.name || `Manager ${item.managerId}`;
        button.dataset.tooltip = itemName;
        button.dataset.tipWhen = 'collapsed';
        // Build HTML without extra whitespace to avoid parsing issues
        let htmlContent = item.iconHtml || '<fa fa-cube></fa>';
        htmlContent += `<span class="menu-item-name">${itemName}</span>`;
        if (item.count > 0) {
          htmlContent += `<span class="menu-item-count">${item.count}</span>`;
        }
        button.innerHTML = htmlContent;

        button.addEventListener('click', () => {
          this.loadManager(item.managerId);
          this.closeMobileSidebar();
        });

        itemsEl.appendChild(button);
      });

      groupEl.appendChild(headerEl);
      groupEl.appendChild(itemsEl);
      menu.appendChild(groupEl);
    });

    // Process icons
    processIcons(menu);
    initTooltips(menu);
  },

  /**
   * Build static sidebar as fallback when menu data is unavailable
   */
  _buildStaticSidebar() {
    const menu = this.elements.sidebarMenu;
    if (!menu) return;

    menu.innerHTML = '';

    this.permittedManagers.forEach((managerId) => {
      if (!canAccessManager(managerId)) return;

      const managerInfo = this.managerIcons[managerId] || { iconHtml: '<fa fa-cube></fa>', name: `Manager ${managerId}` };

      const button = document.createElement('div');
      button.className = 'menu-item';
      button.dataset.managerId = managerId;
      button.dataset.tooltip = managerInfo.name;
      button.dataset.tipWhen = 'collapsed';
      button.innerHTML = `${managerInfo.iconHtml}<span class="menu-item-name">${managerInfo.name}</span>`;

      button.addEventListener('click', () => {
        this.loadManager(managerId);
        this.closeMobileSidebar();
      });

      menu.appendChild(button);
    });

    processIcons(menu);
    initTooltips(menu);
  },

  /**
   * Load a manager into the workspace (creates a slot if needed)
   */
  async loadManager(managerId) {
    if (this.currentManagerId === managerId) return;

    // Clear any active utility button (menu managers take precedence)
    this.clearActiveUtilityButtons();
    // Reset utility key so clicking a utility button will work
    this.currentUtilityKey = null;

    // Update active state in sidebar
    this.updateActiveMenuItem(managerId);

    // Delegate to app to load the manager (app will create/show the slot)
    await this.app.loadManager(managerId);
    this.currentManagerId = managerId;
  },

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
  },

  /**
   * Update which menu items show as "open" (loaded but not active).
   * Applies 'manager-open' class to items whose manager is in the openManagers set.
   */
  updateOpenMenuItems() {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    const openManagers = this.app?.openManagers;

    items?.forEach((item) => {
      const managerId = parseInt(item.dataset.managerId, 10);
      // Check if this manager is in the open set (and is a numeric ID, not utility)
      const isOpen = !isNaN(managerId) && openManagers?.has(managerId);
      item.classList.toggle('manager-open', isOpen);
    });
  },

  /**
   * Set active utility manager button and clear menu selection
   * @param {string} utilityKey - The utility manager key (e.g., 'user-profile', 'session-log')
   */
  setActiveUtilityButton(utilityKey) {
    // Clear any active menu item
    this.clearActiveMenuItem();

    // Clear any other active utility buttons
    this.clearActiveUtilityButtons();

    // Set the active button
    const buttonMap = {
      'user-profile': this.elements.sidebarProfileBtn,
      'session-log': this.elements.sidebarLogsBtn,
    };

    const button = buttonMap[utilityKey];
    if (button) {
      button.classList.add('active');
    }

    this.currentUtilityKey = utilityKey;
    // Reset currentManagerId so that clicking a menu item will always show it
    this.currentManagerId = null;
  },

  /**
   * Clear active utility manager buttons
   */
  clearActiveUtilityButtons() {
    this.elements.sidebarProfileBtn?.classList.remove('active');
    this.elements.sidebarLogsBtn?.classList.remove('active');
    this.currentUtilityKey = null;
  },

  /**
   * Clear active menu item
   */
  clearActiveMenuItem() {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    items?.forEach((item) => {
      item.classList.remove('active');
    });
  },

  /**
   * Load collapsed group states from localStorage
   */
  _loadCollapsedGroups() {
    try {
      const stored = localStorage.getItem(COLLAPSED_GROUPS_KEY);
      if (stored) {
        const groupIds = JSON.parse(stored);
        this.collapsedGroups = new Set(groupIds);
      }
    } catch (e) {
      this.collapsedGroups = new Set();
    }
  },

  /**
   * Save collapsed group states to localStorage
   */
  _saveCollapsedGroups() {
    try {
      localStorage.setItem(COLLAPSED_GROUPS_KEY, JSON.stringify([...this.collapsedGroups]));
    } catch (e) {
      // Non-fatal
    }
  },

  /**
   * Toggle a group's collapsed state
   * @param {number} groupId - The group ID
   */
  _toggleGroup(groupId) {
    if (this.collapsedGroups.has(groupId)) {
      this.collapsedGroups.delete(groupId);
    } else {
      this.collapsedGroups.add(groupId);
    }
    this._saveCollapsedGroups();
    this._updateGroupVisibility(groupId);
  },

  /**
   * Update a group's visibility in the DOM
   * @param {number} groupId - The group ID
   */
  _updateGroupVisibility(groupId) {
    const groupEl = this.elements.sidebarMenu?.querySelector(`[data-group-id="${groupId}"]`);
    if (!groupEl) return;

    const itemsContainer = groupEl.querySelector('.menu-group-items');
    const toggleIcon = groupEl.querySelector('.group-toggle-icon');
    const isCollapsed = this.collapsedGroups.has(groupId);

    if (itemsContainer) {
      itemsContainer.classList.toggle('collapsed', isCollapsed);
    }
    if (toggleIcon) {
      toggleIcon.style.transform = isCollapsed ? 'rotate(-90deg)' : 'rotate(0deg)';
    }
    groupEl.classList.toggle('is-collapsed', isCollapsed);
  },

  /**
   * Collapse all groups and update the collapsedGroups Set
   */
  _collapseAllGroups() {
    if (!this.elements.sidebarMenu) return;
    
    // Update the Set for all groups
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(el => {
      const groupId = parseInt(el.dataset.groupId, 10);
      if (!isNaN(groupId)) {
        this.collapsedGroups.add(groupId);
      }
    });
    
    // Update DOM for all groups
    this.elements.sidebarMenu.querySelectorAll('.menu-group-items').forEach(el => {
      el.classList.add('collapsed');
    });
    this.elements.sidebarMenu.querySelectorAll('.group-toggle-icon').forEach(el => {
      el.style.transform = 'rotate(-90deg)';
    });
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(el => {
      el.classList.add('is-collapsed');
    });
  },

  /**
   * Expand all groups (for sidebar expanded mode)
   */
  _expandAllGroups() {
    if (!this.elements.sidebarMenu) return;
    
    this.elements.sidebarMenu.querySelectorAll('.menu-group-items').forEach(el => {
      const groupId = parseInt(el.closest('.menu-group')?.dataset.groupId, 10);
      el.classList.toggle('collapsed', this.collapsedGroups.has(groupId));
    });
    this.elements.sidebarMenu.querySelectorAll('.group-toggle-icon').forEach(el => {
      const groupId = parseInt(el.closest('.menu-group')?.dataset.groupId, 10);
      el.style.transform = this.collapsedGroups.has(groupId) ? 'rotate(-90deg)' : 'rotate(0deg)';
    });
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(el => {
      const groupId = parseInt(el.dataset.groupId, 10);
      el.classList.toggle('is-collapsed', this.collapsedGroups.has(groupId));
    });
  },

  /**
   * Find which group contains a specific manager ID
   * @param {number} managerId - The manager ID to find
   * @returns {number|null} The group ID, or null if not found
   */
  _findGroupForManager(managerId) {
    if (!this.menuData || !Array.isArray(this.menuData)) {
      return null;
    }
    const targetId = Number(managerId);
    if (isNaN(targetId)) return null;

    for (const group of this.menuData) {
      const hasManager = group.items.some(item => item.managerId === targetId);
      if (hasManager) {
        return group.id;
      }
    }

    return null;
  },

  /**
   * Expand a specific group (remove from collapsed set and update UI).
   * Assumes all groups are already collapsed - this just expands one.
   * @param {number} managerId - The manager ID whose group should be expanded
   */
  _expandGroupForManager(managerId) {
    const targetGroupId = this._findGroupForManager(managerId);
    
    if (targetGroupId === null) {
      return;
    }

    // Remove the target group from collapsed set (expanding it)
    this.collapsedGroups.delete(targetGroupId);

    // Save the state
    this._saveCollapsedGroups();

    // Update the UI for the target group
    this._updateGroupVisibility(targetGroupId);
  },

  /**
   * Expand groups that contain open managers.
   * This is called when expanding the sidebar to ensure open managers are visible.
   * @returns {Set<number>} The group IDs that were expanded
   */
  _expandGroupsWithOpenManagers() {
    if (!this.elements.sidebarMenu || !this.app?.openManagers) return new Set();
    
    // Find all groups that contain open managers
    const groupsToExpand = new Set();
    const openManagers = this.app.openManagers;
    
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(groupEl => {
      const groupId = parseInt(groupEl.dataset.groupId, 10);
      if (isNaN(groupId)) return;
      
      // Check if this group contains any open managers
      const menuItems = groupEl.querySelectorAll('.menu-item');
      for (const item of menuItems) {
        const managerId = parseInt(item.dataset.managerId, 10);
        if (!isNaN(managerId) && openManagers.has(managerId)) {
          groupsToExpand.add(groupId);
          break;
        }
      }
    });
    
    // Expand the identified groups (remove from collapsed set)
    groupsToExpand.forEach(groupId => {
      this.collapsedGroups.delete(groupId);
    });
    
    // Update DOM
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(groupEl => {
      const groupId = parseInt(groupEl.dataset.groupId, 10);
      if (isNaN(groupId)) return;
      
      const isCollapsed = this.collapsedGroups.has(groupId);
      const itemsEl = groupEl.querySelector('.menu-group-items');
      const toggleIcon = groupEl.querySelector('.group-toggle-icon');
      
      if (itemsEl) {
        itemsEl.classList.toggle('collapsed', isCollapsed);
      }
      if (toggleIcon) {
        toggleIcon.style.transform = isCollapsed ? 'rotate(-90deg)' : 'rotate(0deg)';
      }
      groupEl.classList.toggle('is-collapsed', isCollapsed);
    });
    
    // Save the updated state
    this._saveCollapsedGroups();
    
    return groupsToExpand;
  },

  /**
   * Scroll the active menu item into view within the sidebar.
   */
  _scrollActiveManagerIntoView() {
    if (!this.elements.sidebarMenu) return;
    
    const activeItem = this.elements.sidebarMenu.querySelector('.menu-item.active');
    if (activeItem) {
      activeItem.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  },
};

// localStorage keys for sidebar state
const SIDEBAR_WIDTH_KEY = 'lithium_sidebar_width';
const SIDEBAR_COLLAPSED_KEY = 'lithium_sidebar_collapsed';
const LAST_MANAGER_KEY = 'lithium_last_manager';
const COLLAPSED_GROUPS_KEY = 'lithium_collapsed_groups';
