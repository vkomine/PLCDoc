#include "sys/sys.h"
#include "include/platform/stanok/stanok.h"
#include "include/platform/stanok/coolant.h"

void checkCoolant() {
    // контроль двигателя насоса А
    if (mt.OUT.pumpA == 1 && mt.IN.overloadPumpA == 1) {
        coolantOff();
        errorSet(systemErrors.machine.overloadPumpA);
    }

    // контроль уровня СОЖ
    if (mt.IN.coolantLevelHigh == 1) {
        if (mt.State == mtReady) {
            coolantOff();
        }
        errorSet(systemErrors.machine.coolantLevelHigh);
    }
}

// выключение подачи СОЖ
void coolantOff() {
    mt.OUT.pumpA = 0;
    mt.OUT.pumpB = 0;
    mt.OUT.pumpC = 0;
}