// Задание интервалов таймеров
#define MT_TIME_DRIVE_ON        	 10000
#define MT_TIME_DRIVE_PHASE_REF		 22000
#define MT_TIME_DRIVE_OFF       	 10000
#define MT_TIME_DRIVE_STOP      	 5000
#define MT_TIME_DRIVE_ABORT     	 3500
#define MT_TIME_RESET           	 500
#define MT_TIME_FILTRED_CORR    	 500
#define TIMER_ERRORS               (1*2500)
#define TIMER_HOME_INCOMPLETE      (1*2500)
#define TIMER_START_PAUSED         (1*2500)
#define TIMER_START_HELD           (1*2500)

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
		    // Проверка наличия команды сброса в начальное состояние
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			// Станок включен и система готова?
			if (mtIsOn() && !mt.ncNotReadyReq) mt.State=mtStartOn;
			break;
		}
			
		case mtStartOn: {  // Начало включения
		    // Проверка наличия команды сброса в начальное состояние
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			// Проверка готовности системы
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			mt.State = mtDriveOn;
			// Фазировка выполнена?
			if (!axesPhaseRefComplete() || !spinsPhaseRefComplete()) mt.State = mtPhaseRef;
			break;
		}
			
		case mtPhaseRef: {  // Фазировка
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			// Проверка готовности системы
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			if (axesPhaseRef() || spinsPhaseRef()) { // Требуется фазировка?
				mt.State = mtWaitPhaseRef;
				// Запуск таймера фазировки
				timerStart(mt.timerState, MT_TIME_DRIVE_PHASE_REF);
			} else { 
				mt.State = mtDriveOn; 
			}
			break;
		}
			
		case mtWaitPhaseRef: {  // Ожидание окончания фазировки
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
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
			
		case mtDriveOn: {  // Включение приводов
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			axesActivate(); // Включение в слежение всех осей
			// Запуск таймера включения приводов
			timerStart(mt.timerState, MT_TIME_DRIVE_ON);
			mt.State=mtWaitDriveOn;
			break;
		}
			
		case mtWaitDriveOn: {  // Ожидание включения приводов
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
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
			
		case mtOthersMotorOn: {  // Включение вспомогательных моторов
			if (CNC.request == mtcncReset) {mtReset();} // Сброс в начальное состояние
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			mt.State=mtReady;
			// Команда включения ручного режима
			commandPush(CNC.commands, mtcncActivateManual, 6); 
			break;
		}
			
		case mtReady: {  // Станок включен
			if (mt.ncNotReadyReq) {mtAbortRequest(); break;} // Аварийное выключение
			// Штатное выключение станка
			if (CNC.request == mtcncPowerOff) {
				mt.State=mtStartOff;
