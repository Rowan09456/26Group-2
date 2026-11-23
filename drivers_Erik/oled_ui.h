#ifndef OLED_UI_H
#define OLED_UI_H

#include "motor_types.h"

void OledUI_Init(void);
void OledUI_ShowRTC(const char *timestamp);
void OledUI_ShowMotor(const Motor *m);

#endif