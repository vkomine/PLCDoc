#ifndef MACHINE_ERRORS_H
#define MACHINE_ERRORS_H

#define MACHINE_ERRORS_DEFINED

union MachineErrors {
    struct {
        unsigned emergencyStop: 1; // Аварийный останов
        unsigned lubeError: 1; // Ошибка смазки
        unsigned spinChillerError: 1; // Ошибка охлаждения шпинделя
        unsigned toolNotFound: 1; // Инструмент не найден
        unsigned overloadPumpA: 1; // Перегрузка насоса А СОЖ
        unsigned overloadPumpB: 1; // Перегрузка насоса В СОЖ
        unsigned overloadPumpC: 1; // Перегрузка насоса С СОЖ
        unsigned coolantLevelHigh: 1; // Высокий уровень СОЖ
        unsigned coolantLevelLow: 1; // Низкий уровень СОЖ
        unsigned overloadChipConv: 1; // Перегрузка конвейера стружки
        unsigned linkPortablePult: 1;  // Ошибка связи с переносным пультом
        unsigned linkOperatorPult: 1; // Ошибка связи с пультом оператора
        unsigned linkIntIO: 1; // Ошибка связи с платой входов/выходов
    };
    unsigned errors;
};

#endif // MACHINE_ERRORS_H
