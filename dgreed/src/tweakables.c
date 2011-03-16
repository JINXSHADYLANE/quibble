#include "tweakables.h"

#include "gui.h"
#include "mml.h"
#include "memory.h"
#include "utils.h"

// TODO: Store groups/names in big DArrays ?

bool _parse_vars(const char* filename, DArray* dest) {
	assert(filename);
	
	char* unparsed = txtfile_read(filename);
	MMLObject mml;
	if(!mml_deserialize(&mml, unparsed))
		return false;

	MEM_FREE(unparsed);

	NodeIdx root = mml_root(&mml);	
	if(strcmp(mml_get_name(&mml, root), "tweakables") != 0)
		return false;

	for(NodeIdx group = mml_get_first_child(&mml, root);
		group != 0;
		group = mml_get_next(&mml, group)) {
		
		if(strcmp(mml_get_name(&mml, group), "group") != 0)
			return false;
		
		const char* group_name = mml_getval_str(&mml, group);
		char* group_name_clone = strclone(group_name);

		for(NodeIdx var = mml_get_first_child(&mml, group);
			var != 0;
			var = mml_get_next(&mml, var)) {
			
			const char* var_type = mml_get_name(&mml, var);
			const char* var_name = mml_getval_str(&mml, var);

			NodeIdx value = mml_get_child(&mml, var, "=");
			NodeIdx min = mml_get_child(&mml, var, "min");
			NodeIdx max = mml_get_child(&mml, var, "max");
			NodeIdx ovr = mml_get_child(&mml, var, "overload");

			if(value == 0)
				return false;

			TwVar new_var;
			new_var.group = group_name_clone;
			new_var.name = strclone(var_name);
			new_var.overload = strclone("default");

			if(strcmp(var_type, "float") == 0) {
				if(min == 0 || max == 0)
					return false;

				new_var.type = TWEAK_FLOAT;
				new_var.t._float.value = mml_getval_float(&mml, value);
				new_var.t._float.addr = NULL;
				new_var.t._float.min = mml_getval_float(&mml, min);
				new_var.t._float.max = mml_getval_float(&mml, max);
			}
			else if(strcmp(var_type, "int") == 0) {
				if(min == 0 || max == 0)
					return false;

				new_var.type = TWEAK_INT;
				new_var.t._int.value = mml_getval_int(&mml, value);
				new_var.t._int.addr = NULL;
				new_var.t._int.min = mml_getval_int(&mml, min);
				new_var.t._int.max = mml_getval_int(&mml, max);
			}
			else if(strcmp(var_type, "bool") == 0) {
				new_var.type = TWEAK_BOOL;
				new_var.t._bool.value = mml_getval_bool(&mml, value);
				new_var.t._bool.addr = NULL;
			}
			else
				return false;

			if(ovr)
				new_var.overload = strclone(mml_getval_str(&mml, ovr));
			else
				new_var.overload = strclone("default");

			darray_append(dest, (void*)&new_var);	
		}	
	}

	mml_free(&mml);

	return true;
}

