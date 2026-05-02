/**
 * Profile Manager - Photo Page
 *
 * Handles user photo upload, capture, editing, and saving as 200x200 base64 PNG
 * Features: custom sliders with icon thumbs, drag-to-reposition, FloatingUI tooltips
 */

import { BaseSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';
import { Tooltip } from '../../../../core/tooltip.js';

export class PhotoPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -12 });

    // Editor state
    this.photoData = null;
    this.originalPhotoData = null;
    this.originalWidth = null;
    this.originalHeight = null;
    this.initialScale = 1;  // Scale calculated to fit preview when image is first loaded
    this.xPct = 0;  // Horizontal offset as % of displayed bbox width
    this.yPct = 0;  // Vertical offset as % of displayed bbox height
    this.scale = 1;
    this.rotation = 0;
    this.originalFile = null;

    // DOM elements
    this.elements = {};
    this._customSliders = {};
    this._isDragging = false;
    this._dragStart = { x: 0, y: 0, startX: 0, startY: 0 };
    this._saveTimeout = null;
  }

  /**
   * Initialize the page
   */
  async onInit() {
    this._cacheElements();
    this._createCustomSliders();
    this._bindEvents();
    await this._loadExistingPhoto();
    this._syncToolbarWidth();
  }

  /**
   * Cache DOM elements
   * @private
   */
  _cacheElements() {
    const container = this.container;
    this.elements = {
      uploadInput: container.querySelector('#photo-upload'),
      captureBtn: container.querySelector('#photo-capture-btn'),
      removeBtn: container.querySelector('#photo-remove-btn'),
      resetBtn: container.querySelector('#photo-reset-btn'),
      downloadBtn: container.querySelector('.photo-download-btn'),
      status: container.querySelector('#photo-status'),
      editor: container.querySelector('#photo-editor'),
      imageContainer: container.querySelector('#photo-image-container'),
      image: container.querySelector('#photo-image'),
      dimming: container.querySelector('#photo-dimming'),
      previewOverlay: container.querySelector('#photo-preview-overlay'),
       resizeHandle: container.querySelector('#photo-resize-handle'),
       toolbar: container.querySelector('#photo-toolbar'),
       previews: container.querySelector('.photo-previews'),
       preview40: container.querySelector('#photo-preview-40'),
       preview80: container.querySelector('#photo-preview-80'),
       preview150: container.querySelector('#photo-preview-150'),
    };
  }

  /**
   * Create custom sliders with icon thumbs and FloatingUI tooltips
   * @private
   */
  _createCustomSliders() {
    const sliderConfigs = [
      { id: 'x', min: -100, max: 100, value: 0, step: 1, orientation: 'horizontal', icon: 'fa-left-right', label: 'Left/Right', positionClass: 'photo-slider-top', unit: '%' },
      { id: 'y', min: -100, max: 100, value: 0, step: 1, orientation: 'vertical', icon: 'fa-up-down', label: 'Up/Down', positionClass: 'photo-slider-left', unit: '%' },
      { id: 'scale', min: 0.0, max: 4.0, value: 1.0, step: 0.01, orientation: 'vertical', icon: 'fa-up-down-left-right', label: 'Scale', positionClass: 'photo-slider-right' },
      { id: 'rotation', min: -180, max: 180, value: 0, step: 1, orientation: 'horizontal', icon: 'fa-rotate', label: 'Rotation', positionClass: 'photo-slider-bottom' },
    ];

    sliderConfigs.forEach(config => {
      const sliderEl = this.elements.editor.querySelector(`#slider-${config.id}`);
      if (!sliderEl) return;

      const wrapper = this._createCustomSlider(config);
      sliderEl.parentNode.replaceChild(wrapper, sliderEl);
      this._customSliders[config.id] = wrapper;
    });
  }

  /**
   * Create a custom slider element with FloatingUI tooltip
   * @private
   */
  _createCustomSlider(config) {
    const wrapper = document.createElement('div');
    wrapper.className = `photo-custom-slider photo-custom-slider-${config.orientation} ${config.positionClass}`;
    wrapper.dataset.sliderId = config.id;

    const track = document.createElement('div');
    track.className = 'photo-slider-track';

    const thumb = document.createElement('div');
    thumb.className = 'photo-slider-thumb';
    thumb.innerHTML = `<fa ${config.icon}></fa>`;

    // Create FloatingUI tooltip
    const tooltip = new Tooltip(thumb, this._formatSliderValue(config.id, config.value), {
      placement: config.orientation === 'horizontal' ? 'top' : 'right',
      theme: 'default',
      trigger: 'manual',
    }).init();

    // Append track and thumb as siblings (not nested) so track opacity doesn't affect thumb
    wrapper.appendChild(track);
    wrapper.appendChild(thumb);

    // Store config and references
    wrapper._config = config;
    wrapper._thumb = thumb;
    wrapper._tooltip = tooltip;
    wrapper._value = config.value;

    // Set initial position
    this._updateSliderPosition(wrapper, config.value);

    // Add interaction handlers
    this._bindCustomSliderEvents(wrapper);

    return wrapper;
  }

  /**
   * Format slider value for tooltip display
   * @private
   */
  _formatSliderValue(id, value) {
    switch (id) {
      case 'x': return `Horizontal: ${Math.round(value)}%`;
      case 'y': return `Vertical: ${Math.round(value)}%`;
      case 'scale': return `Scale: ${Math.round(value * 100)}%`;
      case 'rotation': return `Rotate: ${value}°`;
      default: return value;
    }
  }

  /**
   * Update custom slider thumb position and tooltip
   * @private
   */
  _updateSliderPosition(sliderWrapper, value) {
    const config = sliderWrapper._config;
    const thumb = sliderWrapper._thumb;
    const tooltip = sliderWrapper._tooltip;

    let percentage;
    if (config.id === 'scale') {
      // Custom thumb positioning for scale slider
      if (value <= 1.0) {
        // 0-1.0 → 0-50% (top half of vertical slider)
        percentage = (value / 1.0) * 0.5 * 100;
      } else {
        // 1.0-4.0 → 50-100% (bottom half)
        percentage = (0.5 + (value - 1.0) / 3.0 * 0.5) * 100;
      }
    } else {
      percentage = (value - config.min) / (config.max - config.min) * 100;
    }

    if (config.orientation === 'horizontal') {
      thumb.style.left = `${percentage}%`;
    } else {
      thumb.style.top = `${percentage}%`;
    }

    sliderWrapper._value = value;
    tooltip?.updateContent(this._formatSliderValue(config.id, value));
  }

  /**
   * Bind events to custom slider
   * @private
   */
  _bindCustomSliderEvents(wrapper) {
    const config = wrapper._config;
    const tooltip = wrapper._tooltip;
    let isDragging = false;

    const updateValue = (clientX, clientY) => {
      const rect = wrapper.getBoundingClientRect();
      let percentage;

      if (config.orientation === 'horizontal') {
        percentage = (clientX - rect.left) / rect.width;
      } else {
        percentage = (clientY - rect.top) / rect.height;
      }

      percentage = Math.max(0, Math.min(1, percentage));

      let val;
      if (config.id === 'scale') {
        // Custom mapping: 100% (1.0) at 50% slider position (middle)
        if (percentage <= 0.5) {
          // Top half (0-50%): 0-100% (0-1.0), 1% increments
          val = (percentage / 0.5) * 1.0;
          const step = 0.01;
          val = Math.round(val / step) * step;
          val = Math.max(0, Math.min(1.0, val));
        } else {
          // Bottom half (50-100%): 100-400% (1.0-4.0), 3% increments
          val = 1.0 + ((percentage - 0.5) / 0.5) * 3.0;
          const step = 0.03;
          val = Math.round(val / step) * step;
          val = Math.max(1.0, Math.min(4.0, val));
        }
      } else {
        // Default linear mapping for other sliders
        val = config.min + percentage * (config.max - config.min);
        val = Math.round(val / config.step) * config.step;
      }

      this._updateSliderPosition(wrapper, val);
      this._onSliderChange(config.id, val);
    };

    wrapper.addEventListener('mousedown', (e) => {
      isDragging = true;
      updateValue(e.clientX, e.clientY);
      tooltip?.show();
      e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      updateValue(e.clientX, e.clientY);
    });

    document.addEventListener('mouseup', () => {
      if (isDragging) {
        isDragging = false;
        tooltip?.hide();
      }
    });

    // Show tooltip on hover
    wrapper.addEventListener('mouseenter', () => {
      if (!isDragging) tooltip?.show();
    });
    wrapper.addEventListener('mouseleave', () => {
      if (!isDragging) tooltip?.hide();
    });
  }

  /**
   * Handle custom slider value change
   * @private
   */
  _onSliderChange(id, value) {
    switch (id) {
      case 'x':
        this.xPct = value;
        break;
case 'y':
         this.yPct = value;
         break;
      case 'scale':
        this.scale = value;
        break;
      case 'rotation':
        this.rotation = value;
        break;
    }
    this._updateImageTransform();
  }

  /**
   * Debounced save to avoid excessive writes
   * @private
   */
  _invokeSaveDebounced() {
    if (this._saveTimeout) {
      clearTimeout(this._saveTimeout);
    }
    this._saveTimeout = setTimeout(() => {
      this._handleSave();
    }, 1000);
  }

  /**
   * Handle download button click - download original image
   * @param {Event} e - Click event
   * @private
   */
  _handleDownload(e) {
    e.preventDefault();
    if (!this.originalPhotoData) {
      // this._showStatus('No original image to download', 'error');
      return;
    }

    // Extract MIME type and extension from data URL
    const mimeMatch = this.originalPhotoData.match(/^data:(image\/\w+);base64,/);
    const mimeType = mimeMatch ? mimeMatch[1] : 'image/jpeg';
    const extension = mimeType.split('/')[1] || 'jpg';
    const filename = `Profile Photo.${extension}`;

    // Trigger download
    const a = document.createElement('a');
    a.href = this.originalPhotoData;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);

    // this._showStatus('Download started', 'success');
  }

  /**
   * Bind event listeners
   * @private
   */
  _bindEvents() {
    // Upload handler
    this.elements.uploadInput.addEventListener('change', (e) => this._handleUpload(e));

    // Capture handler
    this.elements.captureBtn.addEventListener('click', () => this._handleCapture());

    // Remove handler
    this.elements.removeBtn.addEventListener('click', () => this._handleRemove());

    // Download handler
    this.elements.downloadBtn.addEventListener('click', (e) => this._handleDownload(e));

    // Reset handler
    this.elements.resetBtn.addEventListener('click', () => this._handleReset());

    // Image drag handler
    this._setupImageDrag();

    // Scroll wheel zoom
    this._setupScrollZoom();

    // Resize handler (corner only, anchored to top-left)
    this._setupResize();
  }

  /**
   * Setup image drag to reposition
   * @private
   */
  _setupImageDrag() {
    const image = this.elements.image;
    let isDragging = false;
    let startX, startY, startXPct, startYPct;

    image.addEventListener('mousedown', (e) => {
      if (!this.photoData) return;
      isDragging = true;
      startX = e.clientX;
      startY = e.clientY;
      startXPct = this.xPct;
      startYPct = this.yPct;
      e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
      if (!isDragging) return;

      const dx = e.clientX - startX;
      const dy = e.clientY - startY;

      const editorRect = this.elements.editor.getBoundingClientRect();
      const previewSize = editorRect.width * 0.6667; // Reference size for percentages

      // Convert pixel movement to percentage of preview size (independent of scale/rotation)
      const dxPct = (dx / previewSize) * 100;
      const dyPct = (dy / previewSize) * 100;

      this.xPct = startXPct + dxPct;
      this.yPct = startYPct + dyPct;

// Update sliders
       this._updateSliderPosition(this._customSliders.x, this.xPct);
       this._updateSliderPosition(this._customSliders.y, this.yPct);

      this._updateImageTransform();
    });

    document.addEventListener('mouseup', () => {
      isDragging = false;
    });
  }

  /**
   * Handle photo upload
   * @param {Event} e - Input change event
   * @private
   */
  async _handleUpload(e) {
    const file = e.target.files[0];
    if (!file) return;

    // Validate file type
    if (!file.type.startsWith('image/')) {
      // this._showStatus('Please select an image file', 'error');
      return;
    }

    // Validate file size (10MB max)
    if (file.size > 10 * 1024 * 1024) {
      // this._showStatus('File size exceeds 10MB limit', 'error');
      return;
    }

    this.originalFile = file;
    await this._loadImageFromFile(file);
    this._updateButtonStates();
    this._resetToDefaultAndScaleImage();
    await this._handleSave();
  }

  /**
   * Handle photo capture from camera
   * @private
   */
  async _handleCapture() {
    let stream = null;
    let video = null;

    try {
      stream = await navigator.mediaDevices.getUserMedia({
        video: { facingMode: 'user', width: { ideal: 1280 }, height: { ideal: 720 } }
      });

      // Create video element
      video = document.createElement('video');
      video.setAttribute('autoplay', '');
      video.setAttribute('playsinline', '');
      video.setAttribute('muted', '');
      video.srcObject = stream;

      // Add to DOM with visibility hidden (needs to be in viewport to render)
      video.style.position = 'fixed';
      video.style.top = '0px';
      video.style.left = '0px';
      video.style.width = '1px';
      video.style.height = '1px';
      video.style.opacity = '0';
      video.style.zIndex = '-1';
      document.body.appendChild(video);

      // Wait for video to be ready with proper event handling
      await new Promise((resolve, _reject) => {
        let resolved = false;

        const onReady = () => {
          if (resolved) return;
          resolved = true;
          video.removeEventListener('canplay', onReady);
          video.removeEventListener('loadeddata', onReady);
          resolve();
        };

        // Try multiple events for compatibility
        video.addEventListener('canplay', onReady);
        video.addEventListener('loadeddata', onReady);

        // Force play
        const playPromise = video.play();
        if (playPromise != null) {
          playPromise.catch(() => {});
        }

        // Timeout fallback (5 seconds)
        setTimeout(() => {
          if (!resolved) {
            resolved = true;
            onReady();
          }
        }, 5000);
      });

      // Additional wait to ensure frames are rendered
      await new Promise(resolve => setTimeout(resolve, 500));

      // Verify video has dimensions
      if (video.videoWidth === 0 || video.videoHeight === 0) {
        throw new Error('Video has no dimensions');
      }

      // Create canvas and capture frame
      const canvas = document.createElement('canvas');
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const ctx = canvas.getContext('2d');

      // Draw the current video frame
      ctx.drawImage(video, 0, 0, canvas.width, canvas.height);

      // Convert to blob
      const blob = await new Promise((resolve) => {
        canvas.toBlob(resolve, 'image/jpeg', 0.92);
      });

      if (!blob || blob.size === 0) {
        throw new Error('Failed to capture image from video');
      }

      // Create file and load
      const file = new window.File([blob], 'capture.jpg', { type: 'image/jpeg' });
      this.originalFile = file;
      await this._loadImageFromFile(file);
      this._updateButtonStates();
      this._resetToDefaultAndScaleImage();
      await this._handleSave();

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[PhotoPage] Camera capture failed:', error.message);
      // this._showStatus('Camera capture failed. Please check permissions.', 'error');
    } finally {
      // Cleanup
      if (stream) {
        stream.getTracks().forEach(track => track.stop());
      }
      if (video) {
        video.remove();
      }
    }
  }

  /**
   * Load image from file
   * @param {File} file - Image file
   * @private
   */
  async _loadImageFromFile(file) {
    return new Promise((resolve, _reject) => {
      const reader = new window.FileReader();
      reader.onload = (e) => {
      this.photoData = e.target.result;
         this.originalPhotoData = this.photoData; // Store original
         this.elements.image.src = this.photoData;
         this.elements.image.onload = () => {
           this.originalWidth = this.elements.image.naturalWidth;
           this.originalHeight = this.elements.image.naturalHeight;
           this._updateImageTransform();
           resolve();
         };
      };
      reader.onerror = _reject;
      reader.readAsDataURL(file);
    });
  }



  /**
   * Update image transform based on slider values and editor scale
   * Uses left/top to center natural image, sliders control translate() in transform
   * @private
   */
  _updateImageTransform() {
    if (!this.photoData) return;

    const naturalW = this.originalWidth || this.elements.image.naturalWidth;
    const naturalH = this.originalHeight || this.elements.image.naturalHeight;
    if (!naturalW || !naturalH) return;

    const editorRect = this.elements.editor.getBoundingClientRect();
    const centerX = editorRect.width / 2;
    const centerY = editorRect.height / 2;

    // Set left/top to center the natural image size
    const left = centerX - naturalW / 2;
    const top = centerY - naturalH / 2;

    this.elements.image.style.left = `${left}px`;
    this.elements.image.style.top = `${top}px`;

    // Use preview overlay size as reference for translate offsets (independent of scale/rotation)
    const previewSize = editorRect.width * 0.6667; // 400px in 600px editor
    const tx = (previewSize * this.xPct) / 100;
    const ty = (previewSize * this.yPct) / 100;

    const editorScale = editorRect.width / 600;
    const s = this.scale * editorScale;

    // Transform: translate (user control) then scale/rotate
    this.elements.image.style.transform = `translate(${tx}px, ${ty}px) scale(${s}) rotate(${this.rotation}deg)`;

    // Trigger debounced auto-save
    this._invokeSaveDebounced();
  }

  /**
   * Reset sliders to default and scale image to cover preview overlay
   * @private
   */
  _resetToDefaultAndScaleImage() {
    // Reset transform values
    this.xPct = 0;
    this.yPct = 0;
    this.rotation = 0;

    const img = this.elements.image;
    const naturalW = this.originalWidth || img.naturalWidth;
    const naturalH = this.originalHeight || img.naturalHeight;
    if (!naturalW || !naturalH) return;

    // Preview overlay is 66.67% of editor width (square)
    const editorRect = this.elements.editor.getBoundingClientRect();
    const previewSize = editorRect.width * 0.6667;

    const imgAspect = naturalW / naturalH;

    // Cover preview: match height if landscape, match width if portrait
    if (imgAspect >= 1) {
      this.scale = previewSize / naturalH;
    } else {
      this.scale = previewSize / naturalW;
    }

    // Store the initial scale for reset functionality
    this.initialScale = this.scale;

// Update sliders
     this._updateSliderPosition(this._customSliders.x, this.xPct);
     this._updateSliderPosition(this._customSliders.y, this.yPct);
     this._updateSliderPosition(this._customSliders.scale, this.scale);
     this._updateSliderPosition(this._customSliders.rotation, this.rotation);

     this._updateImageTransform();
   }

  /**
   * Handle photo removal
   * @private
   */
  _handleRemove() {
    this.photoData = null;
    this.originalPhotoData = null;
    this.initialScale = 1;
    this.xPct = 0;
    this.yPct = 0;
    this.scale = 1;
    this.rotation = 0;
    this.originalFile = null;

    this.elements.image.src = '';
    this.elements.preview40.src = '';
    this.elements.preview80.src = '';
    this.elements.preview150.src = '';
    this._updateSliderPosition(this._customSliders.x, 0);
    this._updateSliderPosition(this._customSliders.y, 0);
    this._updateSliderPosition(this._customSliders.scale, 1);
    this._updateSliderPosition(this._customSliders.rotation, 0);

    this._updateImageTransform();
    this._updateButtonStates();
    // Remove the photo section from settings
    this.settings.removeSection(this.sectionKey);
  }

  /**
   * Handle save photo
   * @private
   */
  async _handleSave() {
    if (!this.photoData) {
      // this._showStatus('No photo to save', 'error');
      return;
    }

    try {
      // this._showStatus('Processing photo...', 'info');

      // Capture 200x200 PNG
      const base64 = await this._capturePhoto();

      // Save to settings with all data including original dimensions and slider positions
      const photoData = {
        _name: 'Photo',
        photo: base64,
        original: this.originalPhotoData ? this.originalPhotoData.split(',')[1] : null,
        originalWidth: this.originalWidth,
        originalHeight: this.originalHeight,
        timestamp: new Date().toISOString(),
        xPct: this.xPct,
        yPct: this.yPct,
        scale: this.scale,
        rotation: this.rotation,
      };

      // Save entire section to settings service
      this.settings.setSection(this.sectionKey, null, photoData);

      this._updateButtonStates();
      this._updatePreviews();

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[PhotoPage] Save failed:', error.message);
      // this._showStatus('Failed to save photo', 'error');
    }
  }

