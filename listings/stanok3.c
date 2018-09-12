
#define MT_TIME_DRIVE_ON        10000
#define MT_TIME_DRIVE_PHASE_REF	22000
#define MT_TIME_DRIVE_OFF       10000
#define MT_TIME_DRIVE_STOP      5000
#define MT_TIME_DRIVE_ABORT     3500
#define MT_TIME_RESET           500

#define TIMER_ERRORS                (1*2500)
#define TIMER_HOME_INCOMPLETE       (1*2500)
#define TIMER_START_PAUSED          (1*2500)
#define TIMER_START_HELD            (1*2500)

MTDesc mt;

PLC (1, stanokOnOff)

int countErrorLinkPortablePult;
int countErrorLinkOperatorPult;
int countErrorLinkIntIO;
int cncOUT;

void stanokOnOff() {

    readInputs();
    coolSpinControl();
    checkCoolant();
    checkChipConv();
    checkScrewAuger();


    // управление сигналом готовности системы
    if (mt.ncNotReadyReq) {
        mtSignalReady(0);
    } else {
        mtSignalReady(1);
    }

    mtControlRequest();

    if (CNC.request == mtcncNone)
        cncRequest(commandPop(CNC.commands));
    controlPowerCNC(CNC.request);

    // управление сигналом готовности системы
    switch (mt.State) {
    // ждём вход от главного пускателя
    case mtNotReady: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mtIsOn() && !mt.ncNotReadyReq) mt.State=mtStartOn;
        break;
    }
        // начало включения
    case mtStartOn: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        mt.State = mtDriveOn;
        if (!axesPhaseRefComplete() || !spinsPhaseRefComplete()) mt.State = mtPhaseRef;
        break;
    }
        // фазировка
    case mtPhaseRef: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        if (axesPhaseRef() || spinsPhaseRef()) {
            mt.State = mtWaitPhaseRef;
            timerStart(mt.timerState,  MT_TIME_DRIVE_PHASE_REF);
        } else {
            // уже выполнено
            mt.State = mtDriveOn;
        }
        break;
    }
        // ожидание окончания фазировки
    case mtWaitPhaseRef: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        if (timerTimeout(mt.timerState)) {
            errorSet(systemErrors.channel[0].phaseRefTimeout);
            break;
        }
        if (axesPhaseRefComplete() && spinsPhaseRefComplete()) {
            mt.State=mtDriveOn;
        }
        break;
    }
        // включение приводов
    case mtDriveOn: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        axesActivate();
        //spinsActivate();
        timerStart(mt.timerState, MT_TIME_DRIVE_ON);
        mt.State=mtWaitDriveOn;
        break;
    }
        // ожидание включения приводов
    case mtWaitDriveOn: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        if (timerTimeout(mt.timerState)) {
            errorSet(systemErrors.channel[0].driveOnTimeout);
            break;
        }
        if (axesActive()) {
            mt.State=mtOthersMotorOn;
        }
        break;
    }
        // включения вспомогательных моторов
    case mtOthersMotorOn: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        mt.State=mtReady;
        cncRequest(mtcncActivateManual);
        // включаем вспомогательные устройства
        hydroOn(); // гидравлика
        break;
    }
        // когда включен
    case mtReady: {
        // аварийное выключение
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        // штатное выключение станка
        if (CNC.request == mtcncPowerOff) {
            mt.State=mtStartOff;
            break;
        }
        break;

    }

    case mtAbort: {
        if (CNC.request == mtcncReset) { mtReset(); }
        // ждём аварийного останова
        if (timerTimeout(mt.timerState)) {
            errorSet(systemErrors.channel[0].abortTimeout);
            axesDeactivate();
            spinsDeactivate();
            mt.State=mtNotReady;
            CNC.mode = cncOff;
            // выключение вспомогательных прпиводов
            lubeOff(); // выключение смазки
            hydroOff();
            kill(axisW);
            break;
        }
        if (axesAborted() && spinsAborted()) {
            mt.State=mtNotReady;
            CNC.mode = cncOff;
        }
        break;
    }

        // Выключение станка
        // начало выключения
    case mtStartOff: {
        if (CNC.request == mtcncReset) { mtReset(); }
        mt.State=mtOthersMotorOff;
        break;
    }

        // выключаем вспомогательные моторы
    case mtOthersMotorOff: {
        if (CNC.request == mtcncReset) { mtReset(); }
        mt.State=mtAxisStop;
        lubeOff();
        coolantOff();
        screwAugerOff();
        transporterOff();
        hydroOff();
        kill(axisW);
        break;
    }

        // останавливаем оси
    case mtAxisStop: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        mt.State=mtAxisWaitStop;
        axesStopAll();
        spinsStopAll();
        timerStart(mt.timerState, MT_TIME_DRIVE_STOP);
        break;
    }

        // ждём останов осей
    case mtAxisWaitStop: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        if (timerTimeout(mt.timerState)) {
            errorSet(systemErrors.channel[0].stopTimeout);
            break;
        }
        if (axesStopped() && spinsAborted()) {
            mt.State=mtDriveOff;
            axesDeactivate();
            spinsDeactivate();
            timerStart(mt.timerState, MT_TIME_DRIVE_OFF);
        }
        break;
    }

        // выключение осей
    case mtDriveOff: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (timerTimeout(mt.timerState)) {
            errorSet(systemErrors.channel[0].driveOffTimeout);
            axesForceKill();
            spinsForceKill();
            mt.State = mtNotReady;
            break;
        }
        if (axesInactive() && spinsInactive()) {
            mt.State=mtWaitOff;
        }
        break;
    }

        // ожидание выключения станка (ждём выключение входа MT.IN.In_0_0_MtOn, при этом сигнал mtDesc.OUT.controlReady=0)
    case  mtWaitOff: {
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mtIsOff()) {
            mt.State = mtNotReady;
            CNC.mode = cncOff;
        }
        break;
    }

    }

    if (CNC.request == mtcncWorkingLightON) {
        if (mt.OUT.workingLight1 == 1) {
            mt.OUT.workingLight1 = 0;
            mt.OUT.workingLight2 = 0;
        } else {
            mt.OUT.workingLight1 = 1;
            mt.OUT.workingLight2 = 1;
        }
    }
	
    mtUpdateCNCIndication();
    writeOutputs();
}

