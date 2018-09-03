    switch (mt.State) {
    case mtNotReady: {   // Ожидание включения главного пускателя
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mtIsOn() && !mt.ncNotReadyReq) mt.State=mtStartOn;
        break;
    }
    case mtStartOn: {   // начало включения
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        mt.State = mtDriveOn;
        if (!axesPhaseRefComplete() || !spinsPhaseRefComplete()) mt.State = mtPhaseRef;
        break;
    }
    case mtPhaseRef: {   // фазировка
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        if (axesPhaseRef() || spinsPhaseRef()) {
            mt.State = mtWaitPhaseRef;
            timerStart(mt.timerState, MT_TIME_DRIVE_PHASE_REF);
        } else {
            mt.State = mtDriveOn; // уже выполнена
        }
        break;
    }
    case mtWaitPhaseRef: {   // ожидание окончания фазировки
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
    case mtDriveOn: {   // включение приводов
        if (CNC.request == mtcncReset) { mtReset(); }
        if (mt.ncNotReadyReq) { mtAbortRequest(); break; }
        axesActivate();
        timerStart(mt.timerState, MT_TIME_DRIVE_ON);
        mt.State=mtWaitDriveOn;
        break;
    }
    case mtWaitDriveOn: {   // ожидание включения приводов
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
