/**
 * Crimson Citations Module
 *
 * Handles citation parsing, display, and interaction:
 * - Citation parsing and deduplication
 * - Citation popup with LithiumTable
 * - Canvas URL building
 * - Citation marker conversion
 * - Citation link handling
 *
 * @module managers/crimson/crimson-citations
 */

import { processIcons } from '../../core/icons.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { LithiumTable } from '../../tables/lithium-table-main.js';
import { CrimsonManager } from './crimson-core.js';

// ==================== CITATION PARSING ====================

/**
 * No deduplication - citations are added as they come.
 * @param {Object} _citation - Raw or normalized citation (unused)
 * @param {number} index - Citation index for unique key
 * @returns {string} Unique key
 */
CrimsonManager.prototype._citationDedup = function(_citation, index) {
  return `citation_${index}_${Date.now()}`;
};

/**
 * Parse citations from metadata, deduplicate, and collect into allCitations.
 * Returns a mapping of original per-turn indices to global citation numbers.
 * @param {string} messageId - The message element ID
 * @param {Array} citations - Array of citation objects from metadata
 * @returns {Map<number,number>} turnIndex (1-based) to global citation number
 */
CrimsonManager.prototype.parseCitations = function(messageId, citations) {
  log(Subsystems.CRIMSON, Status.DEBUG, `parseCitations called for ${messageId}: ${Array.isArray(citations) ? citations.length : 'invalid'} items`);
  if (!citations || !Array.isArray(citations) || citations.length === 0) {
    log(Subsystems.CRIMSON, Status.DEBUG, 'parseCitations: no citations to process');
    return new Map();
  }

  const turnMapping = new Map();
  const startIndex = this.allCitations.length;

  citations.forEach((citation, index) => {
    log(Subsystems.CRIMSON, Status.DEBUG, `Raw citation[${index}]: ${JSON.stringify(citation)}`);

    let filename = citation.filename || citation.name || citation.title || '';
    let url = citation.url || citation.uri || citation.source || citation.filename || '';
    const isCanvas = url.startsWith('canvas') || filename.startsWith('canvas');
    const turnIndex = index + 1;
    const globalNumber = startIndex + turnIndex;

    // Handle canvas- prefixed URLs - convert to actual course page URL
    if (url.startsWith('canvas-')) {
      const canvasUrl = this.buildCanvasUrl(url);
      if (canvasUrl) {
        url = canvasUrl;
      }
    }

    // Clean up filename: if it starts with canvas-PH-*, remove prefix and clean up
    if (filename.startsWith('canvas-PH-')) {
      // Remove the prefix up to the module number (e.g., "canvas-PH-003-I2L-mod-2-")
      const match = filename.match(/^canvas-PH-\d+-I2L-mod-\d+-(.+)$/);
      if (match) {
        let cleaned = match[1];
        // Remove .md extension
        if (cleaned.endsWith('.md')) {
          cleaned = cleaned.slice(0, -3);
        }
        // Replace dashes with spaces and title case
        filename = cleaned.replace(/-/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
      }
    }

    // Generate link icon based on processed URL
    const link = this.generateLinkIcon(url);

    const normalized = {
      number: globalNumber,
      name: filename || 'Untitled',
      url: url,
      link: link,
      type: isCanvas ? 'canvas' : (citation.type || 'web'),
      source: citation.source || citation.filename || '',
      score: citation.score != null 
        ? Math.max(-1000, Math.min(1000, citation.score)) 
        : null,
      pageContent: citation.page_content || null,
      dataSourceId: citation.data_source_id || null,
    };

    this.allCitations.push(normalized);
    turnMapping.set(turnIndex, globalNumber);
  });

  this.updateCitationHeaderButton();

  log(Subsystems.CRIMSON, Status.DEBUG, `Citations collected: ${this.allCitations.length} total, ${citations.length} from this turn`);
  return turnMapping;
};

/**
 * Build a canvas URL from a citation filename.
 * @param {string} filename - The citation filename
 * @returns {string|null} The constructed URL or null
 */
CrimsonManager.prototype.buildCanvasUrl = function(filename) {
  if (!filename) return null;

  if (filename.includes('/')) {
    const parts = filename.split('/');
    const courseCodeIndex = parts.indexOf('courses') + 1;
    if (courseCodeIndex > 0 && courseCodeIndex < parts.length) {
      const pageName = parts[parts.length - 1];
      const courseId = '801';
      return `https://canvas.500courses.com/courses/${courseId}/pages/${pageName}`;
    }
  }

  if (filename.startsWith('canvas')) {
    const cleanFilename = filename.endsWith('.md') ? filename.slice(0, -3) : filename;

    const modMatch = cleanFilename.match(/^canvas-[^-]+-[^-]+-[^-]+-mod-(\d+)-(.+)$/);
    if (modMatch) {
      const pageName = modMatch[2];
      const courseId = '801';
      return `https://canvas.500courses.com/courses/${courseId}/pages/${pageName}`;
    }

    const parts = cleanFilename.split('-');
    if (parts.length >= 5) {
      const modIndex = parts.indexOf('mod');
      if (modIndex > 0 && modIndex < parts.length - 2) {
        const pageName = parts.slice(modIndex + 2).join('-');
        const courseId = '801';
        return `https://canvas.500courses.com/courses/${courseId}/pages/${pageName}`;
      }
    }
  }

  return null;
};

/**
 * Generate link icon based on URL.
 * @param {string} url - The URL to analyze
 * @returns {string} The FA icon HTML
 */
CrimsonManager.prototype.generateLinkIcon = function(url) {
  if (!url) return '<fa fa-link></fa>';
  
  // GitHub
  if (url.includes('github.com')) {
    return '<i class="fa-brands fa-github"></i>';
  }
  
  // Markdown (.md file)
  if (!url.startsWith('http') && url.endsWith('.md')) {
    return '<i class="fa-brands fa-markdown"></i>';
  }
  
  // JSON (.json file)
  if (url.endsWith('.json')) {
    return '<fa fa-brackets-curly></fa>';
  }
  
  // Canvas (check both canvas- prefix and canvas.500courses.com domain)
  if (url.startsWith('canvas') || url.includes('canvas/') || url.includes('canvas.500courses.com')) {
    return '<fa fa-graduation-cap></fa>';
  }
  
  // Generic web
  return '<fa fa-arrow-up-right-from-square></fa>';
};

/**
 * Open a citation URL in a new tab, translating canvas URLs if needed
 * @param {string} url - The URL to open
 */
CrimsonManager.prototype.openCitationUrl = function(url) {
  if (!url) return;
  if (url.startsWith('canvas') || url.includes('canvas/')) {
    const canvasUrl = this.buildCanvasUrl(url);
    if (canvasUrl) {
      window.open(canvasUrl, '_blank', 'noopener,noreferrer');
    } else {
      log(Subsystems.CRIMSON, Status.WARN, `Failed to build canvas URL for: ${url}`);
    }
  } else {
    window.open(url, '_blank', 'noopener,noreferrer');
  }
};

/**
 * Format a URL for display as a Reference.
 * @param {string} url - The URL to format
 * @returns {string} Formatted reference string
 */
CrimsonManager.prototype.formatReference = function(url) {
  if (!url) return '';
  return url;
};

// ==================== CITATION HEADER BUTTON ====================

/**
 * Update the citation count in the header button
 */
CrimsonManager.prototype.updateCitationHeaderButton = function() {
  const count = this.allCitations.length;

  if (this.citationCounterEl) {
    this.citationCounterEl.textContent = count > 0 ? String(count).padStart(3, '0') : '000';
  }
};

/**
 * Toggle citation popup from the header button
 */
CrimsonManager.prototype.toggleCitationPopup = function() {
  if (this.activeCitationPopup) {
    this.closeCitationPopup();
    return;
  }

  this.showCitationPopup(this.citationHeaderBtn, this.allCitations);
};

/**
 * Load citation popup position and size from localStorage
 * @returns {Object|null} Saved position/size or null
 */
CrimsonManager.prototype.loadCitationPopupState = function() {
  try {
    const x = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_X);
    const y = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_Y);
    const width = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_WIDTH);
    const height = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_HEIGHT);
    if (x !== null && y !== null && width !== null && height !== null) {
      return {
        x: parseInt(x, 10),
        y: parseInt(y, 10),
        width: parseInt(width, 10),
        height: parseInt(height, 10),
      };
    }
  } catch (e) {
    // localStorage may not be available
  }
  return null;
};

