#include "objects.h"

#include <datastruct.h>
#include <darray.h>
#include <mempool.h>
#include <gfx_utils.h>

#define MAX_GAMEOBJECT_SIZE (sizeof(GameObject) * 3)

RectF objects_camera[2] = {
	{0.0f, 0.0f, 1024.0f, 768.0f},
	{0.0f, 0.0f, 1024.0f, 768.0f}
};

static CDWorld cdworld;
CDWorld* objects_cdworld = &cdworld;

// Object type to GameObjectDesc map
static AATree desc_map;
static GameObjectDesc* desc_cache = NULL;

// Game object pool
static MemPool objects;

// Component containers

static DArray physics;
static DArray render;
static DArray update;

// Ptrs to objects to remove and to add
static DArray to_remove;
static DArray to_add;

typedef struct {
	Vector2 pos;
	GameObject* obj;
	GameObjectDesc* desc;
	void* user_data;
} NewObject;

void objects_init(void) {
	aatree_init(&desc_map);
	mempool_init(&objects, MAX_GAMEOBJECT_SIZE);

	physics = darray_create(sizeof(PhysicsComponent), 0);
	render = darray_create(sizeof(RenderComponent), 0);
	update = darray_create(sizeof(UpdateComponent), 0);
	to_remove = darray_create(sizeof(GameObject*), 0);
	to_add = darray_create(sizeof(NewObject), 0);

	coldet_init(objects_cdworld, 256.0f);
}

void objects_close(void) {
	coldet_close(objects_cdworld);

	darray_free(&to_add);
	darray_free(&to_remove);
	darray_free(&physics);
	darray_free(&render);
	darray_free(&update);

	mempool_drain(&objects);
	aatree_free(&desc_map);
}

GameObject* objects_create(GameObjectDesc* desc, Vector2 pos, void* user_data) {
	assert(desc);
	assert(desc->size <= MAX_GAMEOBJECT_SIZE);
	assert(desc->construct);

	GameObject* obj = mempool_alloc(&objects);
	assert(((size_t)obj & 1) == 0);

	obj->type = 0;

	NewObject new = {
		.pos = pos,
		.obj = obj,
		.desc = desc,
		.user_data = user_data
	};

	darray_append(&to_add, &new);

	return obj;
}

void objects_destroy(GameObject* obj) {
	assert(obj);
	assert(mempool_owner(&objects, obj));

	if((obj->type & DESTROY_BIT) == 0) {
		darray_append(&to_remove, &obj);
		obj->type |= DESTROY_BIT;
	}
}

void objects_destroy_all(void) {
	mempool_free_all(&objects);

	// Empty coldet world
	for(uint i = 0; i < physics.size; ++i) {
		PhysicsComponent* p = darray_get(&physics, i);
		coldet_remove_obj(objects_cdworld, p->cd_obj);
	}

	// Reset cameras
	for(uint i = 0; i < ARRAY_SIZE(objects_camera); ++i) {
		objects_camera[i] = rectf(0.0f, 0.0f, 1024.0f, 768.0f);
	}

	physics.size = 0;
	render.size = 0;
	update.size = 0;
	to_remove.size = 0;
	to_add.size = 0;
}

void objects_apply_force(GameObject* obj, Vector2 f) {
	assert(obj->physics);

	obj->physics->acc = vec2_add(obj->physics->acc, f);
}

static void objects_update_tick(uint n_components, float ts) {
	assert(update.size >= n_components);
	UpdateComponent* upd = darray_get(&update, 0);

	// Count frames to correctly update
	// components every n-th frame
	static uint update_frame = 0;

	for(uint i = 0; i < n_components; ++i) {
		if(upd[i].update && update_frame % upd[i].interval == 0) {
			uint delta_tics = update_frame - upd[i].last_update_tick;
			(upd[i].update)(upd[i].owner, ts, delta_tics * PHYSICS_DT);
			upd[i].last_update_tick = update_frame;
		}
	}

	update_frame++;
}

static void objects_collision_callback(CDObj* a, CDObj* b) {
	GameObject* oa = a->userdata;
	GameObject* ob = b->userdata;

	bool call_a = oa->physics && oa->physics->hit_callback;
	bool call_b = ob->physics && ob->physics->hit_callback;

	if(call_a) 
		(oa->physics->hit_callback)(oa, ob);

	if(call_b)
		(ob->physics->hit_callback)(ob, oa);
}

bool draw_physics_debug = false;

static void objects_physics_tick(uint n_components) {
	assert(physics.size >= n_components);
	PhysicsComponent* phys = darray_get(&physics, 0);

	for(uint i = 0; i < n_components; ++i) {
		CDObj* cd = phys[i].cd_obj;

		Vector2 a = vec2_scale(phys[i].acc, phys[i].inv_mass * PHYSICS_DT);
		Vector2 vel = vec2_add(phys[i].vel, a);
		phys[i].acc = vec2(0.0f, 0.0f);
		Vector2 pos = cd->pos;

		pos = vec2_add(pos, vec2_scale(vel, PHYSICS_DT));
		phys[i].vel = vel;

		Vector2 offset = vec2_sub(pos, cd->pos);
		cd->offset = vec2_add(cd->offset, offset);

#ifndef NO_DEVMODE
		if(draw_physics_debug) {
			Vector2 collider_pos = vec2_add(cd->pos, cd->offset);
			collider_pos = vec2_sub(collider_pos, vec2(
				objects_camera[0].left,
				objects_camera[0].top
			));
			RectF collider = {
				collider_pos.x, 
				collider_pos.y,
				collider_pos.x + cd->size.size.x,
				collider_pos.y + cd->size.size.y
			};
			gfx_draw_rect(15, &collider, COLOR_RGBA(128, 128, 128, 255));
		}
#endif
	}

	coldet_process(objects_cdworld, objects_collision_callback);
}

