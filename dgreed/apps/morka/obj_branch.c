#include "obj_types.h"
#include "common.h"
#include <system.h>

static void obj_branch_became_invisible(GameObject* self) {
	// empty
}

static void obj_branch_collide(GameObject* self, GameObject* other) {
	ObjBranch* branch = (ObjBranch*)self;
	if(other->type == OBJ_RABBIT_TYPE) {
		PhysicsComponent* rabbit_physics = other->physics;
		if(rabbit_physics->vel.y > 0.0f) {
			branch->dh -= 10.0f;
		}
	}
}

static void obj_branch_update_pos(GameObject* self) {
	ObjMushroom* branch = (ObjMushroom*)self;
	RenderComponent* r = self->render;

	float f = 30.0f * (branch->oh - branch->h);
	branch->dh += f * (time_delta()/1000.0f);
	branch->h += (branch->dh * 10.0f) * (time_delta()/1000.0f);
	branch->dh *= 0.9f;

	r->world_dest.top = r->world_dest.bottom - branch->h;
}

static void obj_branch_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjBranch* branch = (ObjBranch*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	branch->oh = height;
	branch->h = height;
	branch->dh = 0.0f;

	float adjust = 20.0f;

	// Take 10 pixel slice at the top as a collider geometry
	RectF collider = {
		pos.x  + adjust, 		pos.y - height,
		pos.x  + width - adjust, pos.y - height + 10.0f
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = obj_branch_collide;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x, pos.y - height,
			pos.x + width, pos.y
	);
	render->layer = branch_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_branch_update_pos;
	render->became_invisible = obj_branch_became_invisible;
}

GameObjectDesc obj_branch_desc = {
	.type = OBJ_BRANCH_TYPE,
	.size = sizeof(ObjBranch),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_branch_construct,
	.destruct = NULL
};

