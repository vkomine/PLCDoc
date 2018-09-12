#include "sys/sys.h"
#define NUMBER_OF_MOTORS 3

enum MODE { // Режимы работы
    mdNull,
    mdHome,
    mdAuto,
    mdWait,
};

enum COMMANDS { // Команды
    cmdNull,
    cmdStop,
    cmdStart,
};

union Button { // Кнопки пульта оператора
    struct{
        unsigned modeManual:1;		// Ручной режим
        unsigned modeHome:1;		// Ручной выезда в нулевую точку
        unsigned reserved0:4;         
        unsigned modeAuto:1;		// Автоматический режим
        unsigned modeStep:1;		// Пошаговый режим
        unsigned reserved1:6;   			
        unsigned modeStart:1;		// Старт
        unsigned modeStop:1;		// Стоп
        unsigned reserved2:16;			
        unsigned reserved3:32;
        unsigned reserved4:32;
        unsigned reserved5:25;
        unsigned cncOff: 1;			// Вход от ключа
        unsigned reserved6:5;
    };
    int PultBtn[4];
};

int mode;
double programmSpeed = 10;
double speedFirstStage = 700;
double speedSecondStage = 1000;
int paused = 200;
int numStep = 10;
int angleStep = 100;
Button button, doubleButton, light;

PLC(1, stend) // Объявление программы ПЛК

void setup()
{
    enablePLC(1); // Разрешение выполнения программы ПЛК
    Local.coord = 1;
	// Число сегментов движения в буфере опережающего просмотра
    Coord[1].LHSize = 1024; 
	// Включение функции буферизации опережающего просмотра для координатной системы и задание дистанции опережающего просмотра
    Coord[1].LHDistance = 1024; 
    assignMotor(0, {.X = 10000./360.}); // Привязка двигателя 0 к оси X
    assignMotor(1, {.Y = 10000./360.}); // Привязка двигателя 1 к оси Y
    assignMotor(2, {.Z = 10000./360.}); // Привязка двигателя 2 к оси Z
}

void rotate(int a) // Перемещение в заданное положение
{
	// Перемещение в заданное положение в установленном режиме движения
    move({.X = a, .Y = a, .Z = a}); 
}

int isHomeComplete(){ // Проверка выезда в нулевую точку
    int hmcmplt = 0;
    for (int i = 0; i <NUMBER_OF_MOTORS; i++){
        if (Motor[i].Trig.HomeComplete != 1){
            hmcmplt++;
        }
    }

    if (hmcmplt){
        return 0;
    }else{
        return 1;
    }
}