void _save_vars(const char* filename, DArray source) {
	assert(filename);

	MMLObject mml;
	mml_empty(&mml);

	NodeIdx root = mml_root(&mml);
	mml_set_name(&mml, root, "tweakables");

	const char* last_group = "";
	NodeIdx group = 0;

	TwVar* vars = DARRAY_DATA_PTR(source, TwVar);
	for(uint i = 0; i < source.size; ++i) {
		TwVar var = vars[i];

		if(strcmp(last_group, var.group) != 0) {
			group = mml_node(&mml, "group", var.group);
			mml_append(&mml, root, group);
			last_group = var.group;
		}

		assert(group);

		NodeIdx var_node = mml_node(&mml, "?", var.name);
		mml_append(&mml, group, var_node);

		if(var.type == TWEAK_FLOAT) {
			mml_set_name(&mml, var_node, "float");

			NodeIdx value = mml_node(&mml, "=", "?");
			float val = var.t._float.addr ? 
				*var.t._float.addr : var.t._float.value;
			mml_setval_float(&mml, value, val);
			mml_append(&mml, var_node, value);

			NodeIdx min = mml_node(&mml, "min", "?");
			mml_setval_float(&mml, min, var.t._float.min);
			mml_append(&mml, var_node, min);

			NodeIdx max = mml_node(&mml, "max", "?");
			mml_setval_float(&mml, max, var.t._float.max);
			mml_append(&mml, var_node, max);
		}
		else if(var.type == TWEAK_INT) {
			mml_set_name(&mml, var_node, "int");

			NodeIdx value = mml_node(&mml, "=", "?");
			int val = var.t._int.addr ? 
				*var.t._int.addr : var.t._int.value;
			mml_setval_int(&mml, value, val);
			mml_append(&mml, var_node, value);

			NodeIdx min = mml_node(&mml, "min", "?");
			mml_setval_int(&mml, min, var.t._int.min);
			mml_append(&mml, var_node, min);

			NodeIdx max = mml_node(&mml, "max", "?");
			mml_setval_int(&mml, max, var.t._int.max);
			mml_append(&mml, var_node, max);
		}
		else if(var.type == TWEAK_BOOL) {
			mml_set_name(&mml, var_node, "bool");

			NodeIdx value = mml_node(&mml, "=", "?");
			float val = var.t._float.addr ? 
				*var.t._float.addr : var.t._float.value;
			mml_setval_float(&mml, value, val);
			mml_append(&mml, var_node, value);
		}
		else
			LOG_ERROR("Unexpected TwVar type");

		if(strcmp(var.overload, "default") != 0) {
			NodeIdx ovr = mml_node(&mml, "overload", var.overload);
			mml_append(&mml, var_node, ovr);
		}
	}

	char* out = mml_serialize(&mml);
	mml_free(&mml);

	txtfile_write(filename, out);
	MEM_FREE(out);
}

void _var_save(TwVar* var) {
	switch(var->type) {
		case TWEAK_FLOAT:
			if(var->t._float.addr)
				var->t._float.value = *var->t._float.addr;
			break;
		case TWEAK_INT:
			if(var->t._int.addr)
				var->t._int.value = *var->t._int.addr;
			break;
		case TWEAK_BOOL:
			if(var->t._bool.addr)
				var->t._bool.value = *var->t._bool.addr;
			break;
	}
}

void _var_restore(TwVar* var) {
	switch(var->type) {
		case TWEAK_FLOAT:
			if(var->t._float.addr)
				*var->t._float.addr = var->t._float.value;
			break;
		case TWEAK_INT:
			if(var->t._int.addr)
				*var->t._int.addr = var->t._int.value;
			break;
		case TWEAK_BOOL:
			if(var->t._bool.addr)
				*var->t._bool.addr = var->t._bool.value;
			break;
	}
}

Tweaks* tweaks_init(const char* filename, RectF dest, uint layer, 
	FontHandle font) {
	assert(filename);

	Tweaks* result = (Tweaks*)MEM_ALLOC(sizeof(Tweaks)); 

	result->filename = filename;
	result->dest = dest;
	result->layer = layer;
	result->font = font;
	result->color = COLOR_WHITE;
	result->overload_color = COLOR_RGBA(156, 176, 156, 255);
	result->group = "ungrouped";
	result->overload = "default";
	result->vars = darray_create(sizeof(TwVar), 0);
	result->y_spacing = 22.0f;
	result->items_per_page = 
		(uint)floorf(rectf_height(&dest) / result->y_spacing) - 1;

	if(file_exists(filename) && !_parse_vars(filename, &result->vars))
		LOG_ERROR("Unable to parse tweakable vars from %s", filename);

	result->widest_name = 0.0f;
	TwVar* vars = DARRAY_DATA_PTR(result->vars, TwVar);
	for(uint i = 0; i < result->vars.size; ++i) {
		float width = font_width(font, vars[i].name);
		result->widest_name = MAX(result->widest_name, width);
	}	

	result->last_drawn_page = MAX_UINT32;
	
	return result;
};

