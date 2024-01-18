#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "api.h"
#include "encryption.h"
#include "hardware.h"
#include "util.h"

const char* API_HEADER_NAME = "Payload";
const char* API_SERVER_IP   = "192.168.11.214";
const char* API_SERVER_URL  = "/api";
uint32_t    API_SERVER_PORT = 80;

void api_print_opta_payload(APIOptaPayload *payload) {
    uint8_t *payload_bytes = (uint8_t *) payload;
    for (int i = 0; i < API_OPTA_PAYLOAD_LEN; i++) {
        printf("%02x ", *(payload_bytes + i));
    }
    std::cout << '\n';
    printf("APIOptaPayload > counter=%d, timestamp=%d, opta = %d, request = %d, extra = %d\n", payload->counter, payload->timestamp, payload->opta_id, payload->request, payload->extra);
}

void api_print_server_payload(APIServerPayload *payload) {
    uint8_t *payload_bytes = (uint8_t *) payload;
    for (int i = 0; i < API_SERVER_PAYLOAD_LEN; i++) {
        printf("%02x ", *(payload_bytes + i));
    }
    std::cout << '\n';
    printf("APIServerPayload > counter = %d, timestamp = %d, request = %d, extra = %d\n", payload->counter, payload->timestamp, payload->request, payload->extra);
}

uint8_t get_opta_id_dummy() { return 2; }
uint32_t get_unix_timestamp_dummy() { return 17331401; }
uint8_t get_relay_states_dummy() { return 0b1011; }
uint8_t get_interrupts_state_dummy() { return 1; }

uint32_t api_received_reqs_counter = 412;
uint32_t api_sent_reqs_counter     = 621;

void api_send_payload_http(EthernetClient client, uint8_t *encrypted_payload, uint16_t encrypted_payload_len) {
    char http_payload[ENCRYPTED_PAYLOAD_MAX_STR_LEN];
    bytes_to_hex_str(encrypted_payload, encrypted_payload_len, http_payload);

    if (client.connect(API_SERVER_IP, API_SERVER_PORT)) {
        client.print("GET ");
        client.print(API_SERVER_URL);
        client.println(" HTTP/1.0");
        client.println("Connection: close");
        client.print(API_HEADER_NAME);
        client.print(": ");
        client.println(http_payload);
        client.println();
        client.flush();
        api_sent_reqs_counter++;
    }

    std::cout << http_payload << '\n';
}

void api_create_opta_payload(APIOptaPayload *payload, APIOptaRequestType request) {
    payload->counter = api_sent_reqs_counter;
    payload->timestamp = get_unix_timestamp_dummy();
    payload->opta_id = get_opta_id_dummy();
    payload->request = request;
    if (request == API_OPTA_REQ_UPDATE_RELAYS) {
        payload->extra = get_relay_states_dummy();
    } else if (request == API_OPTA_REQ_UPDATE_INTERRUPTS_STATE) {
        payload->extra = get_interrupts_state_dummy();
    }
}

void api_make_opta_request(APIOptaRequestType request_type) {
    APIOptaPayload raw_payload;
    uint8_t encrypted_payload[ENCRYPTED_PAYLOAD_MAX_LEN];
    uint16_t encrypted_payload_len;

    api_create_opta_payload(&raw_payload, request_type);

    encrypt((uint8_t *) &raw_payload, API_OPTA_PAYLOAD_LEN, encrypted_payload, &encrypted_payload_len);

    api_send_payload_http(client, encrypted_payload, encrypted_payload_len);
}


uint16_t api_validate_server_payload(char *encrypted) {
    uint16_t encrypted_strlen = 0;
    while (encrypted[encrypted_strlen] != 0
            && encrypted_strlen < ENCRYPTED_PAYLOAD_MAX_STR_LEN) {
        encrypted_strlen++;        
    }

    if (encrypted_strlen == ENCRYPTED_PAYLOAD_MAX_STR_LEN) {
        printf("Server payload discarded (encrypted payload length exceeds maximum value)\n");
        return 0;
    }

    if (encrypted_strlen % 2 != 0) {
        printf("Server payload discarded (encrypted payload string length is not even)\n");
        return 0;
    }

    uint16_t encrypted_len = encrypted_strlen / 2;
    uint16_t ciphertext_len = encrypted_len - IV_LEN - HMAC_LEN;

    if (ciphertext_len % 16 != 0) {
        printf("Server payload discarded (ciphertext is not a multiple of 16)\n");
        return 0;
    }

    return encrypted_strlen;
};

void api_handle_server_request(char *encrypted_str) {
    uint16_t encrypted_str_len = api_validate_server_payload(encrypted_str);
    if (!encrypted_str_len) {
        return;
    }

    APIServerPayload server_payload;
    uint8_t encrypted[ENCRYPTED_PAYLOAD_MAX_LEN];
    uint16_t encrypted_len = encrypted_str_len / 2;

    hex_str_to_bytes(encrypted_str, encrypted_str_len, encrypted);

    uint16_t server_payload_len;
    uint8_t is_decrypt_successful = decrypt(encrypted, encrypted_len, (uint8_t *) &server_payload, &server_payload_len);

    if (!is_decrypt_successful) {
        return;
    }

    if (server_payload_len != API_SERVER_PAYLOAD_LEN) { // should never be true
        printf("Server payload discarded (invalid decrypted payload length)\n");
        return;
    }

    api_print_server_payload(&server_payload);

    // todo
    if (server_payload.counter < api_received_reqs_counter) {
        printf("Server payload discarded (invalid counter)\n");
        return;
    }

    // if (get_unix_timestamp_dummy() - server_payload.timestamp > API_TIMESTAMP_MAX_DIFF) {
    //     printf("Server payload discarded (invalid timestamp)\n");
    //     return;
    // }

    if (server_payload.request == API_SERVER_REQ_UPDATE_RELAYS) {
        uint8_t relay_states[RELAY_COUNT];
        for (int i = 0; i < RELAY_COUNT; i++) {
            relay_states[i] = (1 << (RELAY_COUNT - i - 1) & server_payload.extra) > 0 ? 1 : 0;
        }
        set_relays_states(relay_states);
    } else if (server_payload.request == API_SERVER_REQ_UPDATE_INTERRUPTS_STATE) {
        set_interrupts_state(server_payload.extra);
    } else {
        printf("Server payload discarded (invalid request type)\n");
        return;
    }

    api_received_reqs_counter++;
}

int main() {
    // srand((unsigned int) time(NULL));

    api_make_opta_request(API_OPTA_REQ_UPDATE_INTERRUPTS_STATE);

    uint8_t server_payload_bytes[] = { 0xe1, 0x10, 0x00, 0x00, 0xf1, 0x74, 0x08, 0x01, 0x00, 0b1111 };
    
    uint8_t encrypted[ENCRYPTED_PAYLOAD_MAX_LEN];
    char    encrypted_str[ENCRYPTED_PAYLOAD_MAX_STR_LEN];
    uint16_t encrypted_len;

    encrypt(server_payload_bytes, API_SERVER_PAYLOAD_LEN, encrypted, &encrypted_len);

    bytes_to_hex_str(encrypted, encrypted_len, encrypted_str);

    printf("%s\n", encrypted_str);
    api_handle_server_request(encrypted_str);

    return 0;
}
