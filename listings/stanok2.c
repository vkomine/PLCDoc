
#define MT_TIME_DRIVE_ON        10000
#define MT_TIME_DRIVE_PHASE_REF	22000
#define MT_TIME_DRIVE_OFF       10000
#define MT_TIME_DRIVE_STOP      5000
#define MT_TIME_DRIVE_ABORT     3500
#define MT_TIME_RESET           500
#define MT_TIME_FILTRED_CORR    500

#define TIMER_ERRORS                (1*2500)
#define TIMER_HOME_INCOMPLETE       (1*2500)
#define TIMER_START_PAUSED          (1*2500)
#define TIMER_START_HELD            (1*2500)

MTDesc mt;

PLC (1, stanokOnOff)

int countErrorLinkPortablePult;
int countErrorLinkOperatorPult;
int countErrorLinkIntIO;

void stanokOnOff() {

    readInputs();

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
				timerStart(mt.timerState, MT_TIME_DRIVE_PHASE_REF);
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
			if (axesActive()/* && spinsActive()*/) {
				mt.State=mtOthersMotorOn;
			}
			break;
		}
			// включения вспомогательных моторов
		case mtOthersMotorOn: {
			if (CNC.request == mtcncReset) { mtReset(); }
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
			mt.State=mtReady;
			commandPush(CNC.commands, mtcncActivateManual, 6);
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
			break;
		}

			// останавливаем оси
		case mtAxisStop: {
			if (CNC.request == mtcncReset) { mtReset(); }
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
			mt.State=mtAxisWaitStop;
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
				timerStart(mt.timerState,  MT_TIME_DRIVE_OFF);
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

    mtUpdateCNCIndication();
    writeOutputs();
}


