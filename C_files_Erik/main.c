#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "motor_types.h"
#include "current_sensor.h"
#include "power_calc.h"
#include "temp_vibration.h"
#include "rpm_direction.h"
#include "oled_ui.h"
#include "ds3231_rtc.h"

// ==========================================
// I2C Configuration
// ==========================================
// i2c1: OLED Display + GY-521 Accelerometer
#define I2C_PORT        i2c1
#define I2C_SDA_PIN     6
#define I2C_SCL_PIN     7

// i2c0: DS3231 Real-Time Clock
#define RTC_I2C_PORT    i2c0
#define RTC_SDA_PIN     0
#define RTC_SCL_PIN     1

// ==========================================
// Button Configuration
// ==========================================
#define BUTTON_PIN      14

// ==========================================
// Fault Thresholds
// ==========================================
#define TEMP_THRESHOLD      75.0f
#define VIBRATION_THRESHOLD 0.100f
#define CURRENT_THRESHOLD   15.0f
#define MIN_RUNNING_CURRENT 1.0f

// ==========================================
// Five Simulated Motors
// ==========================================
Motor motors[5] = {
    {.motorNumber = 1, .state = STATE_NORMAL, .faultReason = "NONE"},
    {.motorNumber = 2, .state = STATE_NORMAL, .faultReason = "NONE"},
    {.motorNumber = 3, .state = STATE_NORMAL, .faultReason = "NONE"},
    {.motorNumber = 4, .state = STATE_NORMAL, .faultReason = "NONE"},
    {.motorNumber = 5, .state = STATE_NORMAL, .faultReason = "NONE"}
};

bool globalEStop = false;
int currentScreen = 0; // 0 = RTC Screen, 1..5 = Motors 1..5
volatile bool buttonPressed = false;

// ==========================================
// Timestamped Fault Event Log
// ==========================================
#define MAX_LOGS 10
typedef struct {
    char timestamp[24];
    int motorNum;
    char event[32];
} FaultLog;

FaultLog faultLogs[MAX_LOGS];
int logCount = 0;

void Add_Fault_Log(int motorNum, const char *eventMsg) {
    // Avoid logging the exact same fault repeatedly every loop
    if (logCount > 0 && faultLogs[0].motorNum == motorNum && strcmp(faultLogs[0].event, eventMsg) == 0) {
        return;
    }

    for (int i = MAX_LOGS - 1; i > 0; i--) {
        faultLogs[i] = faultLogs[i - 1];
    }

    DS3231_GetTimestamp(RTC_I2C_PORT, faultLogs[0].timestamp, sizeof(faultLogs[0].timestamp));
    faultLogs[0].motorNum = motorNum;
    snprintf(faultLogs[0].event, sizeof(faultLogs[0].event), "%s", eventMsg);

    if (logCount < MAX_LOGS) logCount++;

    printf("[FAULT LOGGED] %s | Motor %d: %s\n",
           faultLogs[0].timestamp, motorNum, eventMsg);
}

// ==========================================
// Button Interrupt & Debounce
// ==========================================
void Button_Callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        buttonPressed = true;
    }
}

void Button_Init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_set_irq_enabled_with_callback(
        BUTTON_PIN,
        GPIO_IRQ_EDGE_FALL,
        true,
        &Button_Callback
    );
}

void Check_Button(void) {
    if (buttonPressed) {
        buttonPressed = false;
        sleep_ms(30); // Debounce

        if (!gpio_get(BUTTON_PIN)) {
            currentScreen = (currentScreen + 1) % 6;

            if (currentScreen == 0) {
                printf("\n>>> Switched to RTC Screen <<<\n");
            } else {
                printf("\n>>> Switched to Motor %d <<<\n", currentScreen);
            }

            while (!gpio_get(BUTTON_PIN)) {
                sleep_ms(10);
            }
            sleep_ms(30);
        }
    }
}

