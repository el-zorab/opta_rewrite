#include <mbed_mktime.h>
#include "ntp.h"
#include "rtc.h"

const unsigned int RTC_SYNC_INTERVAL = 30 * 60 * 1000UL; // 30 minutes

unsigned long last_rtc_sync;

void rtc_sync() {
  unsigned long ntp_unix_timestamp = ntp_get_unix_timestamp();
  if (ntp_unix_timestamp != -1) {
    set_time(ntp_unix_timestamp);
    last_rtc_sync = millis();
  }
}

void rtc_check_for_sync() {
  if (millis() - last_rtc_sync > RTC_SYNC_INTERVAL) {
    rtc_sync();
  }
}

time_t rtc_get_unix_timestamp() {
  tm t;
  time_t seconds;
  _rtc_localtime(time(NULL), &t, RTC_FULL_LEAP_YEAR_SUPPORT);
  _rtc_maketime(&t, &seconds, RTC_FULL_LEAP_YEAR_SUPPORT);
  return seconds;
}
