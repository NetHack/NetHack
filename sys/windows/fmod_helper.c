#include "fmod_helper.h"

void init_fmod() {
    FMOD_System_Create(&systemvar, FMOD_VERSION);
    FMOD_System_Init(systemvar, 32, FMOD_INIT_NORMAL, 0);
}