// ==========================================
// Fault Evaluation & State Machine
// ==========================================
void Evaluate_Faults(Motor *m, bool sensorOk) {
    if (globalEStop) {
        m->state = STATE_ESTOP;
        m->faultReason = "E-STOP ACTIVE";
        return;
    }

    bool faultDetected = false;
    const char *reason = "NONE";

    if (!sensorOk && m->motorNumber == 3) {
        faultDetected = true;
        reason = "SENSOR ERROR";
    } else if (m->currentA > CURRENT_THRESHOLD || m->currentB > CURRENT_THRESHOLD) {
        faultDetected = true;
        reason = "OVERCURRENT";
    } else if ((m->currentA > MIN_RUNNING_CURRENT || m->currentB > MIN_RUNNING_CURRENT) && m->rpm == 0) {
        faultDetected = true;
        reason = "STALLED MOTOR";
    } else if (m->temperature > TEMP_THRESHOLD) {
        faultDetected = true;
        reason = "OVERHEATING";
    } else if (m->vibration > VIBRATION_THRESHOLD) {
        faultDetected = true;
        reason = "HIGH VIBRATION";
    }

    if (faultDetected) {
        if (m->state != STATE_ALARM) {
            Add_Fault_Log(m->motorNumber, reason);
        }
        m->state = STATE_ALARM;
        m->faultReason = reason;
    } else {
        m->state = STATE_NORMAL;
        m->faultReason = "NONE";
    }
}

// ==========================================
// Main Program
// ==========================================
int main() {
    stdio_init_all();
    sleep_ms(1500);

    // 1. Initialize i2c1 for OLED + GY-521
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // 2. Initialize i2c0 for DS3231 RTC
    i2c_init(RTC_I2C_PORT, 400 * 1000);
    gpio_set_function(RTC_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(RTC_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(RTC_SDA_PIN);
    gpio_pull_up(RTC_SCL_PIN);

    // 3. Initialize Peripherals
    Button_Init();
    OledUI_Init();

    bool mpuOk = TempVib_Init(I2C_PORT);
    if (!mpuOk) {
        printf("GY-521 (MPU6050) init failed!\n");
    }

    bool rtcOk = DS3231_Init(RTC_I2C_PORT);
    if (!rtcOk) {
        printf("DS3231 RTC not detected on GP0/GP1!\n");
    }

sleep_ms(100);
//DateTime setTime = {0, 3, 22, 6, 21, 11, 26}; // 2026-11-21 10:03:00 PM
//DS3231_SetTime(RTC_I2C_PORT, setTime);


    printf("MOTOR MONITOR SYSTEM STARTED\n");

    float realVibration = 0.0f;
    char timestamp[24];

    // 4. Main Loop
    while (1) {
        Check_Button();

        // Read RTC Timestamp
        DS3231_GetTimestamp(RTC_I2C_PORT, timestamp, sizeof(timestamp));

        // Read Physical GY-521 Vibration
        bool readOk = mpuOk && TempVib_ReadPhysicalVibration(I2C_PORT, &realVibration);

        // Update All 5 Motors
        for (int i = 0; i < 5; i++) {
            CurrentSensor_Update(&motors[i], globalEStop);
            RpmDirection_Update(&motors[i], globalEStop);
            PowerCalc_Update(&motors[i], globalEStop);
            TempVib_UpdateMotor(&motors[i], realVibration, readOk);
            Evaluate_Faults(&motors[i], readOk);
        }

        // Serial Output
        if (currentScreen == 0) {
            printf("[%s] SYSTEM RTC SCREEN\n", timestamp);
        } else {
            Motor *cur = &motors[currentScreen - 1];
            printf("[%s] M%d | Ia:%.1fA Ib:%.1fA | %.1fHz %.1fdeg | %.1fC %.3fg | %dRPM %s | %s\n",
                   timestamp,
                   cur->motorNumber,
                   cur->currentA, cur->currentB,
                   cur->frequency, cur->phaseAngle,
                   cur->temperature, cur->vibration,
                   cur->rpm, cur->direction,
                   cur->faultReason);
        }

        // Update OLED Display
        if (currentScreen == 0) {
            OledUI_ShowRTC(timestamp);
        } else {
            OledUI_ShowMotor(&motors[currentScreen - 1]);
        }

        sleep_ms(100);
    }

    return 0;
}