/**
 * Save citation popup position and size to localStorage
 * @param {HTMLElement} popup - The popup element
 */
CrimsonManager.prototype.saveCitationPopupState = function(popup) {
  try {
    if (!popup) return;
    const rect = popup.getBoundingClientRect();
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_X, String(rect.left));
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_Y, String(rect.top));
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_WIDTH, String(rect.width));
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_HEIGHT, String(rect.height));
  } catch (e) {
    // localStorage may not be available
  }
};

/**
 * Show citation popup anchored to a button, using a LithiumTable for the list.
 * @param {HTMLElement} button - The anchor button
 * @param {Array} citations - Array of normalized citation objects
 */
CrimsonManager.prototype.showCitationPopup = async function(button, citations) {
  const popup = document.createElement('div');
  popup.className = 'crimson-citation-popup';
  this._citationPopupEl = popup;

  popup.innerHTML = `
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-tl" data-tooltip="Resize"></div>
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-tr" data-tooltip="Resize"></div>
    <div class="crimson-citation-popup-header">
      <span class="crimson-citation-popup-title">Citations</span>
      <button type="button" class="crimson-citation-popup-close" title="Close (ESC)">
        <fa fa-xmark></fa>
      </button>
    </div>
    <div class="crimson-citation-popup-content">
      <div class="crimson-citation-table-wrap lithium-table-container"></div>
      <div class="crimson-citation-nav-wrap lithium-navigator-container"></div>
    </div>
    <div class="crimson-citation-popup-arrow"></div>
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-bl" data-tooltip="Resize"></div>
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-br" data-tooltip="Resize"></div>
  `;

  document.body.appendChild(popup);
  processIcons(popup);

  const header = popup.querySelector('.crimson-citation-popup-header');
  const resizeHandleBR = popup.querySelector('.crimson-citation-resize-handle-br');
  const resizeHandleBL = popup.querySelector('.crimson-citation-resize-handle-bl');
  const resizeHandleTR = popup.querySelector('.crimson-citation-resize-handle-tr');
  const resizeHandleTL = popup.querySelector('.crimson-citation-resize-handle-tl');

  header?.addEventListener('mousedown', this.handleCitationDragStart);
  resizeHandleBR?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'br'));
  resizeHandleBL?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'bl'));
  resizeHandleTR?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'tr'));
  resizeHandleTL?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'tl'));

  const savedState = this.loadCitationPopupState();
  if (savedState) {
    popup.style.left = `${savedState.x}px`;
    popup.style.top = `${savedState.y}px`;
    popup.style.width = `${savedState.width}px`;
    popup.style.height = `${savedState.height}px`;
  } else {
    this.positionCitationPopup(popup, button);
  }

  const closeBtn = popup.querySelector('.crimson-citation-popup-close');
  closeBtn?.addEventListener('click', () => this.closeCitationPopup());

  this.activeCitationPopup = popup;
  if (button) button.classList.add('active');

  await new Promise(resolve => {
    requestAnimationFrame(() => {
      popup.classList.add('visible');
      setTimeout(resolve, 50);
    });
  });

  const tableContainer = popup.querySelector('.crimson-citation-table-wrap');
  const navContainer = popup.querySelector('.crimson-citation-nav-wrap');

  const app = window.lithiumApp || null;

  this.citationTable = new LithiumTable({
    container: tableContainer,
    navigatorContainer: navContainer,
    app: app,
    readonly: true,
    cssPrefix: 'lithium',
    storageKey: 'crimson_citations',
    tablePath: 'crimson/citations',
    lookupKeyIdx: 13,
    onRowSelected: (rowData) => {
      log(Subsystems.CRIMSON, Status.DEBUG, `Citation selected: #${rowData.number} ${rowData.name}`);
    },
    onDataLoaded: (rows) => {
      log(Subsystems.CRIMSON, Status.DEBUG, `Citations loaded: ${rows.length} rows`);
    },
  });

  await this.citationTable.init();

  this.citationTable.loadStaticData(citations, { autoSelect: false });

  this.citationTable.table.on('rowDblClick', (_e, row) => {
    const rowData = row.getData();
    // Only open URLs that start with http
    if (rowData?.url && rowData.url.startsWith('http')) {
      this.openCitationUrl(rowData.url);
    }
  });

  const urlCol = this.citationTable.table.getColumn('url');
  if (urlCol) {
    urlCol.update({
      formatter: (cell) => this.formatReference(cell.getValue()),
    });
  }

  const scoreCol = this.citationTable.table.getColumn('score');
  if (scoreCol) {
    scoreCol.update({
      formatter: (cell) => {
        const score = cell.getValue();
        if (score === null || score === undefined) return '';
        return score.toLocaleString();
      },
    });
  }

  const linkCol = this.citationTable.table.getColumn('link');
  if (linkCol) {
    linkCol.update({
      headerSort: false,
      formatter: (cell) => {
        const url = cell.getValue();
        if (!url) return '';
        if (url.startsWith('canvas') || url.includes('canvas/')) {
          return '<fa fa-graduation-cap fa-flip-horizontal></fa>';
        }
        return '<fa fa-arrow-up-right-from-square></fa>';
      },
      cellClick: (_e, cell) => {
        const url = cell.getValue();
        // Only open URLs that start with http
        if (url && url.startsWith('http')) {
          this.openCitationUrl(url);
        }
      },
    });
  }

  this._citationOutsideHandler = (e) => {
    if (!popup.contains(e.target) && !(button && button.contains(e.target))) {
      this.closeCitationPopup();
    }
  };
  document.addEventListener('click', this._citationOutsideHandler);

  log(Subsystems.CRIMSON, Status.DEBUG, `Opened citation popup with ${citations.length} citations`);
};

