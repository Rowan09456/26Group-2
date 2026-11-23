#ifndef TEMP_VIBRATION_H
#define TEMP_VIBRATION_H

#include "motor_types.h"
#include "hardware/i2c.h"

bool TempVib_Init(i2c_inst_t *i2c);
bool TempVib_ReadPhysicalVibration(i2c_inst_t *i2c, float *vibrationOut);
void TempVib_UpdateMotor(Motor *m, float physicalVib, bool sensorOk);

#endif