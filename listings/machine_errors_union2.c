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
        unsigned coolantLevelHigh: 1; // Высокий уровень СОЖ
        unsigned overloadChipConv: 1; // Перегрузка конвейера стружки
        unsigned linkOperatorPult: 1; // Ошибка связи с пультом оператора
    };
    unsigned errors;
};

#endif // MACHINE_ERRORS_H
