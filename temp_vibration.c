#include "temp_vibration.h"
#include "pico/stdlib.h"
#include <math.h>

#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT     0x6B
#define MPU6050_CONFIG       0x1A
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_X      0x3B

#define SAMPLE_COUNT         128
#define SAMPLE_INTERVAL_US   2000

bool TempVib_Init(i2c_inst_t *i2c) {
    uint8_t data[2];

    data[0] = MPU6050_PWR_MGMT; data[1] = 0x00;
    if (i2c_write_blocking(i2c, MPU6050_ADDR, data, 2, false) != 2) return false;
    sleep_ms(100);

    data[0] = MPU6050_CONFIG; data[1] = 0x03;
    if (i2c_write_blocking(i2c, MPU6050_ADDR, data, 2, false) != 2) return false;

    data[0] = MPU6050_ACCEL_CONFIG; data[1] = 0x00;
    if (i2c_write_blocking(i2c, MPU6050_ADDR, data, 2, false) != 2) return false;

    return true;
}

static bool MPU6050_ReadRaw(i2c_inst_t *i2c, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t reg = MPU6050_ACCEL_X;
    uint8_t buffer[6];

    if (i2c_write_blocking(i2c, MPU6050_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(i2c, MPU6050_ADDR, buffer, 6, false) != 6) return false;

    *ax = (int16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);
    *ay = (int16_t)(((uint16_t)buffer[2] << 8) | buffer[3]);
    *az = (int16_t)(((uint16_t)buffer[4] << 8) | buffer[5]);
    return true;
}

bool TempVib_ReadPhysicalVibration(i2c_inst_t *i2c, float *vibrationOut) {
    int16_t ax, ay, az;
    float sumX = 0, sumY = 0, sumZ = 0;
    float sumSqX = 0, sumSqY = 0, sumSqZ = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        if (!MPU6050_ReadRaw(i2c, &ax, &ay, &az)) return false;

        float x = ax / 16384.0f;
        float y = ay / 16384.0f;
        float z = az / 16384.0f;

        sumX += x; sumY += y; sumZ += z;
        sumSqX += x * x; sumSqY += y * y; sumSqZ += z * z;
        sleep_us(SAMPLE_INTERVAL_US);
    }

    float meanX = sumX / SAMPLE_COUNT;
    float meanY = sumY / SAMPLE_COUNT;
    float meanZ = sumZ / SAMPLE_COUNT;

    float varX = fmaxf(0.0f, (sumSqX / SAMPLE_COUNT) - meanX * meanX);
    float varY = fmaxf(0.0f, (sumSqY / SAMPLE_COUNT) - meanY * meanY);
    float varZ = fmaxf(0.0f, (sumSqZ / SAMPLE_COUNT) - meanZ * meanZ);

    *vibrationOut = sqrtf(varX + varY + varZ);
    return true;
}

void TempVib_UpdateMotor(Motor *m, float physicalVib, bool sensorOk) {
    // Simulated temperatures per motor profile
    switch (m->motorNumber) {
        case 1: m->temperature = 35.0f; break;
        case 2: m->temperature = 85.0f; break; // Overheating profile
        case 3: m->temperature = 36.5f; break;
        case 4: m->temperature = 42.0f; break;
        case 5: m->temperature = 48.0f; break;
        default: break;
    }

    // Motor 3 uses physical GY-521; others use simulated baseline
    if (m->motorNumber == 3) {
        m->vibration = sensorOk ? physicalVib : 9.999f;
    } else {
        m->vibration = 0.003f;
    }
}