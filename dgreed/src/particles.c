#include "particles.h"
#include "mml.h"
#include "memory.h"

static uint particles_layer;
static const char* particles_file = "particles.mml";

#define APPEND_NODE(parent, name, value, type) { \
	prop = mml_node(&mml, name, ""); \
	mml_setval_##type(&mml, prop, value); \
	mml_append(&mml, parent, prop); \
}	

#define GET_NODEVAL(node, name, dest, type) { \
	prop = mml_get_sibling(&desc, node, name); \
	if(!prop) LOG_ERROR("Propierty " name "is missing or in wrong place"); \
	dest = mml_getval_##type(&desc, node); \
}	

#ifndef NO_DEVMODE
ParticleStats p_stats;
#endif

ParticleSystemDesc psystem_descs[MAX_PSYSTEM_DESCS];
ParticleSystem psystems[MAX_PSYSTEMS];
uint psystem_descs_count = 0;
TexHandle particles_texture;

static float last_time = 0.0f;

#ifndef NO_DEVMODE
const ParticleStats* particle_stats(void) {
	return &p_stats;
}
#endif

void particles_init(const char* assets_prefix, uint layer) {
	particles_layer = layer;

	// Construct paths
	char desc_path[256];
	if(assets_prefix) {
		strcpy(desc_path, assets_prefix);
	}
	else {
		desc_path[0] = '\0';
	}
	strcat(desc_path, particles_file);

	char* desc_text = txtfile_read(desc_path);
	if(!desc_text)
		LOG_ERROR("Unable to read particle description file");

	MMLObject desc;
	if(!mml_deserialize(&desc, desc_text))
		LOG_ERROR("Failed to deserialize particle description file");

	MEM_FREE(desc_text);	

	NodeIdx root = mml_root(&desc);
	if(strcmp(mml_get_name(&desc, root), "particles") != 0)
		LOG_ERROR("Invalid particle description file");

	uint i = 0;	
	NodeIdx psystem_desc = mml_get_first_child(&desc, root);
	while(psystem_desc) {
		if(strcmp(mml_get_name(&desc, psystem_desc), "psystem") != 0)
			LOG_ERROR("Bad particle system description");

		const char* name = mml_getval_str(&desc, psystem_desc);	
		uint name_len = strlen(name);

		if(name_len > PSYSTEM_DESC_NAME_LENGTH)
			LOG_ERROR("Particle system name %s is too long", name);

		strcpy(psystem_descs[i].name, name);	

		// Get first propierty in a special way
		NodeIdx prop = mml_get_child(&desc, psystem_desc, "texture");
		if(!prop)
			LOG_ERROR("Propierty texture is missing or in wrong place");
		
		// Load texture
		particles_texture = tex_load(mml_getval_str(&desc, prop));
		psystem_descs[i].texture = particles_texture;	

		const char* tex_source;
		// Read other properties by doing sibling search chain
		GET_NODEVAL(prop, "tex_source", tex_source, str);
		sscanf(tex_source, "%f,%f,%f,%f", &(psystem_descs[i].tex_source.left),
			&(psystem_descs[i].tex_source.top),
			&(psystem_descs[i].tex_source.right),
			&(psystem_descs[i].tex_source.bottom));

		GET_NODEVAL(prop, "max_particles", psystem_descs[i].max_particles, uint);
		GET_NODEVAL(prop, "duration", psystem_descs[i].duration, float);
		GET_NODEVAL(prop, "direction", psystem_descs[i].direction, float);
		GET_NODEVAL(prop, "emission_rate", psystem_descs[i].emission_rate, float);
		GET_NODEVAL(prop, "min_lifetime", psystem_descs[i].min_lifetime, float);
		GET_NODEVAL(prop, "max_lifetime", psystem_descs[i].max_lifetime, float);
		GET_NODEVAL(prop, "min_start_dir", psystem_descs[i].min_start_dir, float);
		GET_NODEVAL(prop, "max_start_dir", psystem_descs[i].max_start_dir, float);
		GET_NODEVAL(prop, "min_start_speed", psystem_descs[i].min_start_speed, float);
		GET_NODEVAL(prop, "max_start_speed", psystem_descs[i].max_start_speed, float);
		GET_NODEVAL(prop, "min_end_dir", psystem_descs[i].min_end_dir, float);
		GET_NODEVAL(prop, "max_end_dir", psystem_descs[i].max_end_dir, float);
		GET_NODEVAL(prop, "min_end_speed", psystem_descs[i].min_end_speed, float);
		GET_NODEVAL(prop, "max_end_speed", psystem_descs[i].max_end_speed, float);
		GET_NODEVAL(prop, "min_start_size", psystem_descs[i].min_start_size, float);
		GET_NODEVAL(prop, "max_start_size", psystem_descs[i].max_start_size, float);
		GET_NODEVAL(prop, "min_end_size", psystem_descs[i].min_end_size, float);
		GET_NODEVAL(prop, "max_end_size", psystem_descs[i].max_end_size, float);
		GET_NODEVAL(prop, "min_start_angle", psystem_descs[i].min_start_angle, float);
		GET_NODEVAL(prop, "max_start_angle", psystem_descs[i].max_start_angle, float);
		GET_NODEVAL(prop, "min_end_angle", psystem_descs[i].min_end_angle, float);
		GET_NODEVAL(prop, "max_end_angle", psystem_descs[i].max_end_angle, float);

		psystem_descs[i].min_start_dir *= DEG_TO_RAD;
		psystem_descs[i].max_start_dir *= DEG_TO_RAD;
		psystem_descs[i].min_end_dir *= DEG_TO_RAD;
		psystem_descs[i].max_end_dir *= DEG_TO_RAD;
		psystem_descs[i].min_start_angle *= DEG_TO_RAD;
		psystem_descs[i].max_start_angle *= DEG_TO_RAD;
		psystem_descs[i].min_end_angle *= DEG_TO_RAD;
		psystem_descs[i].max_end_angle *= DEG_TO_RAD;

		const char* color;
		uint r, g, b, a;

		GET_NODEVAL(prop, "min_start_color", color, str);
		sscanf(color, "%u,%u,%u,%u", &r, &g, &b, &a);
		psystem_descs[i].min_start_color = COLOR_RGBA(r, g, b, a);
		
		GET_NODEVAL(prop, "max_start_color", color, str);
		sscanf(color, "%u,%u,%u,%u", &r, &g, &b, &a);
		psystem_descs[i].max_start_color = COLOR_RGBA(r, g, b, a);
		
		GET_NODEVAL(prop, "min_end_color", color, str);
		sscanf(color, "%u,%u,%u,%u", &r, &g, &b, &a);
		psystem_descs[i].min_end_color = COLOR_RGBA(r, g, b, a);
		
		GET_NODEVAL(prop, "max_end_color", color, str);
		sscanf(color, "%u,%u,%u,%u", &r, &g, &b, &a);
		psystem_descs[i].max_end_color = COLOR_RGBA(r, g, b, a);

		psystem_desc = mml_get_next(&desc, psystem_desc);
		i++;
	}	
	psystem_descs_count = i;
	mml_free(&desc);

	for(i = 0; i < MAX_PSYSTEMS; ++i) {
		psystems[i].active = false;
	}	

	#ifndef NO_DEVMODE
	memset(&p_stats, 0, sizeof(p_stats));
	p_stats.psystems = psystem_descs_count;
	#endif
}	

