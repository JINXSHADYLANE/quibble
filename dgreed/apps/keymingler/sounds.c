#include "sounds.h"
#include <system.h>
#include "common.h" 

#define SHOT_FILE ASSETS_DIR "suvis.wav"
#define BURN_FILE ASSETS_DIR "sprogsta.wav"
#define BULBUL_FILE ASSETS_DIR "bulbul.wav"
#define SINKED_FILE ASSETS_DIR "atsimuse.wav"

SoundHandle sound_shot;
SoundHandle sound_burning;
SoundHandle sound_bulbul;
SoundHandle sound_sinked;

void sounds_init(void)
{
	sound_init();

	sound_shot = sound_load_sample(SHOT_FILE);
	sound_burning = sound_load_sample(BURN_FILE);
	sound_bulbul = sound_load_sample(BULBUL_FILE);
	sound_sinked = sound_load_sample(SINKED_FILE);
}

void sounds_close(void)
{
	sound_free(sound_shot);
	sound_free(sound_burning);
	sound_free(sound_bulbul);
	sound_free(sound_sinked);


	sound_close();
}	