int hasEmergencyStopRequest()
{
    if (mt.IN.emergencyStop == 0)
        return 1;
    else
        return 0;
}

int mtIsOn()
{
    return mt.IN.mtOn;
}

int mtIsOff()
{
    return !mt.IN.mtOn;
}

void mtSignalReady(int ready)
{
    if (ready) {
        if (mt.State != mtWaitOff) {
            mt.PultOut.controlReady = 1;
        } else {
            mt.PultOut.controlReady = 0;
        }
    } else {
        mt.PultOut.controlReady = 0;
    }
}

void mtAbortRequest();

void mtReset()
{
    errorScanRequest(clearNCReset);
    CNC.request = mtcncNone;
    commandFlush(CNC.commands);
}

int mtIsReady()
{
    return mt.State == mtReady;
}

void mtUpdateCNCIndication() {

    mt.PultOut.PultLed[0] = 0;
    mt.PultOut.PultLed[1] = 0;

    int homeLed = !isHomeComplete() && timerSc(TIMER_HOME_INCOMPLETE);
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

        // индикация активной оси
        mt.PultOut.axisX    = axesControl.activeAxis == axisX;
        mt.PultOut.axisY    = axesControl.activeAxis == axisY;
        mt.PultOut.axisZ    = axesControl.activeAxis == axisZ;
        mt.PultOut.axisB    = axesControl.activeAxis == axisB;
        mt.PultOut.axisT    = axesControl.activeAxis == axisT;
        mt.PultOut.axisW    = axesControl.activeAxis == axisW;

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
        break;

    case cncMDI:
        mt.PultOut.modeManual  = 0;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 1;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
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
    mt.PultOut.sysLedMtOn = (mt.State == mtReady);

    // подсветка кнопки освещения рабочей зоны
    mt.PultOut.workingLightON = mt.OUT.workingLight1;

    // индикация работы шпинделя
    if (spinControl.spin[spinS].state == spndInactive) {
        mt.PultOut.spndCW = 0;
        mt.PultOut.spndSTOP = 1;
    } else {
        if (spinControl.spin[spinS].state == spndCW) {
            mt.PultOut.spndCW = 1;
            mt.PultOut.spndCCW = 0;
            mt.PultOut.spndSTOP = 0;
        }
        if (spinControl.spin[spinS].state == spndCCW) {
            mt.PultOut.spndCW = 0;
            mt.PultOut.spndCCW = 1;
            mt.PultOut.spndSTOP = 0;
        }
    }

    // индикация работы транспортёров стружки
    mt.PultOut.screwTransporter = mt.OUT.screwTransporter1ForwardOn;
    mt.PultOut.transporterOn = mt.OUT.transporterForwardOn;
    // индикация зажима/разжима устройства смены столов
    mt.PultOut.tableChangerClamp = mt.IN.turningPlatformClamp;
    mt.PultOut.tableChangerDown = mt.OUT.tableChangerDown;
    //индикация положения манипулятора
    mt.PultOut.armAlongXfromMagazin = mt.IN.armAlongXfromMagazin;
    mt.PultOut.armAlongXtoMagazin = mt.IN.armAlongXtoMagazin;
    mt.PultOut.armAlongZfromSpindel = mt.IN.armAlongZfromSpindel;
    mt.PultOut.armAlongZtoSpindel = mt.IN.armAlongZtoSpindel;
    mt.PultOut.armToCW = mt.IN.armToCW;
    mt.PultOut.armToCCW = mt.IN.armToCCW;
    //индикация зажима/разжима инструмента
    mt.PultOut.toolClamp = mt.IN.toolClamp;
    mt.PultOut.toolDrop = mt.IN.toolDrop;
    // индикация разрешения пульта маназина
    mt.OUT.toolPultEnaLamp = toolChange.toolPultEna;
}

