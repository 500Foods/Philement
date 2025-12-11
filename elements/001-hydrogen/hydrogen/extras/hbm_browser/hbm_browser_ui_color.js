/**
 * Hydrogen Build Metrics Browser - UI Color Picker Functions
 * Color picker implementation and interactions
 *
 * @version 1.0.0
 * @license MIT
 */

// Extend the global namespace
var HMB = HMB || {};

// Initialize and manage the color picker
HMB.initColorPicker = function() {
  // console.log('Initializing color picker...');

  // Check if already initialized to prevent duplicate event listeners
  if (this.state.colorPicker && this.state.colorPicker.initialized) {
    // console.log('Color picker already initialized, skipping');
    return;
  }

  // Initialize color picker elements
  this.state.colorPicker = {
    isActive: false,
    initialized: true,
    currentColor: '#87CEEB', // Default sky blue
    hue: 200, // Blue hue
    saturation: 0.6,
    value: 0.9
  };

  // Set up color picker close button
  const closeColorPickerBtn = document.getElementById('close-color-picker');
  if (closeColorPickerBtn) {
    // console.log('Setting up close button event listener');
    closeColorPickerBtn.addEventListener('click', () => {
      // console.log('Close button clicked');
      this.hideColorPicker();
    });
  } else {
    console.warn('Close color picker button not found');
  }

  // Set up color picker hex input
  const hexInput = document.getElementById('color-picker-hex');
  if (hexInput) {
    // console.log('Setting up HEX input event listener');
    hexInput.addEventListener('input', (e) => {
      // console.log('HEX input changed:', e.target.value);
      this.updateColorFromHex(e.target.value);
    });
  }

  // Initialize color swatch click handler
  const colorSwatch = document.getElementById('color-preview');
  if (colorSwatch) {
    // Check if event listener already exists
    if (!colorSwatch.dataset.colorPickerInitialized) {
      // console.log('Setting up color swatch click handler');
      colorSwatch.addEventListener('click', () => {
        // console.log('Color swatch clicked - toggling color picker');
        this.toggleColorPicker();
      });
      colorSwatch.dataset.colorPickerInitialized = 'true';
    } else {
      // console.log('Color swatch event listener already set up');
    }
  } else {
    console.warn('Color preview element not found');
  }

  // Initialize color input change handler
  const colorInput = document.getElementById('metric-color');
  if (colorInput) {
    // console.log('Setting up color input change handler');
    colorInput.addEventListener('input', (e) => {
      // console.log('Color input changed:', e.target.value);
      const preview = document.getElementById('color-preview');
      if (preview) {
        preview.style.backgroundColor = e.target.value || 'transparent';
      }
    });
  } else {
    console.warn('Metric color input not found');
  }

  // Initialize color picker interaction
  this.initColorPickerInteractions();
  // console.log('Color picker initialization complete');
};

// Toggle color picker visibility
HMB.toggleColorPicker = function() {
  // console.log('toggleColorPicker called');
  const colorPicker = document.getElementById('vanilla-color-picker');
  if (colorPicker) {
    // console.log('Color picker element found, current display:', colorPicker.style.display);
    if (colorPicker.style.display === 'block') {
      // console.log('Hiding color picker');
      this.hideColorPicker();
    } else {
      // console.log('Showing color picker');
      this.showColorPicker();
    }
  } else {
    console.warn('Color picker element not found');
  }
};

// Show color picker
HMB.showColorPicker = function() {
  // console.log('showColorPicker called');
  const colorPicker = document.getElementById('vanilla-color-picker');
  if (colorPicker) {
    // console.log('Showing color picker element');

    // Center the color picker in the window
    colorPicker.style.position = 'fixed';
    colorPicker.style.left = '50%';
    colorPicker.style.top = '50%';
    colorPicker.style.transform = 'translate(-50%, -50%)';
    colorPicker.style.zIndex = '10000';

    colorPicker.style.display = 'block';
    this.state.colorPicker.isActive = true;

    // Initialize color picker with current color
    const currentColor = document.getElementById('metric-color').value || '#87CEEB';
    // console.log('Initializing color picker with color:', currentColor);
    this.updateColorPicker(currentColor);
  } else {
    console.warn('Color picker element not found in showColorPicker');
  }
};

// Hide color picker
HMB.hideColorPicker = function() {
  // console.log('hideColorPicker called');
  const colorPicker = document.getElementById('vanilla-color-picker');
  if (colorPicker) {
    // console.log('Hiding color picker element');
    colorPicker.style.display = 'none';
    this.state.colorPicker.isActive = false;
  } else {
    console.warn('Color picker element not found in hideColorPicker');
  }
};