void tweaks_close(Tweaks* tweaks) {
	assert(tweaks);

	_save_vars(tweaks->filename, tweaks->vars);

	TwVar* vars = DARRAY_DATA_PTR(tweaks->vars, TwVar);

	for(uint i = 0; i < tweaks->vars.size; ++i) {
		// Free group name if this is last var in group
		if(i == tweaks->vars.size-1 
			|| strcmp(vars[i].group, vars[i+1].group) != 0) {

			MEM_FREE(vars[i].group);
		}

		MEM_FREE(vars[i].name);
		MEM_FREE(vars[i].overload);
	}

	darray_free(&tweaks->vars);

	MEM_FREE(tweaks);
}

void tweaks_group(Tweaks* tweaks, const char* name) {
	assert(tweaks);
	assert(name);

	tweaks->group = name;
}

void tweaks_overload(Tweaks* tweaks, const char* overload) {
	assert(tweaks);

	if(!overload)
		overload = "default";
	
	if(strcmp(tweaks->overload, overload) != 0) {
		TwVar* vars = DARRAY_DATA_PTR(tweaks->vars, TwVar);
		for(uint i = 0; i < tweaks->vars.size; ++i) {
			if(strcmp(vars[i].overload, tweaks->overload) == 0) {
				TwVar* var = &vars[i];
				_var_save(var);
			}
		}	

		for(uint i = 0; i < tweaks->vars.size; ++i) {
			if(strcmp(vars[i].overload, overload) == 0) {
				TwVar* var = &vars[i];
				_var_restore(var);
			}
			
		}

		tweaks->overload = overload;
	}
}

TwVar* _get_var(DArray* vars, const char* group, const char* name, 
	const char* overload) {
	assert(group);
	assert(name);
	assert(overload);

	TwVar* v = DARRAY_DATA_PTR(*vars, TwVar);
	size_t s_idx, idx = 0;

	// Skip straight to var's group
	while(idx < vars->size && strcmp(v[idx].group, group) != 0) {
		idx++;
	}
	if(idx == vars->size) {
		// Even the group does not exist, add new var at the end
		TwVar new_var;
		memset(&new_var, 0, sizeof(new_var));
		new_var.group = strclone(group);
		new_var.name = strclone(name);
		new_var.overload = strclone(overload);

		darray_append(vars, (void*)&new_var);
		v = DARRAY_DATA_PTR(*vars, TwVar);
		return &v[vars->size-1];
	}

	const char* expected_overload = overload;
	
again:
	// Try all vars in group
	s_idx = idx;
	while(idx < vars->size 
		&& strcmp(v[idx].group, group) == 0 
		&& (strcmp(v[idx].name, name) != 0 
			|| strcmp(v[idx].overload, expected_overload) != 0)) {

		idx++;
	}
	if(idx == vars->size 
		|| strcmp(v[idx].group, group) != 0 
		|| strcmp(v[idx].overload, overload) != 0) {

		// Try to return default var
		if(expected_overload == overload) {
			expected_overload = "default";
			if(expected_overload != overload) {
				idx = s_idx;
				goto again;
			}
		}

		// Add new var at the end of group
		TwVar new_var;
		memset(&new_var, 0, sizeof(new_var));
		new_var.group = v[idx-1].group;
		new_var.name = strclone(name);
		new_var.overload = strclone(overload);

		darray_insert(vars, idx, (void*)&new_var);
		v = DARRAY_DATA_PTR(*vars, TwVar);
	}

	return &v[idx];
}

