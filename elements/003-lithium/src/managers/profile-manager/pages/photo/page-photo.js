/**
 * Profile Manager - Photo Page
 * 
 * Handles user photo upload, capture, editing, and saving as 200x200 base64 PNG
 */

import { BaseSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

export class PhotoPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -12 });
    
    // Editor state
    this.photoData = null;
    this.x = 0;
    this.y = 0;
    this.scale = 1;
    this.rotation = 0;
    this.originalFile = null;
    
    // DOM elements
    this.elements = {};
  }

  /**
   * Initialize the page
   */
  async onInit() {
    this._cacheElements();
    this._bindEvents();
    await this._loadExistingPhoto();
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
      saveBtn: container.querySelector('#photo-save-btn'),
      status: container.querySelector('#photo-status'),
      editor: container.querySelector('#photo-editor'),
      imageContainer: container.querySelector('#photo-image-container'),
      image: container.querySelector('#photo-image'),
      dimming: container.querySelector('#photo-dimming'),
      previewOverlay: container.querySelector('#photo-preview-overlay'),
      sliderX: container.querySelector('#slider-x'),
      sliderY: container.querySelector('#slider-y'),
      sliderScale: container.querySelector('#slider-scale'),
      sliderRotation: container.querySelector('#slider-rotation'),
      resizeHandle: container.querySelector('#photo-resize-handle'),
    };
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
    
    // Save handler
    this.elements.saveBtn.addEventListener('click', () => this._handleSave());
    
    // Slider handlers
    this.elements.sliderX.addEventListener('input', (e) => {
      this.x = parseInt(e.target.value, 10);
      this._updateImageTransform();
    });
    
    this.elements.sliderY.addEventListener('input', (e) => {
      this.y = parseInt(e.target.value, 10);
      this._updateImageTransform();
    });
    
    this.elements.sliderScale.addEventListener('input', (e) => {
      this.scale = parseFloat(e.target.value);
      this._updateImageTransform();
    });
    
    this.elements.sliderRotation.addEventListener('input', (e) => {
      this.rotation = parseInt(e.target.value, 10);
      this._updateImageTransform();
    });
    
    // Resize handler
    this._setupResize();
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
      this._showStatus('Please select an image file', 'error');
      return;
    }
    
    // Validate file size (10MB max)
    if (file.size > 10 * 1024 * 1024) {
      this._showStatus('File size exceeds 10MB limit', 'error');
      return;
    }
    
    this.originalFile = file;
    await this._loadImageFromFile(file);
    this._updateButtonStates();
  }

  /**
   * Handle photo capture from camera
   * @private
   */
  async _handleCapture() {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ 
        video: { facingMode: 'user' } 
      });
      
      // Create video element
      const video = document.createElement('video');
      video.srcObject = stream;
      video.autoplay = true;
      video.playsInline = true;
      
      // Wait for video to load
      await new Promise((resolve) => {
        video.onloadedmetadata = resolve;
      });
      
      // Create canvas to capture frame
      const canvas = document.createElement('canvas');
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const ctx = canvas.getContext('2d');
      ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
      
      // Stop camera stream
      stream.getTracks().forEach(track => track.stop());
      
      // Convert to blob and load
      canvas.toBlob(async (blob) => {
        const file = new File([blob], 'capture.jpg', { type: 'image/jpeg' });
        this.originalFile = file;
        await this._loadImageFromFile(file);
        this._updateButtonStates();
      }, 'image/jpeg', 0.9);
      
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[PhotoPage] Camera capture failed:', error.message);
      this._showStatus('Camera access failed. Please check permissions.', 'error');
    }
  }

  /**
   * Load image from file
   * @param {File} file - Image file
   * @private
   */
  async _loadImageFromFile(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = (e) => {
        this.photoData = e.target.result;
        this.elements.image.src = this.photoData;
        this.elements.image.onload = () => {
          this._updateImageTransform();
          resolve();
        };
      };
      reader.onerror = reject;
      reader.readAsDataURL(file);
    });
  }

  /**
   * Update image transform based on slider values
   * @private
   */
  _updateImageTransform() {
    if (!this.photoData) return;
    
    const transform = `
      translate(${this.x}px, ${this.y}px)
      scale(${this.scale})
      rotate(${this.rotation}deg)
    `;
    this.elements.image.style.transform = transform;
    
    // Update editor dirty state
    this.setDirty(true);
  }

  /**
   * Handle photo removal
   * @private
   */
  _handleRemove() {
    this.photoData = null;
    this.x = 0;
    this.y = 0;
    this.scale = 1;
    this.rotation = 0;
    this.originalFile = null;
    
    this.elements.image.src = '';
    this.elements.sliderX.value = 0;
    this.elements.sliderY.value = 0;
    this.elements.sliderScale.value = 1;
    this.elements.sliderRotation.value = 0;
    
    this._updateImageTransform();
    this._updateButtonStates();
    this.setDirty(true);
  }

  /**
   * Handle save photo
   * @private
   */
  async _handleSave() {
    if (!this.photoData) {
      this._showStatus('No photo to save', 'error');
      return;
    }
    
    try {
      this.elements.saveBtn.disabled = true;
      this._showStatus('Processing photo...', 'info');
      
      // Capture 200x200 PNG
      const base64 = await this._capturePhoto();
      
      // Save to settings
      this.setSetting('photo', base64);
      this.setSetting('timestamp', new Date().toISOString());
      this.setSectionData({
        photo: base64,
        timestamp: new Date().toISOString()
      }, 'Photo');
      
      this._showStatus('Photo saved successfully!', 'success');
      this.setDirty(false);
      this._updateButtonStates();
      
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[PhotoPage] Save failed:', error.message);
      this._showStatus('Failed to save photo', 'error');
    } finally {
      this.elements.saveBtn.disabled = false;
    }
  }

  /**
   * Capture photo as 200x200 PNG base64
   * @returns {Promise<string>} Base64 encoded PNG
   * @private
   */
  async _capturePhoto() {
    return new Promise((resolve) => {
      const canvas = document.createElement('canvas');
      canvas.width = 200;
      canvas.height = 200;
      const ctx = canvas.getContext('2d');
      
      // Get the editor and preview bounds
      const editor = this.elements.editor;
      const preview = this.elements.previewOverlay;
      
      // Calculate scale factors between displayed editor and actual image
      const editorRect = editor.getBoundingClientRect();
      const previewRect = preview.getBoundingClientRect();
      
      // Create a temporary canvas to draw the full transformed image
      const tempCanvas = document.createElement('canvas');
      tempCanvas.width = editorRect.width;
      tempCanvas.height = editorRect.height;
      const tempCtx = tempCanvas.getContext('2d');
      
      // Apply transformations to temp canvas
      tempCtx.translate(
        editorRect.width / 2 + this.x,
        editorRect.height / 2 + this.y
      );
      tempCtx.scale(this.scale, this.scale);
      tempCtx.rotate(this.rotation * Math.PI / 180);
      
      // Draw image centered
      const img = this.elements.image;
      tempCtx.drawImage(
        img,
        -img.naturalWidth / 2,
        -img.naturalHeight / 2,
        img.naturalWidth,
        img.naturalHeight
      );
      
      // Crop to preview area (400x400 centered in 600x600 = 100,100 to 500,500)
      const scaleX = editorRect.width / 600;
      const scaleY = editorRect.height / 600;
      const cropX = 100 * scaleX;
      const cropY = 100 * scaleY;
      const cropSize = 400 * scaleX;
      
      // Draw cropped area to 200x200 canvas
      ctx.drawImage(
        tempCanvas,
        cropX, cropY, cropSize, cropSize,
        0, 0, 200, 200
      );
      
      // Export as PNG base64
      resolve(canvas.toDataURL('image/png').split(',')[1]);
    });
  }

  /**
   * Load existing photo from settings
   * @private
   */
  async _loadExistingPhoto() {
    const section = this.getSectionData();
    if (section?.photo) {
      this.photoData = `data:image/png;base64,${section.photo}`;
      this.elements.image.src = this.photoData;
      this.elements.image.onload = () => {
        this._updateImageTransform();
        this._updateButtonStates();
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
    this.elements.saveBtn.disabled = !hasPhoto;
  }

  /**
   * Setup editor resize functionality
   * @private
   */
  _setupResize() {
    const resizeHandle = this.elements.resizeHandle;
    const editor = this.elements.editor;
    let isResizing = false;
    let startX, startY, startWidth, startHeight;
    
    resizeHandle.addEventListener('mousedown', (e) => {
      isResizing = true;
      startX = e.clientX;
      startY = e.clientY;
      startWidth = parseInt(getComputedStyle(editor).width, 10);
      startHeight = parseInt(getComputedStyle(editor).height, 10);
      e.preventDefault();
    });
    
    document.addEventListener('mousemove', (e) => {
      if (!isResizing) return;
      
      const dx = e.clientX - startX;
      const dy = e.clientY - startY;
      
      const newWidth = Math.max(300, Math.min(800, startWidth + dx));
      const newHeight = Math.max(300, Math.min(800, startHeight + dy));
      
      editor.style.width = `${newWidth}px`;
      editor.style.height = `${newHeight}px`;
    });
    
    document.addEventListener('mouseup', () => {
      isResizing = false;
    });
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
    this.photoData = null;
    this.elements = {};
  }
}

export default PhotoPage;
