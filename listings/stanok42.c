				break;
			}
			break;
		}

		case mtAbort: { // Аварийное торможение
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (timerTimeout(mt.timerState)) {
				// Ошибка: истекло время аварийного торможения
				errorSet(systemErrors.channel[0].abortTimeout);
				axesDeactivate(); // Выключение осей
				spinsDeactivate(); // Выключение шпинделей
				break;
			}
			// Все оси и шпиндели аварийно остановлены?
			if (axesAborted() && spinsAborted()) {
				mt.State=mtNotReady;
				CNC.mode = cncOff;
			}
			break;
		}
		
		// Выключение станка
		case mtStartOff: {  // Начало выключения
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			mt.State=mtOthersMotorOff;
			break;
		}

		case mtOthersMotorOff: {  // Выключение вспомогательных моторов
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			mt.State=mtAxisStop;
			break;
		}

		case mtAxisStop: {  // Останов осей
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			mt.State=mtAxisWaitStop;
			// Запуск таймера останова
			timerStart(mt.timerState, MT_TIME_DRIVE_STOP);
			break;
		}

		case mtAxisWaitStop: {  // Ожидание останова осей
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			if (timerTimeout(mt.timerState)) {
				// Ошибка: истекло время останова осей
				errorSet(systemErrors.channel[0].stopTimeout);
				break;
			}
			// Все оси и шпиндели остановлены?
			if (axesStopped() && spinsAborted()) {
				mt.State=mtDriveOff;
				axesDeactivate(); // Выключение осей
				spinsDeactivate(); // Выключение шпинделей
				// Запуск таймера выключения приводов
				timerStart(mt.timerState, MT_TIME_DRIVE_OFF);
			}
			break;
		}
		
		case mtDriveOff: {  // выключение осей
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (timerTimeout(mt.timerState)) {
				// Ошибка: истекло время выключения приводов
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
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
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
    errorScanRequest(clearNCReset); // Обновление флагов ошибок
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