int isClosedLoop() // Проверка режима слежения
{
    int snps = 0;
    for (int i = 0; i <NUMBER_OF_MOTORS; i++){
        if (Motor[i].Servo.ClosedLoop == 1){
            snps++;
        }
    }

    return snps;
}

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

    //Здесь и далее - Функция осущуствляющая выезд всех моторов в заданное положение
    rotate(angle);

    //Бесконечный цикл
    //Программа работает в два этапа. Каждый этап имеет максимальные скорости: speedFirstStage и speedSecondStage
    while (1) {
        //Цикл разгона
        for (int i = 1; i < numStep; i++){

            //Расчитать новую скорость(чем больше i,  тем больше скорость)
            programmSpeed = speedFirstStage*i/numStep;

            //Принять новую скорость
            setF(programmSpeed*360*sqrt(3));

            //Рассчитать перемещение
            angle = angle + angleStep;

            //Перемещение в заданное положение
            rotate(angle);
        }

        //Присвоить скорость speedFirstStage
        programmSpeed = speedFirstStage;

        //Принять максимальную скорость для первого этапа
        setF(programmSpeed*360*sqrt(3));

        //Рассчитать перемещение (отработать 50 оборотов)
        angle = angle+360*50;

        //Перемещение в заданное положение 
        rotate(angle);

        //Цикл торможения (аналогичен разгону, но скорость уменьшается)
        for (int i = 1; i < numStep; i++){
            programmSpeed = speedFirstStage/i;
            setF(programmSpeed*360*sqrt(3));
            angle = angle + angleStep;
            rotate(angle);
        }

        //Рассчитать скорость второго этапа
        programmSpeed = speedSecondStage;

        //Принять скорость второго этапа
        setF(programmSpeed*360);

        //Приостанов движения всех осей
        dwell(3*paused);

        //Рассчитать перемещение (полоборота в плюс)
        angle=angle+180;

        //Перехать двигателем Х в angle
        move({.X = angle});
        dwell(paused);

        //Перехать двигателем Y в angle
        move({.Y = angle});
        dwell(paused);

        //Перехать двигателем Z в angle
        move({.Z = angle});
        dwell(paused);

         //Рассчитать перемещение (полоборота в минус)
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

        //Принять скорость движения
        setF(programmSpeed*360*sqrt(3));

        //Рассчитать перемещение (один оборот в плюс)
        angle = angle+360;

        //Перемещение в заданное положение
        rotate(angle);
        dwell(3*paused);

        //Рассчитать перемещение (один оборот в плюс)
        angle = angle-360;

        //Перемещение в заданное положение
        rotate(angle);
        dwell(3*paused);
    }
}

void stend()
{
    button.PultBtn[0] = Servo[0].IO[0].DataIn[0];
    button.PultBtn[3] = Servo[0].IO[0].DataIn[3];
	
    if (!button.cncOff){ // Ключ в положении "Включено"
	    // Нажата кнопка "Выезд в нулевую точку"
        if (button.modeHome != doubleButton.modeHome){
            mode = mdHome;
            light.modeHome = 1;
            light.modeAuto = 0;
        }
		// Нажата кнопка "Автоматический режим"
        if (button.modeAuto != doubleButton.modeAuto){
            mode = mdAuto;
            light.modeHome = 0;
            light.modeAuto = 1;
        }
        switch (mode) {
        case mdHome: // Режим выезда в нулевую точку
			// Нажата кнопка "Старт" - начать выезд в нулевую точку
            if (button.modeStart !=doubleButton.modeStart){
                home(2);
                home(1);
                home(0);
                light.modeStart = 1;
            }
            if (isHomeComplete()){ // Выезд в нулевую точку завершён?
                light.modeStart = 0;
            }
			// Нажата кнопка "Стоп"
            if (button.modeStop != doubleButton.modeStop){ // Останов
                for (int i = 0; i < NUMBER_OF_MOTORS; i++){
                    jogStop(i); // Останов толчкового перемещения двигателя
                }
            }
            break;
        case mdAuto: // Автоматический режим
		    // Нажата кнопка "Старт"
            if (button.modeStart != doubleButton.modeStart)
            {
                begin (1,0); // Устанавка программы движения для КС 1
                run(1); // Запуск программы движения в КС 1
            }
			// Нажата кнопка "Стоп"
            if (button.modeStop !=doubleButton.modeStop)
            {
                abort(1); // Прерывание программы движения для КС 1
                light.modeStart = 0;
            }
            break;
        case mdNull:
            if (!isClosedLoop()){ // Хотя бы один двигатель не находится в слежении?
                for (int i = 0; i < NUMBER_OF_MOTORS; i++){
                    jogStop(i); // Включение двигателей в слежение
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
        if (isClosedLoop()){ // Хотя бы один двигатель находится в слежении?
            for (int i = 0; i < NUMBER_OF_MOTORS ; i++){
                kill(i); // Отключить двигатели
            }
        }
    }
	
    doubleButton.PultBtn[0] = button.PultBtn[0]; // Фиксация изменения нажатия кнопки
    doubleButton.PultBtn[3] = button.PultBtn[3]; // Фиксация изменения нажатия кнопки
}
