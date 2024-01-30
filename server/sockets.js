'use strict';

import { Server } from 'socket.io';
import { APIServerRequestType, OPTA_COUNT, RELAYS_PER_OPTA, api_make_server_request } from './api.js';

let io;
let relays = Array(OPTA_COUNT * RELAYS_PER_OPTA).fill(0);
let interrupts_state = 1;

export function sockets_init(server) {
    io = new Server(server);
}

export function sockets_register() {
    io.on('connection', socket => {
        console.log('socket connected');

        socket.emit('set_new_relays', relays);
        socket.emit('set_new_interrupts_state', interrupts_state);

        socket.on('relay_server_change', data => {
            relays[data.relay] = data.state
            socket.broadcast.emit('set_new_relays', relays);

            let opta_id = Math.floor(data.relay / RELAYS_PER_OPTA);
            let extra = 0;
            for (let i = 0; i < RELAYS_PER_OPTA; i++) {
                let current_relay = relays[opta_id * RELAYS_PER_OPTA + i];
                if (current_relay) {
                    extra |= (1 << (RELAYS_PER_OPTA - i - 1));
                }
            }

            api_make_server_request(APIServerRequestType.UPDATE_RELAYS, extra, opta_id);
        });

        socket.on('interrupts_state_server_change', state => {
            interrupts_state = state;
            socket.broadcast.emit('set_new_interrupts_state', interrupts_state);
        });
    });
}