/**
 * Position citation popup relative to button.
 * @param {HTMLElement} popup - The popup element
 * @param {HTMLElement} button - The anchor button
 */
CrimsonManager.prototype.positionCitationPopup = function(popup, button) {
  const buttonRect = button.getBoundingClientRect();
  const popupWidth = 500;

  let left = buttonRect.left;
  let top = buttonRect.bottom + 6;

  if (left + popupWidth > window.innerWidth - 20) {
    left = window.innerWidth - popupWidth - 20;
  }
  if (left < 10) left = 10;

  if (top + 400 > window.innerHeight - 20) {
    top = buttonRect.top - 400 - 6;
  }
  if (top < 10) top = 10;

  popup.style.left = `${left}px`;
  popup.style.top = `${top}px`;

  const arrow = popup.querySelector('.crimson-citation-popup-arrow');
  if (arrow) {
    const arrowLeft = buttonRect.left + buttonRect.width / 2 - left;
    arrow.style.left = `${Math.max(10, Math.min(arrowLeft, popupWidth - 10))}px`;

    if (top > buttonRect.bottom) {
      arrow.classList.add('arrow-top');
    } else {
      arrow.classList.add('arrow-bottom');
    }
  }
};

/**
 * Close citation popup
 */
CrimsonManager.prototype.closeCitationPopup = function() {
  if (this._citationOutsideHandler) {
    document.removeEventListener('click', this._citationOutsideHandler);
    this._citationOutsideHandler = null;
  }

  this.handleCitationDragEnd();
  this.handleCitationResizeEnd();

  if (this.citationTable) {
    this.citationTable.destroy();
    this.citationTable = null;
  }

  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.remove('visible');
    setTimeout(() => {
      this.activeCitationPopup?.remove();
      this.activeCitationPopup = null;
      this._citationPopupEl = null;
    }, 200);
  }

  if (this.citationHeaderBtn) {
    this.citationHeaderBtn.classList.remove('active');
  }
};

