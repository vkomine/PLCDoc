void mtUpdateCNCIndication() { // Функция обновления индикации пульта
    mt.PultOut.PultLed[0] = 0; // Обнуление регистров индикации пульта
    mt.PultOut.PultLed[1] = 0;
	...
    int homeLed = (!isHomeComplete() || !axesRefPosComplete()) && timerSc(TIMER_HOME_INCOMPLETE);
    int resetLed = (mt.ncNotReadyReq || CNC.notReadyReq) && timerSc(TIMER_ERRORS);
    int startLed = 0;
    int stopLed = 0;
    int mode = CNC.mode;
	
    if (mode == cncWaitChangeMode) {
        mode = CNC.prevMode;
    }
	
    switch (mode) {
    case cncManual: // Ручной режим
        mt.PultOut.modeManual = 1;
        mt.PultOut.modeAuto = 0;
        mt.PultOut.modeHome = homeLed;
        mt.PultOut.modeHWL = 0;
        mt.PultOut.modeMDI = 0;
        mt.PultOut.modeReset = resetLed;
        mt.PultOut.modeStep = 0;
        mt.PultOut.modeReposToContour = 0;
		mt.PultOut.speed1 = axesControl.platform.speedSelect == 0;
        mt.PultOut.speed2 = axesControl.platform.speedSelect == 1;
        mt.PultOut.speed3 = axesControl.platform.speedSelect == 2;
        mt.PultOut.speed4 = axesControl.platform.speedSelect == 3;
        mt.PultOut.rapid  = axesControl.platform.speedSelect == -1;
        break;
		
	case cncHome: // Режим выеззда в нулевую точку
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
	...
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
	}
}