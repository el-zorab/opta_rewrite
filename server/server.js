'use strict';

const http = require('http');
const express = require('express');
const app = express();
const server = http.createServer(app);

const fs = require('fs');
const path = require('path');

app.use('/', (req, res, next) => {
    let date = new Date();
    console.log(`[${date.toLocaleDateString()} ${date.toLocaleTimeString()}] Request from ${req.ip}`);
    next();
});

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
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

function api_receive(encrypted) {
    let encrypted_len = encrypted.length;
    let aes_len = encrypted_len - 64 - 32;

    let iv = encrypted.substring(0, 32);
    let aes = encrypted.substring(32, 32 + aes_len);
    let hmac = encrypted.substring(32 + aes_len, encrypted_len);

    let computed_hmac = create_hmac(Buffer.from(iv + aes, 'hex'));

    if (hmac != computed_hmac) {
        console.log('HMAC doesnt match');
        return;
    }

    let decrypted = aes_decrypt_iv(aes, Buffer.from(iv, 'hex'));
    decrypted = Buffer.from(decrypted, 'hex');

    let counter = decrypted.readUint32LE(0);
    let timestamp = decrypted.readUint32LE(4);
    let opta_id = decrypted.readUint8(8);
    let request = decrypted.readUint8(9);
    let extra = decrypted.readUint8(10);

    console.log(aes);

    console.log(counter);
    console.log(timestamp);
    console.log(opta_id);
    console.log(request);
    console.log(extra);
}

function api_send(request, opta) {
    // let data = 
}

const crypto = require('crypto');

const AES_KEY = Buffer.from(fs.readFileSync(path.join(__dirname, 'aes_key.dat'), {encoding: 'utf-8'}), 'hex');
const HMAC_KEY = Buffer.from(fs.readFileSync(path.join(__dirname, 'hmac_key.dat'), {encoding: 'utf-8'}), 'hex');

function aes_decrypt_iv(input, iv) {
    let cipher = crypto.createDecipheriv('aes-256-cbc', AES_KEY, iv);
    let output = cipher.update(input, 'hex', 'hex');
    output += cipher.final('hex');
    return output;
  }

function create_hmac(input) {
    return crypto.createHmac('sha256', HMAC_KEY).update(input).digest('hex');
}
