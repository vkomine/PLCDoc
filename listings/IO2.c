MTDesc mt;  // Объявление структуры с данными станка
...
void readInputs() {    // Функция чтения данных входных регистров
    if (Servo[0].IO[0].Status & 1) { // Проверка корректности данных
		// Чтение регистров пульта оператора
        mt.PultIn.PultBtn[0] = Servo[0].IO[0].DataIn[0];
        mt.PultIn.PultBtn[1] = Servo[0].IO[0].DataIn[1];
        mt.PultIn.PultBtn[2] = Servo[0].IO[0].DataIn[2];
        mt.PultIn.PultBtn[3] = Servo[0].IO[0].DataIn[3];
		// Обнуление числа ошибок соединения с пультом оператора
        countErrorLinkOperatorPult = 0; 
    } else {
        countErrorLinkOperatorPult++; // Инкрементирование числа ошибок
        if (countErrorLinkOperatorPult >= 100)
			// Ошибка соединения с пультом оператора
            errorSet(systemErrors.machine.linkOperatorPult);
    }

    if (Servo[0].IO[1].Status & 1) { // Проверка корректности данных
		//Чтение регистров платы входов/выходов
        mt.IN.Inputs[0] = Servo[0].IO[1].DataIn[0];
		// Обнуление числа ошибок соединения с платой входов
        countErrorLinkIntIO = 0;
    } else {
        countErrorLinkIntIO++; // Инкрементирование числа ошибок
        if (countErrorLinkIntIO >= 100) 
			// Ошибка соединения с платой входов
            errorSet(systemErrors.machine.linkIntIO);
    }
}

void writeOutputs() {  // Функция записи данных в выходные регистры
    // Запись в регистры пульта оператора
    Servo[0].IO[0].DataOut[0] = mt.PultOut.PultLed[0];
    Servo[0].IO[0].DataOut[1] = mt.PultOut.PultLed[1];
    Servo[0].IO[0].DataOut[2] = mt.PultOut.PultLed[2];
    // Запись в регистры платы входов/выходов
    Servo[0].IO[1].DataOut[0] = mt.OUT.Outputs[0];
}