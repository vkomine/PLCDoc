union MTInputs {
  struct {
   // Первая плата входов
   unsigned chillerLubeLevelLow:1;  // Вход 0 - низкий уровень масла в охладителе шпинделя
   unsigned clearCoolantOverload:1;	// Вход 1 - перегрузка очистителя СОЖ
   unsigned onFanSPND:1; // Вход 2 - вентилятор шпинделя включён
   unsigned overloadPumpA:1;  // Вход 3 - перегрузка насоса СОЖ А   
   unsigned mtOn:1;  // Вход 4 - станок включён 
   unsigned reserved1:26;
   unsigned controlServiceDoor:1;  // Вход 31 - контроль сервисной двери  
   //Вторая плата входов
   unsigned overloadChipConv:1;  // Вход 0  - перегрузка щипкового конвеера 
   unsigned lubeLevelLow:1;  // Вход 1  - низкий уровень масла 
   unsigned coolantFilterUnclean:1;	// Вход 2 - фильтр СОЖ засорён
   unsigned unclampingAxisC:1;  // Вход 3  - ось С разжата
   unsigned reserved2:27;
   unsigned coolantLevelLow:1;  // Вход 31 - СОЖ низкий уровень
  };
  int Inputs[2];
};

union MTOutputs {
  struct {
   // Первая плата реле 24 выхода
   unsigned clearCoolantOn:1;  // Выход 0 - включение очистки СОЖ от масла
   unsigned chipConvOn:1;  // Выход 1 - включение конвейера стружки 
   unsigned autoLubeOn:1;	// Выход 2 платы - включение автоматической смазки ШВП
   unsigned workingLight:1;	// Выход 3 - включение рабочего освещения
   unsigned workpieceBlast:1;  // Выход 4 - обдув рабочей зоны 
   unsigned reserved1:18;
   unsigned operatorDoorOpen:1;	// Выход 23 - открытие двери оператора 
   // Вторая плата реле 8 выходов
   unsigned clampingAxisA:1;  // Выход 0 - зажим оси А
   unsigned pumpA:1;  // Выход 1  - включение насоса А подачи СОЖ
   unsigned screwAugerCW:1;	// Выход 2 платы  - вращение шнекового транспортёра по часовой стрелке  
   unsigned screwAugerCCW:1; // Выход 3 платы - вращение шнекового транспортёра против часовой стрелки
   unsigned pumpA:1;  // Выход 4  - включение насоса А подачи СОЖ
   unsigned spindleChiller:1;  // Выход 5 - включение охлаждения шпинделя
   unsigned reserved2:1;
   unsigned unclampingAxisA:1;  // Выход 7 - разжим оси А
  };
  int Outputs[2];
};