int mtIsReady()
{
    return mt.State == mtReady;
}

void mtUpdateCNCIndication() {

    mt.PultOut.PultLed[0] = 0;
    mt.PultOut.PultLed[1] = 0;

    int homeLed = (!isHomeComplete() || !axesRefPosComplete()) && timerSc(TIMER_HOME_INCOMPLETE);
    int resetLed = (mt.ncNotReadyReq || CNC.notReadyReq) && timerSc(TIMER_ERRORS);
    int startLed = 0;
    int stopLed = 0;
    int mode = CNC.mode;
    if (mode == cncWaitChangeMode) {
        mode = CNC.prevMode;
    }
    switch (mode) {
    case cncOff:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = 0;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = 0;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        break;

    default:
        mt.PultOut.modeManual  = 1;
        mt.PultOut.modeAuto    = 1;
        mt.PultOut.modeHome    = 1;
        mt.PultOut.modeHWL     = 1;
        mt.PultOut.modeMDI     = 1;
        mt.PultOut.modeReset   = 1;
        mt.PultOut.modeStep    = 1;
        mt.PultOut.modeReposToContour = 1;
        break;

    case cncManual:
        mt.PultOut.modeManual  = 1;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        mt.PultOut.speed1        = axesControl.platform.speedSelect == 0;
        mt.PultOut.speed2        = axesControl.platform.speedSelect == 1;
        mt.PultOut.speed3        = axesControl.platform.speedSelect == 2;
        mt.PultOut.speed4        = axesControl.platform.speedSelect == 3;
        mt.PultOut.rapid         = axesControl.platform.speedSelect == -1;
        break;

    case cncAuto:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 1;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        mt.PultOut.modeStep = CNC.modeAutoStep;
        mt.PultOut.modeOptStop = CNC.modeAutoOptStop;
        mt.PultOut.modeSkip = CNC.modeAutoSkip;
        mt.PultOut.modeReposToContour = CNC.modeAutoRepos;
        mt.PultOut.modeDryRun = CNC.modeDryRun;
        mt.PultOut.modeReducedRapid = CNC.modeReducedG0;

        startLed = CNC.channel[MACHINE_CH1].running || CNC.channel[MACHINE_CH1].starting;
        if (startLed) {
            if (CNC.channel[MACHINE_CH1].stopped) startLed = timerSc(TIMER_START_PAUSED);
            else if (CNC.channel[MACHINE_CH1].holding) startLed = timerSc(TIMER_START_HELD);
        }
        stopLed = CNC.channel[MACHINE_CH1].holding;
        break;

    case cncHome:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = 1;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        startLed = isHoming();
        break;

    case cncHWL:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 1;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        mt.PultOut.speed1        = axesControl.platform.incSelect == 0;
        mt.PultOut.speed2        = axesControl.platform.incSelect == 1;
        mt.PultOut.speed3        = axesControl.platform.incSelect == 2;
        mt.PultOut.speed4        = axesControl.platform.incSelect == 3;
        mt.PultOut.xMinus        = axesControl.activeAxis == axisX;
        mt.PultOut.xPlus        = axesControl.activeAxis == axisX;
        mt.PultOut.yMinus        = axesControl.activeAxis == axisY;
        mt.PultOut.yPlus        = axesControl.activeAxis == axisY;
        mt.PultOut.zMinus        = axesControl.activeAxis == axisZ;
        mt.PultOut.zPlus        = axesControl.activeAxis == axisZ;
        mt.PultOut.aMinus        = axesControl.activeAxis == axisA;
        mt.PultOut.aPlus        = axesControl.activeAxis == axisA;
        mt.PultOut.cMinus        = axesControl.activeAxis == axisC;
        mt.PultOut.cPlus        = axesControl.activeAxis == axisC;
        break;

    case cncMDI:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 1;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep = CNC.modeAutoStep;
        mt.PultOut.modeOptStop = CNC.modeAutoOptStop;
        mt.PultOut.modeSkip = CNC.modeAutoSkip;
        mt.PultOut.modeReposToContour = CNC.modeAutoRepos;
        mt.PultOut.modeDryRun = CNC.modeDryRun;
        mt.PultOut.modeReducedRapid = CNC.modeReducedG0;
        startLed = CNC.channel[MACHINE_CH1].runningMDI || CNC.channel[MACHINE_CH1].startingMDI;
        if (startLed) {
            if (CNC.channel[MACHINE_CH1].stopped) startLed = timerSc(TIMER_START_PAUSED);
            else if (CNC.channel[MACHINE_CH1].holding) startLed = timerSc(TIMER_START_HELD);
        }
        stopLed = CNC.channel[MACHINE_CH1].holding;
        break;

    case cncStep:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 1;
        mt.PultOut.modeReposToContour = 0;
        break;

    case cncRepos:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 1;
        break;

    case cncReset:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = 1;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        break;

    case cncVirtual:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        break;

    }

    mt.PultOut.modeReset   = resetLed;
    mt.PultOut.modeStart = startLed;
    mt.PultOut.modeStop = stopLed;

    // подсветка кнопки станок включён
    mt.PultOut.sysLedMtOn = mt.State == mtReady;
    mt.PultOut.sysLedMtOff = 0;

    // индикация работы шпинделя
    if (spinControl.spin[spinS].state == spndInactive) {
        mt.PultOut.spndOn = 0;
        mt.PultOut.spndOff = 1;
    } else {
        mt.PultOut.spndOn = 1;
        mt.PultOut.spndOff = 0;
    }
    if (spinControl.spin[spinS].platform.selectDirect == spinDirCW) {
        mt.PultOut.spndCW = 1;
        mt.PultOut.spndCCW = 0;
    } else {
        if (spinControl.spin[spinS].platform.selectDirect == spinDirCCW) {
            mt.PultOut.spndCW = 0;
            mt.PultOut.spndCCW = 1;
        }
    }
}