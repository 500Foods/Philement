// Lithium Network - API communication and offline handling module

class Network {
  constructor() {
    this.isOnline = navigator.onLine;
    this.baseURL = '/api'; // Default API base URL
    this.cache = new Map();
    this.pendingRequests = new Map();
  }

  async init() {
    // Set up online/offline event listeners
    window.addEventListener('online', () => this.setOnlineStatus(true));
    window.addEventListener('offline', () => this.setOnlineStatus(false));

    console.log('Network module initialized');
    return true;
  }

  setOnlineStatus(isOnline) {
    this.isOnline = isOnline;
    console.log(`Network status: ${isOnline ? 'Online' : 'Offline'}`);

    // If coming back online, retry pending requests
    if (isOnline) {
      this.retryPendingRequests();
    }
  }

  // HTTP request methods
  async get(url, options = {}) {
    return this.request('GET', url, null, options);
  }

  async post(url, data, options = {}) {
    return this.request('POST', url, data, options);
  }

  async put(url, data, options = {}) {
    return this.request('PUT', url, data, options);
  }

  async delete(url, options = {}) {
    return this.request('DELETE', url, null, options);
  }

  async request(method, url, data = null, options = {}) {
    const fullUrl = this.buildUrl(url);
    const requestId = this.generateRequestId(method, fullUrl);

    // Check cache first for GET requests
    if (method === 'GET' && this.cache.has(fullUrl)) {
      const cached = this.cache.get(fullUrl);
      if (this.isCacheValid(cached.timestamp)) {
        return cached.data;
      } else {
        this.cache.delete(fullUrl);
      }
    }

    // If offline, queue the request
    if (!this.isOnline && !options.force) {
      return this.queueRequest(requestId, { method, url: fullUrl, data, options });
    }

    try {
      const response = await this.makeRequest(method, fullUrl, data, options);

      // Cache successful GET responses
      if (method === 'GET' && response.ok) {
        this.cache.set(fullUrl, {
          data: response.data,
          timestamp: Date.now()
        });
      }

      return response;
    } catch (error) {
      console.error(`Network request failed: ${method} ${fullUrl}`, error);

      // If it's a network error and we're online, it might be a server issue
      // If offline, queue for later
      if (!this.isOnline) {
        return this.queueRequest(requestId, { method, url: fullUrl, data, options });
      }

      throw error;
    }
  }

  async makeRequest(method, url, data, options) {
    const config = {
      method,
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      },
      ...options
    };

    if (data && (method === 'POST' || method === 'PUT')) {
      config.body = JSON.stringify(data);
    }

    const response = await fetch(url, config);

    let responseData;
    const contentType = response.headers.get('content-type');

    if (contentType && contentType.includes('application/json')) {
      responseData = await response.json();
    } else {
      responseData = await response.text();
    }

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    return {
      ok: response.ok,
      status: response.status,
      statusText: response.statusText,
      data: responseData,
      headers: response.headers
    };
  }

  // Request queuing for offline scenarios
  queueRequest(requestId, request) {
    this.pendingRequests.set(requestId, {
      ...request,
      timestamp: Date.now(),
      retries: 0
    });

    console.log(`Request queued for offline retry: ${requestId}`);

    return {
      queued: true,
      requestId,
      message: 'Request queued for when connection is restored'
    };
  }

  async retryPendingRequests() {
    const requests = Array.from(this.pendingRequests.entries());

    for (const [requestId, request] of requests) {
      try {
        console.log(`Retrying queued request: ${requestId}`);
        const response = await this.makeRequest(
          request.method,
          request.url,
          request.data,
          request.options
        );

        // Remove from pending requests on success
        this.pendingRequests.delete(requestId);
        console.log(`Queued request completed: ${requestId}`);

        // TODO: Notify app of completed request if needed

      } catch (error) {
        request.retries++;

        // Remove after max retries
        if (request.retries >= 3) {
          console.error(`Max retries exceeded for queued request: ${requestId}`);
          this.pendingRequests.delete(requestId);
        }
      }
    }
  }

  // Utility methods
  buildUrl(url) {
    if (url.startsWith('http')) {
      return url;
    }
    return `${this.baseURL}${url.startsWith('/') ? '' : '/'}${url}`;
  }

  generateRequestId(method, url) {
    return `${method}_${url}_${Date.now()}`;
  }

  isCacheValid(timestamp, maxAge = 5 * 60 * 1000) { // 5 minutes default
    return (Date.now() - timestamp) < maxAge;
  }

  // Cache management
  clearCache() {
    this.cache.clear();
    console.log('Network cache cleared');
  }

  getCacheSize() {
    return this.cache.size;
  }

  // Configuration
  setBaseUrl(url) {
    this.baseURL = url;
  }

  getBaseUrl() {
    return this.baseURL;
  }
}

export { Network };