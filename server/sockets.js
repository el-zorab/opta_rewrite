'use strict';

import { Server } from 'socket.io';
import { APIServerRequestType, api_make_server_request } from './api.js';
import { log } from './log.js'
import { OPTA_COUNT, get_interrupts_state, get_opta_id_from_relay, get_opta_relays, get_relays, set_interrupts_state, set_relay } from './opta.js';

let io;

export function sockets_init(server) {
    io = new Server(server);
}

export function sockets_register() {
    io.on('connection', socket => {
        log('SOCKET CONNECTED');

        socket.emit('set_relays', get_relays());
        socket.emit('set_interrupts_state', get_interrupts_state());

        socket.on('relay_server_change', data => {
            set_relay(data.relay, data.state);

            socket.broadcast.emit('set_relays', get_relays());

            let opta_id = get_opta_id_from_relay(data.relay);
            let opta_relays = get_opta_relays(opta_id);

            api_make_server_request(APIServerRequestType.UPDATE_RELAYS, opta_relays, opta_id);
        });

        socket.on('interrupts_state_server_change', state => {
            set_interrupts_state(state);

            socket.broadcast.emit('set_interrupts_state', state);

            for (let i = 0; i < OPTA_COUNT; i++) {
                api_make_server_request(APIServerRequestType.UPDATE_INTERRUPTS_STATE, state, i);
            }
        });
    });
}

export function sockets_set_opta_relays(opta_id, opta_relays) {
    for (let i = 0; i < RELAYS_PER_OPTA; i++) {
        set_relay(opta_id * RELAYS_PER_OPTA + i, (opta_relays >> (RELAYS_PER_OPTA - i - 1)) & 1);
    }

    io.emit('set_relays', get_relays());
}

export function sockets_set_interrupts_state(state) {
    set_interrupts_state(state);

    io.emit('set_interrupts_state', state);

    for (let i = 0; i < OPTA_COUNT; i++) {
        api_make_server_request(APIServerRequestType.UPDATE_INTERRUPTS_STATE, state, i);
    }
}
