#ifndef DS3231_RTC_H
#define DS3231_RTC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hardware/i2c.h"

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year; // 00-99 (representing 2000-2099)
} DateTime;

// Initializes and checks communication with the DS3231
bool DS3231_Init(i2c_inst_t *i2c);

// Sets the time on the DS3231 (run once to program the clock, then comment out)
bool DS3231_SetTime(i2c_inst_t *i2c, DateTime dt);

// Reads the current date and time from the DS3231
bool DS3231_GetTime(i2c_inst_t *i2c, DateTime *dt);

// Formats the timestamp as "YYYY-MM-DD HH:MM:SS"
void DS3231_GetTimestamp(i2c_inst_t *i2c, char *buffer, size_t len);

#endif