void tweaks_float(Tweaks* tweaks, const char* name, float* addr,
	float min, float max) {
	assert(tweaks);
	assert(name);
	assert(addr);
	assert(min < max);
	assert(min <= *addr && *addr <= max);

	TwVar* var = _get_var(&tweaks->vars, tweaks->group, name, tweaks->overload);
	if(var->t._float.min == 0.0f 
		&& var->t._float.max == 0.0f
		&& var->t._float.value == 0.0f) {
		
		// Var was newly inserted, initialize
		var->type = TWEAK_FLOAT;
		var->t._float.addr = addr;
		var->t._float.min = min;
		var->t._float.max = max;
		
		float width = font_width(tweaks->font, name); 
		tweaks->widest_name = MAX(tweaks->widest_name, width);
	}
	else {
		// Var was already there, update parent value and var addr
		*addr = var->t._float.addr ? *var->t._float.addr : var->t._float.value;
		var->t._float.addr = addr;
	}
}

void tweaks_int(Tweaks* tweaks, const char* name, int* addr,
	int min, int max) {
	assert(tweaks);
	assert(name);
	assert(addr);
	assert(min < max);
	assert(min <= *addr && *addr <= max);

	TwVar* var = _get_var(&tweaks->vars, tweaks->group, name, tweaks->overload);
	if(var->t._int.min == 0.0f 
		&& var->t._int.max == 0.0f
		&& var->t._int.value == 0.0f) {
		
		// Var was newly inserted, initialize
		var->type = TWEAK_INT;
		var->t._int.addr = addr;
		var->t._int.min = min;
		var->t._int.max = max;
	
		float width = font_width(tweaks->font, name); 
		tweaks->widest_name = MAX(tweaks->widest_name, width);
	}
	else {
		// Var was already there, update parent value and var addr
		*addr = var->t._int.addr ? *var->t._int.addr : var->t._int.value;
		var->t._int.addr = addr;
	}
}

void tweaks_bool(Tweaks* tweaks, const char* name, bool* addr) {
	assert(tweaks);
	assert(name);
	assert(addr);

	TwVar* var = _get_var(&tweaks->vars, tweaks->group, name, tweaks->overload);
	if(var->t._float.min == 0.0f		// We can still check _float union field 
		&& var->t._float.max == 0.0f
		&& var->t._float.value == 0.0f) {
		
		// Var was newly inserted, initialize
		var->type = TWEAK_BOOL;
		var->t._bool.addr = addr;
	
		float width = font_width(tweaks->font, name); 
		tweaks->widest_name = MAX(tweaks->widest_name, width);
	}
	else {
		// Var was already there, update parent value and var addr
		*addr = var->t._bool.addr ? *var->t._bool.addr : var->t._bool.value;
		var->t._bool.addr = addr;
	}
}

uint _count_pages(Tweaks* tweaks) {
	uint pages = 0;
	TwVar* vars = DARRAY_DATA_PTR(tweaks->vars, TwVar);

	const char* last_group = "";
	uint items_in_group = 0;
	for(uint i = 0; i < tweaks->vars.size; ++i) {
		if(strcmp(last_group, vars[i].group) != 0) {
			pages++;
			items_in_group = 0;
			last_group = vars[i].group;
		}

		// Treat overloads as single var
		if(i && strcmp(vars[i-1].name, vars[i].name) == 0)
			continue;

		if(i < tweaks->vars.size-1 &&
			++items_in_group == tweaks->items_per_page &&
			strcmp(last_group, vars[i+1].group) == 0) {
			pages++;
			items_in_group = 0;
		}
	}
	return items_in_group > 0 ? pages : pages - 1;
}

const char* _page_name(Tweaks* tweaks, uint page, uint* first_var) {
	uint curr_page = 0;
	TwVar* vars = DARRAY_DATA_PTR(tweaks->vars, TwVar);

	static char result[128];
	const char* last_group = "";
	uint items_in_group = 0;
	uint same_group_pages = 0;
	for(uint i = 0; i < tweaks->vars.size; ++i) {
		if(strcmp(last_group, vars[i].group) != 0) {
			curr_page++;
			items_in_group = 0;
			same_group_pages = 1;
			last_group = vars[i].group;
			if(first_var)
				*first_var = i;
		}

		// Treat overloads as single var
		if(i && strcmp(vars[i-1].name, vars[i].name) == 0)
			continue;

		if(i < tweaks->vars.size-1 &&
			++items_in_group == tweaks->items_per_page &&
			strcmp(last_group, vars[i+1].group) == 0) {
			curr_page++;
			items_in_group = 0;
			same_group_pages++;
			if(first_var)
				*first_var = i+1;
		}

		if(page == curr_page-1) {
			sprintf(result, "page: %s %u", last_group, same_group_pages);	
			return result;
		}
	}
	return NULL;
}

