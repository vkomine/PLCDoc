switch (mt.State) {
case mtNotReady: {   // Ожидание включения главного пускателя
	// Проверка наличия команды сброса в начальное состояние
	if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
	// Станок включен и система готова?
	if (mtIsOn() && !mt.ncNotReadyReq) mt.State=mtStartOn; 
	break;
}

case mtStartOn: {   // Начало включения
	// Проверка наличия команды сброса в начальное состояние
	if (CNC.request == mtcncReset) { mtReset(); } 
	// Проверка готовности системы
	if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
	mt.State = mtDriveOn;
	// Фазировка выполнена?
	if (!axesPhaseRefComplete() || !spinsPhaseRefComplete()) mt.State = mtPhaseRef;
	break;
}

case mtPhaseRef: {   // Фазировка
	// Проверка наличия команды сброса в начальное состояние
	if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
	// Проверка готовности системы
	if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
	if (axesPhaseRef() || spinsPhaseRef()) { // Требуется фазировка?
		mt.State = mtWaitPhaseRef;
		// Запуск таймера фазировки
		timerStart(mt.timerState, MT_TIME_DRIVE_PHASE_REF); 
	} else {
		mt.State = mtDriveOn; // Фазировка выполнена
	}
	break;
}

case mtWaitPhaseRef: {   // Ожидание окончания фазировки
	// Проверка наличия команды сброса в начальное состояние
	if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
	// Проверка готовности системы
	if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
	if (timerTimeout(mt.timerState)) {
		// Ошибка: истекло время фазировки
		errorSet(systemErrors.channel[0].phaseRefTimeout); 
		break;
	}
	// Фазировка выполнена?
	if (axesPhaseRefComplete() && spinsPhaseRefComplete()) { 
		mt.State=mtDriveOn;
	}
	break;
}

case mtDriveOn: {   // Включение приводов
	// Проверка наличия команды сброса в начальное состояние
	if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
	// Проверка готовности системы
	if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
	axesActivate(); // Включение в слежение всех осей
	// Запуск таймера включения приводов
	timerStart(mt.timerState, MT_TIME_DRIVE_ON); 
	mt.State=mtWaitDriveOn;
	break;
}

case mtWaitDriveOn: {   // Ожидание включения приводов
	// Проверка наличия команды сброса в начальное состояние
	if (CNC.request == mtcncReset) { mtReset(); } // Сброс в начальное состояние
	// Проверка готовности системы
	if (mt.ncNotReadyReq) { mtAbortRequest(); break; } // Аварийное выключение
	if (timerTimeout(mt.timerState)) {
		// Ошибка: истекло время включения приводов
		errorSet(systemErrors.channel[0].driveOnTimeout); 
		break;
	}
	// Все оси находятся в слежении ?
	if (axesActive()) {
		mt.State=mtOthersMotorOn; // Включение вспомогательных двигателей
	}
	break;
}
