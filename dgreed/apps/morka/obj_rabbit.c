#include "obj_types.h"

#include <system.h>
#include <async.h>

static void obj_rabbit_update(GameObject* self, float ts, float dt) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	PhysicsComponent* p = self->physics;

	Vector2 dir = {.x = 0.0f, .y = 0.0f};

	// Constantly move right
	dir.x += 500.0f;

	// Jump
	if(rabbit->touching_ground && key_down(KEY_A)) {
		rabbit->touching_ground = false;
		rabbit->can_double_jump = true;
		rabbit->jump_time = ts;
		objects_apply_force(self, vec2(0.0f, -100000.0f));
		anim_play(rabbit->anim, "jump");
	}
	else {
		if (key_pressed(KEY_A) && !rabbit->touching_ground) {
			if((ts - rabbit->jump_time) < 0.2f) {
				objects_apply_force(self, vec2(0.0f, -8000.0f));
			}
		}
		if(rabbit->can_double_jump && !rabbit->touching_ground && key_down(KEY_A)) {
			// Double jump
			rabbit->can_double_jump = false;
			objects_apply_force(self, vec2(0.0f, -100000.0f));
			anim_play(rabbit->anim, "double_jump");
		}
	}

	// Jump off mushroom
	float hit_dt = ts - rabbit->mushroom_hit_time;
	if(key_pressed(KEY_A) && hit_dt < 0.3f) {
		float t = (0.3f - hit_dt) / 0.3f;
		objects_apply_force(self, vec2(0.0f, -5000.0f * t));
	}

	// Damping
	if(p->vel.x > 300.0f)
		p->vel.x *= 0.99f;
	else if(p->vel.x > 800.0f)
		p->vel.x *= 0.97f;
	else if(p->vel.x > 1500.0f)
		p->vel.x *= 0.90f;

	objects_apply_force(self, dir);

	// Gravity
	if(!rabbit->touching_ground) {
		objects_apply_force(self, vec2(0.0f, 5000.0f));
	}
}

static void obj_rabbit_update_pos(GameObject* self) {
	// Update render data
	RenderComponent* r = self->render;
	PhysicsComponent* p = self->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	r->world_pos = vec2_add(pos, vec2(45.0f, 40.0f));
	r->extent_min = r->world_pos.x - 90.0f;
	r->extent_max = r->world_pos.x + 90.0f;

	ObjRabbit* rabbit = (ObjRabbit*)self;
	r->anim_frame = anim_frame(rabbit->anim);
}

static void _rabbit_delayed_bounce(void* r) {
	ObjRabbit* rabbit = r;
	GameObject* self = r;
	objects_apply_force(self, rabbit->bounce_force); 
	rabbit->bounce_force = vec2(0.0f, 0.0f);
}

static void obj_rabbit_collide(GameObject* self, GameObject* other) {
	ObjRabbit* rabbit = (ObjRabbit*)self;

	// Collision with ground
	if(other->type == OBJ_GROUND_TYPE) {
		CDObj* cd_rabbit = self->physics->cd_obj;
		CDObj* cd_ground = other->physics->cd_obj;
		float rabbit_bottom = cd_rabbit->pos.y + cd_rabbit->size.size.y;
		float ground_top = cd_ground->pos.y;
		float penetration = (rabbit_bottom + cd_rabbit->offset.y) - ground_top;
		if(penetration > 0.0f) {
			self->physics->vel.y = 0.0f;
			if(!rabbit->touching_ground)
				anim_play(rabbit->anim, "land");
			rabbit->touching_ground = true;
			cd_rabbit->offset = vec2_add(
					cd_rabbit->offset, 
					vec2(0.0f, -penetration)
			);
		}
	}

	// Collision with mushroom
	Vector2 vel = self->physics->vel;
	if(other->type == OBJ_MUSHROOM_TYPE && !rabbit->touching_ground && vel.y > 500.0f) {
		if(rabbit->bounce_force.y == 0.0f) {
			rabbit->mushroom_hit_time = time_s();
			anim_play(rabbit->anim, "bounce");

			vel.y = -vel.y;
			Vector2 f = {
				.x = MIN(vel.x*200.0f, 280000.0f),
				.y = MAX(vel.y*400.0f,-350000.0f)
			};
			rabbit->bounce_force = f;

			// Slow down vertical movevment
			self->physics->vel.y *= 0.2f;

			// Delay actual bouncing 0.1s
			async_schedule(_rabbit_delayed_bounce, 100, self);
		}
	}
}

static void obj_rabbit_construct(GameObject* self, Vector2 pos, void* user_data) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	rabbit->touching_ground = false;

	// Init physics
	PhysicsComponent* physics = self->physics;
	RectF rect = {
		pos.x - 45.0f, pos.y - 40.0f,
		pos.x + 45.0f, pos.y + 40.0f
	};
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &rect, 1, NULL);
	float mass = 3.0f;
	physics->inv_mass = 1.0f / mass;
	physics->vel = vec2(0.0f, 0.0f);
	physics->hit_callback = obj_rabbit_collide;

	// Init render
	RenderComponent* render = self->render;
	render->world_pos = pos;
	render->extent_min = rect.left;
	render->extent_max = rect.right;
	render->scale = 1.0f;
	render->angle = 0.0f;
	render->layer = 3;
	render->anim_frame = 0;
	render->spr = sprsheet_get_handle("rabbit");
	render->update_pos = obj_rabbit_update_pos;

	// Init update
	UpdateComponent* update = self->update;
	update->interval = 1;
	update->update = obj_rabbit_update;

	// Make animation
	rabbit->anim = anim_new("rabbit");
}

static void obj_rabbit_destruct(GameObject* self) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	anim_del(rabbit->anim);
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