float _pack_var(TwVar* var) {
	float result = 0.0f;
	float fval;
	int ival;
	bool bval;
	switch(var->type) {
		case TWEAK_FLOAT:
			fval = var->t._float.addr ? *var->t._float.addr : var->t._float.value;
			result = normalize(fval, var->t._float.min, var->t._float.max);
			break;
		case TWEAK_INT:
			ival = var->t._int.addr ?  *var->t._int.addr : var->t._int.value;
			result = normalize((float)ival, 
				(float)var->t._float.min, (float)var->t._float.max);	
			break;	
		case TWEAK_BOOL:
			bval = var->t._bool.addr ?  *var->t._bool.addr : var->t._bool.value;
			result = bval ? 1.0f : 0.0f;
			break;
		default:
			LOG_ERROR("Unexpected TwVar type");
	}
	return result;
}	

void _unpack_var(TwVar* var, float packed) {
	float* fval;
	int* ival;
	bool* bval;
	switch(var->type) {
		case TWEAK_FLOAT:
			fval = var->t._float.addr ? var->t._float.addr : &var->t._float.value;
			*fval = lerp(var->t._float.min, var->t._float.max, packed);
			break;
		case TWEAK_INT:
			ival = var->t._int.addr ? var->t._int.addr : &var->t._int.value;
			*ival = (int)lerp((float)var->t._float.min, 
				(float)var->t._float.max, packed);	
			break;	
		case TWEAK_BOOL:
			bval = var->t._bool.addr ?  var->t._bool.addr : &var->t._bool.value;
			*bval = packed > 0.5f ? true : false;
			break;
		default:
			LOG_ERROR("Unexpected TwVar type");
	}
}		

