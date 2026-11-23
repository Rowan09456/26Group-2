#include "oled_ui.h"
#include "ssd1306_i2c.h"
#include <stdio.h>
#include <string.h>

static const char* State_Text(SystemState s) {
    switch (s) {
        case STATE_NORMAL:   return "NORMAL";
        case STATE_ALARM:    return "ALARM";
        case STATE_ESTOP:    return "E-STOP";
        case STATE_RECOVERY: return "RECOVERY";
        default:             return "UNKNOWN";
    }
}

void OledUI_Init(void) {
    SSD1306_initialise();
}

void OledUI_ShowRTC(const char *timestamp) {
    char dateLine[20];
    char timeLine[20];

    ClearDisplay();
    WriteString(0, 0, (uint8_t *)"SYSTEM RTC");

    // timestamp format is "YYYY-MM-DD HH:MM:SS" (19 chars)
    if (strlen(timestamp) >= 19) {
        snprintf(dateLine, sizeof(dateLine), "DATE: %.10s", timestamp);
        snprintf(timeLine, sizeof(timeLine), "TIME: %s", timestamp + 11);
        WriteString(0, 20, (uint8_t *)dateLine);
        WriteString(0, 36, (uint8_t *)timeLine);
    } else {
        // Fallback if RTC error or uptime string
        WriteString(0, 24, (uint8_t *)timestamp);
    }

    WriteString(0, 52, (uint8_t *)"[BTN -> MOTORS]");
    UpdateDisplay();
}

void OledUI_ShowMotor(const Motor *m) {
    char text[28];
    const char *stateStr = State_Text(m->state);

    ClearDisplay();

    snprintf(text, sizeof(text), "M%d: %s", m->motorNumber, stateStr);
    WriteString(0, 0, (uint8_t *)text);

    snprintf(text, sizeof(text), "I:%.1f/%.1fA %d%s", m->currentA, m->currentB, m->rpm, m->direction);
    WriteString(0, 16, (uint8_t *)text);

    snprintf(text, sizeof(text), "T:%.1fC V:%.3fg", m->temperature, m->vibration);
    WriteString(0, 32, (uint8_t *)text);

    snprintf(text, sizeof(text), "%s", (m->state == STATE_NORMAL) ? "ALL CHECKS OK" : m->faultReason);
    WriteString(0, 48, (uint8_t *)text);

    UpdateDisplay();
}