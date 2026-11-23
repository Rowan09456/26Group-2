#ifndef MOTOR_TYPES_H
#define MOTOR_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STATE_NORMAL = 0,
    STATE_ALARM = 1,
    STATE_ESTOP = 2,
    STATE_RECOVERY = 3
} SystemState;

typedef struct {
    int motorNumber;
    float currentA;
    float currentB;
    float frequency;
    float phaseAngle;
    float temperature;
    float vibration;
    int rpm;
    const char *direction;
    SystemState state;
    const char *faultReason;
} Motor;

#endif