// Update color picker UI with current color
HMB.updateColorPicker = function(colorHex) {
  // Validate color format
  if (!/^#([0-9A-F]{3}){1,2}$/i.test(colorHex)) {
    colorHex = '#87CEEB'; // Default to sky blue
  }

  // Update state
  this.state.colorPicker.currentColor = colorHex;

  // Update hex input
  const hexInput = document.getElementById('color-picker-hex');
  if (hexInput) {
    hexInput.value = colorHex;
  }

  // Update color picker preview
  const preview = document.getElementById('color-picker-preview');
  if (preview) {
    preview.style.backgroundColor = colorHex;
  }

  // Convert hex to HSV for saturation/hue pickers
  const hsv = this.hexToHsv(colorHex);
  if (hsv) {
    this.state.colorPicker.hue = hsv.h;
    this.state.colorPicker.saturation = hsv.s;
    this.state.colorPicker.value = hsv.v;

    // Update saturation picker to reflect current hue using canvas
    const saturationPicker = document.getElementById('color-picker-saturation');
    if (saturationPicker) {
      // Create canvas if it doesn't exist
      let canvas = saturationPicker.querySelector('canvas');
      if (!canvas) {
        canvas = document.createElement('canvas');
        canvas.width = saturationPicker.offsetWidth;
        canvas.height = saturationPicker.offsetHeight;
        canvas.style.position = 'absolute';
        canvas.style.top = '0';
        canvas.style.left = '0';
        saturationPicker.appendChild(canvas);
      }

      // Draw proper HSV color space on canvas with current hue
      this.drawHSVSaturationPicker(canvas, hsv.h);
    }

    // Update hue picker
    this.updateHuePicker(hsv.h);

    // Update saturation/value cursor position
    this.updateSaturationCursor(hsv.s, hsv.v);
  }
};

// Draw HSV saturation picker on canvas
HMB.drawHSVSaturationPicker = function(canvas, hue) {
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const width = canvas.width;
  const height = canvas.height;

  // Create proper HSV color space by drawing each pixel individually
  // This ensures the top-right corner shows the pure hue
  const imageData = ctx.createImageData(width, height);
  const data = imageData.data;

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      // Calculate saturation (left to right: 0.0 to 1.0)
      const saturation = x / (width - 1);

      // Calculate value (top to bottom: 1.0 to 0.0)
      const value = 1 - (y / (height - 1));

      // Convert HSV to RGB for this pixel
      const rgb = this.hsvToRgb(hue, saturation, value);

      // Set pixel color
      const index = (y * width + x) * 4;
      data[index] = rgb.r;     // R
      data[index + 1] = rgb.g; // G
      data[index + 2] = rgb.b; // B
      data[index + 3] = 255;   // A (fully opaque)
    }
  }

  // Draw the image data to canvas
  ctx.putImageData(imageData, 0, 0);
};

// Helper function to convert HSV to RGB
HMB.hsvToRgb = function(h, s, v) {
  h = Math.min(360, Math.max(0, h));
  s = Math.min(1, Math.max(0, s));
  v = Math.min(1, Math.max(0, v));

  const c = v * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = v - c;

  let r, g, b;
  if (h >= 0 && h < 60) {
    [r, g, b] = [c, x, 0];
  } else if (h >= 60 && h < 120) {
    [r, g, b] = [x, c, 0];
  } else if (h >= 120 && h < 180) {
    [r, g, b] = [0, c, x];
  } else if (h >= 180 && h < 240) {
    [r, g, b] = [0, x, c];
  } else if (h >= 240 && h < 300) {
    [r, g, b] = [x, 0, c];
  } else {
    [r, g, b] = [c, 0, x];
  }

  return {
    r: Math.round((r + m) * 255),
    g: Math.round((g + m) * 255),
    b: Math.round((b + m) * 255)
  };
};

// Initialize color picker interactions
HMB.initColorPickerInteractions = function() {
  // Saturation/Value picker interaction
  const saturationPicker = document.getElementById('color-picker-saturation');
  if (saturationPicker) {
    saturationPicker.addEventListener('click', (e) => {
      this.handleSaturationClick(e);
    });
  }

  // Hue picker interaction
  const huePicker = document.getElementById('color-picker-hue');
  if (huePicker) {
    huePicker.addEventListener('click', (e) => {
      this.handleHueClick(e);
    });
  }
};

// Handle saturation/value picker click
HMB.handleSaturationClick = function(e) {
  const saturationPicker = document.getElementById('color-picker-saturation');
  if (!saturationPicker) return;

  const rect = saturationPicker.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;

  // Calculate saturation and value from position
  // X-axis: Saturation (left to right: 0.0 to 1.0)
  // Y-axis: Value/Brightness - CSS gradient goes from black (bottom) to transparent (top)
  // So bottom = black = low value, top = bright = high value
  // But cursor positioning uses (1 - value), so we need to match that
  const saturation = Math.min(1, Math.max(0, x / rect.width));
  const value = 1 - Math.min(1, Math.max(0, y / rect.height)); // Invert to match cursor positioning

  // Update state
  this.state.colorPicker.saturation = saturation;
  this.state.colorPicker.value = value;

  // Update cursor position
  this.updateSaturationCursor(saturation, value);

  // Convert to hex and update everything
  const hex = this.hsvToHex(
    this.state.colorPicker.hue,
    saturation,
    value
  );
  this.updateColorFromHex(hex);
};

