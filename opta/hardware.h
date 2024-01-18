#pragma once

#include <stdint.h>

static const uint8_t RELAY_COUNT = 4;

void attach_button_interrupts();
void detach_button_interrupts();

void attach_master_button_interrupt();

void buttons_press_listener();
void master_button_press_listener();
void master_button_check_for_unlatch();

void set_relay_state(uint8_t relay, uint8_t state);
void set_relays_states(uint8_t states[]);
void set_interrupts_state(uint8_t state);
