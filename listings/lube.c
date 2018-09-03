#include "sys/sys.h"
#include "include/platform/stanok/stanok.h"
#include "include/platform/stanok/lube.h"

// контроль смазки направляющих
int hasLubeError() {
    if (mt.IN.lubeLevelLow == 0)
        return 1;
    else
        return 0;
}

// включение смазки направляющих
void lubeOn() {
    mt.OUT.autoLubeOn = 1;
}

// выключение смазки направляющих
void lubeOff() {
    mt.OUT.autoLubeOn = 0;
}

