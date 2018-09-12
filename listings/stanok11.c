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
        unsigned modeManual:1;      // Ручной режим
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
    Local.coord = 1; // Задание активной координатной системы
	// Число сегментов движения в буфере опережающего просмотра
    Coord[1].LHSize = 1024; 
	// Включение функции буферизации опережающего просмотра для координатной 
	// системы и задание дистанции опережающего просмотра
    Coord[1].LHDistance = 1024; 
    assignMotor(0, {.X = 10000./360.}); // Привязка двигателя 0 к оси X
    assignMotor(1, {.Y = 10000./360.}); // Привязка двигателя 1 к оси Y
    assignMotor(2, {.Z = 10000./360.}); // Привязка двигателя 2 к оси Z
}

void moveMotors(int a) // Перемещение в заданное положение
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
    if (hmcmplt) return 0;
    else return 1;
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

// Программа движения
void motion_nc(){ 
    //Счетчик угла поворота
    static double angle = 0;
	
    //Включить индикацию работы программы
    light.modeStart = 1;
    //Использовать абсолютные перемещения для моторов осей Х, Y и Z
    absAxes(axXYZ);
    //Использовать линейные интерполированные перемещения для всех осей
    linear();
    //Сбросить счетчик угла поворота
    angle = 0;
    //Задать скорость 100 оборотов в минуту для линейно интерполированных осей
    setF(100*360*sqrt(3));
    //Перемещение в заданное положение всеми двигателем
    moveMotors(angle);

    // Бесконечный цикл. Программа работает в два этапа. 
    // Каждый этап имеет максимальные скорости: speedFirstStage и speedSecondStage
    while (1) {
        //Цикл разгона
        for (int i = 1; i < numStep; i++){

            //Расчитать новую скорость(чем больше i,  тем больше скорость)
            programmSpeed = speedFirstStage*i/numStep;
            //Задание новой скорости
            setF(programmSpeed*360*sqrt(3));
            //Рассчитать перемещение
            angle = angle + angleStep;
            //Перемещение в заданное положение всеми двигателем
            moveMotors(angle);
        }

        //Присвоить скорость speedFirstStage
        programmSpeed = speedFirstStage;
        //Задание максимальной скорости для первого этапа
        setF(programmSpeed*360*sqrt(3));
        //Рассчитать перемещение (отработать 50 оборотов)
        angle = angle+360*50;
        //Перемещение в заданное положение всеми двигателем
        moveMotors(angle);

        //Цикл торможения (аналогичен разгону, но скорость уменьшается)
        for (int i = 1; i < numStep; i++){
            programmSpeed = speedFirstStage/i;
			//Задание новой скорости
            setF(programmSpeed*360*sqrt(3));
            angle = angle + angleStep;
			//Перемещение в заданное положение всеми двигателем
            moveMotors(angle);
        }

        //Рассчитать скорость второго этапа
        programmSpeed = speedSecondStage;
        //Задание скорости второго этапа
        setF(programmSpeed*360);
        //Приостанов движения всех осей
        dwell(3*paused);
        //Рассчитать перемещение (полоборота в плюс)
        angle=angle+180;
        //Перемещение двигателем Х в заданное положение
        move({.X = angle});
        dwell(paused);
        //Перемещение двигателем Y в заданное положение
        move({.Y = angle});
        dwell(paused);
        //Перемещение двигателем Z в заданное положение
        move({.Z = angle});
        dwell(paused);
         //Рассчитать перемещение (полоборота в минус)
        angle=angle-180;
         //Перемещение двигателем Z в заданное положение
        move({.Z = angle});
        dwell(paused);
        //Перемещение двигателем Y в заданное положение
        move({.Y = angle});
        dwell(paused);
