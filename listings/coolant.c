#include "sys/sys.h"
#include "include/platform/stanok/stanok.h"
#include "include/platform/stanok/coolant.h"

void checkCoolant() {
    // Контроль двигателя насоса А
	// Насоса А включен и перегружен?
    if (mt.OUT.pumpA == 1 && mt.IN.overloadPumpA == 1) { 
        coolantOff(); // выключение подачи СОЖ
        // Ошибка: перегрузка насоса А СОЖ 
        errorSet(systemErrors.machine.overloadPumpA);
    }

    // Контроль уровня СОЖ
    if (mt.IN.coolantLevelHigh == 1) { // Высокий уровень СОЖ?
        if (mt.State == mtReady) { // Станок включен?
            coolantOff(); // Выключение подачи СОЖ
        }
        // Ошибка: высокий уровень СОЖ  
        errorSet(systemErrors.machine.coolantLevelHigh);
    }
}

// Выключение подачи СОЖ
void coolantOff() {
    mt.OUT.pumpA = 0; // Отключение насоса А
    mt.OUT.pumpB = 0; // Отключение насоса B
    mt.OUT.pumpC = 0; // Отключение насоса C
}