/**
     * Capture photo as 200x200 PNG base64
     * @returns {Promise<string>} Base64 encoded PNG (without data: prefix)
     * @private
     */
   async _capturePhoto() {
     return new Promise((resolve) => {
       const canvas = document.createElement('canvas');
       canvas.width = 200;
       canvas.height = 200;
       const ctx = canvas.getContext('2d');

       const editor = this.elements.editor;

       const editorRect = editor.getBoundingClientRect();
       const naturalW = this.originalWidth || this.elements.image.naturalWidth;
       const naturalH = this.originalHeight || this.elements.image.naturalHeight;
       if (!naturalW || !naturalH) { resolve(''); return; }

       const fullWidth = editorRect.width;
       const centerX = fullWidth / 2;
       const centerY = fullWidth / 2;

       // Use preview overlay size as reference for translate offsets (66.67% of full width)
       const previewSize = fullWidth * 0.6667;
       const tx = (previewSize * this.xPct) / 100;
       const ty = (previewSize * this.yPct) / 100;
       const editorScale = fullWidth / 600;
       const s = this.scale * editorScale;

       // Create temp canvas sized to editor (including border area for exact match)
       const tempCanvas = document.createElement('canvas');
       tempCanvas.width = fullWidth;
       tempCanvas.height = fullWidth;
       const tempCtx = tempCanvas.getContext('2d');

       // CSS: transform: translate(tx, ty) scale(s) rotate(rad) with transform-origin: center
       // The image's natural center starts at (centerX, centerY) due to left/top positioning.
       // In CSS, the transform moves this center to (centerX + tx, centerY + ty), then
       // scale and rotate happen around that center point.
       //
       // In canvas, to get the same result:
       // 1. Move to the final center position: (centerX + tx, centerY + ty)
       // 2. Apply rotation (around the center)
       // 3. Apply scale (around the center)
       // 4. Draw image centered at origin

       tempCtx.translate(centerX + tx, centerY + ty);
       tempCtx.rotate(this.rotation * Math.PI / 180);
       tempCtx.scale(s, s);
       tempCtx.drawImage(
         this.elements.image,
         -naturalW / 2, -naturalH / 2, naturalW, naturalH
       );

       // Crop to preview overlay area (inset 16.6% from edges)
       const cropInset = fullWidth * 0.166;
       const cropSize = fullWidth * 0.6667;

       ctx.drawImage(
         tempCanvas,
         cropInset, cropInset, cropSize, cropSize,
         0, 0, 200, 200
       );

       resolve(canvas.toDataURL('image/png').split(',')[1]);
     });
   }

  /**
   * Load existing photo from settings
   * @private
   */
  async _loadExistingPhoto() {
    const section = this.getSectionData();
    // section IS the photoData object (not nested under photoData)
    if (section?.photo) {
      // Load saved photo
      this.photoData = `data:image/png;base64,${section.photo}`;
      this.originalPhotoData = section.original
        ? `data:image/png;base64,${section.original}`
        : this.photoData;

      // Restore slider positions
      this.scale = section.scale || 1;
      this.rotation = section.rotation || 0;

      // Restore percentages (migrate from old pixel values if needed)
      if (section.xPct !== undefined && section.yPct !== undefined) {
        this.xPct = section.xPct;
        this.yPct = section.yPct;
      } else {
        // Old format: x/y in pixels — convert to percentages relative to preview size
        const editorRect = this.elements.editor.getBoundingClientRect();
        const previewSize = editorRect.width * 0.6667;
        this.xPct = previewSize ? ((section.x || 0) / previewSize) * 100 : 0;
        this.yPct = previewSize ? ((section.y || 0) / previewSize) * 100 : 0;
      }

      // Restore original dimensions
      this.originalWidth = section.originalWidth || null;
      this.originalHeight = section.originalHeight || null;

// Update sliders (y inverted for display)
       this._updateSliderPosition(this._customSliders.x, this.xPct);
       this._updateSliderPosition(this._customSliders.y, this.yPct);
       this._updateSliderPosition(this._customSliders.scale, this.scale);
       this._updateSliderPosition(this._customSliders.rotation, this.rotation);

      this.elements.image.src = this.originalPhotoData;
        this.elements.image.onload = () => {
          // Fall back to image natural dimensions if not stored
          if (!this.originalWidth || !this.originalHeight) {
            this.originalWidth = this.elements.image.naturalWidth;
            this.originalHeight = this.elements.image.naturalHeight;
          }
          this._updateImageTransform();
          this._updateButtonStates();
          this._updatePreviews();
        };
    }
  }

  /**
   * Update button states based on current photo
   * @private
   */
  _updateButtonStates() {
    const hasPhoto = !!this.photoData;
    this.elements.removeBtn.disabled = !hasPhoto;
    // Download button state
    if (this.originalPhotoData) {
      this.elements.downloadBtn.classList.remove('disabled');
      this.elements.downloadBtn.style.pointerEvents = 'auto';
    } else {
      this.elements.downloadBtn.classList.add('disabled');
      this.elements.downloadBtn.style.pointerEvents = 'none';
    }
  }

  /**
   * Sync toolbar and previews width with editor width
   * @private
   */
  _syncToolbarWidth() {
    const editorWidth = this.elements.editor.getBoundingClientRect().width;
    this.elements.toolbar.style.width = `${editorWidth}px`;
    this.elements.previews.style.width = `${editorWidth}px`;
  }

  /**
   * Setup scroll wheel zoom functionality
   * @private
   */
  _setupScrollZoom() {
    const editor = this.elements.editor;

    editor.addEventListener('wheel', (e) => {
      if (!this.photoData) return;

      e.preventDefault();

      // Zoom in on scroll up, out on scroll down
      const zoomFactor = 0.05; // 5% per scroll step
      const delta = e.deltaY > 0 ? -zoomFactor : zoomFactor;

      // Update scale within bounds
      this.scale = Math.max(0.1, Math.min(4.0, this.scale * (1 + delta)));

      // Update slider and image
      this._updateSliderPosition(this._customSliders.scale, this.scale);
      this._updateImageTransform();
    });
  }

  /**
   * Setup editor resize functionality (anchored to top-left, maintains aspect ratio)
   * Image scales proportionally with the editor
   * @private
   */
  _setupResize() {
    const resizeHandle = this.elements.resizeHandle;
    const editor = this.elements.editor;
    let isResizing = false;
    let startX, startWidth;

    resizeHandle.addEventListener('mousedown', (e) => {
      isResizing = true;
      startX = e.clientX;
      startWidth = parseInt(getComputedStyle(editor).width, 10);
      e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
      if (!isResizing) return;

      const dx = e.clientX - startX;

      // Maintain aspect ratio (1:1) when resizing
      const newWidth = Math.max(300, Math.min(800, startWidth + dx));
      const newHeight = newWidth; // Keep 1:1 aspect ratio

      editor.style.width = `${newWidth}px`;
      editor.style.height = `${newHeight}px`;

      // xPct/yPct unchanged; recompute transform with new editor size
      this._updateImageTransform();
      this._syncToolbarWidth();
    });

    document.addEventListener('mouseup', () => {
      isResizing = false;
    });
  }

  /**
   * Handle reset button click - animate sliders to initial position
   * @private
   */
  async _handleReset() {
    const initial = { xPct:0, yPct:0, scale:this.initialScale, rotation:0 };
    const duration = 300; // ms
    const steps = 10;
    const interval = duration / steps;
    let step = 0;

    const intervalId = setInterval(() => {
      step++;
      const progress = step / steps;
      // Ease-in-out quadratic
      const ease = progress < 0.5 ? 2 * progress * progress : 1 - Math.pow(-2 * progress + 2, 2) / 2;

      this.xPct = this.xPct + (initial.xPct - this.xPct) * ease;
      this.yPct = this.yPct + (initial.yPct - this.yPct) * ease;
      this.scale = this.scale + (initial.scale - this.scale) * ease;
      this.rotation = this.rotation + (initial.rotation - this.rotation) * ease;

this._updateSliderPosition(this._customSliders.x, this.xPct);
       this._updateSliderPosition(this._customSliders.y, this.yPct);
       this._updateSliderPosition(this._customSliders.scale, this.scale);
       this._updateSliderPosition(this._customSliders.rotation, this.rotation);
       this._updateImageTransform();

       if (step >= steps) {
         clearInterval(intervalId);
         // Ensure final values are exact
         this.xPct = initial.xPct;
         this.yPct = initial.yPct;
         this.scale = initial.scale;
         this.rotation = initial.rotation;
         this._updateSliderPosition(this._customSliders.x, this.xPct);
         this._updateSliderPosition(this._customSliders.y, this.yPct);
         this._updateSliderPosition(this._customSliders.scale, this.scale);
         this._updateSliderPosition(this._customSliders.rotation, this.rotation);
         this._updateImageTransform();
       }
     }, interval);
   }

  /**
   * Update preview images with the saved 200px photo
   * @private
   */
  _updatePreviews() {
    const photoData = this.getSectionData();
    if (!photoData?.photo) {
      this.elements.preview40.src = '';
      this.elements.preview80.src = '';
      this.elements.preview150.src = '';
      return;
    }
    const src = `data:image/png;base64,${photoData.photo}`;
    this.elements.preview40.src = src;
    this.elements.preview80.src = src;
    this.elements.preview150.src = src;
  }

  /**
   * Show status message
   * @param {string} message - Status message
   * @param {string} type - Message type (success, error, info)
   * @private
   */
  _showStatus(message, type = 'info') {
    const statusEl = this.elements.status;
    statusEl.textContent = message;
    statusEl.className = 'photo-status';
    if (type) statusEl.classList.add(type);

    if (type !== 'info') {
      setTimeout(() => {
        statusEl.textContent = '';
        statusEl.className = 'photo-status';
      }, 3000);
    }
  }

  /**
   * Save method for settings page
   * @returns {Promise<Object>}
   */
  async save() {
    await this._handleSave();
    return { success: true };
  }

  /**
   * Cancel changes
   */
  cancel() {
    this._loadExistingPhoto();
    this.setDirty(false);
  }

  /**
   * Cleanup on destroy
   */
  destroy() {
    super.destroy();
    // Clear pending save timeout
    if (this._saveTimeout) {
      clearTimeout(this._saveTimeout);
      this._saveTimeout = null;
    }
    // Destroy FloatingUI tooltips
    Object.values(this._customSliders).forEach(slider => {
      slider._tooltip?.destroy();
    });
    // Remove download listener
    if (this.elements.downloadBtn) {
      this.elements.downloadBtn.removeEventListener('click', this._handleDownload);
    }
    this.photoData = null;
    this.originalPhotoData = null;
    this.elements = {};
    this._customSliders = {};
  }
}

export default PhotoPage;