void readInputs() {
    if (Servo[0].IO[0].Status & 1) {
        mt.PultIn.PultBtn[0] = Servo[0].IO[0].DataIn[0];
        mt.PultIn.PultBtn[1] = Servo[0].IO[0].DataIn[1];
        mt.PultIn.PultBtn[2] = Servo[0].IO[0].DataIn[2];
        mt.PultIn.PultBtn[3] = Servo[0].IO[0].DataIn[3];
        countErrorLinkOperatorPult = 0;
    } else {
        countErrorLinkOperatorPult++;
        if (countErrorLinkOperatorPult == 100)
            errorSet(systemErrors.machine.linkOperatorPult);
    }

    if (Servo[0].IO[1].Status & 1) {
        mt.IN.Inputs[0] = Servo[0].IO[1].DataIn[0];
        mt.IN.Inputs[1] = Servo[0].IO[3].DataIn[0];
        mt.IN.Inputs[2] = Servo[1].IO[1].DataIn[0];
        countErrorLinkIntIO = 0;
    } else {
        countErrorLinkIntIO++;
        if (countErrorLinkIntIO == 100)
            errorSet(systemErrors.machine.linkIntIO);
    }

    if (Servo[0].IO[2].Status & 1) {
        mt.PortablePultIn.PultBtn[0] = Servo[0].IO[2].DataIn[0];
        mt.PortablePultIn.PultBtn[1] = Servo[0].IO[2].DataIn[1];
        countErrorLinkPortablePult = 0;
    } else {
        countErrorLinkPortablePult++;
        if (countErrorLinkPortablePult == 100)
            errorSet(systemErrors.machine.linkPortablePult);
    }
}

void writeOutputs() {
    Servo[0].IO[0].DataOut[0] = mt.PultOut.PultLed[0];
    Servo[0].IO[0].DataOut[1] = mt.PultOut.PultLed[1];
    Servo[0].IO[0].DataOut[2] = mt.PultOut.PultLed[2];

    Servo[0].IO[1].DataOut[0] = mt.OUT.Outputs[0];
    Servo[0].IO[3].DataOut[0] = mt.OUT.Outputs[1];
    Servo[1].IO[1].DataOut[0] = mt.OUT.Outputs[2];
}

