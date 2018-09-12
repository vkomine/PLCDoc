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
        unsigned reserved1:1;   			
        unsigned mdAuto:1;			// Режим MDI
        unsigned reserved2:4;
        unsigned modeStart:1;		// Старт
        unsigned modeStop:1;		// Стоп
        unsigned reserved3:16;			
        unsigned reserved4:32;
        unsigned reserved5:32;
        unsigned reserved6:25;
        unsigned cncOff: 1;			// Вход от ключа
        unsigned reserved7:5;
    };
    int PultBtn[4];
};

double XXX,YYY,ZZZ;
int debag = 0, mode;
double programmSpeed = 10;
double kSpeed = 5;
double speedFirstStage = 700;
double speedSecondStage = 1000;
double ks = 500;
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
    assignMotor(1, {.Z = 10000./360.}); // Привязка двигателя 1 к оси Z
    assignMotor(2, {.Y = 4096./360.}); // Привязка двигателя 2 к оси Y
}

void rotate(int a) // Движение в заданное положение
{
    move({.X = a, .Y = a, .Z = a});
}

int isHomeComplete(){
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

int isInPos()
{
    int snps = 0;
    for (int i = 0; i <NUMBER_OF_MOTORS; i++){
        if (Motor[i].Servo.ClosedLoop == 0){
            snps++;
        }
    }

    if (snps){
        return 0;
    }else{
        return 1;
    }
}

