#include "include/cnc/errors.h"
#include "include/platform/stanok/cool_spin.h"
#include "include/platform/stanok/hydro.h"
#include "include/platform/stanok/lube.h"

DEFINE_ERROR(MachineEmergencyStop, 0, reactNCNotReady | reactStartDisable | reactShowAlarm | reactStop, clearNCReset);

DEFINE_ERROR(MachineLubeError, 1, reactStartDisable | reactStopAtEnd | reactShowAlarm, clearSelf);
DEFINE_ERROR(MachineSpinChillerError, 1, reactStartDisable | reactStopAtEnd | reactShowAlarm, clearSelf);
DEFINE_ERROR(MachineToolNotFound, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineOverloadPumpA, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineOverloadPumpB, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineOverloadPumpC, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineCoolantLevelHigh, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineCoolantLevelLow, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCStart);
DEFINE_ERROR(MachineOverloadChipConv, 1, reactStartDisable | reactStop | reactShowAlarm, clearNCReset);
DEFINE_ERROR(MachineLinkPortablePult, 1, reactNCNotReady | reactStartDisable | reactShowAlarm | reactStop, clearNCReset);
DEFINE_ERROR(MachineLinkOperatorPult, 1, reactNCNotReady | reactStartDisable | reactShowAlarm | reactStop, clearNCReset);
DEFINE_ERROR(MachineLinkIntIO, 1,  reactShowAlarm, clearNCReset);

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
    // перегрузка мотора насоса В (стандартная СОЖ)
    errorScanSet(systemErrors.machine.overloadPumpB, 0, descErrorMachineOverloadPumpB, request);
    // перегрузка мотора насоса С (СОЖ через шпиндель)
    errorScanSet(systemErrors.machine.overloadPumpC, 0, descErrorMachineOverloadPumpC, request);
    // высокий уровень СОЖ
    errorScanSet(systemErrors.machine.coolantLevelHigh, 0, descErrorMachineCoolantLevelHigh, request);
    // низкий уровень СОЖ
    errorScanSet(systemErrors.machine.coolantLevelLow, 0, descErrorMachineCoolantLevelLow, request);
    // перегрузка конвейера стружки
    errorScanSet(systemErrors.machine.overloadChipConv, 0, descErrorMachineOverloadChipConv, request);
    // нет связи связи с переносным пультом 
    errorScanSet(systemErrors.machine.linkPortablePult, 0, descErrorMachineLinkPortablePult, request);
    // нет связи связи с пультом оператора
    errorScanSet(systemErrors.machine.linkOperatorPult, 0, descErrorMachineLinkOperatorPult, request);
    // нет связи связи с платой входов/выходов
    errorScanSet(systemErrors.machine.linkIntIO, 0, descErrorMachineLinkIntIO, request);
}

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
	// перегрузка мотора помпы B (стандартная СОЖ)
	errorReaction(systemErrors.machine.overloadPumpB, descErrorMachineOverloadPumpB);
	// перегрузка мотора помпы С (СОЖ через шпиндель)
	errorReaction(systemErrors.machine.overloadPumpC, descErrorMachineOverloadPumpC);
	// высокий уровень СОЖ
	errorReaction(systemErrors.machine.coolantLevelHigh, descErrorMachineCoolantLevelHigh);
	// низкий уровень СОЖ
	errorReaction(systemErrors.machine.coolantLevelLow, descErrorMachineCoolantLevelLow);
	// перегрузка мотора конвеера стружки
	errorReaction(systemErrors.machine.overloadChipConv, descErrorMachineOverloadChipConv);
	// потеря связи с переносным пультом
	errorReaction(systemErrors.machine.linkPortablePult, descErrorMachineLinkPortablePult);
	// потеря связи с пультом оператора
	errorReaction(systemErrors.machine.linkOperatorPult, descErrorMachineLinkOperatorPult);
	// потеря связи платой входов/выходов
	errorReaction(systemErrors.machine.linkIntIO, descErrorMachineLinkIntIO);
}
