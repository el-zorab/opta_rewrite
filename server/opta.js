'use strict';

export const OPTA_COUNT = 3;
export const RELAYS_PER_OPTA = 4;

export const OPTA_IPS = [
    '192.168.11.177',
    '192.168.11.178',
    '192.168.11.179'
];

export const OPTA_PORT = 8088;

let relays = Array(OPTA_COUNT * RELAYS_PER_OPTA).fill(0);
let interrupts_state = 1;

export function get_relays() {
    return relays;
}

export function get_interrupts_state() {
    return interrupts_state;
}

export function get_opta_id_from_relay(relay) {
    return Math.floor(relay / RELAYS_PER_OPTA);
}

export function get_opta_relays(opta_id) {
    let opta_relays = 0;
    for (let i = 0; i < RELAYS_PER_OPTA; i++) {
        let current_relay = relays[opta_id * RELAYS_PER_OPTA + i];
        if (current_relay) {
            opta_relays |= (1 << (RELAYS_PER_OPTA - i - 1));
        }
    }
    return opta_relays;
}

export function set_relay(relay, state) {
    relays[relay] = state;
}

export function set_interrupts_state(state) {
    interrupts_state = state;
}
