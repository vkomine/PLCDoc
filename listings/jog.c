enum Direction { // Направление движения или останов
	dirMinus = 0,
	dirPlus,
	dirStop
};

enum JogSpeed { // Скорость толчкового движения
	jogSpeed0 = 0,
	jogSpeed1 = 1,
	jogSpeed2 = 2,
	jogSpeed3 = 4
};

PLC(1, jogPlc) // Объявление программы ПЛК

void setup() {
    enablePLC(1); // Разрешение выполнения программы ПЛК
}

void jogPlc()
{
   int dir, data;

   // Чтение входных данных: 
   // первые 4 бита определяют направление движения, следующие 3 бита - скорость
   data = Servo[0].IO[3].DataIn[0]; 
   
   // Если данные нулевые, то выполняется останов 
   if (data==0) dir = dirStop;
   // Если данные не нулевые, то первый бит определяет направление движения
   else dir = data & 1;

   // Сдвиг данных на 4 бита 
   data = (data >> 4) & 7;
   
   // Запись скорости толчкового движения в переменную Motor[0].Move.JogSpeed
   switch (data) {
   case jogSpeed0:
       Motor[0].Move.JogSpeed = 0;
       break;
   case jogSpeed1:
       Motor[0].Move.JogSpeed = 10;
       break;
   case jogSpeed2:
        Motor[0].Move.JogSpeed = 25;
        break;
   case jogSpeed3:
        Motor[0].Move.JogSpeed = 50;
        break;	
    default:
        Motor[0].Move.JogSpeed = 0;
   }

   if (dir == dirPlus)
   {
	   // Толчковое перемещение в положительном направлении двигателем 0
       jogPlus(0); 
   }
   else if (dir == dirStop)
   {
	   // Останов толчкового перемещения двигателя 0
       jogStop(0);
   }
   else if (dir == dirMinus)
   {
	   // Толчковое перемещение в отрицательном направлении двигателем 0
       jogMinus(0);
   }
}
