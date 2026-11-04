/**
 * Page switch for Manager 24.
 * Report, Editor, Definition (page id json), Revisions, Compose,
 * Preview HTML, and Preview PDF. Leaving Report commits
 * the meta fields. Leaving JSON commits a parsed edit first.
 * Editor stays the page that opens.
 */

const PAGES = new Set(['report', 'editor', 'json', 'revisions', 'compose', 'html', 'pdf']);

export const reportPageMethods = {
  /**
   * @param {'report'|'editor'|'json'|'revisions'|'compose'|'html'|'pdf'} page
   * @returns {Promise<void>|undefined}
   */
  showPage(page) {
    if (this._closed) return undefined;
    const next = PAGES.has(page) ? page : 'editor';
    if (this._page === 'report' && next !== 'report' && typeof this._commitMetaFields === 'function') {
      this._commitMetaFields();
    }
    if (this._page === 'json' && next !== 'json' && typeof this.commitJson === 'function') {
      this.commitJson();
    }
    this._page = next;
    const root = this.container && this.container.querySelector
      ? this.container.querySelector('.reports-manager')
      : null;
    if (root) root.dataset.page = next;
    if (this.container && this.container.querySelectorAll) {
      for (const el of this.container.querySelectorAll('[data-reports-page]')) {
        el.hidden = !el.dataset.reportsPage.split(' ').includes(next);
      }
    }
    const elements = this.elements || {};
    if (elements.reportTab) elements.reportTab.classList.toggle('active', next === 'report');
    if (elements.editorTab) elements.editorTab.classList.toggle('active', next === 'editor');
    if (elements.jsonTab) elements.jsonTab.classList.toggle('active', next === 'json');
    if (elements.revisionsTab) elements.revisionsTab.classList.toggle('active', next === 'revisions');
    if (elements.composeTab) elements.composeTab.classList.toggle('active', next === 'compose');
    if (elements.htmlTab) elements.htmlTab.classList.toggle('active', next === 'html');
    if (elements.pdfTab) elements.pdfTab.classList.toggle('active', next === 'pdf');
    if (next === 'compose' && typeof this._openCompose === 'function') this._openCompose();
    if (next === 'json' && typeof this._openJson === 'function') this._openJson();
    if ((next === 'json' || next === 'compose') && typeof this._applyReportFont === 'function') this._applyReportFont(next);
    if (next === 'revisions' && typeof this.openRevisions === 'function') return this.openRevisions();
    return undefined;
  },
};
