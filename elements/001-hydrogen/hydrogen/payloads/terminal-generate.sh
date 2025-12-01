#!/usr/bin/env bash

# terminal-generate.sh
# xterm.js Payload Generator for Hydrogen Terminal
# This script downloads xterm.js from GitHub and creates terminal interface assets

# Change Log:
# Version 1.1.0 - Added copying of generated terminal files to tests/artifacts/terminal directory as payload-terminal.css and payload-terminal.html

set -e

# Display script information
echo "terminal-generate.sh version 1.1.0"
echo "xterm.js Payload Generator for Hydrogen Terminal"

# xterm.js versions to use (latest available)
readonly XTERM_JS_VERSION="latest"
readonly XTERM_ATTACH_VERSION="latest"
readonly XTERM_FIT_VERSION="latest"

XTERMJS_DIR="xtermjs"
readonly XTERMJS_VERSION_FILE="${XTERMJS_DIR}/xtermjs_version.txt"

# Function to download xterm.js from CDN
download_xtermjs() {
    echo "📥 Downloading xterm.js from JSDelivr CDN..."

    # Create target directory
    rm -rf "${XTERMJS_DIR}"
    mkdir -p "${XTERMJS_DIR}"

    echo "🔄 Downloading xterm.js core files..."
    # Download xterm.js core library
    curl -L "https://cdn.jsdelivr.net/npm/xterm@${XTERM_JS_VERSION}/lib/xterm.js" -o "${XTERMJS_DIR}/xterm.js"

    # Download xterm.js CSS
    curl -L "https://cdn.jsdelivr.net/npm/xterm@${XTERM_JS_VERSION}/css/xterm.css" -o "${XTERMJS_DIR}/xterm.css"

    echo "🧩 Downloading xterm.js addons..."
    # Download addons
    curl -L "https://cdn.jsdelivr.net/npm/@xterm/addon-attach@${XTERM_ATTACH_VERSION}/lib/addon-attach.js" -o "${XTERMJS_DIR}/xterm-addon-attach.js"

    curl -L "https://cdn.jsdelivr.net/npm/@xterm/addon-fit@${XTERM_FIT_VERSION}/lib/addon-fit.js" -o "${XTERMJS_DIR}/xterm-addon-fit.js"

    # Save version info
    {
        echo "xterm.js: ${XTERM_JS_VERSION}"
        echo "xterm-addon-attach: ${XTERM_ATTACH_VERSION}"
        echo "xterm-addon-fit: ${XTERM_FIT_VERSION}"
    } > "${XTERMJS_VERSION_FILE}"

    echo "📝 Version information saved to ${XTERMJS_VERSION_FILE}"
    echo "✅ xterm.js files downloaded successfully"
}

# Function to create terminal interface files
create_terminal_interface() {
    echo "🎨 Creating terminal interface files..."

    # Create terminal.html (renamed from index.html as preferred by user)
    cat > "${XTERMJS_DIR}/terminal.html" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Hydrogen Terminal</title>
    <link rel="stylesheet" href="xterm.css">
    <link rel="stylesheet" href="terminal.css">
    <style>
        body {
            margin: 0;
            padding: 0;
            background: #1e1e1e;
            font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
            color: #ffffff;
            overflow: hidden;
        }
        .header {
            background: #2d2d30;
            color: #cccccc;
            padding: 8px 16px;
            font-size: 12px;
            border-bottom: 1px solid #3e3e42;
        }
        .terminal-container {
            position: absolute;
            top: 40px;
            left: 0;
            right: 0;
            bottom: 0;
            background: #1e1e1e;
        }
        #terminal {
            width: 100%;
            height: 100%;
            padding: 8px;
        }
        .status {
            display: none;
            position: fixed;
            bottom: 16px;
            left: 16px;
            background: rgba(0, 0, 0, 0.7);
            color: #ffffff;
            padding: 8px 12px;
            border-radius: 4px;
            font-size: 11px;
            z-index: 1000;
        }
        .status.error {
            background: rgba(220, 53, 69, 0.9);
        }
    </style>