/**
 * Highlight a specific citation in the popup via LithiumTable row selection
 * @param {number} citationNumber - The global citation number to highlight
 */
CrimsonManager.prototype.highlightCitation = function(citationNumber) {
  if (!this.citationTable?.table) return;

  this.citationTable.table.deselectRow();

  const rows = this.citationTable.table.getRows();
  for (const row of rows) {
    const data = row.getData();
    if (data.number === citationNumber) {
      row.select();
      row.scrollTo();
      break;
    }
  }
};

// ==================== CITATION MARKERS ====================

/**
 * Convert citation markers [[C1]] in text to clickable icons.
 * @param {string} content - The message content (HTML)
 * @param {string} messageId - The message element ID
 * @param {Map<number,number>} turnMapping - Per-turn index to global number mapping
 * @returns {string} Content with citation links
 */
CrimsonManager.prototype.convertCitationMarkers = function(content, messageId, turnMapping) {
  if (!content) return '';

  return content.replace(/\[\[C(\d+)\]\]/g, (match, num) => {
    const turnIndex = parseInt(num, 10);
    const globalNum = (turnMapping && turnMapping.has(turnIndex))
      ? turnMapping.get(turnIndex)
      : turnIndex;
    const citation = this.allCitations.find(c => c.number === globalNum);
    let tooltipText;
    if (citation?.name && citation.name !== 'Untitled') {
      tooltipText = citation.name;
    } else if (citation?.url) {
      tooltipText = this.formatReference(citation.url);
    } else {
      tooltipText = `Citation ${globalNum}`;
    }
    return `<a href="#" class="crimson-citation-link" data-citation="${globalNum}" title="${this.escapeHtml(tooltipText)}"><fa fa-book-open-lines class="crimson-citation-link-icon"></fa></a>`;
  });
};

/**
 * Handle citation link click (delegated from conversation container)
 * @param {Event} e - Click event
 */
CrimsonManager.prototype.handleCitationLinkClick = function(e) {
  const link = e.target.closest('.crimson-citation-link');
  if (!link) return;

  e.preventDefault();
  e.stopPropagation();

  const citationNum = parseInt(link.getAttribute('data-citation'), 10);
  if (!citationNum || this.allCitations.length === 0) return;

  if (!this.activeCitationPopup) {
    this.showCitationPopup(this.citationHeaderBtn, this.allCitations).then(() => {
      setTimeout(() => this.highlightCitation(citationNum), 100);
    });
  } else {
    this.highlightCitation(citationNum);
  }
};
