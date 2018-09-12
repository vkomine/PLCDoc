int countErrorLinkPortablePult;
int countErrorLinkOperatorPult;
int countErrorLinkIntIO;

double corrFeedDecode(int inputValue);
double corrSpinDecode(int inputValue);

double cFTemp;
double cSTemp;
double cF;
double cS;

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