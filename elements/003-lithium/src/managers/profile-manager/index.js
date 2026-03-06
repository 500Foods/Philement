/**
 * Profile Manager
 * 
 * User preferences and profile settings management.
 * Handles language, date/time format, number format, default database, and theme preferences.
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims, storeJWT } from '../../core/jwt.js';
import { getConfigValue } from '../../core/config.js';
import { createRequest } from '../../core/json-request.js';

/**
 * Profile Manager Class
 */
export default class ProfileManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.user = null;
    this.preferences = {};
    this.themes = [];
    this.databases = [];
    this.api = null;

    // Profile Manager ID = 2
    this.managerId = 2;

    // Default preferences
    this.defaultPreferences = {
      language: 'en-US',
      dateFormat: 'MM/DD/YYYY',
      timeFormat: '12h',
      numberFormat: 'en-US',
      defaultDatabase: '',
      activeTheme: '',
    };

    // Preview update timer
    this.previewTimer = null;
  }

  /**
   * Initialize the profile manager
   */
  async init() {
    // Initialize API client
    this.api = createRequest();

    // Load CSS
    this.loadStyles();

    // Render template
    await this.render();

    // Load user info from JWT
    this.loadUserInfo();

    // Load themes and databases
    await Promise.all([this.loadThemes(), this.loadDatabases()]);

    // Load saved preferences
    this.loadPreferences();

    // Setup event listeners
    this.setupEventListeners();

    // Initialize preview
    this.updatePreview();

    // Show the page
    this.show();
  }

  /**
   * Load manager-specific CSS
   */
  loadStyles() {
    // Check if styles are already loaded
    if (document.getElementById('profile-manager-styles')) return;

    const link = document.createElement('link');
    link.id = 'profile-manager-styles';
    link.rel = 'stylesheet';
    link.href = '/src/managers/profile-manager/profile-manager.css';
    document.head.appendChild(link);
  }

  /**
   * Render the profile manager template
   */
  async render() {
    try {
      const response = await fetch('/src/managers/profile-manager/profile-manager.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[ProfileManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#profile-manager-page'),
      username: this.container.querySelector('#profile-username'),
      email: this.container.querySelector('#profile-email'),
      roles: this.container.querySelector('#profile-roles'),
      database: this.container.querySelector('#profile-database'),
      preferencesForm: this.container.querySelector('#preferences-form'),
      language: this.container.querySelector('#pref-language'),
      dateFormat: this.container.querySelector('#pref-date-format'),
      timeFormat: this.container.querySelector('#pref-time-format'),
      numberFormat: this.container.querySelector('#pref-number-format'),
      defaultDatabase: this.container.querySelector('#pref-database'),
      activeTheme: this.container.querySelector('#pref-theme'),
      btnSave: this.container.querySelector('#btn-save-preferences'),
      btnReset: this.container.querySelector('#btn-reset-preferences'),
      previewDate: this.container.querySelector('#preview-date'),
      previewTime: this.container.querySelector('#preview-time'),
      previewNumber: this.container.querySelector('#preview-number'),
      previewCurrency: this.container.querySelector('#preview-currency'),
      toast: this.container.querySelector('#profile-toast'),
    };
  }

  /**
   * Render fallback if template fails
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="profile-manager-page">
        <p>Profile Manager loading...</p>
      </div>
    `;
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
        email: claims.email || '-',
        roles: claims.roles || [],
        database: claims.database || '-',
      };

      // Update UI
      if (this.elements.username) {
        this.elements.username.textContent = this.user.username;
      }
      if (this.elements.email) {
        this.elements.email.textContent = this.user.email;
      }
      if (this.elements.roles) {
        this.elements.roles.textContent = Array.isArray(this.user.roles)
          ? this.user.roles.join(', ')
          : this.user.roles;
      }
      if (this.elements.database) {
        this.elements.database.textContent = this.user.database;
      }
    }
  }

  /**
   * Load themes from localStorage or mock data
   */
  async loadThemes() {
    // First, try to get from localStorage (set by Style Manager)
    const storedThemes = localStorage.getItem('lithium_themes');
    if (storedThemes) {
      try {
        this.themes = JSON.parse(storedThemes);
      } catch (error) {
        console.warn('[ProfileManager] Failed to parse stored themes:', error);
      }
    }

    // If no themes stored, use mock data
    if (!this.themes || this.themes.length === 0) {
      this.themes = this.getMockThemes();
    }

    // Populate theme select
    this.populateThemeSelect();
  }

  /**
   * Get mock themes
   */
  getMockThemes() {
    return [
      { id: 'default', name: 'Default Dark' },
      { id: 1, name: 'Midnight Indigo' },
      { id: 2, name: 'Ocean Breeze' },
      { id: 3, name: 'Sunset Glow' },
    ];
  }

  /**
   * Populate theme select dropdown
   */
  populateThemeSelect() {
    if (!this.elements.activeTheme) return;

    // Clear existing options except first placeholder
    this.elements.activeTheme.innerHTML = '<option value="">Select a theme...</option>';

    this.themes.forEach((theme) => {
      const option = document.createElement('option');
      option.value = theme.id;
      option.textContent = theme.name;
      this.elements.activeTheme.appendChild(option);
    });
  }

  /**
   * Load databases (mock for now, API integration later)
   */
  async loadDatabases() {
    // TODO: Load from Hydrogen API when available
    // For now, use mock data
    this.databases = [
      { id: 'Demo_PG', name: 'Demo PostgreSQL' },
      { id: 'Demo_MySQL', name: 'Demo MySQL' },
      { id: 'Demo_SQLite', name: 'Demo SQLite' },
    ];

    // Get default from config
    const configDefault = getConfigValue('auth.default_database', '');
    if (configDefault && !this.defaultPreferences.defaultDatabase) {
      this.defaultPreferences.defaultDatabase = configDefault;
    }

    this.populateDatabaseSelect();
  }

  /**
   * Populate database select dropdown
   */
  populateDatabaseSelect() {
    if (!this.elements.defaultDatabase) return;

    // Clear existing options
    this.elements.defaultDatabase.innerHTML = '<option value="">Select a database...</option>';

    this.databases.forEach((db) => {
      const option = document.createElement('option');
      option.value = db.id;
      option.textContent = db.name;
      this.elements.defaultDatabase.appendChild(option);
    });
  }

  /**
   * Load preferences from localStorage
   */
  loadPreferences() {
    const stored = localStorage.getItem('lithium_preferences');
    if (stored) {
      try {
        const parsed = JSON.parse(stored);
        this.preferences = { ...this.defaultPreferences, ...parsed };
      } catch (error) {
        console.warn('[ProfileManager] Failed to parse preferences:', error);
        this.preferences = { ...this.defaultPreferences };
      }
    } else {
      this.preferences = { ...this.defaultPreferences };
    }

    // Apply to form
    this.applyPreferencesToForm();
  }

  /**
   * Apply preferences to form fields
   */
  applyPreferencesToForm() {
    if (this.elements.language) {
      this.elements.language.value = this.preferences.language;
    }
    if (this.elements.dateFormat) {
      this.elements.dateFormat.value = this.preferences.dateFormat;
    }
    if (this.elements.timeFormat) {
      this.elements.timeFormat.value = this.preferences.timeFormat;
    }
    if (this.elements.numberFormat) {
      this.elements.numberFormat.value = this.preferences.numberFormat;
    }
    if (this.elements.defaultDatabase) {
      this.elements.defaultDatabase.value = this.preferences.defaultDatabase;
    }
    if (this.elements.activeTheme) {
      this.elements.activeTheme.value = this.preferences.activeTheme;
    }
  }

  /**
   * Setup event listeners
   */
  setupEventListeners() {
    // Form submission
    this.elements.preferencesForm?.addEventListener('submit', (e) => {
      e.preventDefault();
      this.savePreferences();
    });

    // Reset button
    this.elements.btnReset?.addEventListener('click', () => {
      this.resetPreferences();
    });

    // Real-time preview updates
    const previewFields = [
      this.elements.language,
      this.elements.dateFormat,
      this.elements.timeFormat,
      this.elements.numberFormat,
    ];

    previewFields.forEach((field) => {
      field?.addEventListener('change', () => {
        this.debouncePreviewUpdate();
      });
    });

    // Theme change applies immediately
    this.elements.activeTheme?.addEventListener('change', () => {
      this.applyTheme();
    });
  }

  /**
   * Debounce preview updates
   */
  debouncePreviewUpdate() {
    if (this.previewTimer) {
      clearTimeout(this.previewTimer);
    }
    this.previewTimer = setTimeout(() => {
      this.updatePreview();
    }, 100);
  }

  /**
   * Update preview panel with current format settings
   */
  updatePreview() {
    const language = this.elements.language?.value || 'en-US';
    const dateFormat = this.elements.dateFormat?.value || 'MM/DD/YYYY';
    const timeFormat = this.elements.timeFormat?.value || '12h';
    const numberFormat = this.elements.numberFormat?.value || 'en-US';

    // Sample date/time
    const now = new Date();

    // Format date
    if (this.elements.previewDate) {
      this.elements.previewDate.textContent = this.formatDate(now, dateFormat, language);
    }

    // Format time
    if (this.elements.previewTime) {
      this.elements.previewTime.textContent = this.formatTime(now, timeFormat, language);
    }

    // Format number
    if (this.elements.previewNumber) {
      const sampleNumber = 1234567.89;
      this.elements.previewNumber.textContent = new Intl.NumberFormat(numberFormat).format(sampleNumber);
    }

    // Format currency
    if (this.elements.previewCurrency) {
      const sampleAmount = 1234567.89;
      this.elements.previewCurrency.textContent = new Intl.NumberFormat(numberFormat, {
        style: 'currency',
        currency: 'USD',
      }).format(sampleAmount);
    }
  }

  /**
   * Format date according to preference
   */
  formatDate(date, format, locale) {
    try {
      // Use Intl.DateTimeFormat with locale
      const options = { year: 'numeric', month: 'short', day: 'numeric' };
      return new Intl.DateTimeFormat(locale, options).format(date);
    } catch (error) {
      // Fallback to simple formatting
      return date.toLocaleDateString();
    }
  }

  /**
   * Format time according to preference
   */
  formatTime(date, format, locale) {
    try {
      const options = {
        hour: 'numeric',
        minute: '2-digit',
        hour12: format === '12h',
      };
      return new Intl.DateTimeFormat(locale, options).format(date);
    } catch (error) {
      return date.toLocaleTimeString();
    }
  }

  /**
   * Apply selected theme immediately
   */
  applyTheme() {
    const themeId = this.elements.activeTheme?.value;
    if (!themeId) return;

    // Store in localStorage
    localStorage.setItem('activeThemeId', themeId);

    // Fire theme changed event
    const theme = this.themes.find((t) => String(t.id) === String(themeId));
    eventBus.emit(Events.THEME_CHANGED, {
      themeId: themeId,
      themeName: theme?.name || 'Unknown',
    });

    // Note: Actual theme CSS application is handled by Style Manager or app-level listener
  }

  /**
   * Save preferences
   */
  async savePreferences() {
    const previousLanguage = this.preferences.language;

    // Gather form values
    const newPreferences = {
      language: this.elements.language?.value || 'en-US',
      dateFormat: this.elements.dateFormat?.value || 'MM/DD/YYYY',
      timeFormat: this.elements.timeFormat?.value || '12h',
      numberFormat: this.elements.numberFormat?.value || 'en-US',
      defaultDatabase: this.elements.defaultDatabase?.value || '',
      activeTheme: this.elements.activeTheme?.value || '',
    };

    // Update local state
    this.preferences = newPreferences;

    // Save to localStorage
    localStorage.setItem('lithium_preferences', JSON.stringify(this.preferences));

    // Save to API
    try {
      await this.savePreferencesToApi(newPreferences);
    } catch (error) {
      console.warn('[ProfileManager] API save failed, preferences stored locally:', error);
    }

    // Check if language changed
    const languageChanged = previousLanguage !== newPreferences.language;

    // Fire appropriate events
    if (languageChanged) {
      eventBus.emit(Events.LOCALE_CHANGED, {
        lang: newPreferences.language,
        formats: {
          date: newPreferences.dateFormat,
          time: newPreferences.timeFormat,
          number: newPreferences.numberFormat,
        },
      });
    }

    // Show success notification
    this.showToast('Preferences saved successfully');

    // Update preview
    this.updatePreview();
  }

  /**
   * Save preferences to API
   */
  async savePreferencesToApi(preferences) {
    const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
    const apiPrefix = getConfigValue('server.api_prefix', '/api');

    const response = await fetch(`${serverUrl}${apiPrefix}/user/preferences`, {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(preferences),
    });

    if (!response.ok) {
      throw new Error(`API error: ${response.status}`);
    }

    // Check for new JWT in response (if language changed)
    const data = await response.json();
    if (data.token) {
      storeJWT(data.token);
      eventBus.emit(Events.AUTH_RENEWED, { expiresAt: data.expires_at });
    }

    return data;
  }

  /**
   * Reset preferences to defaults
   */
  resetPreferences() {
    if (confirm('Reset all preferences to default values?')) {
      this.preferences = { ...this.defaultPreferences };
      this.applyPreferencesToForm();
      this.updatePreview();
      this.showToast('Preferences reset to defaults');
    }
  }

  /**
   * Show toast notification
   */
  showToast(message, type = 'success') {
    if (!this.elements.toast) return;

    const toastMessage = this.elements.toast.querySelector('.toast-message');
    if (toastMessage) {
      toastMessage.textContent = message;
    }

    // Update icon based on type
    const icon = this.elements.toast.querySelector('i');
    if (icon) {
      icon.className = type === 'error' ? 'fas fa-exclamation-circle' : 'fas fa-check-circle';
    }

    // Update border color
    this.elements.toast.classList.toggle('error', type === 'error');

    // Show
    this.elements.toast.classList.add('visible');

    // Hide after delay
    setTimeout(() => {
      this.elements.toast?.classList.remove('visible');
    }, 3000);
  }

  /**
   * Show the profile manager
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  /**
   * Teardown the profile manager
   */
  teardown() {
    // Clear preview timer
    if (this.previewTimer) {
      clearTimeout(this.previewTimer);
      this.previewTimer = null;
    }

    // Clear elements
    this.elements = {};
  }
}
