/**
 * Transitions Utility Library
 * 
 * Shared transition functions for fade effects across the application.
 * All transitions use CSS classes and return Promises for easy async/await usage.
 */

/**
 * CSS variable name for transition duration
 */
const TRANSITION_DURATION_VAR = '--transition-duration';

/**
 * Get the current transition duration from CSS variables
 * Reads the --transition-duration CSS custom property
 * @returns {number} Duration in milliseconds
 */
export function getTransitionDuration() {
  if (typeof window === 'undefined') {
    return 500; // Default fallback for SSR/test environments
  }
  
  const root = document.documentElement;
  const computedStyle = getComputedStyle(root);
  const durationValue = computedStyle.getPropertyValue(TRANSITION_DURATION_VAR).trim();
  
  if (!durationValue) {
    return 500; // Default fallback
  }
  
  // Parse CSS duration value (e.g., "500ms" or "0.5s")
  if (durationValue.endsWith('ms')) {
    return parseInt(durationValue, 10);
  } else if (durationValue.endsWith('s')) {
    return parseFloat(durationValue) * 1000;
  }
  
  return 500; // Default fallback
}

/**
 * Default transition durations (in milliseconds)
 * These are calculated from the CSS --transition-duration variable
 */
export function getTransitionDurations() {
  const base = getTransitionDuration();
  return {
    fast: Math.round(base * 0.3),
    normal: base,
    slow: base * 2,
  };
}

/**
 * Legacy constant for backward compatibility
 * @deprecated Use getTransitionDuration() or getTransitionDurations() instead
 */
export const TRANSITION_DURATION = {
  get fast() { return getTransitionDurations().fast; },
  get normal() { return getTransitionDurations().normal; },
  get slow() { return getTransitionDurations().slow; },
};

/**
 * Fade in an element
 * @param {HTMLElement} element - The element to fade in
 * @param {number} duration - Transition duration in ms (default: reads from CSS)
 * @returns {Promise<void>} Resolves when transition completes
 */
export function fadeIn(element, duration = null) {
  return new Promise((resolve) => {
    if (!element) {
      resolve();
      return;
    }

    const dur = duration ?? getTransitionDuration();

    // Ensure element starts hidden
    element.style.opacity = '0';
    element.style.display = '';
    
    // Force reflow
    void element.offsetHeight;
    
    // Set transition duration to match CSS or override
    element.style.transition = `opacity ${dur}ms ease-in-out`;
    
    // Start transition
    requestAnimationFrame(() => {
      element.style.opacity = '1';
    });
    
    // Resolve after transition completes
    setTimeout(() => {
      element.style.transition = '';
      resolve();
    }, dur);
  });
}

/**
 * Fade out an element
 * @param {HTMLElement} element - The element to fade out
 * @param {number} duration - Transition duration in ms (default: reads from CSS)
 * @returns {Promise<void>} Resolves when transition completes
 */
export function fadeOut(element, duration = null) {
  return new Promise((resolve) => {
    if (!element) {
      resolve();
      return;
    }

    const dur = duration ?? getTransitionDuration();

    // Set transition duration
    element.style.transition = `opacity ${dur}ms ease-in-out`;
    
    // Start transition
    requestAnimationFrame(() => {
      element.style.opacity = '0';
    });
    
    // Resolve after transition completes and hide element
    setTimeout(() => {
      element.style.display = 'none';
      element.style.transition = '';
      resolve();
    }, dur);
  });
}

/**
 * Crossfade between two elements
 * @param {HTMLElement} fromElement - Element to fade out
 * @param {HTMLElement} toElement - Element to fade in
 * @param {number} duration - Transition duration in ms (default: reads from CSS)
 * @returns {Promise<void>} Resolves when transition completes
 */
export function crossfade(fromElement, toElement, duration = null) {
  return new Promise((resolve) => {
    if (!fromElement && !toElement) {
      resolve();
      return;
    }

    const dur = duration ?? getTransitionDuration();
    const promises = [];
    
    if (fromElement) {
      promises.push(fadeOut(fromElement, dur));
    }
    
    if (toElement) {
      // Delay fade in slightly for smoother crossfade
      setTimeout(() => {
        fadeIn(toElement, dur);
      }, dur / 4);
    }
    
    setTimeout(resolve, dur + 50);
  });
}

/**
 * Fade in element with display:flex
 * @param {HTMLElement} element - The element to fade in
 * @param {number} duration - Transition duration in ms (default: reads from CSS)
 * @returns {Promise<void>} Resolves when transition completes
 */
export function fadeInFlex(element, duration = null) {
  return new Promise((resolve) => {
    if (!element) {
      resolve();
      return;
    }

    const dur = duration ?? getTransitionDuration();

    element.style.opacity = '0';
    element.style.display = 'flex';
    
    void element.offsetHeight;
    
    element.style.transition = `opacity ${dur}ms ease-in-out`;
    
    requestAnimationFrame(() => {
      element.style.opacity = '1';
    });
    
    setTimeout(() => {
      element.style.transition = '';
      resolve();
    }, dur);
  });
}

/**
 * Fade in element with display:block
 * @param {HTMLElement} element - The element to fade in
 * @param {number} duration - Transition duration in ms (default: reads from CSS)
 * @returns {Promise<void>} Resolves when transition completes
 */
export function fadeInBlock(element, duration = null) {
  return new Promise((resolve) => {
    if (!element) {
      resolve();
      return;
    }

    const dur = duration ?? getTransitionDuration();

    element.style.opacity = '0';
    element.style.display = 'block';
    
    void element.offsetHeight;
    
    element.style.transition = `opacity ${dur}ms ease-in-out`;
    
    requestAnimationFrame(() => {
      element.style.opacity = '1';
    });
    
    setTimeout(() => {
      element.style.transition = '';
      resolve();
    }, dur);
  });
}

/**
 * Swap visibility between two elements with fade
 * @param {HTMLElement} hideElement - Element to hide
 * @param {HTMLElement} showElement - Element to show
 * @param {number} duration - Transition duration in ms (default: reads from CSS)
 * @returns {Promise<void>} Resolves when transition completes
 */
export function swapPanels(hideElement, showElement, duration = null) {
  const dur = duration ?? getTransitionDuration();
  return crossfade(hideElement, showElement, dur);
}

/**
 * Wait for a transition to complete
 * Utility function for timing events with CSS transitions
 * @param {number} duration - Duration to wait in ms (default: reads from CSS)
 * @returns {Promise<void>}
 */
export function waitForTransition(duration = null) {
  const dur = duration ?? getTransitionDuration();
  return new Promise(resolve => setTimeout(resolve, dur));
}

/**
 * Wait for half the transition duration
 * Useful for staggered animations
 * @param {number} duration - Duration to wait in ms (default: reads from CSS)
 * @returns {Promise<void>}
 */
export function waitForHalfTransition(duration = null) {
  const dur = duration ?? getTransitionDuration();
  return new Promise(resolve => setTimeout(resolve, dur / 2));
}
