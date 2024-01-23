#include <Ethernet.h>
#include <stdint.h>
#include <string.h>
#include "api.h"
#include "eth_server.h"

static uint16_t ETH_SERVER_MAX_REQ_LEN = 512;
static uint16_t ETH_SERVER_PORT = 8088;

static EthernetServer eth_server(ETH_SERVER_PORT);

void eth_server_init() {
    eth_server.begin();
}

void eth_server_loop() {
    EthernetClient client = eth_server.available();
    if (!client) {
        return;
    }

    char request[ETH_SERVER_MAX_REQ_LEN];
    uint16_t request_len = 0;

    bool is_curr_line_blank = true;

    while (client.connected()) {
        if (client.available()) {
            char ch = client.read();
            request[request_len++] = ch;

            if (request_len == ETH_SERVER_MAX_REQ_LEN) {
                return;
            }

            if (is_curr_line_blank && ch == '\n') {
                request[request_len] = 0;

                char *payload_line     = strstr(request, API_HEADER_NAME);
                char *payload_line_end = strchr(payload_line, '\r');

                if (payload_line_end != NULL) {
                    *payload_line_end = 0;
                }

                uint8_t header_name_len = strlen(API_HEADER_NAME);

                if (payload_line[header_name_len] == ':'
                    && payload_line[header_name_len + 1] == ' ') {
                    char *payload_str = payload_line + header_name_len + 2;
                    Serial.print("Validating received payload: ");
                    Serial.print(payload_str);
                    Serial.println();
                    api_validate_server_payload(payload_str);
                } else {
                    client.println("HTTP/1.1 401 Unauthorized");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");
                    break;
                }
            }

            if (ch == '\n') {
                is_curr_line_blank = true;
            } else if (ch != '\r') {
                is_curr_line_blank = false;
            }
        }
    }

    delay(1);
    client.stop();
}
