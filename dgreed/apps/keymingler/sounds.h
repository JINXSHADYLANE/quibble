#ifndef SOUNDS_H
#define SOUNDS_H

#include <system.h>

extern SoundHandle sound_burning;
extern SoundHandle sound_bulbul;
extern SoundHandle sound_sinked;
extern SoundHandle sound_missed;

void sounds_init(void);
void sounds_close(void);


#endif

