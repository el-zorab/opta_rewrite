#pragma once

#include <stdint.h>

static const uint8_t RELAY_COUNT = 4;

void switches_attach_interrupts();
void switches_detach_interrupts();
void switches_press_listener();
uint8_t switches_get_interrupts_state();
void switches_set_interrupts_state(uint8_t state);

void user_btn_attach_interrupt();
void user_btn_check_for_unlatch();
void user_btn_press_listener();

uint8_t get_relay_state(uint8_t relay);
void set_relay_state(uint8_t relay, uint8_t state);
void set_relays_states(uint8_t states[]);
