#!/usr/bin/env node
/*
 * Tiny mock OpenAI-compatible chat completions server.
 *
 * Used by blackbox auth_chat tests so Hydrogen can proxy chat requests
 * without spending real LLM credits. Node built-ins only.
 *
 * Endpoints:
 *   GET  /health
 *   POST /v1/chat/completions   (OpenAI-compatible)
 *   POST /                      (same handler; some engines point here)
 *
 * Usage:
 *   node tests/lib/mock_llm/server.js [port]   (default 15901)
 *
 * Prints "READY <port>" once listening. SIGTERM clean shutdown.
 */

import http from 'node:http';
import process from 'node:process';

const port = parseInt(process.argv[2], 10) || 15901;

function readBody(req) {
    return new Promise((resolve, reject) => {
        const chunks = [];
        req.on('data', (c) => chunks.push(c));
        req.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
        req.on('error', reject);
    });
}

function sendJson(res, status, obj) {
    const body = JSON.stringify(obj);
    res.writeHead(status, {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(body),
    });
    res.end(body);
}

function openaiResponse(model, content) {
    return {
        id: 'chatcmpl-mock-1',
        object: 'chat.completion',
        created: Math.floor(Date.now() / 1000),
        model: model || 'mock-gpt',
        choices: [
            {
                index: 0,
                message: {
                    role: 'assistant',
                    content: content || 'Hello from mock LLM.',
                },
                finish_reason: 'stop',
            },
        ],
        usage: {
            prompt_tokens: 8,
            completion_tokens: 6,
            total_tokens: 14,
        },
    };
}

const server = http.createServer(async (req, res) => {
    const url = req.url || '/';

    if (req.method === 'GET' && (url === '/health' || url === '/')) {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('ok');
        return;
    }

    if (req.method === 'POST' &&
        (url.startsWith('/v1/chat/completions') || url === '/' || url.startsWith('/chat'))) {
        let raw = '';
        try {
            raw = await readBody(req);
        } catch {
            sendJson(res, 400, { error: { message: 'failed to read body' } });
            return;
        }

        let parsed = {};
        try {
            parsed = raw ? JSON.parse(raw) : {};
        } catch {
            // Health checks and clients may still get a usable 200 so connectivity works.
            sendJson(res, 200, openaiResponse('mock-gpt', 'parse-fallback'));
            return;
        }

        // Optional mode flags via model name prefix for negative-path testing.
        const model = typeof parsed.model === 'string' ? parsed.model : 'mock-gpt';
        if (model.startsWith('fail-')) {
            sendJson(res, 502, { error: { message: 'mock upstream failure' } });
            return;
        }
        if (model.startsWith('badjson-')) {
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end('not-valid-json{{{');
            return;
        }

        // Echo last user message content when present.
        let content = 'Hello from mock LLM.';
        const messages = Array.isArray(parsed.messages) ? parsed.messages : [];
        for (let i = messages.length - 1; i >= 0; i--) {
            const m = messages[i];
            if (m && m.role === 'user') {
                if (typeof m.content === 'string' && m.content.length > 0) {
                    content = `Echo: ${m.content}`;
                } else if (Array.isArray(m.content)) {
                    content = 'Echo: multimodal';
                }
                break;
            }
        }

        // Streaming mode: emit Server-Sent-Events so the chat proxy's CURL
        // write callback (proxy_mc.c) actually receives chunked lines. The
        // proxy parses SSE and queues chunks for the WebSocket thread.
        if (parsed.stream === true) {
            const chunks = content.match(/.{1,8}/g) || [content];
            res.writeHead(200, {
                'Content-Type': 'text/event-stream',
                'Cache-Control': 'no-cache',
                'Connection': 'keep-alive',
            });
            let index = 0;
            const sendNext = () => {
                if (index >= chunks.length) {
                    res.write(`data: ${JSON.stringify({ choices: [{ delta: { content: '' }, finish_reason: 'stop' }] })}\n\n`);
                    res.write('data: [DONE]\n\n');
                    res.end();
                    return;
                }
                const piece = chunks[index++];
                res.write(`data: ${JSON.stringify({ choices: [{ delta: { content: piece } }] })}\n\n`);
                setTimeout(sendNext, 5);
            };
            sendNext();
            return;
        }

        sendJson(res, 200, openaiResponse(model, content));
        return;
    }

    sendJson(res, 404, { error: { message: `not found: ${req.method} ${url}` } });
});

server.listen(port, '127.0.0.1', () => {
    process.stdout.write(`READY ${port}\n`);
});

function shutdown() {
    server.close(() => process.exit(0));
    setTimeout(() => process.exit(0), 1000).unref();
}

process.on('SIGTERM', shutdown);
process.on('SIGINT', shutdown);
