'use strict';

import { exec } from 'node:child_process';
import { createCipheriv, createDecipheriv, createHmac, randomBytes, timingSafeEqual } from 'node:crypto';
import fs from 'node:fs';

const AES_KEY = Buffer.from(fs.readFileSync(new URL('./aes_key.dat', import.meta.url), { encoding: 'utf-8' }), 'hex');
const HMAC_KEY = Buffer.from(fs.readFileSync(new URL('./hmac_key.dat', import.meta.url), { encoding: 'utf-8' }), 'hex');

const IV_LEN      = 16;
const AES_MAX_LEN = 32;
const HMAC_LEN    = 32;
const ENCRYPTED_PAYLOAD_MAX_LEN = IV_LEN + AES_MAX_LEN + HMAC_LEN;

const OPTA_COUNT = 3;
const OPTA_IPS = [
    '192.168.11.177',
    '192.168.11.178',
    '192.168.11.179'
];
const OPTA_PORT = 8088;

const APIOptaRequestType = {
    UPDATE_RELAYS:             0,
    UPDATE_RELAYS_BY_USER_BTN: 1,
    UPDATE_INTERRUPTS_STATE:   2
};

const APIServerRequestType = {
    UPDATE_RELAYS:           0,
    UPDATE_INTERRUPTS_STATE: 1
};

const API_TIMESTAMP_MAX_DIFF = 5;

class APIOptaPayload {
    constructor(buf) {
        this.counter   = buf.readUInt32LE(0);
        this.timestamp = buf.readUInt32LE(4);
        this.opta_id   = buf.readUInt8(8);
        this.request   = buf.readUInt8(9);
        this.extra     = buf.readUInt8(10);
    }
}

class APIServerPayload {
    set_counter(counter) {
        this.counter = counter;
        return this;
    }

    set_timestamp(timestamp) {
        this.timestamp = timestamp;
        return this;
    }

    set_request(request) {
        this.request = request;
        return this;
    }

    set_extra(extra) {
        this.extra = extra;
        return this;
    }

    get_buffer() {
        let buf = Buffer.alloc(10);
        buf.writeUInt32LE(this.counter, 0);
        buf.writeUInt32LE(this.timestamp, 4);
        buf.writeUInt8(this.request, 8);
        buf.writeUInt8(this.extra, 9);
        return buf;
    }
}

let api_opta_reqs_counter   = Array(OPTA_COUNT).fill(0);
let api_server_reqs_counter = Array(OPTA_COUNT).fill(0);

export function api_handle_opta_request(encrypted_str, callback) {
    if (!encrypted_str.match(/^[0-9a-f]+$/)) {
        callback('Opta payload discarded (not a hex string)');
        return 0;
    }

    let encrypted = Buffer.from(encrypted_str, 'hex');
    let encrypted_len = encrypted.length;

    if (encrypted_len >= ENCRYPTED_PAYLOAD_MAX_LEN) {
        callback('Opta payload discarded (encrypted payload length exceeds maximum value)');
        return 0;
    }

    let iv_start = 0;
    let iv_end   = IV_LEN;
    let ciphertext_start = iv_end;
    let ciphertext_end   = encrypted_len - HMAC_LEN;
    let hmac_start = ciphertext_end;
    let hmac_end   = encrypted_len;

    if ((ciphertext_end - ciphertext_start) % 16 != 0) {
        callback('Opta payload discarded (ciphertext length is not a multiple of 16)');
        return 0;
    }

    let iv            = encrypted.subarray(iv_start, iv_end);
    let ciphertext    = encrypted.subarray(ciphertext_start, ciphertext_end);
    let received_hmac = encrypted.subarray(hmac_start, hmac_end);

    let computed_hmac = create_hmac(Buffer.concat([iv, ciphertext]));

    if (!timingSafeEqual(received_hmac, computed_hmac)) {
        callback('INVALID HMAC');
        return 0;
    }

    let decrypted_buffer = aes_decrypt_iv(ciphertext, iv);
    let payload = new APIOptaPayload(decrypted_buffer);

    if (payload.counter < api_opta_reqs_counter[payload.opta_id]) {
        callback(`INVALID API COUNTER Opta#${payload.opta_id}, received ${payload.counter}, expected < ${api_opta_reqs_counter[payload.opta_id]}`);
        return 0;
    }
    
    let current_timestamp = get_current_timestamp();
    if (Math.abs(payload.timestamp - current_timestamp) > API_TIMESTAMP_MAX_DIFF) {
        callback(`INVALID TIMESTAMP Opta#${payload.opta_id}, received ${payload.timestamp}, expected ${current_timestamp}±${API_TIMESTAMP_MAX_DIFF}`);
        return 0;
    }

    api_opta_reqs_counter[payload.opta_id]++;

    callback(null);

    if (payload.request == APIOptaRequestType.UPDATE_RELAYS || payload.request == APIOptaRequestType.UPDATE_RELAYS_BY_USER_BTN) {
        sockets_update_relays(payload.extra)
    } else if (payload.request == APIOptaRequestType.UPDATE_INTERRUPTS_STATE) {
        sockets_update_interrupts_state(payload.extra);
    }
}

function api_create_server_payload(request, extra) {
    let payload = new APIServerPayload()
                        .set_counter(api_server_reqs_counter)
                        .set_timestamp(get_current_timestamp())
                        .set_request(request);

    if (request == APIServerRequestType.UPDATE_RELAYS) {
        payload.set_extra(extra);
    } else if (request == APIServerRequestType.UPDATE_INTERRUPTS_STATE) {
        payload.set_extra(extra);
    }

    return payload.get_buffer();
}

function api_send_server_request(payload_header, opta_id) {
    exec(`curl --connect-timeout 5 -H "Payload: ${payload_header}" http://${OPTA_IPS[opta_id]}:${OPTA_PORT}/`, error => {
        if (error != null) {
            console.log(`cURL Error on Opta#${opta_id}`);
        } else {
            api_server_reqs_counter[opta_id]++;
        }
    });
}

export function api_make_server_request(request, extra, opta_id) {
    let raw = api_create_server_payload(request, extra);

    let iv = randomBytes(IV_LEN);
    let ciphertext = aes_encrypt_iv(raw, iv);
    let hmac = create_hmac(Buffer.concat([iv, ciphertext]));

    let encrypted_payload_hex = Buffer.concat([iv, ciphertext, hmac]).toString('hex');

    api_send_server_request(encrypted_payload_hex, opta_id);
}

function aes_decrypt_iv(input, iv) {
    let cipher = createDecipheriv('aes-256-cbc', AES_KEY, iv);
    return Buffer.concat([
        cipher.update(input),
        cipher.final()
    ]);
}

function aes_encrypt_iv(input, iv) {
    let cipher = createCipheriv('aes-256-cbc', AES_KEY, iv);
    return Buffer.concat([
        cipher.update(input),
        cipher.final()
    ]);
}

function create_hmac(input) {
    return createHmac('sha256', HMAC_KEY)
            .update(input)
            .digest();
}

function get_current_timestamp() {
    return Math.floor(+new Date() / 1000);
}
