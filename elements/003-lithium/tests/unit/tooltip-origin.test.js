import { afterEach, describe, expect, it, vi } from 'vitest';
import { computePosition } from '@floating-ui/dom';
import { Tooltip, TOOLTIP_ORIGIN_LIMIT, tooltipPositionUsable } from '../../src/core/tooltip.js';
import { destroyAllTooltips, getTip, initTooltips } from '../../src/core/tooltip-api.js';

vi.mock('@floating-ui/dom', () => ({
  computePosition: vi.fn(),
  flip: () => ({}),
  shift: () => ({}),
  offset: () => ({}),
  arrow: () => ({}),
  autoUpdate: () => () => {},
}));

function place(el, box) {
  el.getBoundingClientRect = () => ({
    x: box.x,
    y: box.y,
    left: box.x,
    top: box.y,
    width: box.width,
    height: box.height,
    right: box.x + box.width,
    bottom: box.y + box.height,
    toJSON() { return this; },
  });
}

function mount(label) {
  const trigger = document.createElement('div');
  trigger.dataset.label = label;
  document.body.append(trigger);
  place(trigger, { x: 240, y: 180, width: 80, height: 8 });
  return trigger;
}

describe('tooltip origin', () => {
  afterEach(() => {
    destroyAllTooltips();
    document.body.replaceChildren();
    vi.clearAllMocks();
  });

  it('accepts a point that is away from either window edge', () => {
    expect(tooltipPositionUsable(TOOLTIP_ORIGIN_LIMIT, TOOLTIP_ORIGIN_LIMIT)).toBe(false);
    expect(tooltipPositionUsable(0, 0)).toBe(false);
    expect(tooltipPositionUsable(9, TOOLTIP_ORIGIN_LIMIT)).toBe(true);
    expect(tooltipPositionUsable(TOOLTIP_ORIGIN_LIMIT, 40)).toBe(true);
    expect(tooltipPositionUsable(Number.NaN, 40)).toBe(false);
  });

  it('stays hidden when the resolved point is the window origin', async () => {
    computePosition.mockResolvedValue({
      x: TOOLTIP_ORIGIN_LIMIT,
      y: TOOLTIP_ORIGIN_LIMIT,
      placement: 'top',
      middlewareData: {},
    });
    const trigger = mount('resize');
    const tooltip = new Tooltip(trigger, 'Resize band').init();
    tooltip.show();
    await vi.waitFor(() => {
      expect(tooltip.isVisible).toBe(false);
    });
    expect(tooltip.tooltipEl.classList.contains('li-tooltip--visible')).toBe(false);
    expect(tooltip.tooltipEl.style.left).toBe('');
    tooltip.destroy();
  });

  it('stays hidden when the anchor has no box', () => {
    const trigger = mount('resize');
    place(trigger, { x: 0, y: 0, width: 0, height: 0 });
    const tooltip = new Tooltip(trigger, 'Resize band').init();
    tooltip.show();
    expect(tooltip.isVisible).toBe(false);
    expect(computePosition).not.toHaveBeenCalled();
    expect(tooltip.tooltipEl.classList.contains('li-tooltip--visible')).toBe(false);
    tooltip.destroy();
  });

  it('shows once the point is away from the window origin', async () => {
    computePosition.mockResolvedValue({
      x: 220,
      y: 160,
      placement: 'top',
      middlewareData: {},
    });
    const trigger = mount('resize');
    const tooltip = new Tooltip(trigger, 'Resize band').init();
    tooltip.show();
    await vi.waitFor(() => {
      expect(tooltip.tooltipEl.classList.contains('li-tooltip--visible')).toBe(true);
    });
    expect(tooltip.tooltipEl.style.left).toBe('220px');
    expect(tooltip.tooltipEl.style.top).toBe('160px');
    tooltip.destroy();
  });

  it('does not open after the anchor leaves the document', async () => {
    const trigger = mount('resize');
    const tooltip = new Tooltip(trigger, 'Resize band', {
      delay: { show: 20, hide: 10 },
    }).init();
    trigger.dispatchEvent(new CustomEvent('mouseenter'));
    trigger.remove();
    await new Promise((resolve) => { setTimeout(resolve, 40); });
    expect(tooltip.isVisible).toBe(false);
    expect(tooltip.tooltipEl.classList.contains('li-tooltip--visible')).toBe(false);
    tooltip.show();
    expect(tooltip.isVisible).toBe(false);
    tooltip.destroy();
  });

  it('drops a tooltip when its trigger is removed', async () => {
    const host = document.createElement('div');
    const edge = document.createElement('div');
    edge.title = 'Resize band';
    host.append(edge);
    document.body.append(host);
    initTooltips(document);
    expect(getTip(edge)).toBeTruthy();
    host.remove();
    await vi.waitFor(() => {
      expect(getTip(edge)).toBeNull();
    });
  });
});
