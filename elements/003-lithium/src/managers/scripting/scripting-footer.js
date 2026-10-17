/**
 * Scripting Manager footer actions.
 * Mixed onto ScriptingManager. Save and cancel stay on the edit helper.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { setupManagerFooterIcons, positionPopup, closeAllPopups } from '../../core/manager-ui.js';
import { toast } from '../../shared/toast.js';

export const scriptingFooterMethods = {
  _setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this._handleFooterPrint(),
      onEmail: () => this._handleFooterEmail(),
      onDownload: () => this._handleFooterDownload(),
      onExport: (value, label) => this._handleFooterExport(value, label),
      onClipboard: (value, label) => this._handleFooterClipboard(value, label),
      reportOptions: [
        { value: 'scripts-view', label: 'Scripts View' },
        { value: 'scripts-data', label: 'Scripts Data' },
      ],
      fillerTitle: 'Scripts',
      anchor: placeholder,
      showSaveCancel: true,
      showClipboard: true,
      exportOptions: [
        { value: 'pdf', label: 'PDF', icon: 'fa-file-pdf', enabled: true },
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'xls', label: 'XLS', icon: 'fa-file-excel', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
        { value: 'html', label: 'HTML', icon: 'fa-file-html', enabled: true },
        { value: 'markdown', label: 'Markdown', icon: 'fa-file-code', enabled: true },
        { value: 'txt', label: 'Text', icon: 'fa-file-lines', enabled: true },
      ],
    });

    this._footerDatasource = footerElements.reportSelect;
    this._footerElements = footerElements;

    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );
  },

  _getFooterDatasource() {
    return this._footerDatasource?.value || 'scripts-view';
  },

  _handleFooterPrint() {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to print', duration: 3000 });
      return;
    }

    if (mode.endsWith('-view')) {
      table.print();
    } else {
      table.print('all', true);
    }
  },

  _handleFooterEmail() {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to email', duration: 3000 });
      return;
    }

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) {
      toast.info('No Data', { description: 'No rows to include in the email', duration: 3000 });
      return;
    }

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const separator = headers.map(h => '-'.repeat(h.length)).join('  ');
    const dataLines = rows.slice(0, 50).map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val) : '';
      }).join('\t');
    });

    const totalRows = rows.length;
    const truncated = totalRows > 50 ? `\n... (${totalRows - 50} more rows not shown)` : '';
    const modeLabel = isViewMode ? 'Filtered View' : 'Full Data';

    const subject = encodeURIComponent(`Scripts — ${totalRows} rows (${modeLabel})`);
    const body = encodeURIComponent(
      `Scripts Export (${modeLabel})\n` +
      `${totalRows} row(s)\n\n` +
      `${headers.join('\t')}\n${separator}\n` +
      `${dataLines.join('\n')}${truncated}\n`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  },

  _toggleFooterExportPopup(e) {
    e.stopPropagation();

    if (this._footerExportPopup) {
      this._closeFooterExportPopup();
      return;
    }

    closeAllPopups();

    const btn = e.currentTarget;
    this._footerExportButton = btn;
    btn.classList.add('popup-active');

    const mode = this._getFooterDatasource();
    const formats = [
      { label: 'PDF', action: () => this._handleFooterExport('pdf', mode) },
      { label: 'CSV', action: () => this._handleFooterExport('csv', mode) },
      { label: 'TXT', action: () => this._handleFooterExport('txt', mode) },
      { label: 'XLS', action: () => this._handleFooterExport('xls', mode) },
    ];

    const popup = document.createElement('div');
    popup.className = 'manager-footer-export-popup manager-footer-popup';
    formats.forEach(item => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'manager-footer-export-popup-item';
      row.textContent = item.label;
      row.addEventListener('click', () => {
        this._closeFooterExportPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    document.body.appendChild(popup);
    positionPopup(popup, btn, 'footer-riseup');

    setTimeout(() => popup.classList.add('visible'), 10);

    this._footerExportPopup = popup;

    this._footerExportCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this._closeFooterExportPopup();
      }
    };
    document.addEventListener('click', this._footerExportCloseHandler);

    this._footerExportGlobalCloseHandler = () => {
      this._closeFooterExportPopup();
    };
    document.addEventListener('close-all-popups', this._footerExportGlobalCloseHandler);
  },

  _closeFooterExportPopup() {
    if (this._footerExportButton) {
      this._footerExportButton.classList.remove('popup-active');
      this._footerExportButton = null;
    }
    if (this._footerExportPopup) {
      const popup = this._footerExportPopup;
      this._footerExportPopup = null;
      popup.classList.remove('visible');
      const duration = parseFloat(getComputedStyle(popup).transitionDuration) * 1000 || 350;
      setTimeout(() => popup.remove(), duration);
    }
    if (this._footerExportCloseHandler) {
      document.removeEventListener('click', this._footerExportCloseHandler);
      this._footerExportCloseHandler = null;
    }
    if (this._footerExportGlobalCloseHandler) {
      document.removeEventListener('close-all-popups', this._footerExportGlobalCloseHandler);
      this._footerExportGlobalCloseHandler = null;
    }
  },

  _handleFooterExport(value, _label) {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to export', duration: 3000 });
      return;
    }

    const filename = `scripts-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    this._doDownload(table, value, filename, downloadOpts);
  },

  _handleFooterDownload() {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to download', duration: 3000 });
      return;
    }

    const filename = `scripts-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    const exportFormat = this._footerElements?.exportFormat || 'pdf';
    this._doDownload(table, exportFormat, filename, downloadOpts);
  },

  _doDownload(table, format, filename, downloadOpts) {
    switch (format) {
      case 'pdf':
        table.download('pdf', `${filename}.pdf`, { orientation: 'landscape', ...downloadOpts });
        break;
      case 'csv':
        table.download('csv', `${filename}.csv`, downloadOpts);
        break;
      case 'txt':
        table.download('csv', `${filename}.txt`, downloadOpts);
        break;
      case 'xls':
        table.download('xlsx', `${filename}.xlsx`, downloadOpts);
        break;
      case 'json':
        table.download('json', `${filename}.json`, downloadOpts);
        break;
      case 'html':
        table.download('html', `${filename}.html`, downloadOpts);
        break;
      case 'markdown':
        this._handleMarkdownExport(table, this._getFooterDatasource(), filename);
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `[Scripting] Unknown export format: ${format}`);
    }
  },

  _handleFooterClipboard(_value, _label) {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to copy', duration: 3000 });
      return;
    }

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) {
      toast.info('No Data', { description: 'No rows to copy', duration: 3000 });
      return;
    }

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const dataLines = rows.map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val) : '';
      }).join('\t');
    });

    const text = `${headers.join('\t')}\n${dataLines.join('\n')}`;

    navigator.clipboard.writeText(text).then(() => {
      toast.success('Copied', { description: 'Table data copied to clipboard', duration: 2000 });
    }).catch(() => {
      toast.error('Copy Failed', { description: 'Failed to copy to clipboard', duration: 3000 });
    });
  },

  _handleMarkdownExport(table, mode, filename) {
    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) return;

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const headerRow = `| ${headers.join(' | ')} |`;
    const separatorRow = `| ${headers.map(() => '---').join(' | ')} |`;
    const dataRows = rows.map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val).replace(/\|/g, '\\|') : '';
      }).join(' | ');
    });

    const markdown = `${headerRow}\n${separatorRow}\n${dataRows.join('\n')}`;

    const blob = new Blob([markdown], { type: 'text/markdown' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${filename}.md`;
    a.click();
    URL.revokeObjectURL(url);
  },
};
