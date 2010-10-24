#ifndef PARTICLES_H
#define PARTICLES_H

#include "utils.h"
#include "darray.h"
#include "system.h"

typedef struct {
	float birth_time;
	float death_time;
	Vector2 pos;
	float start_dir;
	float start_speed;
	float end_dir;
	float end_speed;
	Color start_color;
	Color end_color;
	float start_size;
	float end_size;
	float start_angle;
	float end_angle;
} Particle;	

#define PSYSTEM_DESC_NAME_LENGTH 16 

typedef struct {
	char name[PSYSTEM_DESC_NAME_LENGTH+1];
	TexHandle texture;
	RectF tex_source;
	uint max_particles;
	float duration;
	float direction;
	float emission_rate;
	float min_lifetime;
	float max_lifetime;
	float min_start_dir;
	float max_start_dir;
	float min_start_speed;
	float max_start_speed;
	float min_end_dir;
	float max_end_dir;
	float min_end_speed;
	float max_end_speed;
	float min_start_size;
	float max_start_size;
	float min_end_size;
	float max_end_size;
	float min_start_angle;
	float max_start_angle;
	float min_end_angle;
	float max_end_angle;
	Color min_start_color;
	Color max_start_color;
	Color min_end_color;
	Color max_end_color;
} ParticleSystemDesc;	

typedef struct {
	ParticleSystemDesc* desc;
	Vector2 pos;
	float direction;
	float age;
	float emission_acc;
	uint particle_count;
	Particle* particles;
	bool active;
} ParticleSystem;	

#ifndef NO_DEVMODE
typedef struct {
	uint psystems;
	uint active_psystems;
	uint total_particles;
	uint born_count;
	uint born_in_last_second;
	uint dead_count;
	uint dead_in_last_second;
} ParticleStats;	

const ParticleStats* particle_stats(void);
#endif

#define MAX_PSYSTEM_DESCS 16
#define MAX_PSYSTEMS 48

extern ParticleSystemDesc psystem_descs[MAX_PSYSTEM_DESCS];
extern ParticleSystem psystems[MAX_PSYSTEMS];

extern uint psystem_descs_count;

void particles_init(const char* assets_prefix, uint layer);
void particles_save(void);
void particles_close(void);

ParticleSystem* particles_spawn(const char* name, const Vector2* pos, float dir); 
void particles_update(float time);
void particles_draw(void);

#endif

