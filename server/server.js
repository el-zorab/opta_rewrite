'use strict';

import { createServer } from 'node:http';
import express from 'express';
import { api_handle_opta_request } from './api.js';

const app = express();
const server = createServer(app);

app.use('/', (req, res, next) => {
    let date = new Date();
    console.log(`[${date.toLocaleDateString()} ${date.toLocaleTimeString()}] Request from ${req.ip}`);
    next();
});

app.get('/', (req, res) => {
    res.sendFile(join(__dirname, 'index.html'));
});

app.get('/api', (req, res) => {
    let payload = req.headers['payload'];
    if (payload != null) {
        api_receive(payload);
        res.sendStatus(200);
    } else {
        console.log(`Received API request with null header`);
        res.sendStatus(401);
    }
});

server.listen(80, () => {
    console.log('express listening');
});