char* _var_strval(TwVar* var) {
	static char result[128];
	float fval;
	int ival;
	bool bval;
	switch(var->type) {
		case TWEAK_FLOAT:
			fval = var->t._float.addr ? *var->t._float.addr : var->t._float.value;
			sprintf(result, "%f", fval);
			break;
		case TWEAK_INT:
			ival = var->t._int.addr ?  *var->t._int.addr : var->t._int.value;
			sprintf(result, "%i", ival);
			break;	
		case TWEAK_BOOL:
			bval = var->t._bool.addr ?  *var->t._bool.addr : var->t._bool.value;
			sprintf(result, "%s", bval ? "true" : "false");
			break;
		default:
			LOG_ERROR("Unexpected TwVar type");
	}
	return result;
}	
void tweaks_render(Tweaks* tweaks) {
	assert(tweaks);

	bool can_overload = strcmp(tweaks->overload, "default") != 0;

	Vector2 cursor = vec2(tweaks->dest.left, tweaks->dest.top);

	// Overload text
	Vector2 ovr_pos = vec2(tweaks->dest.left, tweaks->dest.top - 35.0f);
	char ovr_text[128];
	sprintf(ovr_text, "overload: %s", tweaks->overload);
	font_draw(tweaks->font, ovr_text, tweaks->layer, &ovr_pos, 
		tweaks->overload_color);
	
	// TODO: Cache this
	uint page_count = _count_pages(tweaks);

	// Draw page selection slider
	cursor.x += tweaks->widest_name + 8.0f;
	static bool first_time = true;
	if(first_time) {
		gui_setstate_slider(&cursor, 0.0f);
		first_time = false;
	}
	uint page = (uint)(gui_slider(&cursor) * (float)page_count);
	page = MIN(page, page_count-1);

	if(page != tweaks->last_drawn_page) {
		// Fill in new page
		tweaks->last_page_name = _page_name(tweaks, page, 
			&tweaks->last_page_first_var);

		tweaks->last_drawn_page = page;	
	}

	// Get mouse pos
	uint x, y;
	mouse_pos(&x, &y);
	Vector2 mpos = vec2((float)x, (float)y);

	// Draw page name
	cursor.x = tweaks->dest.left;
	font_draw(tweaks->font, tweaks->last_page_name, tweaks->layer, 
		&cursor, tweaks->color);
	cursor.y += tweaks->y_spacing;	

	// Draw vars
	uint def_idx = ~0, curr_idx = ~0;
	TwVar* vars = DARRAY_DATA_PTR(tweaks->vars, TwVar);
	for(uint i = 0, j = 0; 
		i < tweaks->items_per_page; 
		j++) {
		uint idx = tweaks->last_page_first_var + j;

		if(idx >= tweaks->vars.size)
			return;
		if(i > 0 && strcmp(vars[idx-1].group, vars[idx].group) != 0)
			return;

		if(strcmp(vars[idx].overload, "default") == 0) {
			def_idx = idx;
			if(curr_idx == ~0) 
				curr_idx = idx;
		}

		if(strcmp(vars[idx].overload, tweaks->overload) == 0) {
			curr_idx = idx;
		}

		if(j < tweaks->vars.size-1 
			&& strcmp(vars[idx+1].group, vars[idx].group) == 0
			&& strcmp(vars[idx+1].name, vars[idx].name) == 0)
			continue;

		assert(curr_idx != ~0 && def_idx != ~0);
		TwVar* var = &vars[curr_idx];

		bool is_overload = can_overload 
			&& strcmp(var->overload, tweaks->overload) == 0;

		// Name
		font_draw(tweaks->font, var->name, tweaks->layer, 
			&cursor, is_overload ? tweaks->overload_color : tweaks->color); 

		if(can_overload && mouse_down(MBTN_LEFT)) {
			float w = font_width(tweaks->font, var->name);
			float h = font_height(tweaks->font);
			RectF rect = rectf(cursor.x, cursor.y, cursor.x + w, cursor.y + h);	

			if(rectf_contains_point(&rect, &mpos)) {
				// Save last value
				_var_save(var);
			
				if(is_overload) {
					// Back to default
					assert(def_idx != curr_idx);
					if(def_idx > curr_idx)
						def_idx--;
					MEM_FREE(var->name);
					MEM_FREE(var->overload);
					darray_remove(&tweaks->vars, curr_idx);
					var = &vars[def_idx];
					if(idx > curr_idx)
						j--;
				}
				else {
					// Create overload
					assert(def_idx == curr_idx);
					TwVar new_var = *var;
					new_var.name = strclone(var->name);
					new_var.overload = strclone(tweaks->overload);
					darray_insert(&tweaks->vars, curr_idx, &new_var);
					vars = DARRAY_DATA_PTR(tweaks->vars, TwVar);
					if(idx == curr_idx)
						j++;
				}

				// Restore new value
				_var_restore(var);
			}
		}
		curr_idx = def_idx = ~0;
		
		cursor.x += tweaks->widest_name + 8.0f;
		// Set value for slider, incase something changed var 
		float slider_val = _pack_var(var);
		gui_setstate_slider(&cursor, slider_val);	

		// Draw slider and change var value
		slider_val = gui_slider(&cursor);
		_unpack_var(var, slider_val);

		// Draw value
		char* strval = _var_strval(var);
		float width = font_width(tweaks->font, strval);
		cursor.x = tweaks->dest.right - width;
		font_draw(tweaks->font, strval, tweaks->layer,
			&cursor, tweaks->color);

		cursor.x = tweaks->dest.left;
		cursor.y += tweaks->y_spacing;
		i++;
	}
}

