#include "obj_types.h"
#include "common.h"

#include <system.h>

static void obj_particle_anchor_update(GameObject* self, float ts, float dt){
	ObjParticleAnchor* anchor = (ObjParticleAnchor*)self;
	
	RectF pos = {
		.left = anchor->pos.x,
		.top = anchor->pos.y
	};
	uint camera = self->type == OBJ_PARTICLE_ANCHOR_TYPE ? 0 : 2;
	RectF result = objects_world2screen(pos,camera);
	
	anchor->screen_pos = vec2(result.left,result.top);
	
	if(result.left < -WIDTH){
		objects_destroy(self);	
	}
}

static void obj_particle_anchor_construct(GameObject* self, Vector2 pos, void* user_data) {
	ObjParticleAnchor* anchor = (ObjParticleAnchor*)self;

	anchor->pos = pos;
	anchor->screen_pos = pos;

	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_particle_anchor_update;	
}

GameObjectDesc obj_particle_anchor_desc = {
	.type = OBJ_PARTICLE_ANCHOR_TYPE,
	.size = sizeof(ObjParticleAnchor),
	.has_physics = false,
	.has_render = false,
	.has_update = true,
	.construct = obj_particle_anchor_construct,
	.destruct = NULL
};

GameObjectDesc obj_bg_particle_anchor_desc = {
	.type = OBJ_BG_PARTICLE_ANCHOR_TYPE,
	.size = sizeof(ObjParticleAnchor),
	.has_physics = false,
	.has_render = false,
	.has_update = true,
	.construct = obj_particle_anchor_construct,
	.destruct = NULL
};

