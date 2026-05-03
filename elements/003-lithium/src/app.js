// Console capture MUST be first - captures all subsequent console output
import './core/console-capture.js';

// Core styles - imported first to establish CSS cascade layers
import './styles/base.css';
import './styles/vendor-overlayscrollbars.css';
import './styles/vendor-fixes.css';
import './styles/scrollbars.css';
import './styles/vendor-fixes-overlay-scrollbars.css';
import './styles/vendor-fixes-tabulator.css';
import './styles/vendor-fixes-codemirror.css';
import './styles/layout.css';
import './styles/components.css';
import './styles/transitions.css';
import './styles/toast.css';
import './styles/tooltip.css';

// Camera popout styles (global component)
import './managers/profile-manager/pages/photo/camera-popout.css';

import { LithiumApp } from './app/lithium-app.js';
import { log, Subsystems, Status, getRawLog, getDisplayLog, getRecentLogs, getCounter, getSessionId, setConsoleLogging, flush, getArchivedSessions, removeArchivedSession } from './core/log.js';
import { tip, untip, initTooltips } from './core/tooltip-api.js';

let app = null;

document.addEventListener('DOMContentLoaded', async () => {
  app = new LithiumApp();
  window.lithiumApp = app;

  window.lithiumLogs = {
    get raw() { return getRawLog(); },
    get display() { return getDisplayLog(); },
    recent(n = 50) { return getRecentLogs(n); },
    print(n = 50) { getRecentLogs(n).forEach(line => console.log(line)); },
    get count() { return getCounter(); },
    get sessionId() { return getSessionId(); },
    setConsoleLogging,
    flush,
    get archived() { return getArchivedSessions(); },
    removeArchived: removeArchivedSession,
  };

  window.lithiumTip = { tip, untip, initTooltips };

  await app.init();
});

export { LithiumApp };
export default app;
