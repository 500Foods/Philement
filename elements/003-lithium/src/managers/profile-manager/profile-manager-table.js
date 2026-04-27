/**
 * Profile Manager - Table Management Module
 *
 * Handles all table-related functionality for the Profile Manager.
 */

import { LithiumTable } from '../../tables/lithium-table-main.js';
import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { normalizeIconHtml } from '../../core/icons.js';
import { closeAllPopups } from '../../core/manager-ui.js';

export class ProfileManagerTable {
  constructor(profileManager) {
    this.pm = profileManager; // Reference to parent ProfileManager
  }

  /**
   * Initialize the options table
   */
  async initOptionsTable() {
    if (!this.pm.elements.tableContainer || !this.pm.elements.navigatorContainer) return;

    this.pm.optionsTable = new LithiumTable({
      container: this.pm.elements.tableContainer,
      navigatorContainer: this.pm.elements.navigatorContainer,
      tablePath: 'profile-manager/user-options',
      lookupKeyIdx: 4,
      cssPrefix: 'profile-options',
      storageKey: 'profile_options_table',
      app: this.pm.app,
      readonly: true,
      panel: this.pm.elements.leftPanel,
      panelStateManager: this.pm.leftPanelState,
      localSearch: true,
      localSearchFields: ['section', 'label', 'search'],
      onRowSelected: (rowData) => this.handleOptionSelected(rowData),
      onRowDeselected: () => this.handleOptionDeselected(),
      onRefresh: () => this.loadUserOptions(),
    });

    this.pm.editHelper.registerTable(this.pm.optionsTable);
    await this.pm.optionsTable.init();
    await this.loadUserOptions();
  }

