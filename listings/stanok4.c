// Задание интервалов таймеров
#define MT_TIME_DRIVE_ON        	 10000
#define MT_TIME_DRIVE_PHASE_REF		 22000
#define MT_TIME_DRIVE_OFF       	 10000
#define MT_TIME_DRIVE_STOP      	 5000
#define MT_TIME_DRIVE_ABORT     	 3500
#define MT_TIME_RESET           	 500
#define MT_TIME_FILTRED_CORR    	 500
#define TIMER_ERRORS                (1*2500)
#define TIMER_HOME_INCOMPLETE       (1*2500)
#define TIMER_START_PAUSED          (1*2500)
#define TIMER_START_HELD            (1*2500)

MTDesc mt; // Объявление структуры с данными станка

PLC (1, stanokOnOff) // Объявление программы ПЛК

void stanokOnOff() {

    readInputs(); // Чтение входных регистров

    // Управление сигналом готовности системы
    if (mt.ncNotReadyReq) {
        mtSignalReady(0); // Система не готова
    } else {
        mtSignalReady(1); // Система готова
    }

    mtControlRequest(); // Обработка команд пульта оператора

    if (CNC.request == mtcncNone)
        cncRequest(commandPop(CNC.commands));
	
	// Обработка запроса выключения УЧПУ и станка
    controlPowerCNC(CNC.request);

    // Автомат управления станком
    switch (mt.State) {
		case mtNotReady: {  // Ожидание входа от главного пускателя
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mtIsOn() && !mt.ncNotReadyReq) mt.State=mtStartOn;
			break;
		}
			
		case mtStartOn: {  // Начало включения
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			mt.State = mtDriveOn;
			if (!axesPhaseRefComplete() || !spinsPhaseRefComplete()) mt.State = mtPhaseRef;
			break;
		}
			
		case mtPhaseRef: {  // Фазировка
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			if (axesPhaseRef() || spinsPhaseRef()) {
				mt.State = mtWaitPhaseRef;
				timerStart(mt.timerState, MT_TIME_DRIVE_PHASE_REF);
			} else { 
				mt.State = mtDriveOn; // Уже выполнено
			}
			break;
		}
			
		case mtWaitPhaseRef: {  // Ожидание окончания фазировки
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			if (timerTimeout(mt.timerState)) {
				errorSet(systemErrors.channel[0].phaseRefTimeout);
				break;
			}
			if (axesPhaseRefComplete() && spinsPhaseRefComplete()) {
				mt.State=mtDriveOn;
			}
			break;
		}
			
		case mtDriveOn: {  // Включение приводов
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			axesActivate();
			timerStart(mt.timerState, MT_TIME_DRIVE_ON);
			mt.State=mtWaitDriveOn;
			break;
		}
			
		case mtWaitDriveOn: {  // Ожидание включения приводов
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			if (timerTimeout(mt.timerState)) {
				errorSet(systemErrors.channel[0].driveOnTimeout);
				break;
			}
			if (axesActive()) {
				mt.State=mtOthersMotorOn;
			}
			break;
		}
			
		case mtOthersMotorOn: {  // Включение вспомогательных моторов
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			mt.State=mtReady;
			commandPush(CNC.commands, mtcncActivateManual, 6);
			break;
		}
			
		case mtReady: {  // когда включен
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			// Штатное выключение станка
			if (CNC.request == mtcncPowerOff) {
				mt.State=mtStartOff;
				break;
			}
			break;
		}

		case mtAbort: {
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			// Ожидание аварийного останова
			if (timerTimeout(mt.timerState)) {
				errorSet(systemErrors.channel[0].abortTimeout);
				axesDeactivate(); // Выключение осей
				spinsDeactivate(); // Выключение шпинделей
				break;
			}
			if (axesAborted() && spinsAborted()) {
				mt.State=mtNotReady;
				CNC.mode = cncOff;
			}
			break;
		}

		// Выключение станка
		case mtStartOff: {  // Начало выключения
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			mt.State=mtOthersMotorOff;
			break;
		}

		case mtOthersMotorOff: {  // Выключение вспомогательных моторов
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			mt.State=mtAxisStop;
			break;
		}

		case mtAxisStop: {  // Останов осей
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			mt.State=mtAxisWaitStop;
			timerStart(mt.timerState, MT_TIME_DRIVE_STOP);
			break;
		}

		case mtAxisWaitStop: {  // Ожидание останова осей
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
			if (timerTimeout(mt.timerState)) {
				errorSet(systemErrors.channel[0].stopTimeout);
				break;
			}
			if (axesStopped() && spinsAborted()) {
				mt.State=mtDriveOff;
				axesDeactivate(); // Выключение осей
				spinsDeactivate(); // Выключение шпинделей
				timerStart(mt.timerState, MT_TIME_DRIVE_OFF);
			}
			break;
		}
			
		case mtDriveOff: {  // выключение осей
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (timerTimeout(mt.timerState)) {
				errorSet(systemErrors.channel[0].driveOffTimeout);
				axesForceKill(); // Принудительное выключение осей
				spinsForceKill(); // Принудительное выключение шпинделей
				mt.State = mtNotReady;
				break;
			}
			if (axesInactive() && spinsInactive()) {
				mt.State=mtWaitOff;
			}
			break;
		}

		case  mtWaitOff: { // Ожидание выключения станка
			if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
			if (mtIsOff()) { // Станок выключен?
				mt.State = mtNotReady;
				CNC.mode = cncOff;
			}
			break;
		}
    }

    mtUpdateCNCIndication(); // Обновление индикации пульта
    writeOutputs(); // Запись в выходные регистры
}

int hasEmergencyStopRequest() // Нажата кнопка аварийного останова?
{
    if (mt.IN.emgButtonUnlocked == 0)  return 1;
    else return 0;
}

int mtIsOn() // Станок включён?
{
    return mt.IN.mtOn;
}

int mtIsOff() // Станок выключен?
{
    return !mt.IN.mtOn;
}

void mtReset() // Сброс в начальное состояние
{
    errorScanRequest(clearNCReset);
    CNC.request = mtcncNone;
    commandFlush(CNC.commands);
}

void mtSignalReady(int ready) // Управление сигналом готовности системы
{
    if (ready) {
        if (mt.State != mtWaitOff) {
            mt.OUT.controlReady = 1; // Система готова
        } else {
            mt.OUT.controlReady = 0; // Система не готова
        }
    } else {
        mt.OUT.controlReady = 0; // Система не готова
    }
}

void mtAbortRequest() // Обработка запроса аварийного торможения
{
    mt.State = mtAbort;
    axesAbortAll(); // Аварийное выключение осей
    spinsAbortAll(); // Аварийное выключение шпинделей
    timerStart(mt.timerState, MT_TIME_DRIVE_ABORT);	 // запускаем таймер
}
