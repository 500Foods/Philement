/**
 * Transitions Utility Unit Tests
 * 
 * Tests cover: getTransitionDuration, getTransitionDurations, TRANSITION_DURATION,
 * fadeIn, fadeOut, crossfade, fadeInFlex, fadeInBlock, swapPanels,
 * waitForTransition, waitForHalfTransition
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import {
  getTransitionDuration,
  getTransitionDurations,
  TRANSITION_DURATION,
  fadeIn,
  fadeOut,
  crossfade,
  fadeInFlex,
  fadeInBlock,
  swapPanels,
  waitForTransition,
  waitForHalfTransition,
} from '../../src/core/transitions.js';

// ── Helpers ──────────────────────────────────────────────────────────────────

/**
 * Create a minimal mock HTMLElement with enough surface area for the transitions code.
 * JSDOM's requestAnimationFrame calls callbacks synchronously in the test environment,
 * so we don't need real async for the style-mutation side effects.
 */
function makeEl(overrides = {}) {
  const el = {
    style: {},
    offsetHeight: 0,   // read to force reflow (no-op in tests)
    classList: { add: vi.fn(), remove: vi.fn(), contains: vi.fn(() => false) },
    ...overrides,
  };
  return el;
}

// ── getTransitionDuration ─────────────────────────────────────────────────────

describe('getTransitionDuration()', () => {
  it('returns default 500 when window is undefined', () => {
    // The module's window check path: JSDOM has window, so we test the real path
    // via a mocked getPropertyValue returning empty string
    const origGetComputedStyle = global.getComputedStyle;
    global.getComputedStyle = () => ({ getPropertyValue: () => '' });
    expect(getTransitionDuration()).toBe(500);
    global.getComputedStyle = origGetComputedStyle;
  });

  it('parses a millisecond CSS value correctly', () => {
    const origGetComputedStyle = global.getComputedStyle;
    global.getComputedStyle = () => ({ getPropertyValue: () => '300ms' });
    expect(getTransitionDuration()).toBe(300);
    global.getComputedStyle = origGetComputedStyle;
  });

  it('parses a second CSS value correctly', () => {
    const origGetComputedStyle = global.getComputedStyle;
    global.getComputedStyle = () => ({ getPropertyValue: () => '0.4s' });
    expect(getTransitionDuration()).toBe(400);
    global.getComputedStyle = origGetComputedStyle;
  });

  it('returns 500 for unrecognised CSS value format', () => {
    const origGetComputedStyle = global.getComputedStyle;
    global.getComputedStyle = () => ({ getPropertyValue: () => 'banana' });
    expect(getTransitionDuration()).toBe(500);
    global.getComputedStyle = origGetComputedStyle;
  });
});

// ── getTransitionDurations ────────────────────────────────────────────────────

describe('getTransitionDurations()', () => {
  beforeEach(() => {
    // Pin base to 500ms for predictable arithmetic
    global.getComputedStyle = () => ({ getPropertyValue: () => '500ms' });
  });
  afterEach(() => {
    delete global.getComputedStyle;
  });

  it('returns fast, normal, slow keys', () => {
    const durations = getTransitionDurations();
    expect(durations).toHaveProperty('fast');
    expect(durations).toHaveProperty('normal');
    expect(durations).toHaveProperty('slow');
  });

  it('normal equals base duration', () => {
    expect(getTransitionDurations().normal).toBe(500);
  });

  it('fast is 30% of base (rounded)', () => {
    expect(getTransitionDurations().fast).toBe(150);
  });

  it('slow is 2× base', () => {
    expect(getTransitionDurations().slow).toBe(1000);
  });
});

// ── TRANSITION_DURATION legacy object ─────────────────────────────────────────

describe('TRANSITION_DURATION (legacy)', () => {
  beforeEach(() => {
    global.getComputedStyle = () => ({ getPropertyValue: () => '500ms' });
  });
  afterEach(() => {
    delete global.getComputedStyle;
  });

  it('exposes fast getter', () => {
    expect(typeof TRANSITION_DURATION.fast).toBe('number');
  });

  it('exposes normal getter', () => {
    expect(typeof TRANSITION_DURATION.normal).toBe('number');
  });

  it('exposes slow getter', () => {
    expect(typeof TRANSITION_DURATION.slow).toBe('number');
  });
});

// ── waitForTransition ─────────────────────────────────────────────────────────

