/**
 * Toast Notification Tests
 *
 * Tests for the toast notification system including:
 * - Unified header button group (icon, title, subsystem, expand, close)
 * - Expand button (shows content and keeps toast)
 * - Countdown progress bar
 * - Collapsible details
 * - Automatic error logging
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';

// Mock the log module before importing toast
vi.mock('../../src/core/log.js', () => ({
  log: vi.fn(),
  logGroup: vi.fn(),
  Subsystems: { CONDUIT: 'Conduit', RESTAPI: 'Conduit' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR', SUCCESS: 'SUCCESS' },
}));

import { toast, TOAST_TYPES } from '../../src/shared/toast.js';
import { log, logGroup, Subsystems, Status } from '../../src/core/log.js';

describe('Toast System', () => {
  let container;

  beforeEach(() => {
    // Clear any existing toasts
    toast.dismissAll();

    // Clear the container contents
    container = document.getElementById('toast-container');
    if (container) {
      container.innerHTML = '';
    }

    // Reset mocks
    vi.clearAllMocks();
  });

  afterEach(() => {
    if (toast) {
      toast.dismissAll();
    }
  });

  describe('show()', () => {
    it('should create a toast element', () => {
      const id = toast.show('Test message');
      expect(id).toBeDefined();
      expect(id).toMatch(/^toast-\d+$/);

      const toastEl = container.querySelector(`[data-id="${id}"]`);
      expect(toastEl).not.toBeNull();
      expect(toastEl.classList.contains('toast')).toBe(true);
    });

    it('should apply correct type class', () => {
      toast.show('Error', { type: 'error' });
      const toastEl = container.querySelector('.toast-error');
      expect(toastEl).not.toBeNull();
    });

    it('should display message content as title', () => {
      const id = toast.show('Hello World');
      const toastEl = container.querySelector(`[data-id="${id}"]`);
      const titleEl = toastEl.querySelector('.toast-header-title');
      expect(titleEl.textContent).toBe('Hello World');
    });

    it('should include Font Awesome icon in header', () => {
      toast.show('Test');
      const iconEl = container.querySelector('.toast-header-icon i');
      expect(iconEl).not.toBeNull();
      expect(iconEl.classList.contains('fa-info-circle')).toBe(true);
    });

    it('should support custom icon', () => {
      const customIcon = '<i class="fas fa-star"></i>';
      toast.show('Test', { icon: customIcon });
      const iconEl = container.querySelector('.toast-header-icon');
      expect(iconEl.innerHTML).toContain('fa-star');
    });

    it('should auto-dismiss after duration', async () => {
      vi.useFakeTimers();
      const id = toast.show('Test', { duration: 1000 });

      const toastEl = container.querySelector(`[data-id="${id}"]`);
      expect(toastEl).not.toBeNull();

      vi.advanceTimersByTime(1000);

      // After duration + animation, toast should be removed
      vi.advanceTimersByTime(300);
      expect(container.querySelector(`[data-id="${id}"]`)).toBeNull();

      vi.useRealTimers();
    });

    it('should not auto-dismiss when duration is 0', () => {
      vi.useFakeTimers();
      const id = toast.show('Persistent', { duration: 0 });

      vi.advanceTimersByTime(10000);

      const toastEl = container.querySelector(`[data-id="${id}"]`);
      expect(toastEl).not.toBeNull();

      vi.useRealTimers();
    });
  });

  describe('error()', () => {
    it('should create error toast with correct icon', () => {
      toast.error('Error message');
      const toastEl = container.querySelector('.toast-error');
      expect(toastEl).not.toBeNull();

      const iconEl = toastEl.querySelector('.toast-header-icon i');
      expect(iconEl.classList.contains('fa-times-circle')).toBe(true);
    });

    it('should use serverError.error as title and build details from serverError object', () => {
      const serverError = {
        query_ref: 27,
        database: 'Lithium',
        error: 'Missing required parameters',
        message: 'Required: QUERYID'
      };

      toast.error('Fallback Title', { serverError });

      // Title should come from serverError.error
      const titleEl = container.querySelector('.toast-header-title');
      expect(titleEl.textContent).toBe('Missing required parameters');

      // Details should contain query_ref and database
      const detailsEl = container.querySelector('.toast-details');
      expect(detailsEl).not.toBeNull();
      expect(detailsEl.textContent).toContain('Query Ref: 27');
      expect(detailsEl.textContent).toContain('Database: Lithium');
    });

    it('should NOT show details expanded by default (user must click expand)', () => {
      toast.error('Error', { details: 'Some details' });
      const contentEl = container.querySelector('.toast-content-visible');
      expect(contentEl).toBeNull();
    });

    it('should auto-build description from serverError.message', () => {
      const serverError = {
        message: 'Detailed server message',
        error: 'Server error',
      };

      toast.error('Short error', { serverError });

      // Title should be from serverError.error
      const titleEl = container.querySelector('.toast-header-title');
      expect(titleEl.textContent).toBe('Server error');

      // Description should be from serverError.message
      const descEl = container.querySelector('.toast-description');
      expect(descEl).not.toBeNull();
      expect(descEl.textContent).toBe('Detailed server message');
    });

    it('should auto-build description even when message matches (for two-part layout)', () => {
      const serverError = {
        message: 'Same message',
        error: 'Server error',
      };

      toast.error('Same message', { serverError });

      // Title should be from serverError.error, not the message param
      const titleEl = container.querySelector('.toast-header-title');
      expect(titleEl.textContent).toBe('Server error');

      const descEl = container.querySelector('.toast-description');
      expect(descEl).not.toBeNull();
      expect(descEl.textContent).toBe('Same message');
    });

    it('should not duplicate log when serverError is provided (json-request handles it)', () => {
      const serverError = {
        query_ref: 27,
        database: 'Lithium',
        error: 'Missing required parameters',
        message: 'Required: QUERYID',
      };

      toast.error('Required: QUERYID', { serverError, subsystem: 'Conduit' });

      // Toast should NOT log directly — json-request.js handles RESTAPI-level logging
      expect(log).not.toHaveBeenCalled();
      expect(logGroup).not.toHaveBeenCalled();
    });

    it('should not log when serverError is not provided', () => {
      toast.error('Simple error message');

      // No serverError, no logging
      expect(log).not.toHaveBeenCalled();
      expect(logGroup).not.toHaveBeenCalled();
    });
  });

  describe('success()', () => {
    it('should create success toast with check icon', () => {
      toast.success('Success!');
      const toastEl = container.querySelector('.toast-success');
      expect(toastEl).not.toBeNull();

      const iconEl = toastEl.querySelector('.toast-header-icon i');
      expect(iconEl.classList.contains('fa-check-circle')).toBe(true);
    });
  });

  describe('warning()', () => {
    it('should create warning toast with triangle icon', () => {
      toast.warning('Warning!');
      const toastEl = container.querySelector('.toast-warning');
      expect(toastEl).not.toBeNull();

      const iconEl = toastEl.querySelector('.toast-header-icon i');
      expect(iconEl.classList.contains('fa-exclamation-triangle')).toBe(true);
    });
  });

  describe('info()', () => {
    it('should create info toast with circle icon', () => {
      toast.info('Info');
      const toastEl = container.querySelector('.toast-info');
      expect(toastEl).not.toBeNull();

      const iconEl = toastEl.querySelector('.toast-header-icon i');
      expect(iconEl.classList.contains('fa-info-circle')).toBe(true);
    });
  });

  describe('dismiss()', () => {
    it('should remove toast from DOM after animation', async () => {
      vi.useFakeTimers();
      const id = toast.show('Test');
      expect(container.querySelector(`[data-id="${id}"]`)).not.toBeNull();

      toast.dismiss(id);

      // After animation delay
      vi.advanceTimersByTime(300);
      expect(container.querySelector(`[data-id="${id}"]`)).toBeNull();

      vi.useRealTimers();
    });

    it('should handle invalid ID gracefully', () => {
      expect(() => toast.dismiss('nonexistent')).not.toThrow();
    });

    it('should clear auto-dismiss timer', () => {
      vi.useFakeTimers();
      const id = toast.show('Test', { duration: 5000 });

      // Dismiss manually before timeout
      toast.dismiss(id);
      vi.advanceTimersByTime(300);

      // Should not throw from double-dismiss via timer
      vi.advanceTimersByTime(5000);

      vi.useRealTimers();
    });
  });

  describe('dismissAll()', () => {
    it('should remove all toasts', async () => {
      toast.show('One');
      toast.show('Two');
      toast.show('Three');

      // Wait for toasts to be added
      await new Promise(resolve => setTimeout(resolve, 50));

      const beforeCount = container.querySelectorAll('.toast').length;
      expect(beforeCount).toBeGreaterThanOrEqual(3);

      toast.dismissAll();

      // Wait for animation
      await new Promise(resolve => setTimeout(resolve, 350));

      expect(container.querySelectorAll('.toast').length).toBe(0);
    });
  });

  describe('Subsystem Badge', () => {
    it('should render subsystem badge when provided', () => {
      toast.show('Test', { subsystem: 'Conduit' });

      const badge = container.querySelector('.toast-header-subsystem');
      expect(badge).not.toBeNull();
      expect(badge.textContent).toBe('Conduit');
    });

    it('should not render subsystem badge when not provided', () => {
      toast.show('Test');

      const badge = container.querySelector('.toast-header-subsystem');
      expect(badge).toBeNull();
    });

    it('should escape HTML in subsystem label', () => {
      toast.show('Test', { subsystem: '<script>alert(1)</script>' });

      const badge = container.querySelector('.toast-header-subsystem');
      expect(badge).not.toBeNull();
      expect(badge.innerHTML).not.toContain('<script>');
    });
  });

  describe('Description', () => {
    it('should render description in expandable content when provided', () => {
      toast.show('Title text', { description: 'Description text' });

      // Description exists but content area is hidden by default
      const contentEl = container.querySelector('.toast-content');
      expect(contentEl).not.toBeNull();

      const descEl = contentEl.querySelector('.toast-description');
      expect(descEl).not.toBeNull();
      expect(descEl.textContent).toBe('Description text');
    });

    it('should always render content area with default description', () => {
      toast.show('Just a title');

      const contentEl = container.querySelector('.toast-content');
      expect(contentEl).not.toBeNull();

      const descEl = contentEl.querySelector('.toast-description');
      expect(descEl).not.toBeNull();
      expect(descEl.textContent).toBe('No additional information provided');
    });

    it('should escape HTML in description', () => {
      toast.show('Title', { description: '<img src=x onerror=alert(1)>' });

      const descEl = container.querySelector('.toast-description');
      expect(descEl).not.toBeNull();
      expect(descEl.innerHTML).not.toContain('<img');
    });
  });

  describe('Countdown Progress Bar', () => {
    it('should render progress bar when duration > 0', () => {
      toast.show('Test', { duration: 5000 });

      const progress = container.querySelector('.toast-progress');
      const bar = container.querySelector('.toast-progress-bar');
      expect(progress).not.toBeNull();
      expect(bar).not.toBeNull();
    });

    it('should set animation duration matching toast duration', () => {
      toast.show('Test', { duration: 3000 });

      const bar = container.querySelector('.toast-progress-bar');
      expect(bar.style.animationDuration).toBe('3000ms');
    });

    it('should not render progress bar when duration is 0', () => {
      toast.show('Persistent', { duration: 0 });

      const progress = container.querySelector('.toast-progress');
      expect(progress).toBeNull();
    });
  });

  describe('Expand Button', () => {
    it('should render expand button when there is expandable content and duration > 0', () => {
      toast.show('Test', { duration: 5000, description: 'Details...' });

      const expandBtn = container.querySelector('.toast-header-expand');
      expect(expandBtn).not.toBeNull();
    });

    it('should ALWAYS render expand button (due to default description)', () => {
      toast.show('Test', { duration: 5000 });

      const expandBtn = container.querySelector('.toast-header-expand');
      expect(expandBtn).not.toBeNull();
    });

    it('should not render expand button when duration is 0', () => {
      toast.show('Persistent', { duration: 0, description: 'Details...' });

      const expandBtn = container.querySelector('.toast-header-expand');
      expect(expandBtn).toBeNull();
    });

    it('should toggle content visibility when expand is clicked', () => {
      toast.show('Test', { duration: 5000, description: 'Details...' });

      const expandBtn = container.querySelector('.toast-header-expand');
      const contentEl = container.querySelector('.toast-content');

      // Initially hidden
      expect(contentEl.classList.contains('toast-content-visible')).toBe(false);

      // Click to expand
      expandBtn.click();
      expect(contentEl.classList.contains('toast-content-visible')).toBe(true);

      // Click to collapse
      expandBtn.click();
      expect(contentEl.classList.contains('toast-content-visible')).toBe(false);
    });

    it('should cancel auto-dismiss when expand is clicked', () => {
      vi.useFakeTimers();
      const id = toast.show('Test', { duration: 3000, description: 'Details' });

      // Click expand button
      const expandBtn = container.querySelector('.toast-header-expand');
      expandBtn.click();

      // Advance past duration
      vi.advanceTimersByTime(5000);

      // Toast should still exist
      expect(container.querySelector(`[data-id="${id}"]`)).not.toBeNull();

      vi.useRealTimers();
    });

    it('should hide progress bar when expand is clicked', () => {
      toast.show('Test', { duration: 5000, description: 'Details...' });

      const expandBtn = container.querySelector('.toast-header-expand');
      expandBtn.click();

      const progress = container.querySelector('.toast-progress');
      expect(progress.classList.contains('toast-progress-kept')).toBe(true);
    });

    it('should mark expand button as active when clicked', () => {
      toast.show('Test', { duration: 5000, description: 'Details...' });

      const expandBtn = container.querySelector('.toast-header-expand');
      expandBtn.click();

      expect(expandBtn.classList.contains('toast-header-expand-active')).toBe(true);
    });

    it('should rotate chevron icon when expanded', () => {
      toast.show('Test', { duration: 5000, description: 'Details...' });

      const expandBtn = container.querySelector('.toast-header-expand');
      const icon = expandBtn.querySelector('i');

      // Initially pointing down
      expect(icon.style.transform).not.toContain('rotate(180deg)');

      expandBtn.click();

      // Icon should have rotated
      expect(expandBtn.classList.contains('toast-header-expand-active')).toBe(true);
    });
  });

  describe('keep()', () => {
    it('should handle invalid ID gracefully', () => {
      expect(() => toast.keep('nonexistent')).not.toThrow();
    });

    it('should hide progress bar when keep is called', () => {
      const id = toast.show('Test', { duration: 5000, description: 'Details...' });

      toast.keep(id);

      const progress = container.querySelector('.toast-progress');
      expect(progress.classList.contains('toast-progress-kept')).toBe(true);
    });

    it('should mark expand button as active when keep is called', () => {
      const id = toast.show('Test', { duration: 5000, description: 'Details...' });

      toast.keep(id);

      const expandBtn = container.querySelector('.toast-header-expand');
      expect(expandBtn.classList.contains('toast-header-expand-active')).toBe(true);
    });
  });

  describe('Details Section', () => {
    it('should create collapsible details section', () => {
      toast.show('Test', { details: 'Hidden details' });

      const toggle = container.querySelector('.toast-details-toggle');
      const details = container.querySelector('.toast-details');

      expect(toggle).not.toBeNull();
      expect(details).not.toBeNull();
      // Details should not be visible by default
      expect(details.classList.contains('toast-details-visible')).toBe(false);
    });

    it('should toggle details on button click', () => {
      toast.show('Test', { details: 'Details content' });

      const toggle = container.querySelector('.toast-details-toggle');
      const details = container.querySelector('.toast-details');

      toggle.click();

      expect(details.classList.contains('toast-details-visible')).toBe(true);

      toggle.click();

      expect(details.classList.contains('toast-details-visible')).toBe(false);
    });

    it('should show details expanded by default when showDetails is true', () => {
      toast.show('Test', { details: 'Details', showDetails: true });

      const details = container.querySelector('.toast-details-visible');
      expect(details).not.toBeNull();
    });

    it('should not render details section when no details provided', () => {
      toast.show('Test');

      const toggle = container.querySelector('.toast-details-toggle');
      expect(toggle).toBeNull();
    });
  });

  describe('Title and Message', () => {
    it('should use message as title by default', () => {
      toast.show('My message');

      const titleEl = container.querySelector('.toast-header-title');
      expect(titleEl.textContent).toBe('My message');
    });

    it('should use custom title when provided', () => {
      toast.show('Actual message', { title: 'Custom Title' });

      const titleEl = container.querySelector('.toast-header-title');
      expect(titleEl.textContent).toBe('Custom Title');
    });
  });

  describe('HTML Structure', () => {
    it('should have unified header button group', () => {
      toast.show('Test');

      const headerGroup = container.querySelector('.toast-header-group');
      expect(headerGroup).not.toBeNull();
      expect(headerGroup.querySelector('.toast-header-icon')).not.toBeNull();
      expect(headerGroup.querySelector('.toast-header-title')).not.toBeNull();
    });

    it('should have close button in header group', () => {
      toast.show('Test');

      const closeBtn = container.querySelector('.toast-header-close');
      expect(closeBtn).not.toBeNull();
    });

    it('should render full structure with all options', () => {
      toast.show('Full toast', {
        type: 'error',
        subsystem: 'Conduit',
        description: 'A detailed description',
        details: 'Query Ref: 27\nDatabase: Lithium',
        duration: 5000,
      });

      const toastEl = container.querySelector('.toast-error');
      expect(toastEl).not.toBeNull();
      expect(toastEl.querySelector('.toast-header-subsystem').textContent).toBe('Conduit');
      expect(toastEl.querySelector('.toast-header-title').textContent).toBe('Full toast');
      expect(toastEl.querySelector('.toast-description').textContent).toBe('A detailed description');
      expect(toastEl.querySelector('.toast-details')).not.toBeNull();
      expect(toastEl.querySelector('.toast-progress')).not.toBeNull();
      expect(toastEl.querySelector('.toast-header-expand')).not.toBeNull();
      expect(toastEl.querySelector('.toast-header-close')).not.toBeNull();
    });

    it('should apply type-specific colors to header group', () => {
      toast.show('Error', { type: 'error', subsystem: 'Test' });
      const errorToast = container.querySelector('.toast-error');
      const errorGroup = errorToast.querySelector('.toast-header-group');
      expect(errorGroup).not.toBeNull();

      toast.show('Success', { type: 'success', subsystem: 'Test' });
      const successToast = container.querySelector('.toast-success');
      const successGroup = successToast.querySelector('.toast-header-group');
      expect(successGroup).not.toBeNull();

      toast.show('Warning', { type: 'warning', subsystem: 'Test' });
      const warningToast = container.querySelector('.toast-warning');
      const warningGroup = warningToast.querySelector('.toast-header-group');
      expect(warningGroup).not.toBeNull();

      toast.show('Info', { type: 'info', subsystem: 'Test' });
      const infoToast = container.querySelector('.toast-info');
      const infoGroup = infoToast.querySelector('.toast-header-group');
      expect(infoGroup).not.toBeNull();
    });
  });
});
