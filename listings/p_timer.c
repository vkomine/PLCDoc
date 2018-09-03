void mtUpdateCNCIndication() {
    mt.PultOut.PultLed[0] = 0;
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
    case cncManual:
        mt.PultOut.modeManual  = 1;
        mt.PultOut.modeAuto    = 0;
        mt.PultOut.modeHome    = homeLed;
        mt.PultOut.modeHWL     = 0;
        mt.PultOut.modeMDI     = 0;
        mt.PultOut.modeReset   = resetLed;
        mt.PultOut.modeStep    = 0;
        mt.PultOut.modeReposToContour = 0;
        break;
	...
	}
}