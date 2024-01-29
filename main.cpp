#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "opta/aes.h"
#include "opta/aes_key.h"
#include "opta/hmac_key.h"
#include "opta/hmac_sha256.h"
#include "opta/sha256.h"

struct APIServerPayload { // Server sends payload to Opta
    uint32_t counter;
    uint32_t timestamp;
    uint8_t request;
    uint8_t extra;
} __attribute__((packed));

static struct AES_ctx aes_ctx;

void aes_decrypt(uint8_t in[], uint16_t in_len, uint8_t iv[], uint8_t out[], uint16_t *out_len) {
    for (int i = 0; i < in_len; i++) {
      out[i] = in[i];
    }

    *out_len = in_len;

    AES_init_ctx_iv(&aes_ctx, AES_KEY, iv);
    AES_CBC_decrypt_buffer(&aes_ctx, out, *out_len);

    *out_len = in_len - out[*out_len - 1]; // sometimes we do a little trolling (remove padding bytes)

    out[*out_len] = 0;
}

void aes_encrypt(uint8_t in[], uint16_t in_len, uint8_t iv[], uint8_t out[], uint16_t *out_len) {
    for (int i = 0; i < in_len; i++) {
      out[i] = in[i];
  }

  *out_len = (in_len / 16 + 1) * 16;

  int bytes_to_add = *out_len - in_len;
  for (int i = in_len; i < *out_len; i++) {
    out[i] = bytes_to_add; // add padding bytes (PKCS7 standard)
  }

  AES_init_ctx_iv(&aes_ctx, AES_KEY, iv); 
  AES_CBC_encrypt_buffer(&aes_ctx, out, *out_len);

  out[*out_len] = 0;
}

static const uint8_t HMAC_KEY_LEN = 64;

void hmac_sha256_wrapper(const void *in, const size_t in_len, void *out, const size_t out_len) {
    hmac_sha256(HMAC_KEY, HMAC_KEY_LEN, in, in_len, out, out_len);
}

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

static const uint16_t IV_LEN = 16;
static const uint16_t AES_MAX_LEN = 32;
static const uint16_t HMAC_LEN = 32;
static const uint16_t ENCRYPTED_PAYLOAD_MAX_LEN = IV_LEN + AES_MAX_LEN + HMAC_LEN;
static const uint16_t ENCRYPTED_PAYLOAD_MAX_STR_LEN = 2 * ENCRYPTED_PAYLOAD_MAX_LEN + 1;

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
        printf("%02x", iv[i]);
    }
    printf("\n");

    for (uint16_t i = 0; i < ciphertext_len; i++) {
        encrypted[i + ciphertext_start] = ciphertext[i];
        printf("%02x", ciphertext[i]);
    }
    printf("\n");

    hmac_sha256_wrapper(encrypted, ciphertext_end, hmac, HMAC_LEN);

    for (uint16_t i = 0; i < HMAC_LEN; i++) {
        encrypted[i + hmac_start] = hmac[i];
        printf("%02x", hmac[i]);
    }
    printf("\n");

    *encrypted_len = hmac_end;
}

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
        printf("%02x ", iv[i]);    
    }
    printf("\n");

    for (int i = 0; i < ciphertext_len; i++) {
        ciphertext[i] = encrypted[i + ciphertext_start];
        printf("%02x ", ciphertext[i]);
    }
    printf("\n");

    for (int i = 0; i < HMAC_LEN; i++) {
        received_hmac[i] = encrypted[i + hmac_start];
        printf("%02x ", received_hmac[i]);
    }
    printf("\n");

    hmac_sha256_wrapper(encrypted, ciphertext_end, computed_hmac, HMAC_LEN);

    for (int i = 0; i < HMAC_LEN; i++) {
      printf("%02x ", computed_hmac[i]);
        if (received_hmac[i] != computed_hmac[i]) {
            printf("Server payload discarded (invalid HMAC)\n");
            return 0;
        }
    }

    printf("\n");

    aes_decrypt(encrypted + ciphertext_start, ciphertext_len, iv, raw, raw_len);

    return 1;
}

int main() {
    // uint8_t server_payload_bytes[] = { 0xe1, 0x10, 0x00, 0x00, 0xf1, 0x74, 0x08, 0x01, 0x00, 0b1111 };
    // APIServerPayload *server_payload = (APIServerPayload *) &server_payload_bytes;

    APIServerPayload payload = {
      .counter = 4321,
      .timestamp = 17331441,
      .request = 0,
      .extra = 15
    };

    printf("APIServerPayload > counter = %d, timestamp = %d, request = %d, extra = %d\n", payload.counter, payload.timestamp, payload.request, payload.extra);
    
    uint8_t  encrypted_payload[ENCRYPTED_PAYLOAD_MAX_LEN];
    uint16_t encrypted_payload_len;
    char     encrypted_payload_str[ENCRYPTED_PAYLOAD_MAX_STR_LEN];

    printf("Encrypted: (IV, AES, HMAC)\n");
    encrypt((uint8_t *) &payload, sizeof(APIServerPayload), encrypted_payload, &encrypted_payload_len);

    bytes_to_hex_str(encrypted_payload, encrypted_payload_len, encrypted_payload_str);
    printf("Result:\n%s\n", encrypted_payload_str);

    // decryption

    APIServerPayload recv_payload;
    uint8_t  recv_encrypted_payload[ENCRYPTED_PAYLOAD_MAX_LEN];
    uint16_t recv_encrypted_payload_len = encrypted_payload_len;

    hex_str_to_bytes(encrypted_payload_str, recv_encrypted_payload_len * 2, recv_encrypted_payload);

    for (int i = 0; i < recv_encrypted_payload_len; i++) {
      printf("%02x", recv_encrypted_payload[i]);
    }
    printf("\n");

    uint16_t recv_payload_len;
    uint8_t is_successful = decrypt(recv_encrypted_payload, recv_encrypted_payload_len, (uint8_t *) &recv_payload, &recv_payload_len);

    printf("%d\n", is_successful);

    return 0;
}