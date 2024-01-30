'use strict';

import { createServer } from 'node:http';
import express from 'express';
import { fileURLToPath } from 'node:url';
import { api_handle_opta_request } from './api.js';
import { log } from './log.js';
import { sockets_init, sockets_register } from './sockets.js';

const app = express();
const server = createServer(app);

let request_id = 0;

app.use('/', (req, res, next) => {
    req.id = request_id++;
    log(`REQUEST INCOMING from ${req.ip} with ID #${req.id}`);
    next();
});

app.get('/', (req, res) => {
    console.log(req.headers['payload']);
    res.sendFile(fileURLToPath(new URL('./index.html', import.meta.url)));
});

app.get('/api', (req, res) => {
    let payload = req.headers['payload'];
    if (payload != null) {
        api_handle_opta_request(payload, error => {
            if (error != null) {
                log(`REQUEST #${req.id} INVALID: ${error}`);
                res.sendStatus(401);
            } else {
                res.sendStatus(200);
            }
        });
    } else {
        log('Received API request with null header');
        res.sendStatus(401);
    }
});

sockets_init(server);
sockets_register();

server.listen(80, () => {
    log('express listening');
});

// api_handle_opta_request('69c4bdff8f4533e500b9d004e7f0cfcbabad44187907849416f90bd9f08bb70c0959493150b8c00f23453f4e82e6a10b79ccf5f6e57493717a796997eb2fdab8');