static void objects_render_tick(uint n_components) {
	assert(render.size >= n_components);
	RenderComponent* rndr = darray_get(&render, 0);
	
	// This checks visibility of every object,
	// might be a good idea to optimize later on
	for(uint i = 0; i < n_components; ++i) {
		RenderComponent* r = &rndr[i];

		if(r->update_pos)
			(r->update_pos)(r->owner);
	
		RectF* camera = &objects_camera[r->camera];

		bool is_visible = 
			(r->extent_max >= camera->left) 
			&& (r->extent_min <= camera->right);

		if(is_visible) {
			if(!r->was_visible && r->became_visible)
				(r->became_visible)(r->owner);

			if(r->pre_render)
				(r->pre_render)(r->owner);

			Vector2 camera_topleft = {
				.x = camera->left,
				.y = camera->top
			};

			Vector2 screen_pos = vec2_sub(r->world_pos, camera_topleft);

			// Render
			if(r->anim_frame != MAX_UINT16) {
				// Animation sprite
				spr_draw_anim_cntr_h(
						r->spr, r->anim_frame, r->layer,
						screen_pos, r->angle, r->scale, r->color
				);
			}
			else {
				// Static sprite
				spr_draw_cntr_h(
						r->spr, r->layer, screen_pos,
						r->angle, r->scale, r->color
				);
			}

			if(r->post_render)
				(r->post_render)(r->owner);
		}
		else {
			if(r->was_visible && r->became_invisible)
				(r->became_invisible)(r->owner);
		}

		r->was_visible = is_visible;
	}
}

static void objects_destroy_internal(GameObject* obj) {
	// Get description
	uint32 type = obj->type & TYPE_MASK;
	GameObjectDesc* desc = desc_cache->type == type ? desc_cache : NULL;
	if(!desc) {
		desc = aatree_find(&desc_map, type);
		desc_cache = desc;
	}

	// Call destructor
	if(desc->destruct)
		desc->destruct(obj);

	// Destroy components
	if(desc->has_physics) {
		PhysicsComponent* this = obj->physics;
		coldet_remove_obj(objects_cdworld, this->cd_obj);

		PhysicsComponent* last = darray_get(&physics, physics.size-1);
		last->owner->physics = this;
		*this = *last;
		physics.size--;
	}

	if(desc->has_render) {
		RenderComponent* this = obj->render;
		RenderComponent* last = darray_get(&render, render.size-1);
		last->owner->render = this;
		*this = *last;
		render.size--;
	}

	if(desc->has_update) {
		UpdateComponent* this = obj->update;
		UpdateComponent* last = darray_get(&update, update.size-1);
		last->owner->update = this;
		*this = *last;
		update.size--;
	}

	mempool_free(&objects, obj);
}

static void objects_remove_tick(void) {
	for(uint i = 0; i < to_remove.size; ++i) {
		GameObject** pobj = darray_get(&to_remove, i);
		objects_destroy_internal(*pobj);
	}
	to_remove.size = 0;
}

static void objects_create_internal(NewObject* new) {
	GameObjectDesc* desc = new->desc;
	GameObject* obj = new->obj;
	obj->type = desc->type;

	if(desc->has_physics) {
		PhysicsComponent* old_place = physics.data;
		darray_append_nulls(&physics, 1);
		if(old_place != physics.data) {
			// Triggerred realloc, update owner pointer
			for(uint i = 0; i < physics.size-1; ++i) {
				PhysicsComponent* p = darray_get(&physics, i);
				p->owner->physics = p;
			}
		}
		obj->physics = darray_get(&physics, physics.size-1);
		obj->physics->owner = obj;
	}
	else {
		obj->physics = NULL;
	}

	if(desc->has_render) {
		RenderComponent* old_place = render.data;
		darray_append_nulls(&render, 1);
		if(old_place != render.data) {
			// Triggered realloc, update owner pointer
			for(uint i = 0; i < render.size-1; ++i) {
				RenderComponent* r = darray_get(&render, i);
				r->owner->render = r;
			}
		}
		obj->render = darray_get(&render, render.size-1);
		obj->render->owner = obj;
		obj->render->color = COLOR_WHITE;
		obj->render->became_invisible = objects_destroy;
	}
	else {
		obj->render = NULL;
	}

	if(desc->has_update) {
		UpdateComponent* old_place = update.data;
		darray_append_nulls(&update, 1);
		if(old_place != update.data) {
			// Triggered realloc, update owner pointer
			for(uint i = 0; i < update.size-1; ++i) {
				UpdateComponent* u = darray_get(&update, i);
				u->owner->update = u;
			}
		}
		obj->update = darray_get(&update, update.size-1);
		obj->update->owner = obj;
		obj->update->interval = 1;
	}
	else {
		obj->update = NULL;
	}

	desc->construct(obj, new->pos, new->user_data);

	if(desc->has_physics) {
		obj->physics->cd_obj->userdata = obj;
	}

	if(desc_cache != desc) {
		aatree_insert(&desc_map, desc->type, desc);
		desc_cache = desc;
	}
}

static void objects_add_tick(void) {
	for(uint i = 0; i < to_add.size; ++i) {
		NewObject* obj = darray_get(&to_add, i);
		objects_create_internal(obj);
	}
	to_add.size = 0;
}

void objects_tick(bool paused) {
	uint n_update = update.size;
	uint n_physics = physics.size;
	uint n_render = render.size;

	if(!paused) {
		if(n_update)
			objects_update_tick(n_update, time_s());

		if(n_physics)
			objects_physics_tick(n_physics);
	}

	if(n_render)
		objects_render_tick(n_render);

	objects_remove_tick();
	objects_add_tick();
}