void mtAbortRequest()
{
    mt.State = mtAbort;
    axesAbortAll();
    spinsAbortAll();
    hydroOff();
    timerStart(mt.timerState, MT_TIME_DRIVE_ABORT);			// запускаем таймер
}

int systemPlcActive()
{
    return !Plc[PLC_MACHINE_ON_OFF].Error &&
            !Plc[PLC_AXIS_CTRL].Error &&
            !Plc[PLC_CNC_CTRL].Error;
}

void mtControlRequest()
{
    COMMAND_ON_FALL(btnCncOff, mt.PultIn.cncOff, mtcncCncOff, CNC.commands, 0);
    COMMAND_ON_RISE(btnReset, mt.PultIn.modeReset, mtcncReset, CNC.commands, 1);
    COMMAND_ON_HIGH(btnEmergencyStop, hasEmergencyStopRequest(), mtcncEmergencyStop, CNC.commands, 2);
    COMMAND_ON_LOW (btnPowerOff, mt.PultIn.mtOff, mtcncPowerOff, CNC.commands, 3);


    COMMAND_ON_RISE(btnStop, mt.PultIn.modeStop, mtcncStop, CNC.commands, 4);
    COMMAND_ON_RISE(btnStart, mt.PultIn.modeStart, mtcncStart, CNC.commands, 5);

    COMMAND_ON_RISE(btnManual, mt.PultIn.modeManual, mtcncActivateManual, CNC.commands, 6);
    COMMAND_ON_RISE(btnHandwheel, mt.PultIn.modeHWL, mtcncActivateHandwheel, CNC.commands, 6);
    COMMAND_ON_RISE(btnRef, mt.PultIn.modeHome, mtcncActivateRef, CNC.commands, 6);
    COMMAND_ON_RISE(btnMDI, mt.PultIn.modeMDI, mtcncActivateMDI, CNC.commands, 6);
    COMMAND_ON_RISE(btnAuto, mt.PultIn.modeAuto, mtcncActivateAuto, CNC.commands, 6);
    COMMAND_ON_RISE(btnRepos, mt.PultIn.modeReposToContour, mtcncActivateRepos, CNC.commands, 6);

    COMMAND_ON_RISE(btnStep, mt.PultIn.modeStep, mtcncToggleStep, CNC.commands, 7);
    COMMAND_ON_RISE(btnOptStop, mt.PultIn.modeOptStop, mtcncToggleOptionalStop, CNC.commands, 7);
    COMMAND_ON_RISE(btnSkip, mt.PultIn.modeSkip, mtcncToggleOptionalSkip, CNC.commands, 7);

    COMMAND_ON_RISE(btn0001, mt.PultIn.speed1, mtcncSelectSpeed1, CNC.commands, 7);
    COMMAND_ON_RISE(btn001, mt.PultIn.speed2, mtcncSelectSpeed2, CNC.commands, 7);
    COMMAND_ON_RISE(btn01, mt.PultIn.speed3, mtcncSelectSpeed3, CNC.commands, 7);
    COMMAND_ON_RISE(btn1, mt.PultIn.speed4, mtcncSelectSpeed4, CNC.commands, 7);
    COMMAND_ON_RISE(btnPortablePultEna, mt.IN.portablePultEna, mtcncPortablePultEna, CNC.commands, 7);

    COMMAND_ON_RISE(cmdAlarmCancel, CNC.alarmCancel, mtcncAlarmCancel, CNC.commands, 8);
    CNC.alarmCancel = 0;

    COMMAND_ON_RISE(btnIncPosMagazin, mt.IN.toolPultCW, mtcncIncPosMagazin, CNC.commands, 9);
    COMMAND_ON_RISE(btnDecPosMagazin, mt.IN.toolPultCCW, mtcncDecPosMagazin, CNC.commands, 9);

    COMMAND_ON_RISE(btnChipConv, mt.PultIn.transporterOn, mtcncChipConv, CNC.commands, 9);
    COMMAND_ON_RISE(btnScrewAuger, mt.PultIn.screwTransporter, mtcncScrewAuger, CNC.commands, 9);

    COMMAND_ON_RISE(btnSpinOff, mt.PultIn.spndSTOP, mtcncSpinOff, CNC.commands, 9);
    COMMAND_ON_RISE(btnSpinCW, mt.PultIn.spndCW, mtcncSpinCW, CNC.commands, 9);
    COMMAND_ON_RISE(btnSpinCCW, mt.PultIn.spndCCW, mtcncSpinCCW, CNC.commands, 9);

    COMMAND_ON_RISE(btnWorkingLightON, mt.PultIn.lightingZoneCutting, mtcncWorkingLightON, CNC.commands, 9);
    COMMAND_ON_RISE(btnTableChangerClamp, mt.PultIn.tableChangerClamp, mtcncTableChangerClamp, CNC.commands, 15);
    COMMAND_ON_RISE(btnTableChangerDown, mt.PultIn.tableChangerDown, mtcncTableChangerDown, CNC.commands, 15);

    COMMAND_ON_RISE(btnToolDrop, mt.PultIn.toolDrop, mtcncToolDrop, CNC.commands, 12);
    COMMAND_ON_RISE(btnToolClamp, mt.PultIn.toolClamp, mtcncToolClamp, CNC.commands, 12);

    COMMAND_ON_RISE(btnArmAlongXfromMagazin,mt.PultIn.armAlongXfromMagazin, mtcncArmAlongXfromMagazin, CNC.commands,16);
    COMMAND_ON_RISE(btnArmAlongXtoMagazin,mt.PultIn.armAlongXtoMagazin, mtcncArmAlongXtoMagazin, CNC.commands,16);

    COMMAND_ON_RISE(btnArmAlongZfromSpindel,mt.PultIn.armAlongZfromSpindel, mtcncArmAlongZfromSpindel, CNC.commands,16);
    COMMAND_ON_RISE(btnArmAlongZtoSpindel,mt.PultIn.armAlongZtoSpindel, mtcncArmAlongZtoSpindel, CNC.commands,16);

    COMMAND_ON_RISE(btnArmtoCW, mt.PultIn.armCW, mtcncArmtoCW, CNC.commands,16);
    COMMAND_ON_RISE(btnArmtoCCW, mt.PultIn.armCCW, mtcncArmtoCCW, CNC.commands,16);

    //Пульт магазина инструмента
    COMMAND_ON_RISE(btnToolPultDrop, mt.IN.toolPultToolDrop, mtcncToolPultDrop, CNC.commands,16);
    COMMAND_ON_RISE(btnToolPultClamp, mt.IN.toolPultToolClamp, mtcncToolPultClamp, CNC.commands,16);

    COMMAND_ON_RISE(btnToolPultOn, mt.IN.toolPultEna, mtcncToolPultOn, CNC.commands,16);
    COMMAND_ON_FALL(btnToolPultOff, mt.IN.toolPultEna, mtcncToolPultOff, CNC.commands,16);

    COMMAND_ON_RISE(btnToolPultCW,mt.IN.toolPultCW,mtcncToolPultMagazinCW,CNC.commands,0);
    COMMAND_ON_RISE(btnToolPultCCW,mt.IN.toolPultCCW,mtcncToolPultMagazinCCW,CNC.commands,0);

    COMMAND_ON_FALL(btnToolPultStopCW,mt.IN.toolPultCW,mtcncToolPultMagazinStop,CNC.commands,16);
    COMMAND_ON_FALL(btnToolPultStopCCW,mt.IN.toolPultCCW,mtcncToolPultMagazinStop,CNC.commands,16)

    // работа с переносным пультом
    if (CNC.enablePortablePult) {
        // переключение режима
        COMMAND_ON_RISE(btnHandwheel, mt.PortablePultIn.modeHWL, mtcncActivateHandwheel, CNC.commands, 6);
        COMMAND_ON_RISE(btnManual, mt.PortablePultIn.modeManual, mtcncActivateManual, CNC.commands, 6);

        //дискретные перемещения оси Х от кнопок + и -
        if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 1) {
            COMMAND_ON_RISE(btnXMinus, mt.PortablePultIn.minus, mtcncIncXMinus, CNC.commands, 10);
            COMMAND_ON_RISE(btnXPlus, mt.PortablePultIn.plus, mtcncIncXPlus, CNC.commands, 10);
        }

        //дискретные перемещения оси Y от кнопок + и -
        if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 2) {
            COMMAND_ON_RISE(btnYMinus, mt.PortablePultIn.minus, mtcncIncYMinus, CNC.commands, 10);
            COMMAND_ON_RISE(btnYPlus, mt.PortablePultIn.plus, mtcncIncYPlus, CNC.commands, 10);
        }

        //дискретные перемещения оси Z от кнопок + и -
        if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 3) {
            COMMAND_ON_RISE(btnZMinus, mt.PortablePultIn.minus, mtcncIncZMinus, CNC.commands, 10);
            COMMAND_ON_RISE(btnZPlus, mt.PortablePultIn.plus, mtcncIncZPlus, CNC.commands, 10);
        }
    } else {

        COMMAND_ON_RISE(btnX, mt.PultIn.axisX, mtcncSelectX, CNC.commands, 7);
        COMMAND_ON_RISE(btnY, mt.PultIn.axisY, mtcncSelectY, CNC.commands, 7);
        COMMAND_ON_RISE(btnZ, mt.PultIn.axisZ, mtcncSelectZ, CNC.commands, 7);
        COMMAND_ON_RISE(btnB, mt.PultIn.axisB, mtcncSelectB, CNC.commands, 7);
        COMMAND_ON_RISE(btnT, mt.PultIn.axisT, mtcncSelectT, CNC.commands, 7);
        COMMAND_ON_RISE(btnW, mt.PultIn.axisW, mtcncSelectW, CNC.commands, 7);
    }
    COMMAND_ON_RISE(btnX, mt.PultIn.axisX, mtcncMoveX, CNC.commands, 10);
    COMMAND_ON_RISE(btnY, mt.PultIn.axisY, mtcncMoveY, CNC.commands, 10);
    COMMAND_ON_RISE(btnZ, mt.PultIn.axisZ, mtcncMoveZ, CNC.commands, 10);

    static int prevSpeed = -1;
    if (mt.PortablePultIn.selectSpeed != prevSpeed) {
        prevSpeed = mt.PortablePultIn.selectSpeed;
        commandPush(CNC.commands, mtcncPortablePultSpeed, 7);
    }

    if (CNC.mode == cncManual) {
        int m, p, s;
        int inM, inP;
        int stateA;
        if (CNC.enablePortablePult == 0) {
            inM = mt.PultIn.plus; inP = mt.PultIn.minus;
            // управление осью X
            if (axesControl.activeAxis == axisX) {
                if (axesControl.axis[axisX].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisX].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveXMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveXPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveXStop, 10);
            } else {
                if (axesControl.axis[axisX].state == axisJoggingPlus || axesControl.axis[axisX].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveXStop, 10);
                stateA = 0;
            }
            // управление осью Y
            if (axesControl.activeAxis == axisY) {
                if (axesControl.axis[axisY].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisY].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveYMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveYPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveYStop, 10);
            } else {
                if (axesControl.axis[axisY].state == axisJoggingPlus || axesControl.axis[axisY].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveYStop, 10);
                stateA = 0;
            }
            // управление осью Z
            if (axesControl.activeAxis == axisZ) {
                if (axesControl.axis[axisZ].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisZ].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveZMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveZPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveZStop, 10);
            } else {
                if (axesControl.axis[axisZ].state == axisJoggingPlus || axesControl.axis[axisZ].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveZStop, 10);
                stateA = 0;
            }

            // управление осью B
            if (axesControl.activeAxis == axisB) {
                if (axesControl.axis[axisB].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisB].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveWMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveWPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveWStop, 10);
            } else {
                if (axesControl.axis[axisB].state == axisJoggingPlus || axesControl.axis[axisB].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveWStop, 10);
                stateA = 0;
            }

            // управление осью W
            if (axesControl.activeAxis == axisW) {
                if (tableChanger.state == tableChangerJoggingPlus) stateA = 1;
                else if (tableChanger.state == tableChangerJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveWMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveWPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveWStop, 10);
            } else {
                if (tableChanger.state == tableChangerJoggingPlus || tableChanger.state == tableChangerJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveWStop, 10);
                stateA = 0;
            }
        } else {
            inM = mt.PortablePultIn.plus; inP = mt.PortablePultIn.minus;
            if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 1) {
                if (axesControl.axis[axisX].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisX].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveXMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveXPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveXStop, 10);
            } else {
                if (axesControl.axis[axisX].state == axisJoggingPlus || axesControl.axis[axisX].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveXStop, 10);
                stateA = 0;
            }
            if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 2) {
                if (axesControl.axis[axisY].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisY].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveYMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveYPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveYStop, 10);
            } else {
                if (axesControl.axis[axisY].state == axisJoggingPlus || axesControl.axis[axisY].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveYStop, 10);
                stateA = 0;
            }
            if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 3) {
                if (axesControl.axis[axisZ].state == axisJoggingPlus) stateA = 1;
                else if (axesControl.axis[axisZ].state == axisJoggingMinus) stateA = 2;
                else stateA = 0;
                m = !inP && inM && (stateA != 2);
                p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
                s = !inP && !inM && (stateA != 0);
                if (m) commandPush(CNC.commands, mtcncMoveZMinus, 10);
                if (p) commandPush(CNC.commands, mtcncMoveZPlus, 10);
                if (s) commandPush(CNC.commands, mtcncMoveZStop, 10);
            } else {
                if (axesControl.axis[axisZ].state == axisJoggingPlus || axesControl.axis[axisZ].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveZStop, 10);
                stateA = 0;
            }
            if (portablePultdecodeAxis(mt.PortablePultIn.selectAxis) == 0) {
                stateA = 0;
                if (axesControl.axis[axisX].state == axisJoggingPlus || axesControl.axis[axisX].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveXStop, 10);
                if (axesControl.axis[axisY].state == axisJoggingPlus || axesControl.axis[axisY].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveYStop, 10);
                if (axesControl.axis[axisZ].state == axisJoggingPlus || axesControl.axis[axisZ].state == axisJoggingMinus)
                    commandPush(CNC.commands, mtcncMoveZStop, 10);
            }
        }
    }

    // Корректоры S и F
    unsigned corrF = mt.PultIn.corrFeed;
    unsigned corrS = mt.PultIn.corrSpin;
    double cF = round(corrF*150.0/255.0)/100;
    double cS = round(corrS*70.0/255.0+50.0)/100;
    if (fabs(CNC.channel[0].runtime.feedOverride-cF) > 1e-3)
        csFeedOverride(CNC.channel[0].runtime, cF);
    if (fabs(CNC.channel[0].runtime.spinOverride-cS) > 1e-3)
        csSpindleOverride(CNC.channel[0].runtime, cS);

    // Ручной разжим инструмента
    if (CNC.mode == cncManual) {

        if (toolChange.magazinState == toolReady || toolChange.magazinState == toolNotReady) {
            mt.OUT.armAlongXfromMagazin = mt.PultIn.armAlongXfromMagazin;
            mt.OUT.armAlongXtoMagazin   = mt.PultIn.armAlongXtoMagazin;
            mt.OUT.armAlongZfromSpindel = mt.PultIn.armAlongZfromSpindel;
            mt.OUT.armAlongZtoSpindel   = mt.PultIn.armAlongZtoSpindel;
            mt.OUT.armCW  = mt.PultIn.armCW;
            mt.OUT.armCCW = mt.PultIn.armCCW;
        }
    }
}

