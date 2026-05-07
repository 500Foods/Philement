/**
 * Camera Popout Component for Photo Capture
 *
 * Provides a popout interface for camera capture with live preview,
 * camera selection, flip/rotate controls, and photo capture.
 */

import { log, Subsystems, Status } from '../../../../core/log.js';

export class CameraPopup {
  constructor(options = {}) {
    this.options = options;
    this.isVisible = false;
    this.stream = null;
    this.currentDeviceId = null;
    this.rotation = 0;
    this.hFlip = false;
    this.vFlip = false;
    this.videoElement = null;
    this.canvas = null;
    this.overlay = null;
    this.popup = null;

    // Bind methods
    this.handleOverlayClick = this.handleOverlayClick.bind(this);
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleResizeStart = this.handleResizeStart.bind(this);
    this.handleDragStart = this.handleDragStart.bind(this);
    this.handleDragMove = this.handleDragMove.bind(this);
    this.handleDragEnd = this.handleDragEnd.bind(this);
    this.handleDeviceChange = this.handleDeviceChange.bind(this);
    this.handleCapture = this.handleCapture.bind(this);
    this.closeAllPopupsHandler = this.closeAllPopupsHandler.bind(this);
  }

  /**
   * Initialize the popup (called on first show)
   */
  init() {
    if (this.popup) return;

    // Create overlay
    this.overlay = document.createElement('div');
    this.overlay.className = 'camera-popout-overlay';
    this.overlay.addEventListener('click', this.handleOverlayClick);
    document.body.appendChild(this.overlay);

    // Create popup
    this.popup = document.createElement('div');
    this.popup.className = 'camera-popout';
    this.popup.innerHTML = `
      <div class="camera-resize-handle camera-resize-handle-tl" data-tooltip="Resize"></div>
      <div class="camera-resize-handle camera-resize-handle-tr" data-tooltip="Resize"></div>
      <div class="camera-header">
        <div class="subpanel-header-group">
          <button type="button" class="camera-header-primary">
            <fa fa-camera></fa>
            <span>Take Photo</span>
          </button>
          <button type="button" class="camera-header-placeholder"></button>
          <button type="button" class="camera-header-close" data-tooltip="Close (ESC)">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
      <div class="camera-content">
        <div class="camera-video-container">
          <video id="camera-video" autoplay playsinline muted></video>
        </div>
        <div class="camera-controls">
          <div class="camera-device-selector">
            <label for="camera-select">Camera:</label>
            <select id="camera-select" disabled>
              <option value="">Loading cameras...</option>
            </select>
          </div>
          <div class="camera-iso-selector" id="camera-iso-container" style="display:none;">
            <label for="camera-iso">ISO:</label>
            <select id="camera-iso">
              <option value="auto">Auto</option>
            </select>
          </div>
          <div class="camera-gain-selector" id="camera-gain-container" style="display:none;">
            <label for="camera-gain">Gain:</label>
            <select id="camera-gain">
              <option value="auto">Auto</option>
            </select>
          </div>
          <div class="camera-transform-controls">
            <button id="flip-h-btn" class="camera-flip-btn" disabled data-tooltip="Flip Horizontal">
              <fa fa-left-right></fa>
            </button>
            <button id="flip-v-btn" class="camera-flip-btn" disabled data-tooltip="Flip Vertical">
              <fa fa-up-down></fa>
            </button>
            <button id="rotate-left-btn" class="camera-rotate-btn" disabled data-tooltip="Rotate Left">
              <fa fa-rotate-left></fa>
            </button>
            <button id="rotate-right-btn" class="camera-rotate-btn" disabled data-tooltip="Rotate Right">
              <fa fa-rotate-right></fa>
            </button>
            <button id="reset-transform-btn" disabled data-tooltip="Reset Transforms">
              <fa fa-arrow-rotate-left></fa>
            </button>
          </div>
        </div>
        <div class="camera-actions">
          <button id="camera-start-btn">Start Camera</button>
          <button id="camera-capture-btn" disabled>Capture Photo</button>
          <button id="camera-retake-btn" style="display:none;">Retake</button>
        </div>
      </div>
      <div class="camera-resize-handle camera-resize-handle-bl" data-tooltip="Resize"></div>
      <div class="camera-resize-handle camera-resize-handle-br" data-tooltip="Resize"></div>
    `;

    document.body.appendChild(this.popup);

    // Cache elements
    this.videoElement = this.popup.querySelector('#camera-video');
    this.cameraSelect = this.popup.querySelector('#camera-select');
    this.isoContainer = this.popup.querySelector('#camera-iso-container');
    this.isoSelect = this.popup.querySelector('#camera-iso');
    this.gainContainer = this.popup.querySelector('#camera-gain-container');
    this.gainSelect = this.popup.querySelector('#camera-gain');
    this.startBtn = this.popup.querySelector('#camera-start-btn');
    this.captureBtn = this.popup.querySelector('#camera-capture-btn');
    this.retakeBtn = this.popup.querySelector('#camera-retake-btn');
    this.flipHBtn = this.popup.querySelector('#flip-h-btn');
    this.flipVBtn = this.popup.querySelector('#flip-v-btn');
    this.rotateLeftBtn = this.popup.querySelector('#rotate-left-btn');
    this.rotateRightBtn = this.popup.querySelector('#rotate-right-btn');
    this.resetBtn = this.popup.querySelector('#reset-transform-btn');

    // Create canvas for capture
    this.canvas = document.createElement('canvas');
    this.canvas.style.display = 'none';
    document.body.appendChild(this.canvas);

    // Bind events
    this.bindEvents();

    // Add drag functionality to header
    const header = this.popup.querySelector('.camera-header');
    if (header) {
      header.addEventListener('mousedown', (e) => this.handleDragStart(e));
    }

    // Initialize tooltips
    if (window.initTooltips) {
      window.initTooltips(this.popup);
    }
  }

