#include <stdint.h>
#include <time.h>
#include "api.h"
#include "encryption.h"
#include "hardware.h"
#include "opta.h"
#include "rtc.h"
#include "util.h"

static const char* API_SERVER_IP   = "192.168.11.108";
static const char* API_SERVER_URL  = "/api";
static uint32_t    API_SERVER_PORT = 80;

static const uint32_t API_TIMESTAMP_MAX_DIFF = 5;

uint32_t api_received_reqs_counter = 0;
uint32_t api_sent_reqs_counter     = 0;

void api_send_payload_http(uint8_t *encrypted_payload, uint16_t encrypted_payload_len) {
    char http_payload[ENCRYPTED_PAYLOAD_MAX_STR_LEN];
    bytes_to_hex_str(encrypted_payload, encrypted_payload_len, http_payload);

    EthernetClient *client = get_eth_client();

    if (client->connect(API_SERVER_IP, API_SERVER_PORT)) {
        client->print("GET ");
        client->print(API_SERVER_URL);
        client->println(" HTTP/1.0");
        client->println("Connection: close");
        client->print(API_HEADER_NAME);
        client->print(": ");
        client->println(http_payload);
        client->println();
        client->flush();

        api_sent_reqs_counter++;
    }
    
    client->stop();
}

void api_create_opta_payload(struct APIOptaPayload *payload, enum APIOptaRequestType request) {
    payload->counter = api_sent_reqs_counter;
    payload->timestamp = rtc_get_unix_timestamp();
    payload->opta_id = get_opta_id();
    payload->request = request;

    if (request == API_OPTA_REQ_UPDATE_RELAYS) {
        payload->extra = 0;
        for (uint8_t i = 0; i < RELAY_COUNT; i++) {
            if (get_relay_state(i)) {
                payload->extra |= 1 << (RELAY_COUNT - i - 1);
            }
        }
    } else if (request == API_OPTA_REQ_UPDATE_INTERRUPTS_STATE) {
        payload->extra = switches_get_interrupts_state();
    }
}

void api_make_opta_request(enum APIOptaRequestType request_type) {
    APIOptaPayload raw_payload;
    uint8_t encrypted_payload[ENCRYPTED_PAYLOAD_MAX_LEN];
    uint16_t encrypted_payload_len;

    api_create_opta_payload(&raw_payload, request_type);

    encrypt((uint8_t *) &raw_payload, API_OPTA_PAYLOAD_LEN, encrypted_payload, &encrypted_payload_len);

    api_send_payload_http(encrypted_payload, encrypted_payload_len);
}

uint16_t api_validate_server_payload_len(char *encrypted) {
    uint16_t encrypted_strlen = 0;
    while (encrypted[encrypted_strlen] != 0
            && encrypted_strlen < ENCRYPTED_PAYLOAD_MAX_STR_LEN) {
        encrypted_strlen++;
    }

    if (encrypted_strlen == ENCRYPTED_PAYLOAD_MAX_STR_LEN) {
        Serial.println("Server payload discarded (encrypted payload length exceeds maximum value");
        return 0;
    }

    for (uint16_t i = 0; i < encrypted_strlen; i++) {
        char c = encrypted[i];
        if (('0' > c || c > '9') && ('a' > c || c > 'f')) {
            Serial.println("Server payload discarded (encrypted payload is not a hex string)");
            return 0;
        }
    }

    if (encrypted_strlen % 2 != 0) {
        Serial.println("Server payload discarded (encrypted payload string length is not even)");
        return 0;
    }

    uint16_t encrypted_len = encrypted_strlen / 2;
    uint16_t ciphertext_len = encrypted_len - IV_LEN - HMAC_LEN;

    if (ciphertext_len % 16 != 0) {
        Serial.println("Server payload discarded (ciphertext length is not a multiple of 16)");
        return 0;
    }

    return encrypted_strlen;
};

void api_handle_server_request(char *encrypted_str) {
    uint16_t encrypted_str_len = api_validate_server_payload_len(encrypted_str);

    Serial.println(encrypted_str_len);
    if (encrypted_str_len == 0) {
        return;
    }

    APIServerPayload server_payload;
    uint8_t encrypted[ENCRYPTED_PAYLOAD_MAX_LEN];
    uint16_t encrypted_len = encrypted_str_len / 2;

    hex_str_to_bytes(encrypted_str, encrypted_str_len, encrypted);

    for (uint16_t i = 0; i < encrypted_len; i++) {
        if (encrypted[i] < 0x10) Serial.print("0");
        Serial.print(encrypted[i], HEX);
    }
    Serial.println();

    uint16_t server_payload_len;
    uint8_t is_decrypt_successful = decrypt(encrypted, encrypted_len, (uint8_t *) &server_payload, &server_payload_len);

    Serial.println(is_decrypt_successful);

    if (!is_decrypt_successful) {
        return;
    }

    if (server_payload_len != API_SERVER_PAYLOAD_LEN) { // should never be true
        Serial.println("Server payload discarded (invalid decrypted payload length)");
        return;
    }

    api_print_server_payload(&server_payload);

    if (server_payload.counter < api_received_reqs_counter) {
        Serial.println("Server payload discarded (invalid counter)");
        return;
    }

    if (rtc_get_unix_timestamp() - server_payload.timestamp > API_TIMESTAMP_MAX_DIFF) {
        Serial.println("Server payload discarded (invalid timestamp)");
        return;
    }

    if (server_payload.request == API_SERVER_REQ_UPDATE_RELAYS) {
        uint8_t relay_states[RELAY_COUNT];
        for (int i = 0; i < RELAY_COUNT; i++) {
            // relay_states[i] = (1 << (RELAY_COUNT - i - 1) & server_payload.extra) > 0 ? 1 : 0;
            relay_states[i] = (server_payload.extra >> (RELAY_COUNT - i - 1)) & 1;
        }
        set_relays_states(relay_states);
    } else if (server_payload.request == API_SERVER_REQ_UPDATE_INTERRUPTS_STATE) {
        switches_set_interrupts_state(server_payload.extra);
    } else {
        Serial.println("Server payload discarded (invalid request type)");
        return;
    }

    api_received_reqs_counter++;
}

void api_print_opta_payload(struct APIOptaPayload *payload) {
    uint8_t *payload_bytes = (uint8_t *) payload;
    for (int i = 0; i < API_OPTA_PAYLOAD_LEN; i++) {
        // printf("%02x ", *(payload_bytes + i));
    }
    // printf("\nAPIOptaPayload > counter=%d, timestamp=%d, opta = %d, request = %d, extra = %d\n", payload->counter, payload->timestamp, payload->opta_id, payload->request, payload->extra);
}

void api_print_server_payload(struct APIServerPayload *payload) {
    uint8_t *payload_bytes = (uint8_t *) payload;
    for (int i = 0; i < API_SERVER_PAYLOAD_LEN; i++) {
        Serial.print(*(payload_bytes + i));
    }
    Serial.println();
    // printf("\nAPIServerPayload > counter = %d, timestamp = %d, request = %d, extra = %d\n", payload->counter, payload->timestamp, payload->request, payload->extra);
}