</head>
<body>
    <div class="header">
        <span id="header-title">Hydrogen Terminal</span>
        <span id="header-info" style="position:absolute;right:16px">[Disconnected]</span>
    </div>
    <div class="terminal-container">
        <div id="terminal"></div>
    </div>
    <div id="status" class="status"></div>

    <script src="xterm.js"></script>
    <script src="xterm-addon-attach.js"></script>
    <script src="xterm-addon-fit.js"></script>

    <script>
        // Initialize terminal
        const terminalElement = document.getElementById('terminal');
        const headerInfo = document.getElementById('header-info');
        const statusElement = document.getElementById('status');

        let term = null;
        let websocket = null;
        let isConnected = false;

        function showStatus(message, isError = false) {
            statusElement.textContent = message;
            statusElement.className = 'status' + (isError ? ' error' : '');
            statusElement.style.display = 'block';
            setTimeout(() => {
                statusElement.style.display = 'none';
            }, 3000);
        }

        function updateHeaderInfo() {
            headerInfo.textContent = isConnected ? `[Connected - ${term.cols}x${term.rows}]` : '[Disconnected]';
        }

        function initializeTerminal() {
            term = new Terminal({
                cursorBlink: true,
                cursorStyle: 'block',
                fontSize: 14,
                fontFamily: "'Fira Code', 'Cascadia Code', 'Monaco', 'Courier New', monospace",
                letterSpacing: 0.5,
                lineHeight: 1.2,
                theme: {
                    background: '#1e1e1e',
                    foreground: '#d4d4d4',
                    black: '#000000',
                    blue: '#569cd6',
                    brightBlack: '#000000',
                    brightBlue: '#569cd6',
                    brightCyan: '#4ec9b0',
                    brightGreen: '#6a9955',
                    brightMagenta: '#c586c0',
                    brightRed: '#f44747',
                    brightWhite: '#d4d4d4',
                    brightYellow: '#dcdcaa',
                    cyan: '#4ec9b0',
                    green: '#6a9955',
                    magenta: '#c586c0',
                    red: '#f44747',
                    white: '#d4d4d4',
                    yellow: '#dcdcaa'
                }
            });

            // Load addons
            const fitAddon = new FitAddon.FitAddon();
            term.loadAddon(fitAddon);

            // Open terminal
            term.open(terminalElement);

            // Fit to container
            fitAddon.fit();

            // Handle resize
            window.addEventListener('resize', () => {
                fitAddon.fit();
                if (websocket && websocket.readyState === WebSocket.OPEN) {
                    websocket.send(JSON.stringify({
                        type: 'resize',
                        cols: term.cols,
                        rows: term.rows
                    }));
                }
                updateHeaderInfo();
            });

            // Handle terminal input
            term.onData(data => {
                if (websocket && websocket.readyState === WebSocket.OPEN) {
                    websocket.send(JSON.stringify({
                        type: 'input',
                        data: data
                    }));
                }
            });

            term.writeln('\x1b[32mInitializing session...\x1b[0m');

            updateHeaderInfo();
        }

        function connectWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            // Connect to WebSocket server on port 5261
            const hostname = window.location.hostname;
            const wsPort = '5261';
            
            // Get WebSocket key from environment or use a default
            // In production, this should come from server-side configuration
            const wsKey = 'ABDEFGHIJKLMNOP';
            const wsUrl = `${protocol}//${hostname}:${wsPort}?key=${encodeURIComponent(wsKey)}`;

            showStatus('Connecting to terminal...');

            // Create WebSocket with terminal protocol
            websocket = new WebSocket(wsUrl, 'terminal');

            websocket.onopen = () => {
                isConnected = true;
                showStatus('Terminal connected successfully');
                updateHeaderInfo();

                // Send initial terminal size
                websocket.send(JSON.stringify({
                    type: 'resize',
                    cols: term.cols,
                    rows: term.rows
                }));
            };

            websocket.onmessage = (event) => {
                try {
                    const message = JSON.parse(event.data);
                    if (message.type === 'output') {
                        term.write(message.data);
                    }
                } catch (e) {
                    term.write(event.data);
                }
            };

            websocket.onclose = () => {
                isConnected = false;
                showStatus('Terminal disconnected', false);
                updateHeaderInfo();
                term.writeln('\r\n\x1b[31mTerminal disconnected\x1b[0m');
                term.writeln('$ ');
            };

            websocket.onerror = (error) => {
                isConnected = false;
                showStatus('Connection error', true);
                updateHeaderInfo();
                term.writeln(`\r\n\x1b[31mTerminal connection error: ${error}\x1b[0m\r\n$ `);
            };
        }

        // Initialize and connect
        try {
            initializeTerminal();
            setTimeout(connectWebSocket, 500);
        } catch (error) {
            showStatus(`Initialization error: ${error.message}`, true);
            console.error('Terminal initialization error:', error);
        }

        // Handle page visibility for reconnection
        document.addEventListener('visibilitychange', () => {
            if (document.hidden && websocket) {
                websocket.close();
            } else if (!document.hidden && !isConnected) {
                connectWebSocket();
            }
        });
    </script>
