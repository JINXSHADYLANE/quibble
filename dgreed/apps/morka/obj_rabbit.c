#include "obj_rabbit.h"

#include <system.h>

static void obj_rabbit_update(GameObject* self, float ts, float dt) {
	PhysicsComponent* p = self->physics;

	Vector2 dir = {.x = 0.0f, .y = 0.0f};

	if(key_pressed(KEY_UP))
		dir.y -= 4000.0f;

	if(key_pressed(KEY_DOWN))
		dir.y += 4000.0f;

	if(key_pressed(KEY_LEFT))
		dir.x -= 4000.0f;

	if(key_pressed(KEY_RIGHT))
		dir.x += 4000.0f;

	if(key_pressed(KEY_A))
		objects_destroy(self);

	objects_apply_force(self, dir);

	// Damp
	p->vel = vec2_scale(p->vel, 0.8f);
}

static void obj_rabbit_update_pos(GameObject* self) {
	// Update render data
	RenderComponent* r = self->render;
	PhysicsComponent* p = self->physics;
	r->world_pos = p->cd_obj->pos;
	r->extent_min = r->world_pos.x - 10.0f;
	r->extent_max = r->world_pos.x + 10.0f;
}

static void obj_rabbit_became_visible(GameObject* self) {
	printf("Rabbit became visible\n");
}

static void obj_rabbit_became_invisible(GameObject* self) {
	printf("Rabbit became invisible\n");
}

static void obj_rabbit_construct(GameObject* self, Vector2 pos, void* user_data) {
	// Init physics
	PhysicsComponent* physics = self->physics;
	RectF rect = {
		pos.x - 16.0f, pos.y - 16.0f,
		pos.x + 16.0f, pos.y + 16.0f
	};
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &rect, 0xFFFFFFFF, physics);
	float mass = 3.0f;
	physics->inv_mass = 1.0f / mass;
	physics->vel = vec2(0.0f, 0.0f);

	// Init render
	RenderComponent* render = self->render;
	render->world_pos = pos;
	render->extent_min = rect.left;
	render->extent_max = rect.right;
	render->scale = 10.0f;
	render->angle = 0.0f;
	render->layer = 1;
	render->anim_frame = MAX_UINT16;
	render->spr = sprsheet_get_handle("rabbit");
	render->update_pos = obj_rabbit_update_pos;
	render->became_visible = obj_rabbit_became_visible;
	render->became_invisible = obj_rabbit_became_invisible;

	// Init update
	UpdateComponent* update = self->update;
	update->interval = 1;
	update->update = obj_rabbit_update;

	printf("Rabbit created\n");
}

static void obj_rabbit_destruct(GameObject* self) {
	printf("Rabbit destroyed\n");
}

GameObjectDesc obj_rabbit_desc = {
	.type = OBJ_RABBIT_TYPE,
	.size = sizeof(ObjRabbit),
	.has_physics = true,
	.has_render = true,
	.has_update = true,
	.construct = obj_rabbit_construct,
	.destruct = obj_rabbit_destruct
};

