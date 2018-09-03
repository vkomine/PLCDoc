union MTInputs {
  struct {
   // Первая плата входов
   unsigned chillerLubeLevelLow:1;  // Вход 0 - низкий уровень масла в охладителе шпинделя
   ...
   unsigned overloadPumpA:1;  // Вход 6 - перегрузка насоса СОЖ А 
   ...       
   unsigned mtOn:1;  // Вход 16 - станок включён 
   ...   
   unsigned controlServiceDoor:1;  // Вход 31 - контроль сервисной двери  
   //Вторая плата входов
   unsigned overloadChipConv:1;  // Вход 0  - перегрузка щипкового конвеера 
   ... 
   unsigned lubeLevelLow:1;  // Вход 8  - низкий уровень масла 
   ...
   unsigned unclampingAxisC:1;  // Вход 24  - ось С разжата
   ...
   unsigned coolantLevelLow:1;  // Вход 31 - СОЖ низкий уровень
  };
  int Inputs[2];
};

union MTOutputs {
  struct {
   // Первая плата реле 
   unsigned clearCoolantOn:1;  // Выход 0 - включение очистки СОЖ от масла
   ...
   unsigned chipConvOn:1;  // Выход 5 - включение конвейера стружки 
   ...
   unsigned workpieceBlast:1;  // Выход 14 - обдув рабочей зоны 
   ...
   unsigned operatorDoorOpen:1;	// Выход 23 - открытие двери оператора 
   // Вторая плата реле 
   unsigned clampingAxisA:1;  // Выход 0 - зажим оси А
   ...
   unsigned pumpA:1;  // Выход 4  - включение насоса А подачи СОЖ
   ...
   unsigned spindleChiller:1;  // Выход 16 - включение охлаждения шпинделя
   ...
   unsigned unclampingAxisA:1;  // Выход 23 - разжим оси А
  };
  int Outputs[2];
};