int hasEmergencyStopRequest()
{
    if (mt.IN.emgButtonUnlocked == 0)
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
            mt.OUT.controlReady = 1;
        } else {
            mt.OUT.controlReady = 0;
        }
    } else {
        mt.OUT.controlReady = 0;
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
        //mt.PultOut.modeVirtual = 1;
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

void readInputs() {
    if (Servo[0].IO[0].Status & 1) {
        mt.PultIn.PultBtn[0] = Servo[0].IO[0].DataIn[0];
        mt.PultIn.PultBtn[1] = Servo[0].IO[0].DataIn[1];
        mt.PultIn.PultBtn[2] = Servo[0].IO[0].DataIn[2];
        mt.PultIn.PultBtn[3] = Servo[0].IO[0].DataIn[3];
        countErrorLinkOperatorPult = 0;
    } else {
        countErrorLinkOperatorPult++;
        if (countErrorLinkOperatorPult >= 100)
            errorSet(systemErrors.machine.linkOperatorPult);
    }

    if (Servo[0].IO[1].Status & 1) {
        mt.IN.Inputs[0] = Servo[0].IO[1].DataIn[0];
        mt.IN.Inputs[1] = Servo[0].IO[1].DataIn[1];
        countErrorLinkIntIO = 0;
    } else {
        countErrorLinkIntIO++;
        if (countErrorLinkIntIO >= 100)
            errorSet(systemErrors.machine.linkIntIO);
    }
}

void writeOutputs() {
    Servo[0].IO[0].DataOut[0] = mt.PultOut.PultLed[0];
    Servo[0].IO[0].DataOut[1] = mt.PultOut.PultLed[1];
    Servo[0].IO[0].DataOut[2] = mt.PultOut.PultLed[2];
    Servo[0].IO[1].DataOut[0] = mt.OUT.Outputs[0];
    Servo[0].IO[3].DataOut[0] = mt.OUT.Outputs[1];
}

void mtAbortRequest()
{
    mt.State = mtAbort;
    axesAbortAll();
    spinsAbortAll();
    timerStart(mt.timerState,
               MT_TIME_DRIVE_ABORT);			// запускаем таймер
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
    COMMAND_ON_RISE(btnDRF, mt.PultIn.modeDryRun, mtcncDryRun, CNC.commands, 7);
    COMMAND_ON_RISE(btnRG0, mt.PultIn.modeReducedRapid, mtcncReducedRapid, CNC.commands, 7);

    COMMAND_ON_RISE(btn0001, mt.PultIn.speed1, mtcncSelectSpeed1, CNC.commands, 7);
    COMMAND_ON_RISE(btn001, mt.PultIn.speed2, mtcncSelectSpeed2, CNC.commands, 7);
    COMMAND_ON_RISE(btn01, mt.PultIn.speed3, mtcncSelectSpeed3, CNC.commands, 7);
    COMMAND_ON_RISE(btn1, mt.PultIn.speed4, mtcncSelectSpeed4, CNC.commands, 7);
    COMMAND_ON_RISE(btnRapid, mt.PultIn.rapid, mtcncSelectRapid, CNC.commands, 7);

    COMMAND_ON_RISE(btnPortablePultEnaOn, enaPortablePult(), mtcncPortablePultEna, CNC.commands, 9);
    COMMAND_ON_FALL(btnPortablePultEnaOff, enaPortablePult(), mtcncPortablePultDis, CNC.commands, 9);

    COMMAND_ON_RISE(cmdAlarmCancel, CNC.alarmCancel, mtcncAlarmCancel, CNC.commands, 8);
    CNC.alarmCancel = 0;


    COMMAND_ON_RISE(btnSpinOn, mt.PultIn.spndOn, mtcncSpinOn, CNC.commands, 9);
    COMMAND_ON_RISE(btnSpinOff, mt.PultIn.spndOff, mtcncSpinOff, CNC.commands, 9);
    COMMAND_ON_RISE(btnSpinCW, mt.PultIn.spndCW, mtcncSelectSpinCW, CNC.commands, 9);
    COMMAND_ON_RISE(btnSpinCCW, mt.PultIn.spndCCW, mtcncSelectSpinCCW, CNC.commands, 9);

    // работа с переносным пультом
    if (CNC.enablePortablePult) {

    } else {
        COMMAND_ON_RISE(btnXMinus, mt.PultIn.xMinus, mtcncIncXMinus, CNC.commands, 10);
        COMMAND_ON_RISE(btnXPlus, mt.PultIn.xPlus, mtcncIncXPlus, CNC.commands, 10);
        COMMAND_ON_RISE(btnYMinus, mt.PultIn.yMinus, mtcncIncYMinus, CNC.commands, 10);
        COMMAND_ON_RISE(btnYPlus, mt.PultIn.yPlus, mtcncIncYPlus, CNC.commands, 10);
        COMMAND_ON_RISE(btnZMinus, mt.PultIn.zMinus, mtcncIncZMinus, CNC.commands, 10);
        COMMAND_ON_RISE(btnZPlus, mt.PultIn.zPlus, mtcncIncZPlus, CNC.commands, 10);
        COMMAND_ON_RISE(btnAMinus, mt.PultIn.aMinus, mtcncIncAMinus, CNC.commands, 10);
        COMMAND_ON_RISE(btnAPlus, mt.PultIn.aPlus, mtcncIncAPlus, CNC.commands, 10);
        COMMAND_ON_RISE(btnCMinus, mt.PultIn.cMinus, mtcncIncCMinus, CNC.commands, 10);
        COMMAND_ON_RISE(btnCPlus, mt.PultIn.cPlus, mtcncIncCPlus, CNC.commands, 10);
    }

    COMMAND_ON_RISE(btnX, mt.PultIn.xMinus || mt.PultIn.xPlus, mtcncMoveX, CNC.commands, 10);
    COMMAND_ON_RISE(btnY, mt.PultIn.yMinus || mt.PultIn.yPlus, mtcncMoveY, CNC.commands, 10);
    COMMAND_ON_RISE(btnZ, mt.PultIn.zMinus || mt.PultIn.zPlus, mtcncMoveZ, CNC.commands, 10);
    COMMAND_ON_RISE(btnA, mt.PultIn.aMinus || mt.PultIn.aPlus, mtcncMoveA, CNC.commands, 10);
    COMMAND_ON_RISE(btnC, mt.PultIn.cMinus || mt.PultIn.cPlus, mtcncMoveC, CNC.commands, 10);

    static int prevSpeed = -1;
    if (CNC.enablePortablePult) {
    if (mt.PultIn.selectSpeed != prevSpeed) {
        prevSpeed = mt.PultIn.selectSpeed;
        commandPush(CNC.commands, mtcncPortablePultSpeed, 7);
    }
    } else
        prevSpeed = -1;

    if (CNC.mode == cncManual) {
        int m, p, s;
        int inM, inP;
        int stateA;
        inM = mt.PultIn.xMinus; inP = mt.PultIn.xPlus;
        if (axesControl.axis[axisX].state == axisJoggingPlus) stateA = 1;
        else if (axesControl.axis[axisX].state == axisJoggingMinus) stateA = 2;
        else stateA = 0;
        m = !inP && inM && (stateA != 2);
        p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
        s = !inP && !inM && (stateA != 0);
        if (m) commandPush(CNC.commands, mtcncMoveXMinus, 10);
        if (p) commandPush(CNC.commands, mtcncMoveXPlus, 10);
        if (s) commandPush(CNC.commands, mtcncMoveXStop, 10);

        inM = mt.PultIn.yMinus; inP = mt.PultIn.yPlus;
        if (axesControl.axis[axisY].state == axisJoggingPlus) stateA = 1;
        else if (axesControl.axis[axisY].state == axisJoggingMinus) stateA = 2;
        else stateA = 0;
        m = !inP && inM && (stateA != 2);
        p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
        s = !inP && !inM && (stateA != 0);
        if (m) commandPush(CNC.commands, mtcncMoveYMinus, 10);
        if (p) commandPush(CNC.commands, mtcncMoveYPlus, 10);
        if (s) commandPush(CNC.commands, mtcncMoveYStop, 10);

        inM = mt.PultIn.zMinus; inP = mt.PultIn.zPlus;
        if (axesControl.axis[axisZ].state == axisJoggingPlus) stateA = 1;
        else if (axesControl.axis[axisZ].state == axisJoggingMinus) stateA = 2;
        else stateA = 0;
        m = !inP && inM && (stateA != 2);
        p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
        s = !inP && !inM && (stateA != 0);
        if (m) commandPush(CNC.commands, mtcncMoveZMinus, 10);
        if (p) commandPush(CNC.commands, mtcncMoveZPlus, 10);
        if (s) commandPush(CNC.commands, mtcncMoveZStop, 10);

        inM = mt.PultIn.aMinus; inP = mt.PultIn.aPlus;
        if (axesControl.axis[axisA].state == axisJoggingPlus) stateA = 1;
        else if (axesControl.axis[axisA].state == axisJoggingMinus) stateA = 2;
        else stateA = 0;
        m = !inP && inM && (stateA != 2);
        p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
        s = !inP && !inM && (stateA != 0);
        if (m) commandPush(CNC.commands, mtcncMoveAMinus, 10);
        if (p) commandPush(CNC.commands, mtcncMoveAPlus, 10);
        if (s) commandPush(CNC.commands, mtcncMoveAStop, 10);

        inM = mt.PultIn.cMinus; inP = mt.PultIn.cPlus;
        if (axesControl.axis[axisC].state == axisJoggingPlus) stateA = 1;
        else if (axesControl.axis[axisC].state == axisJoggingMinus) stateA = 2;
        else stateA = 0;
        m = !inP && inM && (stateA != 2);
        p = inP && ((!inM && (stateA != 1)) || (inM && (stateA == 0)));
        s = !inP && !inM && (stateA != 0);
        if (m) commandPush(CNC.commands, mtcncMoveCMinus, 10);
        if (p) commandPush(CNC.commands, mtcncMoveCPlus, 10);
        if (s) commandPush(CNC.commands, mtcncMoveCStop, 10);

    }

    // Корректоры S и F
    cSTemp = corrSpinDecode(mt.PultIn.corrSpin);
    if (mt.PultIn.corrFeed != 31) {
        cFTemp = corrFeedDecode(mt.PultIn.corrFeed);
    }
    // фильтр на переключение крутилки
    if (cFTemp != cF || cSTemp != cS) {
        if (timerTimeout(mt.timerScan)) {
            cF = corrFeedDecode(mt.PultIn.corrFeed);
            cS = corrSpinDecode(mt.PultIn.corrSpin);
        }
    }
    else {
        timerStart(mt.timerScan, MT_TIME_FILTRED_CORR);
    }

    if (fabs(CNC.channel[0].runtime.feedOverride-cF) > 1e-3) {
        csFeedOverride(CNC.channel[0].runtime, cF);
        commandPush(CNC.commands, mtcncFeedOverride, 10);
    }
    if (fabs(CNC.channel[0].runtime.spinOverride-cS) > 1e-3) {
        csSpindleOverride(CNC.channel[0].runtime, cS);
        commandPush(CNC.commands, mtcncSpinOverride, 10);
    }
}

double corrFeedDecode(int inputValue) {
    const double decodeValue[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.5, 1.4, 1.3, 1.2, 1.1, 1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.06, 0.02, 0};
    return decodeValue[inputValue];
}

double corrSpinDecode(int inputValue) {
    const double decodeValue[] = {1.2, 1.1, 1, 0.9, 0.8, 0.7, 0.6, 0.5};
    return decodeValue[inputValue];
}
