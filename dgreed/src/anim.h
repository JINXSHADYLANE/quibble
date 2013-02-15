#ifndef ANIM_H
#define ANIM_H

#include "utils.h"
#include "sprsheet.h"

// Sprite animation system

// Everything is based around sequences, which are simply lists of 
// frame numbers. Sequences have actions which happen when they 
// finish - loop, play (other sequence), stop. Multiple sequences
// are combined into an animation. Animation has a starting sequence
// and a number of frames to show per second. It is not associated
// with sprite in any way! You can use same animations on different
// sprites.
//
// Functions with _ex postfix allows to explicitly provide time to them,
// otherwise real system time is used.

/*

(anims morka
	(anim rabbit
		(fps 15)
		(start_seq run)

		(seq run
			(frames "0 1 2 3 4")
			(on_finish loop)
		)

		(seq land
			(frames "11 12 13")
			(on_finish play.run)
		)
	)
)

*/

typedef enum {
	AS_LOOP = 1,
	AS_STOP = 2,
	AS_PLAY = 3
} AnimSeqFinish;

typedef struct FwdAnimSeq {
	byte* frames;
	uint n_frames;
	uint on_finish;
	struct FwdAnimSeq* play_seq;
} AnimSeq;

typedef struct {
	uint fps;
	AnimSeq* start_seq;
	AnimSeq* seqs;
	uint n_seqs;
} AnimDesc;

typedef struct {
	const char* name;		// This is used to construct seq lookup keys
	const AnimDesc* desc;
	const AnimSeq* seq;
	float play_t;
} Anim;

// Init anim subsystem, load desc mml file
void anim_init(const char* desc);
// Close anim subsystem
void anim_close(void);

// Create instance of named animation
Anim* anim_new(const char* name);
Anim* anim_new_ex(const char* name, float current_time);
// Free animation
void anim_del(Anim* anim);

// Play named sequence
void anim_play(Anim* anim, const char* seq);
void anim_play_ex(Anim* anim, const char* seq, float current_time);

// Get current animation frame
uint anim_frame(Anim* anim);
uint anim_frame_ex(Anim* anim, float current_time);

// Animated sprite draw helpers

void anim_draw(Anim* anim, const char* spr, uint layer, Vector2 dest,
		float rot, float scale, Color tint);

void anim_draw_h(Anim* anim, SprHandle spr, uint layer, Vector2 dest, 
		float rot, float scale, Color tint);

void anim_draw_ex(Anim* anim, float current_time, const char* spr,
		uint layer, Vector2 dest, float rot, float scale, Color tint);

void anim_draw_h_ex(Anim* anim, float current_time, SprHandle spr,
		uint layer, Vector2 dest, float rot, float scale, Color tint);


#endif

