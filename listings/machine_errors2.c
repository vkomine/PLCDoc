#include "include/cnc/errors.h"
#include "include/platform/stanok/cool_spin.h"
#include "include/platform/stanok/coolant.h"
#include "include/platform/stanok/lube.h"

// Создания списка ошибок
DEFINE_ERROR(MachineEmergencyStop, 0, reactNCNotReady | reactStartDisable | reactShowAlarm | reactStop, clearNCReset);
DEFINE_ERROR(MachineLubeError, 1, reactStartDisable | reactStopAtEnd | reactShowAlarm, clearSelf);
DEFINE_ERROR(MachineSpinChillerError, 1, reactStartDisable | reactStopAtEnd | reactShowAlarm, clearSelf);
DEFINE_ERROR(MachineToolNotFound, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineOverloadPumpA, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineCoolantLevelHigh, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineOverloadChipConv, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCReset);
DEFINE_ERROR(MachineLinkOperatorPult, 1, reactNCNotReady | reactStartDisable | reactShowAlarm | reactStop, clearNCReset);

// Функция обновления флагов ошибок 
void errorsMachineScan(int request) 
{
    // аварийный останов
    errorScanSet(systemErrors.machine.emergencyStop, hasEmergencyStopRequest(), descErrorMachineEmergencyStop, request);
    // ошибка смазки
    errorScanSet(systemErrors.machine.lubeError, hasLubeError(), descErrorMachineLubeError, request);
    // ошибка охлаждения шпинделя
    errorScanSet(systemErrors.machine.spinChillerError, hasSpinChillerError(), descErrorMachineSpinChillerError, request);
    // инструмент не найден 
    errorScanSet(systemErrors.machine.toolNotFound, 0, descErrorMachineToolNotFound, request);
    // перегрузка мотора насоса А (обмывочная СОЖ)
    errorScanSet(systemErrors.machine.overloadPumpA, 0, descErrorMachineOverloadPumpA, request);
    // высокий уровень СОЖ
    errorScanSet(systemErrors.machine.coolantLevelHigh, 0, descErrorMachineCoolantLevelHigh, request);
    // перегрузка конвейера стружки
    errorScanSet(systemErrors.machine.overloadChipConv, 0, descErrorMachineOverloadChipConv, request);
    // нет связи связи с пультом оператора
    errorScanSet(systemErrors.machine.linkOperatorPult, 0, descErrorMachineLinkOperatorPult, request);
}

// Функция установки флагов действий системы согласно реакциям на ошибки
void errorsMachineReaction() 
{
	// аварийное выключение
	errorReaction(systemErrors.machine.emergencyStop, descErrorMachineEmergencyStop);
	// ошибка смазки направляющих
	errorReaction(systemErrors.machine.lubeError, descErrorMachineLubeError);
	// ошибка системы охлаждения шпинделя
	errorReaction(systemErrors.machine.spinChillerError, descErrorMachineSpinChillerError);
	// инструмент не найден
	errorReaction(systemErrors.machine.toolNotFound, descErrorMachineToolNotFound);
	// перегрузка мотора помпы А (обмывочная СОЖ)
	errorReaction(systemErrors.machine.overloadPumpA, descErrorMachineOverloadPumpA);
	// высокий уровень СОЖ
	errorReaction(systemErrors.machine.coolantLevelHigh, descErrorMachineCoolantLevelHigh);
	// перегрузка конвеера стружки
	errorReaction(systemErrors.machine.overloadChipConv, descErrorMachineOverloadChipConv);
	// потеря связи с пультом оператора
	errorReaction(systemErrors.machine.linkOperatorPult, descErrorMachineLinkOperatorPult);
}
