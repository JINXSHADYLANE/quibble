#include "obj_types.h"

#include <system.h>

// Clock

extern float rabbit_remaining_time;
extern bool game_over;

static void obj_clock_hit_callback(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE && !game_over) {
		// Destroy self, make fading clock
		objects_destroy(self);
		objects_create(&obj_clock_fading_desc, self->render->world_pos, NULL);

		rabbit_remaining_time = MIN(60.0f, rabbit_remaining_time + 0.5f);
	}
}

static void obj_clock_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = sprsheet_get_handle("clock");
	Vector2 size = sprsheet_get_size_h(spr_handle);

	RectF collider = {
		pos.x - size.x / 2.0f, pos.y - size.y / 2.0f,
		pos.x + size.x / 2.0f, pos.y + size.y / 2.0f
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.1f;
	physics->hit_callback = obj_clock_hit_callback;

	// Render
	RenderComponent* render = self->render;
	render->world_pos = pos;
	render->extent_min = pos.x - size.x / 2.0f;
	render->extent_max = pos.x + size.x / 2.0f;
	render->scale = 1.0f;
	render->layer = 2;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
}

GameObjectDesc obj_clock_desc = {
	.type = OBJ_CLOCK_TYPE,
	.size = sizeof(ObjClock),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_clock_construct,
	.destruct = NULL
};

// Fading clock

static void obj_clock_fading_update_pos(GameObject* self) {
	ObjClockFading* clock = (ObjClockFading*)self;
	RenderComponent* render = self->render;

	float t = clock->t;
	Vector2 offset = vec2(0.0f, - t*t * 60.0f);
	render->world_pos = vec2_add(clock->original_pos, offset); 
	byte alpha = 255 - lrintf(t * 255.0f);
	render->color = COLOR_RGBA(255, 255, 255, alpha);
}

static void obj_clock_fading_update(GameObject* self, float ts, float dt) {
	ObjClockFading* clock = (ObjClockFading*)self;
	const float fade_length = 0.3f;

	clock->t = (ts - clock->birth_time) / fade_length;
	assert(clock->t >= 0.0f);
	if(clock->t > 1.0f) {
		clock->t = 1.0f;
		objects_destroy(self);
	}
}

static void obj_clock_fading_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = sprsheet_get_handle("clock");
	Vector2 size = sprsheet_get_size_h(spr_handle);

	// Render
	RenderComponent* render = self->render;
	render->world_pos = pos;
	render->extent_min = pos.x - size.x / 2.0f;
	render->extent_max = pos.x + size.x / 2.0f;
	render->scale = 1.0f;
	render->layer = 2;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_clock_fading_update_pos;

	// Update
	UpdateComponent* update = self->update;
	update->update = obj_clock_fading_update;

	ObjClockFading* clock = (ObjClockFading*)self;
	clock->t = 0.0f;
	clock->birth_time = time_s();
	clock->original_pos = render->world_pos;
}

GameObjectDesc obj_clock_fading_desc = {
	.type = OBJ_CLOCK_FADING_TYPE,
	.size = sizeof(ObjClockFading),
	.has_physics = false,
	.has_render = true,
	.has_update = true,
	.construct = obj_clock_fading_construct,
	.destruct = NULL
};

