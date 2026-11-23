#include "rpm_direction.h"

void RpmDirection_Update(Motor *m, bool eStopActive) {
    if (eStopActive) {
        m->rpm = 0;
        m->direction = "STOP";
        return;
    }

    switch (m->motorNumber) {
        case 1: m->rpm = 1750; m->direction = "CW";   break;
        case 2: m->rpm = 1740; m->direction = "CW";   break;
        case 3: m->rpm = 1745; m->direction = "CW";   break;
        case 4: m->rpm = 1710; m->direction = "CCW";  break;
        case 5: m->rpm = 0;    m->direction = "STOP"; break; // Stalled (0 RPM)
        default: break;
    }
}