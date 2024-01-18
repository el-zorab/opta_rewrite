#pragma once

#include <stdint.h>

void aes_decrypt(uint8_t in[], uint16_t in_len, uint8_t iv[], uint8_t out[], uint16_t *out_len);
void aes_encrypt(uint8_t in[], uint16_t in_len, uint8_t iv[], uint8_t out[], uint16_t *out_len);
