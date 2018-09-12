        //Перемещение двигателем Х в заданное положение
        move({.X = angle});
        dwell(paused);
        //Задание новой скорости
        setF(programmSpeed*360*sqrt(3));
        //Рассчитать перемещение (один оборот в плюс)
        angle = angle+360;
        //Перемещение в заданное положение всеми двигателем
        moveMotors(angle);
        dwell(3*paused);
        //Рассчитать перемещение (один оборот в минус)
        angle = angle-360;
        //Перемещение в заданное положение всеми двигателем
        moveMotors(angle);
        dwell(3*paused);
    }
}
// Программа ПЛК
void stend()
{
    button.PultBtn[0] = Servo[0].IO[0].DataIn[0];
    button.PultBtn[3] = Servo[0].IO[0].DataIn[3];
	
    if (!button.cncOff){ // Ключ в положении "Включено"
	    // Кнопка "Выезд в нулевую точку" нажата?
        if (button.modeHome != doubleButton.modeHome){ 
            // Нажата кнопка "Выезд в нулевую точку"
            mode = mdHome;
            light.modeHome = 1;
            light.modeAuto = 0;
        }
		// Кнопка "Автоматический режим" нажата?
        if (button.modeAuto != doubleButton.modeAuto){ 
            // Нажата кнопка "Автоматический режим"
            mode = mdAuto;
            light.modeHome = 0;
            light.modeAuto = 1;
        }
        switch (mode) {
        case mdHome: // Режим выезда в нулевую точку
            // Кнопка "Старт" нажата?
            if (button.modeStart !=doubleButton.modeStart){ 
                // Нажата кнопка "Старт" - начать выезд в нулевую точку
                home(2); // Начать выезд в нулевую точку двигателем Z
                home(1); // Начать выезд в нулевую точку двигателем Y
                home(0); // Начать выезд в нулевую точку двигателем Х
                light.modeStart = 1;
            }
            if (isHomeComplete()){ // Выезд в нулевую точку завершён?
                light.modeStart = 0;
            }
			// Кнопка "Стоп" нажата?
            if (button.modeStop != doubleButton.modeStop){ 
			    // Нажата кнопка "Стоп"
                for (int i = 0; i < NUMBER_OF_MOTORS; i++){
                    jogStop(i); // Останов толчкового перемещения двигателя
                }
            }
            break;
        case mdAuto: // Автоматический режим
            // Кнопка "Старт" нажата?
            if (button.modeStart != doubleButton.modeStart) 
            {
				// Нажата кнопка "Старт"
                begin (1,0); // Задание программы движения "motion_nc" для КС 1
                run(1); // Запуск программы движения в КС 1
            }
            // Кнопка "Стоп" нажата?
            if (button.modeStop !=doubleButton.modeStop) 
            {
				// Нажата кнопка "Стоп"
                abort(1); // Прерывание программы движения для КС 1
                light.modeStart = 0;
            }
            break;
        case mdNull:
            // Хотя бы один двигатель не находится в слежении?
            if (!isClosedLoop()){ 
                for (int i = 0; i < NUMBER_OF_MOTORS; i++){
                    jogStop(i); // Включение двигателей в слежение
                }
            }
            mode = mdWait;
        default:
        }
        Servo[0].IO[0].DataOut[0]=light.PultBtn[0];
        Servo[0].IO[0].DataOut[2] = 8;
    }else{ // Ключ в положении "Выключено"
        mode = cmdNull;
        Servo[0].IO[0].DataOut[2] = 0;
        Servo[0].IO[0].DataOut[0] = 0;
        light.PultBtn[0] = 0;
        if (isClosedLoop()){ // Хотя бы один двигатель находится в слежении?
            for (int i = 0; i < NUMBER_OF_MOTORS ; i++){
                kill(i); // Отключить двигатели
            }
        }
    }
	// Фиксация изменения нажатия кнопок
    doubleButton.PultBtn[0] = button.PultBtn[0]; 
	// Фиксация изменения нажатия кнопок
    doubleButton.PultBtn[3] = button.PultBtn[3]; 
}