// This is not used anywhere
/*
void particles_save(void) {
	MMLObject mml;
	mml_empty(&mml);

	mml_set_name(&mml, mml_root(&mml), "particles");

	uint i;
	for(i = 0; i < psystem_descs_count; ++i) {
		ParticleSystemDesc* desc = &(psystem_descs[i]);
		NodeIdx mml_desc = mml_node(&mml, "psystem", desc->name);

		NodeIdx prop;
		char rect_desc[128];
		sprintf(rect_desc, "%f,%f,%f,%f", desc->tex_source.left,
			desc->tex_source.top, desc->tex_source.right, 
			desc->tex_source.bottom);

		APPEND_NODE(mml_desc, "texture", PARTICLES_TEXTURE, str);
		APPEND_NODE(mml_desc, "tex_source", rect_desc, str);
		APPEND_NODE(mml_desc, "max_particles", desc->max_particles, uint);
		APPEND_NODE(mml_desc, "duration", desc->duration, float);
		APPEND_NODE(mml_desc, "direction", desc->direction, float);
		APPEND_NODE(mml_desc, "emission_rate", desc->emission_rate, float);
		APPEND_NODE(mml_desc, "min_lifetime", desc->min_lifetime, float);
		APPEND_NODE(mml_desc, "max_lifetime", desc->max_lifetime, float);
		APPEND_NODE(mml_desc, "min_start_dir", desc->min_start_dir, float);
		APPEND_NODE(mml_desc, "max_start_dir", desc->max_start_dir, float);
		APPEND_NODE(mml_desc, "min_start_speed", desc->min_start_speed, float);
		APPEND_NODE(mml_desc, "max_start_speed", desc->max_start_speed, float);
		APPEND_NODE(mml_desc, "min_end_dir", desc->min_end_speed, float);
		APPEND_NODE(mml_desc, "max_end_dir", desc->max_end_dir, float);
		APPEND_NODE(mml_desc, "min_end_speed", desc->min_end_speed, float);
		APPEND_NODE(mml_desc, "max_end_speed", desc->max_end_speed, float);
		APPEND_NODE(mml_desc, "min_start_size", desc->min_start_size, float);
		APPEND_NODE(mml_desc, "max_start_size", desc->max_start_size, float);
		APPEND_NODE(mml_desc, "min_end_size", desc->min_end_size, float);
		APPEND_NODE(mml_desc, "max_end_size", desc->max_end_size, float);

		byte r, g, b, a;
		char color[128];

		COLOR_DECONSTRUCT(desc->min_start_color, r, g, b, a);
		sprintf(color, "%hhu,%hhu,%hhu,%hhu", r, g, b, a);
		APPEND_NODE(mml_desc, "min_start_color", color, str);

		COLOR_DECONSTRUCT(desc->max_start_color, r, g, b, a);
		sprintf(color, "%hhu,%hhu,%hhu,%hhu", r, g, b, a);
		APPEND_NODE(mml_desc, "max_start_color", color, str);

		COLOR_DECONSTRUCT(desc->min_end_color, r, g, b, a);
		sprintf(color, "%hhu,%hhu,%hhu,%hhu", r, g, b, a);
		APPEND_NODE(mml_desc, "min_end_color", color, str);

		COLOR_DECONSTRUCT(desc->max_end_color, r, g, b, a);
		sprintf(color, "%hhu,%hhu,%hhu,%hhu", r, g, b, a);
		APPEND_NODE(mml_desc, "max_end_color", color, str);

		mml_append(&mml, mml_root(&mml), mml_desc);
	}	

	char* serialized_desc = mml_serialize(&mml);
	txtfile_write(PARTICLES_FILENAME, serialized_desc); 
	MEM_FREE(serialized_desc);
}
*/