describe('waitForTransition()', () => {
  it('resolves after provided duration', async () => {
    const start = Date.now();
    await waitForTransition(50);
    expect(Date.now() - start).toBeGreaterThanOrEqual(40); // allow 10ms tolerance
  });

  it('uses getTransitionDuration() when no duration supplied', async () => {
    global.getComputedStyle = () => ({ getPropertyValue: () => '50ms' });
    const start = Date.now();
    await waitForTransition();
    expect(Date.now() - start).toBeGreaterThanOrEqual(40);
    delete global.getComputedStyle;
  });
});

// ── waitForHalfTransition ─────────────────────────────────────────────────────

describe('waitForHalfTransition()', () => {
  it('resolves after half the provided duration', async () => {
    const start = Date.now();
    await waitForHalfTransition(100);
    const elapsed = Date.now() - start;
    expect(elapsed).toBeGreaterThanOrEqual(40);
    expect(elapsed).toBeLessThan(120);
  });
});

// ── fadeIn ────────────────────────────────────────────────────────────────────

describe('fadeIn()', () => {
  it('resolves immediately when element is null/undefined', async () => {
    await expect(fadeIn(null, 0)).resolves.toBeUndefined();
    await expect(fadeIn(undefined, 0)).resolves.toBeUndefined();
  });

  it('sets opacity to 1 after transition', async () => {
    const el = makeEl();
    await fadeIn(el, 10);
    expect(el.style.opacity).toBe('1');
  });

  it('clears transition style after fade completes', async () => {
    const el = makeEl();
    await fadeIn(el, 10);
    expect(el.style.transition).toBe('');
  });
});

// ── fadeOut ───────────────────────────────────────────────────────────────────

describe('fadeOut()', () => {
  it('resolves immediately when element is null', async () => {
    await expect(fadeOut(null, 0)).resolves.toBeUndefined();
  });

  it('sets display to none after fade', async () => {
    const el = makeEl();
    await fadeOut(el, 10);
    expect(el.style.display).toBe('none');
  });

  it('clears transition style after fade completes', async () => {
    const el = makeEl();
    await fadeOut(el, 10);
    expect(el.style.transition).toBe('');
  });
});

// ── fadeInFlex ────────────────────────────────────────────────────────────────

describe('fadeInFlex()', () => {
  it('resolves immediately when element is null', async () => {
    await expect(fadeInFlex(null, 0)).resolves.toBeUndefined();
  });

  it('sets display to flex before fade', async () => {
    const el = makeEl();
    const promise = fadeInFlex(el, 10);
    expect(el.style.display).toBe('flex');
    await promise;
  });

  it('clears transition style after fade completes', async () => {
    const el = makeEl();
    await fadeInFlex(el, 10);
    expect(el.style.transition).toBe('');
  });
});

// ── fadeInBlock ───────────────────────────────────────────────────────────────

describe('fadeInBlock()', () => {
  it('resolves immediately when element is null', async () => {
    await expect(fadeInBlock(null, 0)).resolves.toBeUndefined();
  });

  it('sets display to block before fade', async () => {
    const el = makeEl();
    const promise = fadeInBlock(el, 10);
    expect(el.style.display).toBe('block');
    await promise;
  });

  it('clears transition style after fade completes', async () => {
    const el = makeEl();
    await fadeInBlock(el, 10);
    expect(el.style.transition).toBe('');
  });
});

// ── crossfade ─────────────────────────────────────────────────────────────────

describe('crossfade()', () => {
  it('resolves when both elements are null', async () => {
    await expect(crossfade(null, null, 0)).resolves.toBeUndefined();
  });

  it('resolves with only a fromElement', async () => {
    const from = makeEl();
    await expect(crossfade(from, null, 10)).resolves.toBeUndefined();
  });

  it('resolves with only a toElement', async () => {
    const to = makeEl();
    await expect(crossfade(null, to, 10)).resolves.toBeUndefined();
  });

  it('resolves with both elements provided', async () => {
    const from = makeEl();
    const to = makeEl();
    await expect(crossfade(from, to, 10)).resolves.toBeUndefined();
  });
});

// ── swapPanels ────────────────────────────────────────────────────────────────

describe('swapPanels()', () => {
  it('resolves when both elements are provided', async () => {
    const hide = makeEl();
    const show = makeEl();
    await expect(swapPanels(hide, show, 10)).resolves.toBeUndefined();
  });

  it('resolves when both elements are null', async () => {
    await expect(swapPanels(null, null, 0)).resolves.toBeUndefined();
  });
});
