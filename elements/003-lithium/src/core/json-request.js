/**
 * JSON Request - Centralized fetch wrapper for API calls
 * 
 * Handles authentication, error handling, logging, and provides
 * a consistent interface for all API requests.
 */

import { retrieveJWT } from './jwt.js';
import { eventBus, Events } from './event-bus.js';
import { logHttpRequest, logHttpResponse, logGroup, Subsystems, Status } from './log.js';

/**
 * Build full URL from path
 * @param {string} path - API path
 * @param {Object} config - App configuration
 * @returns {string} Full URL
 */
function buildUrl(path, config) {
  const baseUrl = config?.server?.url || '';
  const apiPrefix = config?.server?.api_prefix || '/api';
  
  // If path already starts with http, use as-is
  if (path.startsWith('http')) {
    return path;
  }
  
  // Remove leading slash from path if present
  const cleanPath = path.startsWith('/') ? path.slice(1) : path;
  
  // Build full URL
  return `${baseUrl}${apiPrefix}/${cleanPath}`;
}

/**
 * Get default headers for requests
 * @param {boolean} includeAuth - Whether to include auth header
 * @returns {Object} Headers object
 */
function getHeaders(includeAuth = true) {
  const headers = {
    'Content-Type': 'application/json',
    'Accept': 'application/json',
  };

  if (includeAuth) {
    const token = retrieveJWT();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }
  }

  return headers;
}

/**
 * Handle HTTP errors
 * @param {Response} response - Fetch response
 * @returns {Promise} Resolved with data or rejected with error
 */
async function handleResponse(response, requestNum) {
  // Parse JSON body if present
  let data = null;
  const contentType = response.headers.get('content-type');
  
  if (contentType && contentType.includes('application/json')) {
    try {
      data = await response.json();
    } catch (e) {
      // JSON parse error, continue with null data
    }
  }

  // Log error response body as RESTAPI grouped entry when available
  if (!response.ok && data && typeof data === 'object' && requestNum) {
    const items = Object.entries(data).map(([key, value]) => `${key}: ${value}`);
    if (items.length > 0) {
      logGroup(Subsystems.RESTAPI, Status.ERROR, `Response ${requestNum}: Body`, items);
    }
  }

  // Handle errors
  if (!response.ok) {
    const error = new Error(data?.message || `HTTP ${response.status}: ${response.statusText}`);
    error.status = response.status;
    error.data = data;
    error.response = response;

    // Handle specific status codes
    switch (response.status) {
      case 401:
        // Unauthorized - token expired or invalid
        eventBus.emit(Events.AUTH_EXPIRED, { error });
        break;
      case 403:
        error.message = data?.message || 'Access forbidden';
        break;
      case 429:
        // Rate limited
        const retryAfter = response.headers.get('retry-after');
        error.retryAfter = retryAfter ? parseInt(retryAfter, 10) : null;
        break;
      case 500:
      case 502:
      case 503:
      case 504:
        error.message = data?.message || 'Server error. Please try again later.';
        break;
    }

    throw error;
  }

  return data;
}

/**
 * Make a GET request
 * @param {string} path - API path
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
export async function get(path, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('GET', path);
  
  const response = await fetch(url, {
    method: 'GET',
    headers: getHeaders(includeAuth),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  if (response.ok) {
    logHttpResponse(requestNum, 'GET', path, response.status, contentLength, duration);
  } else {
    logHttpResponse(requestNum, 'GET', path, response.status, null, duration);
  }
  
  return handleResponse(response, requestNum);
}

/**
 * Make a POST request
 * @param {string} path - API path
 * @param {Object} body - Request body
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
export async function post(path, body, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('POST', path);
  
  const response = await fetch(url, {
    method: 'POST',
    headers: getHeaders(includeAuth),
    body: JSON.stringify(body),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  if (response.ok) {
    logHttpResponse(requestNum, 'POST', path, response.status, contentLength, duration);
  } else {
    logHttpResponse(requestNum, 'POST', path, response.status, null, duration);
  }
  
  return handleResponse(response, requestNum);
}

/**
 * Make a PUT request
 * @param {string} path - API path
 * @param {Object} body - Request body
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
export async function put(path, body, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('PUT', path);
  
  const response = await fetch(url, {
    method: 'PUT',
    headers: getHeaders(includeAuth),
    body: JSON.stringify(body),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  if (response.ok) {
    logHttpResponse(requestNum, 'PUT', path, response.status, contentLength, duration);
  } else {
    logHttpResponse(requestNum, 'PUT', path, response.status, null, duration);
  }
  
  return handleResponse(response, requestNum);
}

/**
 * Make a DELETE request
 * @param {string} path - API path
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
export async function del(path, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('DELETE', path);
  
  const response = await fetch(url, {
    method: 'DELETE',
    headers: getHeaders(includeAuth),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  if (response.ok) {
    logHttpResponse(requestNum, 'DELETE', path, response.status, contentLength, duration);
  } else {
    logHttpResponse(requestNum, 'DELETE', path, response.status, null, duration);
  }
  
  return handleResponse(response, requestNum);
}

/**
 * Make a PATCH request
 * @param {string} path - API path
 * @param {Object} body - Request body
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
export async function patch(path, body, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('PATCH', path);
  
  const response = await fetch(url, {
    method: 'PATCH',
    headers: getHeaders(includeAuth),
    body: JSON.stringify(body),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  if (response.ok) {
    logHttpResponse(requestNum, 'PATCH', path, response.status, contentLength, duration);
  } else {
    logHttpResponse(requestNum, 'PATCH', path, response.status, null, duration);
  }
  
  return handleResponse(response, requestNum);
}

/**
 * Create a configured request instance with default config
 * @param {Object} defaultConfig - Default configuration
 * @returns {Object} Request methods bound to config
 */
export function createRequest(defaultConfig) {
  return {
    get: (path, options = {}) => get(path, { ...options, config: defaultConfig }),
    post: (path, body, options = {}) => post(path, body, { ...options, config: defaultConfig }),
    put: (path, body, options = {}) => put(path, body, { ...options, config: defaultConfig }),
    del: (path, options = {}) => del(path, { ...options, config: defaultConfig }),
    patch: (path, body, options = {}) => patch(path, body, { ...options, config: defaultConfig }),
  };
}

// Default export
export default {
  get,
  post,
  put,
  del,
  patch,
  create: createRequest,
};
