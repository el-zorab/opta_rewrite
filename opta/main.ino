#include <Ethernet.h>
#include <mbed_mktime.h>
#include <SPI.h>
#include <stdint.h>
#include <time.h>
#include "api.h"
#include "hardware.h"

// which Opta is the code running on
static const uint8_t OPTA_ID = 0;

uint8_t IP_ADDRS[][] = {
    {192, 168, 11, 177},
    {192, 168, 11, 178},
    {192, 168, 11, 179}
}

IPAddress ip_addr(IP_ADDRS[OPTA_ID][0], IP_ADDRS[OPTA_ID][1], IP_ADDRS[OPTA_ID][2], IP_ADDRS[OPTA_ID][3]);
EthernetClient eth_client;
EthernetServer eth_server(8088);

void setup() {
    Serial.begin(9600);

    pinMode(LEDR, OUTPUT);

    attach_button_interrupts();
    attach_master_button_interrupt();

    Ethernet.begin(ip_addr);

    eth_server.begin();
    ntp_init();
    rtc_sync();

    srand((unsigned int) time(NULL));
}


void loop() {
    // Ethernet.maintain();
    eth_server_loop();
    rtc_check_for_sync();
    buttons_press_listener();
    master_button_press_listener();
    master_button_check_for_unlatch();
}

