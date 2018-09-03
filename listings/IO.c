MTDesc mt;  // Объявление переменной mt типа MTDesc
...
void readInputs() {   // Чтение входов
    if (Servo[0].IO[0].Status & 1) { // Проверка корректности данных
        mt.PultIn.PultBtn[0] = Servo[0].IO[0].DataIn[0];
        mt.PultIn.PultBtn[1] = Servo[0].IO[0].DataIn[1];
        mt.PultIn.PultBtn[2] = Servo[0].IO[0].DataIn[2];
        mt.PultIn.PultBtn[3] = Servo[0].IO[0].DataIn[3];
		// Обнуление числа ошибок соединения с пультом оператора
        countErrorLinkOperatorPult = 0; 
    } else {
        countErrorLinkOperatorPult++;
        if (countErrorLinkOperatorPult >= 100)
			// Сообщение об ошибке соединения с пультом оператора
            errorSet(systemErrors.machine.linkOperatorPult);
    }

    if (Servo[0].IO[1].Status & 1) { // Проверка корректности данных
        mt.IN.Inputs[0] = Servo[0].IO[1].DataIn[0];
		// Обнуление числа ошибок соединения с платой входов
        countErrorLinkIntIO = 0;
    } else {
        countErrorLinkIntIO++;
        if (countErrorLinkIntIO >= 100) 
			// Сообщение об ошибке соединения с платой входов
            errorSet(systemErrors.machine.linkIntIO);
    }
}

void writeOutputs() { // Запись выходов
    Servo[0].IO[0].DataOut[0] = mt.PultOut.PultLed[0];
    Servo[0].IO[0].DataOut[1] = mt.PultOut.PultLed[1];
    Servo[0].IO[0].DataOut[2] = mt.PultOut.PultLed[2];
    Servo[0].IO[1].DataOut[0] = mt.OUT.Outputs[0];
}