  /**
   * Bind event listeners
   */
  bindEvents() {
    // Close button
    const closeBtn = this.popup.querySelector('.camera-header-close');
    closeBtn.addEventListener('click', () => this.hide());

    // Camera controls
    this.startBtn.addEventListener('click', () => this.startCamera());
    this.captureBtn.addEventListener('click', () => this.handleCapture());
    this.retakeBtn.addEventListener('click', () => this.retake());

    // Transform controls
    this.flipHBtn.addEventListener('click', () => this.toggleFlip('h'));
    this.flipVBtn.addEventListener('click', () => this.toggleFlip('v'));
    this.rotateLeftBtn.addEventListener('click', () => this.rotate(-90));
    this.rotateRightBtn.addEventListener('click', () => this.rotate(90));
    this.resetBtn.addEventListener('click', () => this.resetTransforms());

    // Camera selection
    this.cameraSelect.addEventListener('change', this.handleDeviceChange);

    // ISO selection
    this.isoSelect.addEventListener('change', () => this.handleIsoChange());

    // Gain selection
    this.gainSelect.addEventListener('change', () => this.handleGainChange());

    // Resize handles
    const resizeHandles = this.popup.querySelectorAll('.camera-resize-handle');
    resizeHandles.forEach(handle => {
      handle.addEventListener('mousedown', (e) => this.handleResizeStart(e, handle.classList[1].split('-')[3]));
    });

    // Listen for close-all-popups
    document.addEventListener('close-all-popups', this.closeAllPopupsHandler);
  }

  /**
   * Handle close-all-popups event
   */
  closeAllPopupsHandler() {
    if (this.isVisible) {
      this.hide();
    }
  }

  /**
   * Show the popup
   */
  async show() {
    if (!this.popup) {
      this.init();
    }

    if (this.isVisible) {
      return;
    }

    // Dispatch close-all-popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    // Position popup (center if no saved position)
    this.centerPopup();

    // Show overlay and popup
    this.overlay.classList.add('visible');
    this.popup.classList.add('visible');
    this.isVisible = true;

    // Add global keydown listener
    document.addEventListener('keydown', this.handleKeyDown);

    // Try to start camera automatically
    try {
      await this.startCamera();
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[CameraPopout] Auto-start failed:', error.message);
    }

    log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Shown');
  }

