#include "api.h"
#include "hardware.h"

// const int RELAYS[] = { D0, D1, D2, D3 };
// const int LEDS[]   = { LED_D0, LED_D1, LED_D2, LED_D3 };

static const uint8_t BUTTON_COUNT = 3;

static const unsigned long BUTTON_DEBOUNCE_DELAY = 150;
static const unsigned long RELAY_UNLATCH_TIME    = 1500;

static uint8_t relay_states[RELAY_COUNT];
static uint8_t relay_latched_states[RELAY_COUNT];
static uint8_t relays_latching = 0;

static volatile uint8_t button_pressed[BUTTON_COUNT];
static unsigned long    button_last_debounce[BUTTON_COUNT];

static uint8_t interrupts_state = 1;

void set_relay_state(uint8_t relay, uint8_t state) {
    relay_states[relay] = state;
    // digitalWrite(LEDS[relay], state);
    // digitalWrite(RELAYS[relay], state);
}

void set_relays_states(uint8_t states[]) {
    for (int relay = 0; relay < RELAY_COUNT; relay++) {
        set_relay_state(relay, states[relay]);
    }
}

void set_interrupts_state(uint8_t state) {
    if (interrupts_state == state) {
        return;
    }

    interrupts_state = state;

    if (interrupts_state == 0) {
        // detach_button_interrupts();
    } else if (interrupts_state == 1) {
        // attach_button_interrupts();
    }

}

void buttons_press_listener() {
  for (int i = 0; i < 2; i++) {
    if (!button_pressed[i]) continue;
    button_pressed[i] = 0;

    if (millis() - button_last_debounce[i] < BUTTON_DEBOUNCE_DELAY) continue;
    button_last_debounce[i] = millis();

    uint8_t a, b;
    a = relay_states[2 * i];
    b = relay_states[2 * i + 1];

    if (a == 0 && b == 0) {
      a = 1;
      b = 0;
    } else if (a == 1 && b == 0) {
      a = 1;
      b = 1;
    } else if (a == 1 && b == 1) {
      a = 0;
      b = 1;
    } else if (a == 0 && b == 1) {
      a = 0;
      b = 0;
    }

    set_relay_state(2 * i,     a);
    set_relay_state(2 * i + 1, b);

    api_make_opta_request(API_OPTA_REQ_UPDATE_RELAYS);
  }
}

void master_button_press_listener() {
  if (!button_pressed[2]) return;
  button_pressed[2] = 0;

  if (millis() - button_last_debounce[2] < BUTTON_DEBOUNCE_DELAY) return;
  button_last_debounce[2] = millis();

  if (!interrupts_state) {
    set_interrupts_state(1);
    api_make_opta_request(API_OPTA_REQ_UPDATE_INTERRUPTS_STATE);
    return;
  }

  if (!relays_latching) {
    relays_latching = 1;
    detach_button_interrupts(); // ??

    for (int i = 0; i < 4; i++) {
      relay_latched_states[i] = relay_states[i];
    }
  }

  uint8_t counter = relay_latched_states[0] << 3 | relay_latched_states[1] << 2 | relay_latched_states[2] << 1 | relay_latched_states[3];
  counter = (counter + 1) & 15;

  relay_latched_states[0] = (counter >> 3) & 1;
  relay_latched_states[1] = (counter >> 2) & 1;
  relay_latched_states[2] = (counter >> 1) & 1;
  relay_latched_states[3] = counter & 1;

  for (int i = 0; i < 4; i++) {
    // digitalWrite(LEDS[i], relay_latched_states[i]);
  }
}

void master_button_check_for_unlatch() {
  if (relays_latching && millis() - button_last_debounce[2] > RELAY_UNLATCH_TIME) {
    set_relays_states(relay_latched_states);
    relays_latching = 1;

    attach_button_interrupts();

    api_make_opta_request(API_OPTA_REQ_UPDATE_RELAYS);
  }
}

void attach_button_interrupts() {
//   attachInterrupt(digitalPinToInterrupt(A0), button0_isr, RISING);
//   attachInterrupt(digitalPinToInterrupt(A1), button1_isr, RISING);
//   digitalWrite(LEDR, LOW);
}

void detach_button_interrupts() {
//   detachInterrupt(digitalPinToInterrupt(A0));
//   detachInterrupt(digitalPinToInterrupt(A1));
//   digitalWrite(LEDR, HIGH);
}

static void button0_isr() {
  button_pressed[0] = 1;
}

static void button1_isr() {
  button_pressed[1] = 1;
}

static void master_button_isr() {
  button_pressed[2] = 1;
}
