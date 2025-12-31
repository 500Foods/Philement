// Lithium Logger - Logging utility module

class Logger {
  constructor(context = 'Lithium') {
    this.context = context;
    this.levels = {
      debug: 0,
      info: 1,
      warn: 2,
      error: 3,
      success: 4
    };
    this.currentLevel = 1; // Default to info
  }

  debug(message, ...args) {
    this.log('debug', message, ...args);
  }

  info(message, ...args) {
    this.log('info', message, ...args);
  }

  warn(message, ...args) {
    this.log('warn', message, ...args);
  }

  error(message, ...args) {
    this.log('error', message, ...args);
  }

  success(message, ...args) {
    this.log('success', message, ...args);
  }

  log(level, message, ...args) {
    if (this.levels[level] < this.currentLevel) return;

    const timestamp = new Date().toISOString();
    const formattedMessage = `[${timestamp}] [${this.context}] ${message}`;

    const styles = {
      debug: 'color: #9E9E9E',
      info: 'color: #2196F3',
      warn: 'color: #FF9800',
      error: 'color: #F44336',
      success: 'color: #4CAF50'
    };

    console.log(`%c${level.toUpperCase()}`, styles[level], formattedMessage, ...args);
  }

  setLevel(level) {
    if (this.levels.hasOwnProperty(level)) {
      this.currentLevel = this.levels[level];
    }
  }
}

export { Logger };