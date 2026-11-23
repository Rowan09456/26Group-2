#include "ds3231_rtc.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define DS3231_ADDR 0x68

static uint8_t DecToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

static uint8_t BcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

bool DS3231_Init(i2c_inst_t *i2c) {
    uint8_t reg = 0x00;
    uint8_t dummy;
    // Verify DS3231 responds at 0x68
    if (i2c_write_blocking(i2c, DS3231_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(i2c, DS3231_ADDR, &dummy, 1, false) != 1) return false;
    return true;
}

bool DS3231_SetTime(i2c_inst_t *i2c, DateTime dt) {
    uint8_t buf[8];
    buf[0] = 0x00; // Start at seconds register
    buf[1] = DecToBcd(dt.seconds);
    buf[2] = DecToBcd(dt.minutes);
    buf[3] = DecToBcd(dt.hours); // 24-hour mode
    buf[4] = DecToBcd(dt.day);
    buf[5] = DecToBcd(dt.date);
    buf[6] = DecToBcd(dt.month);
    buf[7] = DecToBcd(dt.year);

    return i2c_write_blocking(i2c, DS3231_ADDR, buf, 8, false) == 8;
}

bool DS3231_GetTime(i2c_inst_t *i2c, DateTime *dt) {
    uint8_t reg = 0x00;
    uint8_t buf[7];

    if (i2c_write_blocking(i2c, DS3231_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(i2c, DS3231_ADDR, buf, 7, false) != 7) return false;

    dt->seconds = BcdToDec(buf[0] & 0x7F);
    dt->minutes = BcdToDec(buf[1] & 0x7F);
    dt->hours   = BcdToDec(buf[2] & 0x3F);
    dt->day     = BcdToDec(buf[3] & 0x07);
    dt->date    = BcdToDec(buf[4] & 0x3F);
    dt->month   = BcdToDec(buf[5] & 0x1F);
    dt->year    = BcdToDec(buf[6]);
    return true;
}

void DS3231_GetTimestamp(i2c_inst_t *i2c, char *buffer, size_t len) {
    DateTime dt;
    if (DS3231_GetTime(i2c, &dt)) {
        const char *ampm = (dt.hours >= 12) ? "PM" : "AM";
        uint8_t hour12 = dt.hours % 12;
        if (hour12 == 0) hour12 = 12; // Convert 0 (midnight) and 12 (noon) to 12

        snprintf(buffer, len, "20%02d-%02d-%02d %02d:%02d:%02d %s",
                 dt.year, dt.month, dt.date, hour12, dt.minutes, dt.seconds, ampm);
    } else {
        uint32_t sec = to_ms_since_boot(get_absolute_time()) / 1000;
        snprintf(buffer, len, "RTC ERR +%lus", sec);
    }
}