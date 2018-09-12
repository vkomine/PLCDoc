void motion_nc(){
   //Включить индикацию работы программы
    light.modeStart = 1;

    //Счетчик угла поворота
    static double angle = 0;

    //Использовать абсолютные перемещения для моторов осей Х, Y и Z
    absAxes(axXYZ);

    //Использовать линейные интерполированные перемещения для всех осей
    linear();

    //Сбросить счетчик угла поворота
    angle = 0;

    //Задать скорость 100 оборотов в минуту для линейно интерполированных осей
    setF(100*360*sqrt(3));

    //Здесь и далее - Функция осущуствляющая выезд всех моторов в заданный угол
    rotate(angle);

    //Бесконечный цикл
    //Программа работает в два этапа. Каждый этап имеет свои скорости:
    //speedFirstStage
    //speedSecondStage
    while (1) {

        //Это костыль - улучшить динамику вентильного мотора(он второй), когда остальные два
        //мотора были асинхронные
        Motor[2].Move.InvAmax = 25;

        //Цикл разгона
        for (int i = 1; i < numStep; i++){

            //Расчитать новую скорость(чем больше i,  тем больше скорость)
            programmSpeed = speedFirstStage*i/numStep;

            //Принять новую скорость
            setF(programmSpeed*360*sqrt(3));

            //Рассчитать угол в который необходимо приехать
            angle = angle + angleStep;

            //Выехать всеми моторами в рассчитанный угол
            rotate(angle);
        }

        //Рассчитать скорость максимальной для первого этапа
        programmSpeed = speedFirstStage;

        //Принять максимальную скорость для первого этапа
        setF(programmSpeed*360*sqrt(3));

        //Рассчитать угол в который необходимо переехать(отработать 50 оборотов)
        angle = angle+360*50;

        //Отработать переезд в 50 оборотов
        rotate(angle);

        //Цикл торможения(аналогичен разгону, но скорость уменьшается)
        for (int i = 1; i < numStep; i++){
            programmSpeed = speedFirstStage/i;
            setF(programmSpeed*360*sqrt(3));
            angle = angle + angleStep;
            rotate(angle);
        }

        //Это костыль - ухудшить динамику вентильного мотора(он второй), когда остальные два
        //мотора были асинхронные
        Motor[2].Move.InvAmax = 5;

        //Рассчитать скорость второго этапа
        programmSpeed = speedSecondStage;

        //Принять скорость второго этапа
        setF(programmSpeed*360);

        //Комманда здесь и далее - Подождать (время в миллисекундах указывается в скобках)
        dwell(3*paused);

        //Рассчитать для переезда(пол оборота в плюс)
        angle=angle+180;

        //Перехать мотором Х в angle
        move({.X = angle});
        dwell(paused);

        //Перехать мотором Y в angle
        move({.Y = angle});
        dwell(paused);

        //Перехать мотором Z в angle
        move({.Z = angle});
        dwell(paused);

         //Рассчитать для переезда(пол оборота минус)
        angle=angle-180;

         //Перехать мотором Z в angle
        move({.Z = angle});
        dwell(paused);

        //Перехать мотором Y в angle
        move({.Y = angle});
        dwell(paused);

        //Перехать мотором Х в angle
        move({.X = angle});
        dwell(paused);

        //Принять скорость движения моторов(отличается о вышеописанного в sqrt(3))
        setF(programmSpeed*360*sqrt(3));

        //Рассчитать для переезда(один оборот в плюс)
        angle = angle+360;

        //Переехать тремя моторами
        rotate(angle);
        dwell(3*paused);

        //Рассчитать для переезда(один оборот в плюс)
        angle = angle-360;

        //Переехать тремя моторами
        rotate(angle);
        dwell(3*paused);
    }
}

void stend()
{
    XXX = (Motor[0].Move.ActPos - Motor[0].Trig.HomePos)/10000;
    YYY = (Motor[1].Move.ActPos - Motor[1].Trig.HomePos)/10000;
    ZZZ = (Motor[2].Move.ActPos - Motor[2].Trig.HomePos)/4096;
    button.PultBtn[0] = Servo[0].IO[0].DataIn[0];
    button.PultBtn[3] = Servo[0].IO[0].DataIn[3];
    if (!button.cncOff){ // Ключ в положении "Включено"
        if (button.modeHome != doubleButton.modeHome){
            mode = mdHome;
            light.modeHome = 1;
            light.mdAuto = 0;
        }
        if (button.mdAuto != doubleButton.mdAuto){
            mode = mdAuto;
            light.modeHome = 0;
            light.mdAuto = 1;
        }
        switch (mode) {
        case mdHome: // Выезд в нулевую точку
            if (button.modeStart !=doubleButton.modeStart){
                homez(2);
                home(1);
                home(0);
                light.modeStart = 1;
            }
            if (isHomeComplete()){ // Выезд в нулевую точку завершён?
                light.modeStart = 0;
            }
            if (button.modeStop != doubleButton.modeStop){ // Останов
                for (int i = 0; i < NUMBER_OF_MOTORS; i++){
                    jogStop(i);
                }
            }
            break;
        case mdAuto: // Автоматический режим
            if (button.modeStart != doubleButton.modeStart)
            {
                begin (1,0); // Устанавка программы движения для КС 1
                run(1); // Запуск программы движения в КС 1
            }
            if (button.modeStop !=doubleButton.modeStop)
            {
                abort(1); // Прерывание программы движения для КС 1
                light.modeStart = 0;
            }
            break;
        case mdNull:
            if (!isInPos()){
                for (int i = 0; i < NUMBER_OF_MOTORS; i++){
                    jogStop(i); // Останов толчковых перемещений двигателей
                }
            }
            mode = mdWait;
        default:
            break;
        }
        Servo[0].IO[0].DataOut[0]=light.PultBtn[0];
        Servo[0].IO[0].DataOut[2] = 8;
    }else{ // Ключ в положении "Выключено"
        mode = cmdNull;
        Servo[0].IO[0].DataOut[2] = 0;
        Servo[0].IO[0].DataOut[0] = 0;
        light.PultBtn[0] = 0;
        if (isInPos()){
            for (int i = 0; i < NUMBER_OF_MOTORS ; i++){
                kill(i); // Отключить двигатели
            }
        }
    }
    doubleButton.PultBtn[0] = button.PultBtn[0];
    doubleButton.PultBtn[3] = button.PultBtn[3];
}
