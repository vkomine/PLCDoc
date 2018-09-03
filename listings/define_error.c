DEFINE_ERROR(MachineEmergencyStop, 0, reactNCNotReady | reactStartDisable | reactShowAlarm | reactStop, clearNCReset);

DEFINE_ERROR(MachineLubeError, 1, reactStartDisable | reactStopAtEnd | reactShowAlarm, clearSelf);
