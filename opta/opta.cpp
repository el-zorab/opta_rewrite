#include <Ethernet.h>
#include <mbed_mktime.h>
#include <SPI.h>
#include <stdint.h>
#include <time.h>
#include "aes.h" // do not remove this 'useless' #include, or compilation will fail in Arduino IDE
#include "api.h"
#include "eth_server.h"
#include "hardware.h"
#include "ntp.h"
#include "opta.h"
#include "rtc.h"

// which Opta is the code running on
static const uint8_t OPTA_ID = 0;

uint8_t IP_ADDRS[3][4] = {
    {192, 168, 11, 177},
    {192, 168, 11, 178},
    {192, 168, 11, 179}
};

IPAddress ip_addr(IP_ADDRS[OPTA_ID][0], IP_ADDRS[OPTA_ID][1], IP_ADDRS[OPTA_ID][2], IP_ADDRS[OPTA_ID][3]);
EthernetClient eth_client;

void setup() {
    Serial.begin(9600);

    pinMode(LEDR, OUTPUT);

    switches_attach_interrupts();
    user_btn_attach_interrupt();

    Ethernet.begin(ip_addr);
    eth_server_init();
    
    ntp_init();
    rtc_sync();

    srand((unsigned int) time(NULL));
}

void loop() {
    eth_server_loop();
    rtc_check_for_sync();
    switches_press_listener();
    user_btn_press_listener();
    user_btn_check_for_unlatch();
}

uint8_t get_opta_id() {
    return OPTA_ID;
}

EthernetClient *get_eth_client() {
    return &eth_client;
}
