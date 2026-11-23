#include "power_calc.h"
#include "pico/stdlib.h"
#include <math.h>

void PowerCalc_Update(Motor *m, bool eStopActive) {
    if (eStopActive) {
        m->frequency = 0.0f;
        m->phaseAngle = 0.0f;
        return;
    }

    float t = to_ms_since_boot(get_absolute_time()) / 1000.0f;

    // Simulate AC line frequency around 60 Hz
    m->frequency = 60.0f + 0.15f * sinf(t + m->motorNumber);

    // Calculate phase angle from simulated power factor (high phase lag when stalled)
    float powerFactor = (m->rpm == 0) ? 0.15f : 0.86f;
    m->phaseAngle = acosf(powerFactor) * (180.0f / 3.14159f);
}