  /**
   * Hide the popup
   */
  hide() {
    if (!this.isVisible) return;

    // Stop camera stream
    this.stopCamera();

    // Hide popup
    this.overlay.classList.remove('visible');
    this.popup.classList.remove('visible');
    this.isVisible = false;

    // Remove global listeners
    document.removeEventListener('keydown', this.handleKeyDown);

    log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Hidden');
  }

  /**
   * Center the popup on screen
   */
  centerPopup() {
    if (!this.popup) return;

    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const popupWidth = 600; // Default width
    const popupHeight = 500; // Default height

    this.popup.style.left = `${Math.round((viewportWidth - popupWidth) / 2)}px`;
    this.popup.style.top = `${Math.round((viewportHeight - popupHeight) / 2)}px`;
    this.popup.style.width = `${popupWidth}px`;
    this.popup.style.height = `${popupHeight}px`;
  }

  /**
   * Start camera with selected device
   */
  async startCamera(deviceId = null) {
    this.stopCamera();

    const baseConstraints = {
      width: { ideal: 4096 },
      height: { ideal: 2160 },
      facingMode: 'user',
      noiseSuppression: false,
      advanced: [
        { exposureMode: 'manual' },
        { iso: { ideal: 100 } },
        { brightness: { ideal: 128 } },
        { exposureCompensation: { ideal: 0 } },
      ],
    };

    const constraints = {
      video: deviceId
        ? { ...baseConstraints, deviceId: { exact: deviceId } }
        : baseConstraints
    };

    try {
      this.stream = await navigator.mediaDevices.getUserMedia(constraints);
      this.videoElement.srcObject = this.stream;

      // Get the actual device ID and applied settings
      const track = this.stream.getVideoTracks()[0];
      const settings = track.getSettings();
      this.currentDeviceId = settings.deviceId || deviceId;

      // Log actual resolution for debugging
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] Camera started: ${settings.width}x${settings.height}`);

      // Detect and populate ISO / Gain capabilities
      await this._detectIsoCapabilities(track);
      await this._detectGainCapabilities(track);

      // Populate camera list if not already done
      if (this.cameraSelect.options.length <= 1) {
        await this.populateCameraList();
      }

      // Select current camera
      this.cameraSelect.value = this.currentDeviceId || '';

      // Update UI
      this.startBtn.style.display = 'none';
      this.captureBtn.disabled = false;
      this.updateButtonStates();

      // Apply current transforms
      this.applyVideoTransforms();

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to start camera:', error.message);
      alert('Camera access failed. Please check permissions.');
    }
  }

  /**
   * Stop current camera stream
   */
  stopCamera() {
    if (this.stream) {
      this.stream.getTracks().forEach(track => track.stop());
      this.stream = null;
    }
    this.videoElement.srcObject = null;
  }

  /**
   * Detect ISO capabilities and populate ISO selector
   * @param {MediaStreamTrack} track - Active video track
   * @private
   */
  async _detectIsoCapabilities(track) {
    try {
      const capabilities = track.getCapabilities();
      const settings = track.getSettings();

      if (!capabilities.iso) {
        this.isoContainer.style.display = 'none';
        log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] ISO control not supported');
        return;
      }

      // Build ISO options from capabilities
      const { min, max, step } = capabilities.iso;
      const isoValues = [];
      for (let v = min; v <= max; v = Math.round(v * step > 1 ? v * step : v + step)) {
        isoValues.push(v);
        if (v >= max) break;
      }
      // Add common standard ISO values within range
      const standardIsos = [50, 100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600];
      standardIsos.forEach(iso => {
        if (iso >= min && iso <= max && !isoValues.includes(iso)) {
          isoValues.push(iso);
        }
      });
      isoValues.sort((a, b) => a - b);

      // Populate selector
      this.isoSelect.innerHTML = '<option value="auto">Auto</option>';
      isoValues.forEach(iso => {
        const opt = document.createElement('option');
        opt.value = iso;
        opt.textContent = iso;
        this.isoSelect.appendChild(opt);
      });

      // Set current value
      const currentIso = settings.iso;
      if (currentIso) {
        this.isoSelect.value = currentIso;
      }

      this.isoContainer.style.display = '';
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] ISO range: ${min}-${max}, current: ${currentIso || 'auto'}`);

    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[CameraPopout] ISO detection failed:', error.message);
      this.isoContainer.style.display = 'none';
    }
  }

  /**
   * Handle ISO selection change
   * @private
   */
  async handleIsoChange() {
    if (!this.stream) return;

    const track = this.stream.getVideoTracks()[0];
    if (!track) return;

    const value = this.isoSelect.value;
    const constraints = {};

    if (value === 'auto') {
      constraints.advanced = [{ iso: 'auto' }];
    } else {
      const isoValue = parseInt(value, 10);
      constraints.advanced = [{ iso: isoValue }];
    }

    try {
      await track.applyConstraints(constraints);
      const newSettings = track.getSettings();
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] ISO set to: ${newSettings.iso || 'auto'}`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to set ISO:', error.message);
    }
  }

  /**
   * Detect Gain capabilities and populate Gain selector
   * @param {MediaStreamTrack} track - Active video track
   * @private
   */
  async _detectGainCapabilities(track) {
    try {
      const capabilities = track.getCapabilities();
      const settings = track.getSettings();

      if (!capabilities.gain) {
        this.gainContainer.style.display = 'none';
        log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Gain control not supported');
        return;
      }

      // Build Gain options from capabilities
      const { min, max, step } = capabilities.gain;
      const gainValues = [];
      for (let v = min; v <= max; v = Math.round(v * step > 1 ? v * step : v + step)) {
        gainValues.push(v);
        if (v >= max) break;
      }
      gainValues.sort((a, b) => a - b);

      // Populate selector
      this.gainSelect.innerHTML = '<option value="auto">Auto</option>';
      gainValues.forEach(gain => {
        const opt = document.createElement('option');
        opt.value = gain;
        opt.textContent = gain;
        this.gainSelect.appendChild(opt);
      });

      // Set current value
      const currentGain = settings.gain;
      if (currentGain !== undefined) {
        this.gainSelect.value = currentGain;
      }

      this.gainContainer.style.display = '';
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] Gain range: ${min}-${max}, current: ${currentGain !== undefined ? currentGain : 'auto'}`);

    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[CameraPopout] Gain detection failed:', error.message);
      this.gainContainer.style.display = 'none';
    }
  }

  /**
   * Handle Gain selection change
   * @private
   */
  async handleGainChange() {
    if (!this.stream) return;

    const track = this.stream.getVideoTracks()[0];
    if (!track) return;

    const value = this.gainSelect.value;
    const constraints = {};

    if (value === 'auto') {
      constraints.advanced = [{ gain: 'auto' }];
    } else {
      const gainValue = parseInt(value, 10);
      constraints.advanced = [{ gain: gainValue }];
    }

    try {
      await track.applyConstraints(constraints);
      const newSettings = track.getSettings();
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] Gain set to: ${newSettings.gain !== undefined ? newSettings.gain : 'auto'}`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to set Gain:', error.message);
    }
  }

  /**
   * Populate camera selection dropdown
   */
  async populateCameraList() {
    try {
      const devices = await navigator.mediaDevices.enumerateDevices();
      const videoDevices = devices.filter(d => d.kind === 'videoinput');

      this.cameraSelect.innerHTML = '';

      if (videoDevices.length === 0) {
        const opt = document.createElement('option');
        opt.textContent = 'No cameras found';
        this.cameraSelect.appendChild(opt);
        return;
      }

      for (const device of videoDevices) {
        const opt = document.createElement('option');
        opt.value = device.deviceId;

        // Query capabilities to show resolution info
        let resolutionInfo = '';
        try {
          const testStream = await navigator.mediaDevices.getUserMedia({
            video: { deviceId: { exact: device.deviceId }, width: { ideal: 4096 }, height: { ideal: 2160 } }
          });
          const track = testStream.getVideoTracks()[0];
          const settings = track.getSettings();
          resolutionInfo = ` (${settings.width}x${settings.height})`;
          track.stop();
        } catch {
          // Fallback: no resolution info
        }

        opt.textContent = (device.label || `Camera ${videoDevices.indexOf(device) + 1}`) + resolutionInfo;
        this.cameraSelect.appendChild(opt);
      }

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to enumerate cameras:', error.message);
    }
  }

  /**
   * Apply transforms to video preview
   */
  applyVideoTransforms() {
    if (!this.videoElement) return;

    let transform = `rotate(${this.rotation}deg)`;
    if (this.hFlip) transform += ' scaleX(-1)';
    if (this.vFlip) transform += ' scaleY(-1)';

    this.videoElement.style.transform = transform;
  }

  /**
   * Update button states
   */
  updateButtonStates() {
    const hasStream = !!this.stream;
    this.captureBtn.disabled = !hasStream;
    this.flipHBtn.disabled = !hasStream;
    this.flipVBtn.disabled = !hasStream;
    this.rotateLeftBtn.disabled = !hasStream;
    this.rotateRightBtn.disabled = !hasStream;
    this.resetBtn.disabled = !hasStream || (this.rotation === 0 && !this.hFlip && !this.vFlip);
    this.cameraSelect.disabled = !hasStream;
  }

  /**
   * Toggle horizontal/vertical flip
   */
  toggleFlip(type) {
    if (type === 'h') {
      this.hFlip = !this.hFlip;
    } else if (type === 'v') {
      this.vFlip = !this.vFlip;
    }
    this.applyVideoTransforms();
    this.updateButtonStates();
  }

  /**
   * Rotate video
   */
  rotate(degrees) {
    this.rotation = (this.rotation + degrees + 360) % 360;
    this.applyVideoTransforms();
    this.updateButtonStates();
  }

  /**
   * Reset all transforms
   */
  resetTransforms() {
    this.rotation = 0;
    this.hFlip = false;
    this.vFlip = false;
    this.applyVideoTransforms();
    this.updateButtonStates();
  }

  /**
   * Handle camera device change
   */
  handleDeviceChange() {
    if (this.cameraSelect.value) {
      this.startCamera(this.cameraSelect.value);
    }
  }

  /**
   * Capture photo with current transforms
   */
  async handleCapture() {
    if (!this.stream || !this.videoElement) return;

    try {
      const w = this.videoElement.videoWidth;
      const h = this.videoElement.videoHeight;

      // Determine canvas size (swap for 90°/270° rotation)
      let cw = w;
      let ch = h;
      if (this.rotation % 180 !== 0) {
        cw = h;
        ch = w;
      }

      this.canvas.width = cw;
      this.canvas.height = ch;

      const ctx = this.canvas.getContext('2d', { alpha: false });
      ctx.save();

      // Move origin to center
      ctx.translate(cw / 2, ch / 2);

      // Apply rotation
      ctx.rotate(this.rotation * Math.PI / 180);

      // Apply flips
      ctx.scale(this.hFlip ? -1 : 1, this.vFlip ? -1 : 1);

      // Draw the current video frame centered
      ctx.drawImage(this.videoElement, -w / 2, -h / 2, w, h);

      ctx.restore();

      // Apply mild noise reduction via temporary downscale/upscale
      // Step 1: draw to a small temp canvas (blur/noise average out)
      const tempCanvas = document.createElement('canvas');
      const scaleFactor = 0.5;
      tempCanvas.width = Math.max(1, Math.round(cw * scaleFactor));
      tempCanvas.height = Math.max(1, Math.round(ch * scaleFactor));
      const tempCtx = tempCanvas.getContext('2d', { alpha: false });
      tempCtx.imageSmoothingEnabled = true;
      tempCtx.imageSmoothingQuality = 'high';
      tempCtx.drawImage(this.canvas, 0, 0, tempCanvas.width, tempCanvas.height);

      // Step 2: draw back to original canvas at full size
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = 'high';
      ctx.drawImage(tempCanvas, 0, 0, cw, ch);

      // Convert to blob at maximum quality
      const blob = await new Promise(resolve => {
        this.canvas.toBlob(resolve, 'image/png');
      });

      if (!blob || blob.size === 0) {
        throw new Error('Failed to capture image');
      }

      // Create file
      const file = new window.File([blob], 'capture.png', { type: 'image/png' });

      // Call callback if provided
      if (this.options.onCapture) {
        this.options.onCapture(file);
      }

      // Hide popout after successful capture

      this.hide();

      log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Photo captured successfully');

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Capture failed:', error.message);
      alert('Failed to capture photo. Please try again.');
    }
  }

  /**
   * Retake photo (restart camera)
   */
  async retake() {
    // Show controls again
    this.startBtn.style.display = 'inline-block';
    this.captureBtn.style.display = 'inline-block';
    this.retakeBtn.style.display = 'none';

    // Restart camera
    await this.startCamera(this.currentDeviceId);
  }

  /**
   * Handle overlay click (close popup)
   */
  handleOverlayClick(e) {
    if (e.target === this.overlay) {
      this.hide();
    }
  }

  /**
   * Handle keydown events
   */
  handleKeyDown(e) {
    if (e.key === 'Escape') {
      this.hide();
      e.preventDefault();
    } else if (e.key === ' ' && !this.captureBtn.disabled) {
      // Space to capture
      this.handleCapture();
      e.preventDefault();
    }
  }

  /**
   * Handle resize start
   */
  handleResizeStart(e, corner) {
    // Basic resize implementation (similar to Terminal popup)
    e.preventDefault();

    const startX = e.clientX;
    const startY = e.clientY;
    const startRect = this.popup.getBoundingClientRect();

    const handleMouseMove = (e) => {
      const deltaX = e.clientX - startX;
      const deltaY = e.clientY - startY;

      let newWidth = startRect.width;
      let newHeight = startRect.height;

      if (corner.includes('r')) newWidth = startRect.width + deltaX;
      if (corner.includes('l')) {
        newWidth = startRect.width - deltaX;
        this.popup.style.left = `${startRect.left + deltaX}px`;
      }
      if (corner.includes('b')) newHeight = startRect.height + deltaY;
      if (corner.includes('t')) {
        newHeight = startRect.height - deltaY;
        this.popup.style.top = `${startRect.top + deltaY}px`;
      }

      // Enforce minimum size
      newWidth = Math.max(400, newWidth);
      newHeight = Math.max(300, newHeight);

      this.popup.style.width = `${newWidth}px`;
      this.popup.style.height = `${newHeight}px`;
    };

    const handleMouseUp = () => {
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };

    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
  }

  /**
   * Destroy the popup (cleanup)
   */
  destroy() {
    this.hide();

    if (this.popup) {
      document.body.removeChild(this.popup);
      this.popup = null;
    }

    if (this.overlay) {
      document.body.removeChild(this.overlay);
      this.overlay = null;
    }

    if (this.canvas) {
      document.body.removeChild(this.canvas);
      this.canvas = null;
    }

    document.removeEventListener('close-all-popups', this.closeAllPopupsHandler);
  }

  /**
   * Handle drag start
   */
  handleDragStart(e) {
    // Allow dragging from the title area and placeholder, but not from actionable controls
    if (e.target.closest([
      '.camera-header-close',
      '#camera-select'
    ].join(', '))) return;

    this.isDragging = true;
    this.dragStartX = e.clientX;
    this.dragStartY = e.clientY;

    const rect = this.popup.getBoundingClientRect();
    this.popupStartX = rect.left;
    this.popupStartY = rect.top;

    this.popup.classList.add('dragging');

    document.addEventListener('mousemove', this.handleDragMove);
    document.addEventListener('mouseup', this.handleDragEnd);

    e.preventDefault();
  }

  /**
   * Handle drag move
   */
  handleDragMove(e) {
    if (!this.isDragging) return;

    const dx = e.clientX - this.dragStartX;
    const dy = e.clientY - this.dragStartY;

    let newX = this.popupStartX + dx;
    let newY = this.popupStartY + dy;

    // Keep some part of the popout visible
    const rect = this.popup.getBoundingClientRect();
    const minVisible = 50;

    newX = Math.max(minVisible - rect.width, Math.min(newX, window.innerWidth - minVisible));
    newY = Math.max(0, Math.min(newY, window.innerHeight - minVisible));

    this.popup.style.left = `${newX}px`;
    this.popup.style.top = `${newY}px`;
  }

  /**
   * Handle drag end
   */
  handleDragEnd() {
    if (!this.isDragging) return;

    this.isDragging = false;
    this.popup.classList.remove('dragging');

    document.removeEventListener('mousemove', this.handleDragMove);
    document.removeEventListener('mouseup', this.handleDragEnd);
  }
}