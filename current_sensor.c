#include "current_sensor.h"
#include "pico/stdlib.h"
#include <math.h>

void CurrentSensor_Update(Motor *m, bool eStopActive) {
    if (eStopActive) {
        m->currentA = 0.0f;
        m->currentB = 0.0f;
        return;
    }

    float t = to_ms_since_boot(get_absolute_time()) / 1000.0f;
    float ripple = 0.1f * sinf(t * 2.0f + m->motorNumber);

    // Base simulated currents per motor profile
    switch (m->motorNumber) {
        case 1: m->currentA = 5.2f + ripple;  m->currentB = 5.1f + ripple;  break; // Normal
        case 2: m->currentA = 6.0f + ripple;  m->currentB = 5.9f + ripple;  break; // Overheating
        case 3: m->currentA = 5.5f + ripple;  m->currentB = 5.4f + ripple;  break; // Vibration
        case 4: m->currentA = 18.5f + ripple; m->currentB = 18.2f + ripple; break; // Overcurrent
        case 5: m->currentA = 12.0f + ripple; m->currentB = 12.1f + ripple; break; // Stalled
        default: break;
    }
}