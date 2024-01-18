#pragma once

#include <stdint.h>

struct APIOptaPayload { // Opta sends payload to Server
    uint32_t counter;
    uint32_t timestamp;
    uint8_t opta_id;
    uint8_t request;
    uint8_t extra;
} __attribute__((packed));

struct APIServerPayload { // Server sends payload to Opta
    uint32_t counter;
    uint32_t timestamp;
    uint8_t request;
    uint8_t extra;
} __attribute__((packed));

enum APIOptaRequestType {
    API_OPTA_REQ_UPDATE_RELAYS           = 0,
    API_OPTA_REQ_UPDATE_INTERRUPTS_STATE = 1
};

enum APIServerRequestType {
    API_SERVER_REQ_UPDATE_RELAYS           = 0,
    API_SERVER_REQ_UPDATE_INTERRUPTS_STATE = 1
};

uint8_t API_OPTA_PAYLOAD_LEN   = sizeof(APIOptaPayload);
uint8_t API_SERVER_PAYLOAD_LEN = sizeof(APIServerPayload);

static const uint32_t API_TIMESTAMP_MAX_DIFF = 5;

void api_make_opta_request(APIOptaRequestType request_type);
