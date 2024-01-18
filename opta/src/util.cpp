#include "util.h"

void bytes_to_hex_str(uint8_t in[], uint16_t in_len, char out[]) {
  for (int i = 0; i < in_len; i++) {
    uint8_t high_nibble = in[i] >> 4;
    uint8_t low_nibble = in[i] & 15;

    if (high_nibble < 10) {
      high_nibble = high_nibble + '0';
    } else {
      high_nibble = high_nibble + 'a' - 10;
    }

    if (low_nibble < 10) {
      low_nibble = low_nibble + '0';
    } else {
      low_nibble = low_nibble + 'a' - 10;
    }

    out[2 * i] = high_nibble;
    out[2 * i + 1] = low_nibble;
  }

  out[2 * in_len] = 0;
}

void hex_str_to_bytes(char in[], uint16_t in_len, uint8_t out[]) {
  for (int i = 0; i < in_len / 2; i++) {
    uint8_t high_nibble = in[2 * i];
    uint8_t low_nibble  = in[2 * i + 1];
    
    if (high_nibble <= '9') {
      high_nibble = high_nibble - '0';
    } else {
      high_nibble = high_nibble - 'a' + 10;
    }

    if (low_nibble <= '9') {
      low_nibble = low_nibble - '0';
    } else {
      low_nibble = low_nibble - 'a' + 10;
    }

    out[i] = (high_nibble << 4) | low_nibble;
  }
}
