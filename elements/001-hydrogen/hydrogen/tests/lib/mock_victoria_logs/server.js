#!/usr/bin/env node
/*
 * Tiny mock VictoriaLogs insert endpoint.
 *
 * Accepts POST /insert/jsonline (and any path) with application/stream+json
 * bodies and responds 204 No Content so Hydrogen's victoria_logs client
 * treats the flush as success.
 *
 * Usage:
 *   node tests/lib/mock_victoria_logs/server.js [port] [receipt_file]
 *   port 0 (default) = ephemeral; prints "READY <port>" when listening.
 *   Optional receipt_file: append one line per accepted POST (byte count).
 *
 * SIGTERM / SIGINT: clean shutdown.
 */

import http from 'node:http';
import fs from 'node:fs';
import process from 'node:process';

const portArg = parseInt(process.argv[2], 10);
const port = Number.isFinite(portArg) ? portArg : 0;
const receiptFile = process.argv[3] || '';

let postCount = 0;
let totalBytes = 0;

function readBody(req) {
    return new Promise((resolve, reject) => {
        const chunks = [];
        req.on('data', (c) => chunks.push(c));
        req.on('end', () => resolve(Buffer.concat(chunks)));
        req.on('error', reject);
    });
}

function recordReceipt(byteLen) {
    postCount += 1;
    totalBytes += byteLen;
    if (receiptFile) {
        try {
            fs.appendFileSync(receiptFile, `${Date.now()} ${byteLen}\n`);
        } catch {
            /* ignore receipt write failures */
        }
    }
}

const server = http.createServer(async (req, res) => {
    const url = req.url || '/';

    if (req.method === 'GET' && (url === '/health' || url === '/')) {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end(`ok posts=${postCount} bytes=${totalBytes}`);
        return;
    }

    if (req.method === 'POST') {
        let body = Buffer.alloc(0);
        try {
            body = await readBody(req);
        } catch {
            res.writeHead(400, { 'Content-Type': 'text/plain' });
            res.end('bad body');
            return;
        }
        recordReceipt(body.length);
        res.writeHead(204, {
            'Content-Length': '0',
            Connection: 'close',
        });
        res.end();
        return;
    }

    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end('not found');
});

server.listen(port, '127.0.0.1', () => {
    const addr = server.address();
    const bound = typeof addr === 'object' && addr ? addr.port : port;
    process.stdout.write(`READY ${bound}\n`);
});

function shutdown() {
    server.close(() => process.exit(0));
    setTimeout(() => process.exit(0), 1000).unref();
}

process.on('SIGTERM', shutdown);
process.on('SIGINT', shutdown);
