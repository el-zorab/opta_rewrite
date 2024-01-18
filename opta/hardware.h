#pragma once

#include <stdint.h>

void set_relay_state(uint8_t relay, uint8_t state);
void set_relays_states(uint8_t states);
void set_interrupts_state(uint8_t state);