</body>
</html>
EOF

    # Create additional terminal CSS
    cat > "${XTERMJS_DIR}/terminal.css" << 'EOF'
/* Additional terminal-specific styling */
.terminal {
    background: transparent !important;
}

.terminal .xterm-rows {
    background: transparent !important;
}

.terminal .xterm-viewport {
    background: #1e1e1e !important;
}

.terminal .xterm-scroll-area {
    background: #1e1e1e !important;
}

/* Custom scrollbar styling */
.terminal .xterm-viewport::-webkit-scrollbar {
    width: 12px;
    background: #1e1e1e;
}

.terminal .xterm-viewport::-webkit-scrollbar-track {
    background: #2d2d30;
}

.terminal .xterm-viewport::-webkit-scrollbar-thumb {
    background: #3e3e42;
    border-radius: 6px;
}

.terminal .xterm-viewport::-webkit-scrollbar-thumb:hover {
    background: #569cd6;
}

/* Better font rendering */
.terminal {
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    font-variant-ligatures: none;
    text-rendering: optimizeSpeed;
}

/* Focus styling */
.terminal:focus {
    outline: none;
}

.terminal.focus .xterm-cursor {
    background-color: #d4d4d4 !important;
    color: #1e1e1e !important;
}

/* Selection styling */
.terminal .xterm-selection div {
    background-color: rgba(86, 156, 214, 0.5) !important;
}
EOF

    echo "✅ Terminal interface files created"
}

# Function to copy files to artifacts
copy_to_artifacts() {
    echo "📋 Copying terminal files to artifacts directory..."

    # Ensure the target directory exists
    mkdir -p "../tests/artifacts/terminal"

    # Copy terminal.css as payload-terminal.css
    cp "${XTERMJS_DIR}/terminal.css" "../tests/artifacts/terminal/payload-terminal.css"

    # Copy terminal.html as payload-terminal.html
    cp "${XTERMJS_DIR}/terminal.html" "../tests/artifacts/terminal/payload-terminal.html"

    echo "✅ Files copied to tests/artifacts/terminal/"
}

# Function to validate files
validate_files() {
    echo "🔍 Validating downloaded files..."

    local missing_files=()

    # Check for required files
    if [[ ! -f "${XTERMJS_DIR}/xterm.js" ]]; then
        missing_files+=("xterm.js")
    fi

    if [[ ! -f "${XTERMJS_DIR}/xterm.css" ]]; then
        missing_files+=("xterm.css")
    fi

    if [[ ! -f "${XTERMJS_DIR}/terminal.html" ]]; then
        missing_files+=("terminal.html")
    fi

    if [[ ! -f "${XTERMJS_DIR}/terminal.css" ]]; then
        missing_files+=("terminal.css")
    fi

    if [[ -f "${XTERMJS_DIR}/xterm-addon-attach.js" ]]; then
        echo "✅ Found xterm-addon-attach.js"
    else
        echo "⚠️  xterm-addon-attach.js not found (optional)"
    fi

    if [[ -f "${XTERMJS_DIR}/xterm-addon-fit.js" ]]; then
        echo "✅ Found xterm-addon-fit.js"
    else
        echo "⚠️  xterm-addon-fit.js not found (optional)"
    fi

    if [[ ${#missing_files[@]} -ne 0 ]]; then
        echo "❌ Missing required files: ${missing_files[*]}"
        return 1
    fi

    echo "✅ All required files present"
    return 0
}

# Main execution flow
main() {
    # Check for required dependencies
    if ! command -v curl >/dev/null 2>&1; then
        echo "❌ curl is required but not installed"
        exit 1
    fi

    # Execute workflow
    download_xtermjs
    create_terminal_interface
    validate_files
    copy_to_artifacts

    echo "🎉 Terminal payload generation completed successfully!"
    echo "📁 Files ready in: ${XTERMJS_DIR}/"
    echo "📋 Copies created in: tests/artifacts/terminal/"
}

# Execute main function
main "$@"