  /**
   * Load user options from Lookup 60 and merge with Main Menu data
   */
  async loadUserOptions() {
    if (!this.pm.app?.api || !this.pm.optionsTable?.table) return;

    try {
      this.pm.optionsTable.showLoading();

      const rows = await authQuery(this.pm.app.api, 26, { INTEGER: { LOOKUPID: 60 } });
      const profileSectionsRow = rows?.find(row => row.key_idx === 0);

      if (profileSectionsRow && profileSectionsRow.collection) {
        let collection = profileSectionsRow.collection;
        if (typeof collection === 'string') {
          try {
            collection = JSON.parse(collection);
          } catch (_e) {
            collection = {};
          }
        }

        const locale = navigator.language || 'en-US';
        const sections = collection[locale] || collection.default || [];

        // Transform sections, merging with menu data for manager entries
        this.pm.userOptions = this.transformSections(sections);
      } else {
        this.pm.userOptions = this.getDefaultUserOptions();
      }

      // Don't auto-select - we'll handle selection manually in _restoreSelectedSection
      this.pm.optionsTable.loadStaticData(this.pm.userOptions, { autoSelect: false });
      log(Subsystems.TABLE, Status.INFO,
        `[ProfileManager] Loaded ${this.pm.userOptions.length} user options`);

      // Process icons after the table has rendered HTML formatter cells.
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          processIcons(this.pm.elements.tableContainer);
        });
      });

      // Restore previously selected section after data is loaded
      this._restoreSelectedSection();
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR,
        `[ProfileManager] Failed to load user options: ${error.message}`);
      this.pm.userOptions = this.getDefaultUserOptions();
      // Don't auto-select on error either
      this.pm.optionsTable.loadStaticData(this.pm.userOptions, { autoSelect: false });

      // Process icons after error recovery once the table DOM is present.
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          processIcons(this.pm.elements.tableContainer);
        });
      });

      // Restore previously selected section even on error (using default options)
      this._restoreSelectedSection();
    } finally {
      this.pm.optionsTable.hideLoading();
    }
  }

  /**
   * Restore the previously selected section from localStorage
   * If no saved state (e.g., after localStorage purge), select the first record (Account)
   * @private
   */
  _restoreSelectedSection() {
    try {
      const savedIndex = localStorage.getItem(this.pm._selectedSectionStorageKey);
      if (savedIndex === null) {
        log(Subsystems.MANAGER, Status.INFO,
          '[ProfileManager] No saved section, selecting first (Account)');
        this._selectSectionByIndex(-1); // Default to Account page
        return;
      }

      const index = parseInt(savedIndex, 10);
      if (isNaN(index)) {
        log(Subsystems.MANAGER, Status.WARN,
          `[ProfileManager] Invalid saved index "${savedIndex}", selecting first`);
        this._selectSectionByIndex(-1);
        return;
      }

      const table = this.pm.optionsTable?.table;
      if (!table) return;

      // Find row with matching index
      const rows = table.getRows();
      const tabulatorRow = rows.find(row => {
        const data = row.getData();
        return data && data.index === index;
      });

      if (tabulatorRow) {
        table.selectRow(tabulatorRow);
        log(Subsystems.MANAGER, Status.INFO,
          `[ProfileManager] Restored section: index ${index}`);
      } else {
        log(Subsystems.MANAGER, Status.WARN,
          `[ProfileManager] Saved section index ${index} not found, selecting first`);
        this._selectSectionByIndex(-1);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR,
        `[ProfileManager] Failed to restore selected section: ${error.message}`);
      this._selectSectionByIndex(-1);
    }
  }

  /**
   * Select a section by its index value
   * @private
   */
  _selectSectionByIndex(index) {
    const table = this.pm.optionsTable?.table;
    if (!table) return;

    const rows = table.getRows();
    const tabulatorRow = rows.find(row => {
      const data = row.getData();
      return data && data.index === index;
    });

    if (tabulatorRow) {
      table.selectRow(tabulatorRow);
      log(Subsystems.MANAGER, Status.INFO,
        `[ProfileManager] Selected section by index: ${index}`);
    } else {
      log(Subsystems.MANAGER, Status.WARN,
        `[ProfileManager] Section with index ${index} not found`);
    }
  }

  /**
   * Transform sections from Lookup 60, merging with Main Menu data for manager entries
   */
  transformSections(sections) {
    return sections.map((section, position) => {
      const index = section[3] ?? (position + 1);
      const isInternal = index < 0;
      const sectionIcon = normalizeIconHtml(section[1], '<fa fa-cube></fa>');
      const searchString = section[4] || '';

      // For manager entries (index >= 0), merge with menu data
      if (!isInternal && this.pm._managerIcons[index]) {
        const menuInfo = this.pm._managerIcons[index];
        const groupName = this.pm._getManagerGroup(index);
        return {
          key_idx: position + 1,
          section: groupName,
          icon: normalizeIconHtml(menuInfo.iconHtml, sectionIcon || '<fa fa-cube></fa>') || sectionIcon || '<fa fa-cube></fa>',
          label: menuInfo.name || section[2] || 'Unknown',
          index: index,
          search: searchString,
          status_a1: 1,
        };
      }

      // Internal sections or managers not in menu
      return {
        key_idx: position + 1,
        section: section[0] || '',
        icon: sectionIcon || '',
        label: section[2] || '',
        index: index,
        search: searchString,
        status_a1: 1,
      };
    });
  }

  /**
   * Get default user options when lookup data is not available
   */
  getDefaultUserOptions() {
    const baseOptions = [
      { key_idx: 1, section: 'General', icon: '<fa fa-id-card></fa>', label: 'Account', index: -1, search: '', status_a1: 1 },
      { key_idx: 2, section: 'General', icon: '<fa fa-id-badge></fa>', label: 'Names', index: -2, search: '', status_a1: 1 },
      { key_idx: 3, section: 'General', icon: '<fa fa-address-book></fa>', label: 'Addresses', index: -3, search: '', status_a1: 1 },
      { key_idx: 4, section: 'General', icon: '<fa fa-at></fa>', label: 'E-Mail', index: -4, search: '', status_a1: 1 },
      { key_idx: 5, section: 'General', icon: '<fa fa-phone></fa>', label: 'Phone', index: -5, search: '', status_a1: 1 },
      { key_idx: 6, section: 'Security', icon: '<fa fa-key></fa>', label: 'Authentication', index: -6, search: '', status_a1: 1 },
      { key_idx: 7, section: 'Security', icon: '<fa fa-user-key></fa>', label: 'Tokens', index: -7, search: '', status_a1: 1 },
      { key_idx: 8, section: 'Formatting', icon: '<fa fa-globe></fa>', label: 'Language', index: -8, search: '', status_a1: 1 },
      { key_idx: 9, section: 'Formatting', icon: '<fa fa-calendar></fa>', label: 'Date Formats', index: -9, search: '', status_a1: 1 },
      { key_idx: 10, section: 'Formatting', icon: '<fa fa-00></fa>', label: 'Number Formats', index: -10, search: '', status_a1: 1 },
      { key_idx: 11, section: 'Application', icon: '<fa fa-atom-simple></fa>', label: 'Startup', index: -11, search: '', status_a1: 1 },
      { key_idx: 12, section: 'Application', icon: '<fa fa-bell></fa>', label: 'Notifications', index: -12, search: '', status_a1: 1 },
      { key_idx: 13, section: 'Application', icon: '<fa fa-bell-concierge></fa>', label: 'Concierge', index: -13, search: '', status_a1: 1 },
      { key_idx: 14, section: 'Application', icon: '<fa fa-note-sticky></fa>', label: 'Annotations', index: -14, search: '', status_a1: 1 },
      { key_idx: 15, section: 'Application', icon: '<fa fa-signs-post></fa>', label: 'Tours', index: -15, search: '', status_a1: 1 },
      { key_idx: 16, section: 'Application', icon: '<fa fa-graduation-cap></fa>', label: 'Training', index: -16, search: '', status_a1: 1 },
      { key_idx: 17, section: 'Security', icon: '<fa fa-receipt></fa>', label: 'Login History', index: -17, search: '', status_a1: 1 },
    ];

    // Add manager entries from menu data, using group names from Main Menu
    const managerOptions = Object.entries(this.pm._managerIcons).map(([id, info], position) => {
      const managerId = parseInt(id, 10);
      const groupName = this.pm._getManagerGroup(managerId);
      return {
        key_idx: 100 + position,
        section: groupName,
        icon: info.iconHtml || '<fa fa-cube></fa>',
        label: info.name || `Manager ${id}`,
        index: managerId,
        search: '',
        status_a1: 1,
      };
    });

    return [...baseOptions, ...managerOptions];
  }

  /**
   * Handle when an option is selected from the table
   */
  async handleOptionSelected(rowData) {
    if (!rowData) return;

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] Option selected: ${rowData.label}, index: ${rowData.index}`);

    // Close any open popups (timezone picker, datepicker, etc.)
    closeAllPopups();

    // Save selected section index to localStorage
    if (rowData.index !== undefined) {
      localStorage.setItem(this.pm._selectedSectionStorageKey, String(rowData.index));
    }

    // Update section label button using the data directly
    if (this.pm.elements.sectionLabelBtn && rowData.label) {
      this.pm.elements.sectionLabelBtn.classList.add('active');
      this.pm.elements.sectionLabelBtn.title = rowData.label;
      const iconHtml = rowData.icon || '<fa fa-cube></fa>';
      this.pm.elements.sectionLabelBtn.innerHTML = `${iconHtml} <span>${rowData.label}</span>`;
      processIcons(this.pm.elements.sectionLabelBtn);
    }

    // Switch to settings tab if not already there
    if (this.pm.currentTab !== 'settings') {
      this.pm.switchTab('settings');
    }

    // Show the settings page for this section
    await this.pm.settingsHandler.showPage(rowData.index, rowData);
  }

  /**
   * Handle when an option is deselected from the table
   */
  handleOptionDeselected() {
    log(Subsystems.MANAGER, Status.DEBUG, '[ProfileManager] Option deselected');

    // Clear section label when nothing is selected
    if (this.pm.elements.sectionLabelBtn) {
      this.pm.elements.sectionLabelBtn.classList.remove('active');
      this.pm.elements.sectionLabelBtn.title = 'Select a Section';
      this.pm.elements.sectionLabelBtn.innerHTML = '<fa fa-cog></fa> <span>Select a Section</span>';
    }
  }
}