void particles_close(void) {
	for(uint i = 0; i < MAX_PSYSTEMS; ++i) {
		if(psystems[i].active) {
			MEM_FREE(psystems[i].particles);
		}
	}
	tex_free(particles_texture);
}	

ParticleSystem* particles_spawn(const char* name, const Vector2* pos,
	float dir) {
	assert(name);
	assert(pos);

	// Find desc
	uint desc_idx = 0;
	while(strcmp(psystem_descs[desc_idx].name, name) != 0) {
		desc_idx++;
	}	
	if(desc_idx == psystem_descs_count) 
		LOG_ERROR("Trying to spawn psystem with non-existing description");
		
	// Find empty slot in psystems pool
	uint psystem_idx = 0;
	while(psystems[psystem_idx].active) {
		psystem_idx++;
	}	
	if(psystem_idx == MAX_PSYSTEMS) {
		LOG_INFO("too many psystems, skipping spawn");
		return NULL;
	}	
		
	// Fill struct
	ParticleSystem* psystem = &(psystems[psystem_idx]);
	psystem->desc = &(psystem_descs[desc_idx]);
	psystem->pos = *pos;
	psystem->direction = dir + psystem->desc->direction;
	psystem->age = 0.0f;
	psystem->emission_acc = 0.0f;
	psystem->particle_count = 0;
	psystem->particles = (Particle*)MEM_ALLOC(sizeof(Particle) *
		psystem->desc->max_particles);	
	psystem->active = true;

	return psystem;
}	

