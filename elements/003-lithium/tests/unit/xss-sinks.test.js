/**
 * CATCHUP Phase 3 — XSS sinks.
 * Payloads must not become elements. Text and allowlisted icons may remain.
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { sanitizeHtml } from '../../src/core/utils.js';
import {
  appendTextWithIcons,
  faNameFromTag,
  isFaNameToken,
} from '../../src/core/icons.js';
import { highlightInit } from '../../src/init/highlight-init.js';
import { fillGroupingTitle } from '../../src/tables/popups/grouping-popup.js';
import {
  fillQueriesNavLabel,
  fillQueriesTemplateLabel,
} from '../../src/managers/queries/queries-ui.js';
import { EditorManager } from '../../src/managers/queries/queries-editors.js';
import LookupsManager from '../../src/managers/lookups/lookups.js';
import VersionHistoryManager from '../../src/managers/version-history/version-history.js';
import { CrimsonManager } from '../../src/managers/crimson/crimson-core.js';
import '../../src/managers/crimson/crimson-chat.js';
import '../../src/managers/crimson/crimson-citations.js';

const IMG = '<img src="x" onerror="alert(1)">';
const SCRIPT = '<script>alert(1)</script>';
const PAYLOAD = `${IMG}${SCRIPT}`;

function expectInactive(root) {
  expect(root.querySelector('script, iframe, object, embed')).toBeNull();
  expect(root.querySelector('[onerror]')).toBeNull();
  for (const el of root.querySelectorAll('*')) {
    for (const attr of el.attributes) {
      expect(attr.name.toLowerCase().startsWith('on')).toBe(false);
      if (attr.name === 'href' || attr.name === 'src') {
        expect(attr.value).not.toMatch(/^\s*javascript:/i);
      }
    }
  }
}

describe('Phase 3 XSS sinks', () => {
  describe('lookup preview', () => {
    const lookups = Object.create(LookupsManager.prototype);

    it('escapes an image and a script before highlight', () => {
      const host = document.createElement('div');
      host.innerHTML = sanitizeHtml(lookups.processContentWithHighlighting(PAYLOAD));
      expectInactive(host);
      expect(host.textContent).toContain('onerror');
      expect(host.textContent).toContain('alert(1)');
    });

    it('escapes the same payload in the basic processor', () => {
      const processed = lookups.basicContentProcessing(`${PAYLOAD}\nnext`);
      expect(processed).toContain('<br>');
      expect(processed.toLowerCase()).not.toContain('<script');
      expect(processed.toLowerCase()).not.toContain('<img');
      const host = document.createElement('div');
      host.innerHTML = sanitizeHtml(processed);
      expectInactive(host);
    });
  });

  describe('markdown previews', () => {
    it('sanitizes the Queries summary preview', async () => {
      const container = document.createElement('div');
      const editor = new EditorManager({
        elements: { previewContainer: container },
        _pendingSummaryContent: PAYLOAD,
        currentQuery: null,
      });
      await editor.renderMarkdownPreview();
      expectInactive(container);
    });

    it('sanitizes Crimson markdown', async () => {
      const crimson = Object.create(CrimsonManager.prototype);
      const host = document.createElement('div');
      host.innerHTML = await crimson.formatMessageContentMarkdown(PAYLOAD);
      expectInactive(host);
    });

    it('sanitizes the Crimson stream formatter', () => {
      const crimson = Object.create(CrimsonManager.prototype);
      const host = document.createElement('div');
      host.innerHTML = sanitizeHtml(crimson.formatMessageContent(`**${PAYLOAD}**`));
      expectInactive(host);
      expect(host.textContent).toContain('alert(1)');
    });
  });

  describe('version history preview', () => {
    it('renders status, Released, and Focus as text and sanitizes HTML', () => {
      const manager = Object.create(VersionHistoryManager.prototype);
      manager.elements = { previewContent: document.createElement('div') };
      manager.selectedVersionData = {
        summary: PAYLOAD,
        value_txt: SCRIPT,
        Released: IMG,
        Focus: 'plain',
      };
      manager.expandContentMacros = (value) => value;
      manager.updatePreview();

      const host = manager.elements.previewContent;
      expectInactive(host);
      expect(host.querySelector('h2').textContent).toBe(SCRIPT);
      expect(host.textContent).toContain(`Released: ${IMG}`);
      expect(host.textContent).toContain('Focus: plain');
    });

    it('puts a plain summary through escapeHtml inside pre when marked is absent', () => {
      const manager = Object.create(VersionHistoryManager.prototype);
      manager.elements = { previewContent: document.createElement('div') };
      manager.selectedVersionData = {
        summary: `line\n${PAYLOAD}`,
        value_txt: '1.0',
      };
      manager.expandContentMacros = (value) => value;
      manager.detectContentType = () => 'markdown';
      manager.updatePreview();

      const pre = manager.elements.previewContent.querySelector('pre');
      expect(pre).not.toBeNull();
      expect(pre.classList.contains('version-preview-plain')).toBe(true);
      expectInactive(manager.elements.previewContent);
      expect(pre.textContent).toContain('line');
    });
  });

  describe('citation links and follow-ups', () => {
    it('builds a citation link after sanitize and keeps the tooltip out of HTML', () => {
      const crimson = Object.create(CrimsonManager.prototype);
      crimson.allCitations = [{ number: 1, name: PAYLOAD }];
      crimson.formatReference = (url) => url;
      const host = document.createElement('div');
      host.innerHTML = sanitizeHtml('<p>See [[C1]]</p>');
      crimson.applyCitationLinks(host, 'msg-1', new Map());

      expectInactive(host);
      const link = host.querySelector('a.crimson-citation-link');
      expect(link).not.toBeNull();
      expect(link.getAttribute('title')).toBe(PAYLOAD);
      expect(link.getAttribute('data-citation')).toBe('1');
      expect(host.querySelector('i.fa-book-open-lines, i[class*="fa-book-open-lines"]')).not.toBeNull();
    });

    it('puts follow-up text in a text node', () => {
      const parent = document.createElement('div');
      const message = document.createElement('div');
      parent.appendChild(message);
      const crimson = Object.create(CrimsonManager.prototype);
      crimson.input = { value: '' };
      crimson.handleSend = () => {};
      crimson.addFollowUpQuestions(message, [PAYLOAD]);

      expectInactive(parent);
      expect(parent.querySelector('.crimson-followup-text').textContent).toBe(PAYLOAD);
      expect(parent.querySelector('fa, i')).not.toBeNull();
    });
  });

  describe('highlightInline', () => {
    let element;

    beforeEach(() => {
      element = document.createElement('div');
      element.id = 'xss-highlight-inline';
      document.body.appendChild(element);
    });

    afterEach(() => {
      element.remove();
    });

    it('does not interpolate code or an unsafe language into HTML', () => {
      element.textContent = PAYLOAD;
      highlightInit.highlightInline(element.id, 'js"><img src=x onerror=alert(1)>');
      const span = element.querySelector('span');
      expect(span).not.toBeNull();
      expect(span.className).toBe('hljs-inline');
      expect(span.textContent).toBe(PAYLOAD);
      expectInactive(element);
    });

    it('adds a language class for a safe token', () => {
      element.textContent = 'const a = 1;';
      highlightInit.highlightInline(element.id, 'javascript');
      expect(element.querySelector('span').classList.contains('language-javascript')).toBe(true);
    });
  });

  describe('icons, grouping titles, and nav labels', () => {
    it('turns one fa tag in a caption into an icon and leaves other markup as text', () => {
      const caption = document.createElement('div');
      appendTextWithIcons(caption, `Hello <fa fa-party-horn></fa> ${PAYLOAD}`);
      expect(faNameFromTag('<fa fa-party-horn></fa>')).toBe('fa-party-horn');
      expect(caption.querySelector('fa').getAttribute('fa-party-horn')).toBe('');
      expectInactive(caption);
      expect(caption.textContent).toContain('Hello ');
      expect(caption.textContent).toContain(PAYLOAD);
    });

    it('renders a grouping title icon only when the title is that one tag', () => {
      const title = document.createElement('span');
      fillGroupingTitle(title, '<fa fa-wrench></fa>', {});
      expect(title.querySelector('fa').getAttribute('fa-wrench')).toBe('');

      fillGroupingTitle(title, `<fa fa-wrench></fa>${PAYLOAD}`, {});
      expect(title.querySelector('fa')).toBeNull();
      expectInactive(title);
      expect(title.textContent).toContain(PAYLOAD);
    });

    it('renders the lookup group title as text', () => {
      const title = document.createElement('span');
      fillGroupingTitle(title, 'Name', {
        lookupRef: 1,
        groupStyle: 'iconLabel',
        groupTitle: PAYLOAD,
        field: 'status',
      });
      expectInactive(title);
      expect(title.querySelector('.lithium-grouping-grouptitle').textContent).toBe(PAYLOAD);
    });

    it('renders a nav icon only for an fa-name token', () => {
      expect(isFaNameToken('fa-expand')).toBe(true);
      const row = document.createElement('button');
      fillQueriesNavLabel(row, { icon: 'fa-expand', label: PAYLOAD });
      expect(row.querySelector('fa').getAttribute('fa-expand')).toBe('');
      expectInactive(row);
      expect(row.textContent).toBe(PAYLOAD);

      fillQueriesNavLabel(row, { icon: 'fa-expand"><img src=x onerror=alert(1)>', label: 'Expand' });
      expect(row.querySelector('fa')).toBeNull();
      expect(row.textContent).toBe('Expand');
    });

    it('renders a template name as text', () => {
      const label = document.createElement('button');
      fillQueriesTemplateLabel(label, { label: PAYLOAD, isDefault: true });
      expectInactive(label);
      expect(label.querySelector('.queries-nav-popup-template-name').textContent).toBe(PAYLOAD);
      expect(label.textContent).toContain('(default)');
    });
  });
});
