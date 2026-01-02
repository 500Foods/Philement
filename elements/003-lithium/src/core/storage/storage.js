// Lithium Storage - Data persistence module
// Handles IndexedDB and localStorage operations

class Storage {
  constructor() {
    this.dbName = 'LithiumDB';
    this.version = 1;
    this.db = null;
    this.isInitialized = false;
  }

  async init() {
    try {
      this.db = await this.openDB();
      this.isInitialized = true;
      console.log('Storage module initialized');
      return true;
    } catch (error) {
      console.error('Failed to initialize storage:', error);
      return false;
    }
  }

  openDB() {
    return new Promise((resolve, reject) => {
      const request = indexedDB.open(this.dbName, this.version);

      request.onerror = () => reject(request.error);
      request.onsuccess = () => resolve(request.result);

      request.onupgradeneeded = (event) => {
        const db = event.target.result;

        // Create object stores if they don't exist
        if (!db.objectStoreNames.contains('keyValue')) {
          db.createObjectStore('keyValue');
        }

        if (!db.objectStoreNames.contains('appData')) {
          const appDataStore = db.createObjectStore('appData', { keyPath: 'id' });
          appDataStore.createIndex('type', 'type', { unique: false });
        }
      };
    });
  }

  // Key-Value storage operations
  async set(key, value) {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['keyValue'], 'readwrite');
      const store = transaction.objectStore('keyValue');
      const request = store.put(value, key);

      request.onsuccess = () => resolve(true);
      request.onerror = () => reject(request.error);
    });
  }

  async get(key) {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['keyValue'], 'readonly');
      const store = transaction.objectStore('keyValue');
      const request = store.get(key);

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  async remove(key) {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['keyValue'], 'readwrite');
      const store = transaction.objectStore('keyValue');
      const request = store.delete(key);

      request.onsuccess = () => resolve(true);
      request.onerror = () => reject(request.error);
    });
  }

  // App data operations
  async saveAppData(data) {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['appData'], 'readwrite');
      const store = transaction.objectStore('appData');
      const request = store.put(data);

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  async getAppData(id) {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['appData'], 'readonly');
      const store = transaction.objectStore('appData');
      const request = store.get(id);

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  async getAllAppData(type = null) {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['appData'], 'readonly');
      const store = transaction.objectStore('appData');
      const request = type ?
        store.index('type').getAll(type) :
        store.getAll();

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  // localStorage fallback methods
  setLocalStorage(key, value) {
    try {
      localStorage.setItem(key, JSON.stringify(value));
      return true;
    } catch (error) {
      console.error('localStorage set failed:', error);
      return false;
    }
  }

  getLocalStorage(key) {
    try {
      const item = localStorage.getItem(key);
      return item ? JSON.parse(item) : null;
    } catch (error) {
      console.error('localStorage get failed:', error);
      return null;
    }
  }

  removeLocalStorage(key) {
    try {
      localStorage.removeItem(key);
      return true;
    } catch (error) {
      console.error('localStorage remove failed:', error);
      return false;
    }
  }

  // Utility methods
  async clear() {
    if (!this.isInitialized) await this.init();

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction(['keyValue', 'appData'], 'readwrite');

      const keyValueStore = transaction.objectStore('keyValue');
      const appDataStore = transaction.objectStore('appData');

      const requests = [
        new Promise((res, rej) => {
          const req = keyValueStore.clear();
          req.onsuccess = () => res(true);
          req.onerror = () => rej(req.error);
        }),
        new Promise((res, rej) => {
          const req = appDataStore.clear();
          req.onsuccess = () => res(true);
          req.onerror = () => rej(req.error);
        })
      ];

      Promise.all(requests)
        .then(() => resolve(true))
        .catch(reject);
    });
  }
}

export { Storage };