void _psystem_update(ParticleSystem* psystem, float dt) {
	assert(psystem);

	psystem->age += dt;
	if(psystem->age < psystem->desc->duration)
		psystem->emission_acc += dt;

	// Update old particles
	uint i;
	for(i = 0; i < psystem->particle_count; ++i) {
		Particle* p = &(psystem->particles[i]);
		if(psystem->age >= p->death_time) {
			// Particle is dead, copy last one to its place

			#ifndef NO_DEVMODE
			p_stats.dead_count++;
			#endif

			*p = psystem->particles[psystem->particle_count-1];
			psystem->particle_count--;
			i--;
			continue;
		}	

		float lifetime = p->death_time - p->birth_time;
		float t = (psystem->age - p->birth_time) / lifetime;

		float dir = lerp(p->start_dir, p->end_dir, t);
		float speed = lerp(p->start_speed, p->end_speed, t);

		Vector2 shift = vec2(1.0f, 0.0f);
		shift = vec2_scale(vec2_rotate(shift, dir), speed * dt);
		p->pos = vec2_add(p->pos, shift); 
	}	

	#ifndef NO_DEVMODE
	p_stats.total_particles += psystem->particle_count;
	#endif

	// Spawn new particles
	float inv_emission_rate = 1.0f / psystem->desc->emission_rate;
	while(psystem->emission_acc - inv_emission_rate > 0.0f) {

		#ifndef NO_DEVMODE
		p_stats.born_count++;
		#endif

		psystem->emission_acc -= inv_emission_rate;

		if(psystem->particle_count == psystem->desc->max_particles) {
			LOG_WARNING("Maximum particle count reached, skipping particle");
			continue;
		}	

		uint i = psystem->particle_count++;
		Particle* particles = psystem->particles;

		particles[i].birth_time = psystem->age;
		particles[i].pos = psystem->pos;

		float lifetime = lerp(psystem->desc->min_lifetime, 
			psystem->desc->max_lifetime, rand_float()); 
		particles[i].death_time = particles[i].birth_time + lifetime;	

		particles[i].start_dir = lerp(psystem->desc->min_start_dir,
			psystem->desc->max_start_dir, rand_float());
		particles[i].start_dir += psystem->direction;	
		particles[i].start_speed = lerp(psystem->desc->min_start_speed,
			psystem->desc->max_start_speed, rand_float());
		particles[i].start_color = color_lerp(psystem->desc->min_start_color,
			psystem->desc->max_start_color, rand_float());
		particles[i].start_size = lerp(psystem->desc->min_start_size,
			psystem->desc->max_start_size, rand_float());
		particles[i].start_angle = lerp(psystem->desc->min_start_angle,
			psystem->desc->max_start_angle, rand_float());

		if(psystem->desc->min_end_dir * RAD_TO_DEG < -100.0f && 
			psystem->desc->max_end_dir * RAD_TO_DEG < -100.0f) {
			particles[i].end_dir = particles[i].start_dir;
		}
		else {
			particles[i].end_dir = lerp(psystem->desc->min_end_dir,
				psystem->desc->max_end_dir, rand_float());
			particles[i].end_dir += psystem->direction;	
		}

		if(psystem->desc->min_end_speed < -100.0f &&
			psystem->desc->max_end_speed < -100.0f) {
			particles[i].end_speed = particles[i].start_speed;
		}
		else {
			particles[i].end_speed = lerp(psystem->desc->min_end_speed,
				psystem->desc->max_end_speed, rand_float());
		}		

		if(psystem->desc->min_end_color == 0 &&
			psystem->desc->max_end_color == 0) {
				particles[i].end_color = particles[i].start_color;
		}
		else {
			particles[i].end_color = color_lerp(psystem->desc->min_end_color,
				psystem->desc->max_end_color, rand_float());
		}		
		
		if(psystem->desc->min_end_size < -100.0f &&
			psystem->desc->max_end_size < -100.0f) {
			particles[i].end_size = particles[i].start_size;
		}
		else {
		 	particles[i].end_size = lerp(psystem->desc->min_end_size,
				psystem->desc->max_end_size, rand_float());
		}		

		if(psystem->desc->min_end_angle * RAD_TO_DEG < -100.0f &&
			psystem->desc->max_end_angle * RAD_TO_DEG < -100.0f) {
			particles[i].end_angle = particles[i].start_angle;
		}
		else {
		 	particles[i].end_angle = lerp(psystem->desc->min_end_angle,
				psystem->desc->max_end_angle, rand_float());
		}	
	}	

	// Self-destruct if age is reached and all particles are dead
	if(psystem->age > psystem->desc->duration && psystem->particle_count == 0) {
		MEM_FREE(psystem->particles);
		psystem->active = false;
	}		
}		

