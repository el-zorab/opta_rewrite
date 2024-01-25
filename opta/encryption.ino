#include <stdlib.h>
#include "aes_impl.h"
#include "encryption.h"
#include "hmac_impl.h"

uint8_t decrypt(uint8_t *encrypted, uint16_t encrypted_len, uint8_t *raw, uint16_t *raw_len) {
    uint8_t iv[IV_LEN];
    uint8_t ciphertext[AES_MAX_LEN];
    uint8_t received_hmac[HMAC_LEN];
    uint8_t computed_hmac[HMAC_LEN];

    uint16_t iv_start = 0;
    uint16_t iv_end   = IV_LEN;
    uint16_t ciphertext_start = iv_end;
    uint16_t ciphertext_end   = encrypted_len - HMAC_LEN;
    uint16_t hmac_start = ciphertext_end;
    uint16_t hmac_end   = encrypted_len;

    uint16_t ciphertext_len = ciphertext_end - ciphertext_start;

    for (int i = 0; i < IV_LEN; i++) {
        iv[i] = encrypted[i + iv_start];
        Serial.print(iv[i], HEX);
        Serial.print(" ");        
    }
    Serial.println();

    for (int i = 0; i < ciphertext_len; i++) {
        ciphertext[i] = encrypted[i + ciphertext_start];
        Serial.print(ciphertext[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    for (int i = 0; i < HMAC_LEN; i++) {
        received_hmac[i] = encrypted[i + hmac_start];
        Serial.print(received_hmac[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    hmac_sha256_wrapper(encrypted, ciphertext_end, computed_hmac, HMAC_LEN);

    for (int i = 0; i < HMAC_LEN; i++) {
        if (received_hmac[i] != computed_hmac[i]) {
            Serial.println("Server payload discarded (invalid HMAC)");
            return 0;
        }
    }

    aes_decrypt(encrypted + ciphertext_start, ciphertext_len, iv, raw, raw_len);

    return 1;
}

void encrypt(uint8_t *raw, uint16_t raw_len, uint8_t *encrypted, uint16_t *encrypted_len) {
    uint8_t iv[IV_LEN] = {0x69, 0xc4, 0xbd, 0xff, 0x8f, 0x45, 0x33, 0xe5, 0x00, 0xb9, 0xd0, 0x04, 0xe7, 0xf0, 0xcf, 0xcb};
    uint8_t ciphertext[AES_MAX_LEN];
    uint8_t hmac[HMAC_LEN];

    uint16_t ciphertext_len;

    // for (uint16_t i = 0; i < IV_LEN; i++) {
    //     iv[i] = rand();
    // }

    aes_encrypt(raw, raw_len, iv, ciphertext, &ciphertext_len);

    uint16_t iv_start = 0;
    uint16_t iv_end   = IV_LEN;
    uint16_t ciphertext_start = iv_end;
    uint16_t ciphertext_end   = ciphertext_start + ciphertext_len;
    uint16_t hmac_start = ciphertext_end;
    uint16_t hmac_end   = hmac_start + HMAC_LEN;

    for (uint16_t i = 0; i < IV_LEN; i++) {
        encrypted[i + iv_start] = iv[i];
    }

    for (uint16_t i = 0; i < ciphertext_len; i++) {
        encrypted[i + ciphertext_start] = ciphertext[i];
    }

    hmac_sha256_wrapper(encrypted, ciphertext_end, hmac, HMAC_LEN);

    for (uint16_t i = 0; i < HMAC_LEN; i++) {
        encrypted[i + hmac_start] = hmac[i];
    }

    *encrypted_len = hmac_end;
}
