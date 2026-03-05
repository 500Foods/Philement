/**
 * Style Manager
 * 
 * Theme management: create, edit, apply, and share user themes.
 * Implements List View with Tabulator and CSS View with CodeMirror.
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims } from '../../core/jwt.js';
import { getConfig, getConfigValue } from '../../core/config.js';
import { hasFeature, getFeaturesForManager } from '../../core/permissions.js';

// Dynamic imports for third-party libraries
let Tabulator;
let DOMPurify;

/**
 * Style Manager Class
 */
export default class StyleManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.themes = [];
    this.currentTheme = null;
    this.table = null;
    this.editor = null;
    
    // Style Manager ID = 1
    this.managerId = 1;
  }

  /**
   * Initialize the style manager
   */
  async init() {
    // Load third-party libraries
    await this.loadDependencies();
    
    // Load CSS
    this.loadStyles();
    
    // Render template
    await this.render();
    
    // Setup event listeners
    this.setupEventListeners();
    
    // Apply feature-gated UI
    this.applyPermissions();
    
    // Load themes from API (or mock data for now)
    await this.loadThemes();
    
    // Show the page
    this.show();
  }

  /**
   * Load dependencies (Tabulator, DOMPurify)
   */
  async loadDependencies() {
    try {
      // Dynamic import Tabulator
      const tabulatorModule = await import('tabulator-tables');
      Tabulator = tabulatorModule.default;
      
      // Dynamic import DOMPurify
      const dompurifyModule = await import('dompurify');
      DOMPurify = dompurifyModule.default;
    } catch (error) {
      console.error('[StyleManager] Failed to load dependencies:', error);
    }
  }

  /**
   * Load manager-specific CSS
   */
  loadStyles() {
    // Check if styles are already loaded
    if (document.getElementById('style-manager-styles')) return;
    
    const link = document.createElement('link');
    link.id = 'style-manager-styles';
    link.rel = 'stylesheet';
    link.href = '/src/managers/style-manager/style-manager.css';
    document.head.appendChild(link);
  }

  /**
   * Render the style manager template
   */
  async render() {
    try {
      const response = await fetch('/src/managers/style-manager/style-manager.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[StyleManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#style-manager-page'),
      tabButtons: this.container.querySelectorAll('.tab-btn'),
      listView: this.container.querySelector('#list-view'),
      cssView: this.container.querySelector('#css-view'),
      themeTable: this.container.querySelector('#theme-table'),
      cssEditor: this.container.querySelector('#css-editor'),
      themeSelect: this.container.querySelector('#theme-select'),
      btnNewTheme: this.container.querySelector('#btn-new-theme'),
      btnApplyCss: this.container.querySelector('#btn-apply-css'),
      btnSaveCss: this.container.querySelector('#btn-save-css'),
      themeModal: this.container.querySelector('#theme-modal'),
      deleteModal: this.container.querySelector('#delete-modal'),
      modalTitle: this.container.querySelector('#modal-title'),
      modalClose: this.container.querySelector('#modal-close'),
      themeForm: this.container.querySelector('#theme-form'),
      themeId: this.container.querySelector('#theme-id'),
      themeName: this.container.querySelector('#theme-name'),
      themeVariables: this.container.querySelector('#theme-variables'),
      themeRawCss: this.container.querySelector('#theme-raw-css'),
      btnSaveTheme: this.container.querySelector('#btn-save-theme'),
      btnCancelTheme: this.container.querySelector('#btn-cancel-theme'),
      deleteThemeName: this.container.querySelector('#delete-theme-name'),
      deleteModalClose: this.container.querySelector('#delete-modal-close'),
      btnCancelDelete: this.container.querySelector('#btn-cancel-delete'),
      btnConfirmDelete: this.container.querySelector('#btn-confirm-delete'),
    };
  }

  /**
   * Render fallback if template fails
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="style-manager-page">
        <p>Style Manager loading...</p>
      </div>
    `;
  }

  /**
   * Apply feature-gated UI based on permissions
   */
  applyPermissions() {
    const claims = getClaims();
    const features = getFeaturesForManager(this.managerId, claims?.punchcard);
    
    // Feature 1: View all styles (read-only) - always show
    // Feature 2: Edit all styles - controls New/Edit buttons
    const canEdit = hasFeature(this.managerId, 2, claims?.punchcard);
    // Feature 4: Use CSS code editor - controls CSS View tab
    const canUseCssEditor = hasFeature(this.managerId, 4, claims?.punchcard);
    // Feature 5: Delete themes - controls Delete button
    const canDelete = hasFeature(this.managerId, 5, claims?.punchcard);
    
    // Store permissions for use in actions
    this.permissions = { canEdit, canUseCssEditor, canDelete };
    
    // Show/hide elements based on permissions
    if (!canEdit) {
      this.elements.btnNewTheme?.classList.add('hidden');
    }
    
    if (!canUseCssEditor) {
      const cssTab = this.container.querySelector('[data-view="css"]');
      cssTab?.classList.add('hidden');
    }
    
    // Store canDelete for table row actions
    this.canDelete = canDelete;
  }

  /**
   * Set up event listeners
   */
  setupEventListeners() {
    // Tab switching
    this.elements.tabButtons?.forEach((btn) => {
      btn.addEventListener('click', () => {
        const view = btn.dataset.view;
        this.switchView(view);
      });
    });

    // New theme button
    this.elements.btnNewTheme?.addEventListener('click', () => {
      this.openThemeModal();
    });

    // Apply CSS button
    this.elements.btnApplyCss?.addEventListener('click', () => {
      this.applyCurrentCss();
    });

    // Save CSS button
    this.elements.btnSaveCss?.addEventListener('click', () => {
      this.saveCss();
    });

    // Theme select change
    this.elements.themeSelect?.addEventListener('change', (e) => {
      this.loadThemeForEditor(e.target.value);
    });

    // Modal: Close buttons
    this.elements.modalClose?.addEventListener('click', () => {
      this.closeThemeModal();
    });

    this.elements.btnCancelTheme?.addEventListener('click', () => {
      this.closeThemeModal();
    });

    this.elements.btnSaveTheme?.addEventListener('click', () => {
      this.saveTheme();
    });

    // Modal overlay click
    this.elements.themeModal?.querySelector('.modal-overlay')?.addEventListener('click', () => {
      this.closeThemeModal();
    });

    // Delete modal
    this.elements.deleteModalClose?.addEventListener('click', () => {
      this.closeDeleteModal();
    });

    this.elements.btnCancelDelete?.addEventListener('click', () => {
      this.closeDeleteModal();
    });

    this.elements.btnConfirmDelete?.addEventListener('click', () => {
      this.confirmDelete();
    });

    this.elements.deleteModal?.querySelector('.modal-overlay')?.addEventListener('click', () => {
      this.closeDeleteModal();
    });
  }

  /**
   * Switch between views
   */
  switchView(view) {
    // Update tab buttons
    this.elements.tabButtons?.forEach((btn) => {
      btn.classList.toggle('active', btn.dataset.view === view);
    });

    // Update view visibility
    this.elements.listView?.classList.toggle('hidden', view !== 'list');
    this.elements.cssView?.classList.toggle('hidden', view !== 'css');

    // Initialize CodeMirror when switching to CSS view
    if (view === 'css' && !this.editor) {
      this.initCodeMirror();
    }
  }

  /**
   * Load themes from API (or use mock data)
   */
  async loadThemes() {
    try {
      const config = getConfig();
      const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
      const apiPrefix = getConfigValue('server.api_prefix', '/api');

      const response = await fetch(`${serverUrl}${apiPrefix}/themes`, {
        headers: {
          'Accept': 'application/json',
        },
      });

      if (response.ok) {
        this.themes = await response.json();
      } else {
        // Use mock data if API fails
        this.themes = this.getMockThemes();
      }
    } catch (error) {
      console.warn('[StyleManager] Failed to fetch themes, using mock data:', error);
      this.themes = this.getMockThemes();
    }

    this.renderThemeTable();
    this.populateThemeSelect();
  }

  /**
   * Get mock themes for development
   */
  getMockThemes() {
    return [
      {
        id: 1,
        name: 'Midnight Indigo',
        owner_id: 1,
        shared_with: [],
        public: false,
        variables: { '--bg-primary': '#0D1117', '--accent-primary': '#58A6FF' },
        raw_css: '',
        created_at: '2025-01-15T10:30:00Z',
        updated_at: '2025-03-01T14:22:00Z',
      },
      {
        id: 2,
        name: 'Ocean Breeze',
        owner_id: 1,
        shared_with: [3, 12],
        public: false,
        variables: { '--bg-primary': '#0a192f', '--accent-primary': '#64ffda' },
        raw_css: '',
        created_at: '2025-02-10T09:15:00Z',
        updated_at: '2025-02-28T16:45:00Z',
      },
      {
        id: 3,
        name: 'Sunset Glow',
        owner_id: 2,
        shared_with: [],
        public: true,
        variables: { '--bg-primary': '#1a1a2e', '--accent-primary': '#f39c12' },
        raw_css: '/* Custom sunset effects */\n.btn { border-radius: 20px; }',
        created_at: '2025-01-20T11:00:00Z',
        updated_at: '2025-02-15T08:30:00Z',
      },
    ];
  }

  /**
   * Render the theme table with Tabulator
   */
  renderThemeTable() {
    if (!this.elements.themeTable || !Tabulator) return;

    // Clear existing table
    this.elements.themeTable.innerHTML = '';

    const tableData = this.themes.map((theme) => ({
      id: theme.id,
      name: theme.name,
      created: new Date(theme.created_at).toLocaleDateString(),
      updated: new Date(theme.updated_at).toLocaleDateString(),
      owner: `User ${theme.owner_id}`,
      isPublic: theme.public ? 'Yes' : 'No',
    }));

    this.table = new Tabulator(this.elements.themeTable, {
      data: tableData,
      layout: 'fitColumns',
      responsiveLayout: 'hide',
      height: '100%',
      columns: [
        { title: 'Name', field: 'name', minWidth: 150 },
        { title: 'Created', field: 'created', width: 120 },
        { title: 'Updated', field: 'updated', width: 120 },
        { title: 'Owner', field: 'owner', width: 100 },
        { title: 'Public', field: 'isPublic', width: 80 },
        {
          title: 'Actions',
          field: 'actions',
          width: 150,
          formatter: (cell, formatterParams, onRendered) => {
            const themeId = cell.getData().id;
            const canEdit = this.permissions?.canEdit;
            const canDelete = this.canDelete;
            
            let html = '<div class="theme-actions">';
            
            // Apply button
            html += `<button class="theme-action-btn btn-apply" data-action="apply" data-id="${themeId}" title="Apply theme">
              <i class="fas fa-check"></i>
            </button>`;
            
            // Edit button (if canEdit)
            if (canEdit) {
              html += `<button class="theme-action-btn btn-edit" data-action="edit" data-id="${themeId}" title="Edit theme">
                <i class="fas fa-edit"></i>
              </button>`;
            }
            
            // Delete button (if canDelete)
            if (canDelete) {
              html += `<button class="theme-action-btn btn-delete" data-action="delete" data-id="${themeId}" title="Delete theme">
                <i class="fas fa-trash"></i>
              </button>`;
            }
            
            html += '</div>';
            return html;
          },
        },
      ],
    });

    // Add action event listeners
    this.elements.themeTable.addEventListener('click', (e) => {
      const btn = e.target.closest('.theme-action-btn');
      if (!btn) return;

      const action = btn.dataset.action;
      const themeId = parseInt(btn.dataset.id, 10);

      switch (action) {
        case 'apply':
          this.applyTheme(themeId);
          break;
        case 'edit':
          this.editTheme(themeId);
          break;
        case 'delete':
          this.deleteTheme(themeId);
          break;
      }
    });
  }

  /**
   * Populate theme select dropdown
   */
  populateThemeSelect() {
    if (!this.elements.themeSelect) return;

    // Clear existing options (keep first)
    this.elements.themeSelect.innerHTML = '<option value="">Select a theme...</option>';

    this.themes.forEach((theme) => {
      const option = document.createElement('option');
      option.value = theme.id;
      option.textContent = theme.name;
      this.elements.themeSelect.appendChild(option);
    });
  }

  /**
   * Initialize CodeMirror editor
   */
  async initCodeMirror() {
    if (!this.elements.cssEditor) return;

    try {
      // Dynamic import CodeMirror
      const { EditorState } = await import('@codemirror/state');
      const { EditorView, keymap, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine } = await import('@codemirror/view');
      const { defaultKeymap, history, historyKeymap } = await import('@codemirror/commands');
      const { css } = await import('@codemirror/lang-css');
      const { oneDark } = await import('@codemirror/theme-one-dark');

      const startState = EditorState.create({
        doc: '/* Select a theme to edit its CSS */\n',
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          css(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%' },
            '.cm-scroller': { overflow: 'auto' },
          }),
        ],
      });

      this.editor = new EditorView({
        state: startState,
        parent: this.elements.cssEditor,
      });
    } catch (error) {
      console.error('[StyleManager] Failed to initialize CodeMirror:', error);
      // Fallback to textarea
      this.elements.cssEditor.innerHTML = '<textarea id="css-editor-fallback" style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);"></textarea>';
    }
  }

  /**
   * Load theme for CSS editor
   */
  loadThemeForEditor(themeId) {
    if (!themeId || !this.editor) return;

    const theme = this.themes.find((t) => t.id === parseInt(themeId, 10));
    if (!theme) return;

    const css = this.buildCssFromTheme(theme);
    
    // Update editor content
    this.editor.dispatch({
      changes: { from: 0, to: this.editor.state.doc.length, insert: css },
    });
  }

  /**
   * Build CSS string from theme variables
   */
  buildCssFromTheme(theme) {
    let css = '/* Theme CSS Variables */\n:root {\n';
    
    if (theme.variables) {
      Object.entries(theme.variables).forEach(([key, value]) => {
        css += `  ${key}: ${value};\n`;
      });
    }
    
    css += '}\n\n';
    
    if (theme.raw_css) {
      css += '/* Raw CSS */\n' + theme.raw_css;
    }
    
    return css;
  }

  /**
   * Apply theme to the application
   */
  applyTheme(themeId) {
    const theme = this.themes.find((t) => t.id === themeId);
    if (!theme) return;

    this.applyThemeToDom(theme);
    
    // Persist active theme ID in user preferences
    localStorage.setItem('activeThemeId', themeId);
    
    // Fire theme:changed event
    eventBus.emit(Events.THEME_CHANGED, {
      themeId: theme.id,
      themeName: theme.name,
      vars: theme.variables,
    });
  }

  /**
   * Apply theme CSS to DOM
   */
  applyThemeToDom(theme) {
    // Build CSS from variables
    let css = ':root {\n';
    
    if (theme.variables) {
      Object.entries(theme.variables).forEach(([key, value]) => {
        css += `  ${key}: ${value};\n`;
      });
    }
    
    css += '}\n';
    
    // Add raw CSS if present
    if (theme.raw_css) {
      css += '\n' + theme.raw_css;
    }

    // Sanitize with DOMPurify
    const sanitizedCss = DOMPurify.sanitize(css, {
      ALLOWED_TAGS: [],
      ALLOWED_ATTR: [],
    });

    // Find or create dynamic style element
    let styleEl = document.getElementById('dynamic-theme');
    if (!styleEl) {
      styleEl = document.createElement('style');
      styleEl.id = 'dynamic-theme';
      document.head.appendChild(styleEl);
    }

    // Inject sanitized CSS
    styleEl.textContent = sanitizedCss;
  }

  /**
   * Apply current CSS from editor
   */
  applyCurrentCss() {
    if (!this.editor) return;

    const css = this.editor.state.doc.toString();
    
    // Sanitize with DOMPurify
    const sanitizedCss = DOMPurify.sanitize(css, {
      ALLOWED_TAGS: [],
      ALLOWED_ATTR: [],
    });

    // Find or create dynamic style element
    let styleEl = document.getElementById('dynamic-theme');
    if (!styleEl) {
      styleEl = document.createElement('style');
      styleEl.id = 'dynamic-theme';
      document.head.appendChild(styleEl);
    }

    // Inject sanitized CSS
    styleEl.textContent = sanitizedCss;

    // Fire theme:changed event
    eventBus.emit(Events.THEME_CHANGED, {
      themeId: null,
      themeName: 'Custom CSS',
      vars: {},
    });
  }

  /**
   * Save CSS changes to theme
   */
  async saveCss() {
    if (!this.editor || !this.elements.themeSelect?.value) return;

    const themeId = parseInt(this.elements.themeSelect.value, 10);
    const theme = this.themes.find((t) => t.id === themeId);
    if (!theme) return;

    const css = this.editor.state.doc.toString();
    
    // Sanitize before saving
    const sanitizedCss = DOMPurify.sanitize(css, {
      ALLOWED_TAGS: [],
      ALLOWED_ATTR: [],
    });

    // TODO: Save to API
    console.log('[StyleManager] Saving CSS for theme:', themeId, sanitizedCss);
    
    // Update local state
    theme.raw_css = sanitizedCss;
    
    // Show success feedback
    this.showNotification('CSS saved successfully');
  }

  /**
   * Open theme modal for create/edit
   */
  openThemeModal(theme = null) {
    this.currentTheme = theme;
    
    if (theme) {
      // Edit mode
      this.elements.modalTitle.textContent = 'Edit Theme';
      this.elements.themeId.value = theme.id;
      this.elements.themeName.value = theme.name;
      this.elements.themeVariables.value = JSON.stringify(theme.variables || {}, null, 2);
      this.elements.themeRawCss.value = theme.raw_css || '';
    } else {
      // Create mode
      this.elements.modalTitle.textContent = 'New Theme';
      this.elements.themeId.value = '';
      this.elements.themeName.value = '';
      this.elements.themeVariables.value = '{\n  "--bg-primary": "#0D1117",\n  "--accent-primary": "#58A6FF"\n}';
      this.elements.themeRawCss.value = '';
    }

    this.elements.themeModal?.classList.add('visible');
  }

  /**
   * Close theme modal
   */
  closeThemeModal() {
    this.elements.themeModal?.classList.remove('visible');
    this.currentTheme = null;
  }

  /**
   * Save theme from modal form
   */
  async saveTheme() {
    const name = this.elements.themeName?.value?.trim();
    const variablesStr = this.elements.themeVariables?.value?.trim();
    const rawCss = this.elements.themeRawCss?.value?.trim();

    if (!name) {
      this.showError('Theme name is required');
      return;
    }

    // Parse variables JSON
    let variables = {};
    try {
      if (variablesStr) {
        variables = JSON.parse(variablesStr);
      }
    } catch (error) {
      this.showError('Invalid JSON in CSS variables');
      return;
    }

    // Sanitize raw CSS
    const sanitizedRawCss = DOMPurify.sanitize(rawCss || '', {
      ALLOWED_TAGS: [],
      ALLOWED_ATTR: [],
    });

    const themeId = this.elements.themeId?.value;
    
    if (themeId) {
      // Update existing theme
      const theme = this.themes.find((t) => t.id === parseInt(themeId, 10));
      if (theme) {
        theme.name = name;
        theme.variables = variables;
        theme.raw_css = sanitizedRawCss;
        theme.updated_at = new Date().toISOString();
      }
    } else {
      // Create new theme
      const newTheme = {
        id: Date.now(),
        name,
        owner_id: 1,
        shared_with: [],
        public: false,
        variables,
        raw_css: sanitizedRawCss,
        created_at: new Date().toISOString(),
        updated_at: new Date().toISOString(),
      };
      this.themes.push(newTheme);
    }

    // Re-render table and select
    this.renderThemeTable();
    this.populateThemeSelect();
    
    // Close modal
    this.closeThemeModal();
    
    // Show success
    this.showNotification(themeId ? 'Theme updated' : 'Theme created');
  }

  /**
   * Edit theme
   */
  editTheme(themeId) {
    const theme = this.themes.find((t) => t.id === themeId);
    if (theme) {
      this.openThemeModal(theme);
    }
  }

  /**
   * Open delete confirmation modal
   */
  deleteTheme(themeId) {
    const theme = this.themes.find((t) => t.id === themeId);
    if (!theme) return;

    this.themeToDelete = themeId;
    this.elements.deleteThemeName.textContent = theme.name;
    this.elements.deleteModal?.classList.add('visible');
  }

  /**
   * Close delete modal
   */
  closeDeleteModal() {
    this.elements.deleteModal?.classList.remove('visible');
    this.themeToDelete = null;
  }

  /**
   * Confirm delete
   */
  async confirmDelete() {
    if (!this.themeToDelete) return;

    // Remove from array
    this.themes = this.themes.filter((t) => t.id !== this.themeToDelete);
    
    // Re-render
    this.renderThemeTable();
    this.populateThemeSelect();
    
    // Close modal
    this.closeDeleteModal();
    
    // Show success
    this.showNotification('Theme deleted');
  }

  /**
   * Show notification
   */
  showNotification(message) {
    // Simple notification - could be enhanced with toast UI
    console.log('[StyleManager]', message);
    
    // Dispatch custom event for app-level notification
    eventBus.emit('notification', { type: 'success', message });
  }

  /**
   * Show error
   */
  showError(message) {
    console.error('[StyleManager]', message);
    eventBus.emit('notification', { type: 'error', message });
  }

  /**
   * Show the style manager
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  /**
   * Teardown the style manager
   */
  teardown() {
    // Destroy Tabulator table
    if (this.table) {
      this.table.destroy();
      this.table = null;
    }

    // Destroy CodeMirror editor
    if (this.editor) {
      this.editor.destroy();
      this.editor = null;
    }

    // Clear elements
    this.elements = {};
  }
}
