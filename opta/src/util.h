#pragma once

#include <stdint.h>

void bytes_to_hex_str(uint8_t in[], uint16_t byte_len, char out[]);
void hex_str_to_bytes(char in[], uint16_t byte_len, uint8_t out[]);
