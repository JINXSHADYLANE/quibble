#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <mfx.h>

static void obj_token_collide(GameObject* self, GameObject* other) {
	ObjToken* token = (ObjToken*)self;
	if(other->type == OBJ_RABBIT_TYPE && token->value != 0) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;
		d->tokens += token->value;
		token->value = 0;

		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		objects_destroy(self);

	}
}

static void obj_token_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjToken* token = (ObjToken*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	token->value = 1;
	
	RectF collider = {
		pos.x - width/2.0f, pos.y - height/2.0f,
		pos.x + width/2.0f, pos.y + height/2.0f
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = obj_token_collide;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x - width/2.0f, pos.y - height/2.0f,
			pos.x + width/2.0f, pos.y + height/2.0f
	);
	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
}

GameObjectDesc obj_token_desc = {
	.type = OBJ_TOKEN_TYPE,
	.size = sizeof(ObjToken),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_token_construct,
	.destruct = NULL
};

