#pragma once

#include <stdint.h>
#include "lib/sha256.h"

static const uint16_t IV_LEN = 16;
static const uint16_t AES_MAX_LEN = 32;
static const uint16_t HMAC_LEN = SHA256_HASH_SIZE;
static const uint16_t ENCRYPTED_PAYLOAD_MAX_LEN = IV_LEN + AES_MAX_LEN + HMAC_LEN;
static const uint16_t ENCRYPTED_PAYLOAD_MAX_STR_LEN = 2 * ENCRYPTED_PAYLOAD_MAX_LEN + 1;

uint8_t decrypt(uint8_t *encrypted, uint16_t encrypted_len, uint8_t *raw, uint16_t *raw_len);
void encrypt(uint8_t *plaintext, uint16_t plaintext_len, uint8_t *encrypted, uint16_t *encrypted_len);