// Handle hue picker click
HMB.handleHueClick = function(e) {
  const huePicker = document.getElementById('color-picker-hue');
  if (!huePicker) return;

  const rect = huePicker.getBoundingClientRect();
  const x = e.clientX - rect.left;

  // Calculate hue from position (0-360 degrees)
  // The hue picker is horizontal, so we use X position instead of Y
  const hue = 360 * Math.min(1, Math.max(0, x / rect.width));

  // Update state
  this.state.colorPicker.hue = hue;

  // Update hue cursor position
  this.updateHueCursor(hue);

  // Convert to hex and update everything
  const hex = this.hsvToHex(
    hue,
    this.state.colorPicker.saturation,
    this.state.colorPicker.value
  );
  this.updateColorFromHex(hex);
};

// Update saturation cursor position
HMB.updateSaturationCursor = function(saturation, value) {
  const cursor = document.getElementById('color-picker-cursor');
  if (cursor) {
    const saturationPicker = document.getElementById('color-picker-saturation');
    if (saturationPicker) {
      const x = saturation * saturationPicker.offsetWidth;
      const y = (1 - value) * saturationPicker.offsetHeight;
      cursor.style.left = `${x}px`;
      cursor.style.top = `${y}px`;
    }
  }
};

// Update hue cursor position
HMB.updateHueCursor = function(hue) {
  const cursor = document.getElementById('color-picker-hue-cursor');
  if (cursor) {
    const huePicker = document.getElementById('color-picker-hue');
    if (huePicker) {
      // The hue picker is a horizontal bar, so we position along the X-axis
      // hue ranges from 0-360, we map this to the width of the hue picker
      const x = (hue / 360) * huePicker.offsetWidth;
      cursor.style.left = `${x}px`;
      cursor.style.top = '50%'; // Center vertically
      cursor.style.transform = 'translateY(-50%)'; // Perfect vertical centering
    }
  }
};

// Update hue picker gradient
HMB.updateHuePicker = function(hue) {
  // Hue picker shows the current hue selection
  const huePicker = document.getElementById('color-picker-hue');
  if (huePicker) {
    // This could be enhanced with a proper hue gradient
  }
};

// Update color from hex input
HMB.updateColorFromHex = function(hex) {
  // Validate hex format
  if (!/^#([0-9A-F]{3}){1,2}$/i.test(hex)) {
    return;
  }

  // Update main color swatch
  const colorSwatch = document.getElementById('color-preview');
  if (colorSwatch) {
    colorSwatch.style.backgroundColor = hex;
  }

  // Update main color input field
  const colorInput = document.getElementById('metric-color');
  if (colorInput) {
    colorInput.value = hex;
  }

  // Update color picker state and UI
  this.updateColorPicker(hex);
};

// Convert HEX to HSV
HMB.hexToHsv = function(hex) {
  // Remove # if present
  hex = hex.replace('#', '');

  // Parse r, g, b values
  let r, g, b;
  if (hex.length === 3) {
    r = parseInt(hex[0] + hex[0], 16) / 255;
    g = parseInt(hex[1] + hex[1], 16) / 255;
    b = parseInt(hex[2] + hex[2], 16) / 255;
  } else if (hex.length === 6) {
    r = parseInt(hex.substring(0, 2), 16) / 255;
    g = parseInt(hex.substring(2, 4), 16) / 255;
    b = parseInt(hex.substring(4, 6), 16) / 255;
  } else {
    return null;
  }

  // Find min, max, delta
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const delta = max - min;

  // Calculate hue
  let h = 0;
  if (delta !== 0) {
    if (max === r) {
      h = ((g - b) / delta) % 6;
    } else if (max === g) {
      h = (b - r) / delta + 2;
    } else {
      h = (r - g) / delta + 4;
    }
  }
  h = Math.round(h * 60);
  if (h < 0) h += 360;

  // Calculate saturation
  const s = max === 0 ? 0 : delta / max;

  // Calculate value
  const v = max;

  return { h, s, v };
};

// Convert HSV to HEX
HMB.hsvToHex = function(h, s, v) {
  h = Math.min(360, Math.max(0, h));
  s = Math.min(1, Math.max(0, s));
  v = Math.min(1, Math.max(0, v));

  const c = v * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = v - c;

  let r, g, b;
  if (h >= 0 && h < 60) {
    [r, g, b] = [c, x, 0];
  } else if (h >= 60 && h < 120) {
    [r, g, b] = [x, c, 0];
  } else if (h >= 120 && h < 180) {
    [r, g, b] = [0, c, x];
  } else if (h >= 180 && h < 240) {
    [r, g, b] = [0, x, c];
  } else if (h >= 240 && h < 300) {
    [r, g, b] = [x, 0, c];
  } else {
    [r, g, b] = [c, 0, x];
  }

  r = Math.round((r + m) * 255);
  g = Math.round((g + m) * 255);
  b = Math.round((b + m) * 255);

  return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`.toUpperCase();
};