void particles_update(float time) {
 	// Skip first update to get proper delta
	if(last_time == 0.0f) {
		last_time = time;
		return;
	}	

	#ifndef NO_DEVMODE
	p_stats.total_particles = 0;
	p_stats.active_psystems = 0;
	static float count_update_t = 0.0f;
	if(time - count_update_t > 1.0f) {
		p_stats.born_in_last_second = p_stats.born_count;
		p_stats.dead_in_last_second = p_stats.dead_count;
		p_stats.born_count = p_stats.dead_count = 0;
		count_update_t = time;
	}
	#endif

	assert(time > last_time);
	float dt = time - last_time;
	last_time = time;

	uint i;
	for(i = 0; i < MAX_PSYSTEMS; ++i) {
		if(psystems[i].active) {
			_psystem_update(&(psystems[i]), dt);

			#ifndef NO_DEVMODE
			p_stats.active_psystems++;
			#endif
		}	
	}		
}	

void _psystem_draw(ParticleSystem* psystem) {
	// TODO: Do additive blending on particles

	uint i;
	for(i = 0; i < psystem->particle_count; ++i) {
		Particle* p = &(psystem->particles[i]);

		float lifetime = p->death_time - p->birth_time;
		float t = (psystem->age - p->birth_time) / lifetime; 

		float size = lerp(p->start_size, p->end_size, t);
		Color color = color_lerp(p->start_color, p->end_color, t);
		float angle = lerp(p->start_angle, p->end_angle, t);

		RectF source = psystem->desc->tex_source;
		float width = (source.right - source.left) * size;
		float height = (source.bottom - source.top) * size;
		RectF dest = rectf(p->pos.x - width/2.0f, p->pos.y - height/2.0f,
			p->pos.x + width/2.0f, p->pos.y + height/2.0f);

		video_draw_rect_rotated(psystem->desc->texture, particles_layer, 
			&source, &dest, angle, color); 
	}		
}	

void particles_draw(void) {
	uint i;
	for(i = 0; i < MAX_PSYSTEMS; ++i) {
		if(psystems[i].active)
			_psystem_draw(&(psystems[i]));
	}		
}	

