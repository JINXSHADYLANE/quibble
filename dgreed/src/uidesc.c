#include "uidesc.h"

#include "darray.h"
#include "memory.h"
#include "mml.h"

static DArray ui_elements;
static DArray name_strings;
static Dict ui_dict;

static bool _is_vec2_fun(MMLObject* mml, NodeIdx node) {
	static const char* vec2_fun_names[] = {
		"vec2", "get_vec2", "add", "sub", "avg", "middle", 
		"x", "y", "tl", "tr", "bl", "br", "spr_size"
	};

	const char* name = mml_get_name(mml, node);

	for(uint i = 0; i < ARRAY_SIZE(vec2_fun_names); ++i) {
		if(strcmp(name, vec2_fun_names[i]) == 0)
			return true;
	}
	return false;
}

static RectF _rect_fun(MMLObject* mml, NodeIdx node, UIElement* context, UIElement* parent);

static Vector2 _vec2_fun(MMLObject* mml, NodeIdx node, UIElement* context, UIElement* parent) {
	assert(_is_vec2_fun(mml, node));

	const char* name = mml_get_name(mml, node);

	if(strcmp(name, "vec2") == 0) {
		return mml_getval_vec2(mml, node);
	}
	else if(strcmp(name, "get_vec2") == 0) {
		// TODO: check local defs too
		const char* element_name = mml_getval_str(mml, node);

		const UIElement* el = uidesc_get_child(context, element_name);

		if(!el && parent)
			el = uidesc_get_child(parent, element_name);

		if(!el)
			el = dict_get(&ui_dict, element_name);

		if(el && (el->members & UI_EL_VEC2)) {
			return el->vec2;
		}
		else {
			LOG_WARNING("Unable to get_vec2 %s", element_name);
			return vec2(0.0f, 0.0f);
		}
	}
	else {
		bool add = strcmp(name, "add") == 0;
		bool avg = !add && (strcmp(name, "avg") == 0);
		bool sub = !add && !avg && (strcmp(name, "sub") == 0);
		if(add || avg || sub) {
			// Iterate over children, add them
			uint n = 0;
			Vector2 sum = vec2(0.0f, 0.0f);
			NodeIdx child = mml_get_first_child(mml, node);
			for(; child != 0; child = mml_get_next(mml, child)) {
				if(n > 0 && sub)
					sum = vec2_sub(sum, _vec2_fun(mml, child, context, parent));
				else
					sum = vec2_add(sum, _vec2_fun(mml, child, context, parent));
				n++;
			}

			if(avg) {
				sum = vec2_scale(sum, 1.0f / (float)n);
			}

			if(n) {
				return sum;
			}
			else {
				LOG_WARNING("Unable to calculate sum/avg");
				return vec2(0.0f, 0.0f);
			}
		}
		else if(strcmp(name, "middle") == 0) {
			NodeIdx child = mml_get_first_child(mml, node);
			RectF rect = _rect_fun(mml, child, context, parent);
			return rectf_center(&rect);
		}
		else if(strcmp(name, "spr_size") == 0) {
			const char* spr_name = mml_getval_str(mml, node);
			return sprsheet_get_size(spr_name);
		}
		else if(strcmp(name, "x") == 0) {
			NodeIdx child = mml_get_first_child(mml, node);
			Vector2 vec2 = _vec2_fun(mml, child, context, parent);
			vec2.y = 0.0f;
			return vec2;
		}
		else if(strcmp(name, "y") == 0) {
			NodeIdx child = mml_get_first_child(mml, node);
			Vector2 vec2 = _vec2_fun(mml, child, context, parent);
			vec2.x = 0.0f;
			return vec2;
		}
		else {
			bool tl = strcmp(name, "tl") == 0;
			bool tr = !tl && (strcmp(name, "tr") == 0);
			bool bl = !tl && !tr && (strcmp(name, "bl") == 0);
			bool br = !tl && !tr && !bl && (strcmp(name, "br") == 0);

			if(tl || tr || bl || br) {
				NodeIdx child = mml_get_first_child(mml, node);
				RectF rect = _rect_fun(mml, child, context, parent);
				if(tl)
					return vec2(rect.left, rect.top);
				if(tr)
					return vec2(rect.right, rect.top);
				if(bl)
					return vec2(rect.left, rect.bottom);
				if(br)
					return vec2(rect.right, rect.bottom);
			}

			LOG_WARNING("Unable to eval vec2 function %s", name);
			return vec2(0.0f, 0.0f);
		}
	}
}

static bool _is_rect_fun(MMLObject* mml, NodeIdx node) {
	static const char* rectf_fun_names[] = {
		"rect", "get_rect", "radd"
	};

	const char* name = mml_get_name(mml, node);

	for(uint i = 0; i < ARRAY_SIZE(rectf_fun_names); ++i) {
		if(strcmp(name, rectf_fun_names[i]) == 0)
			return true;
	}
	return false;
}

static RectF _rect_fun(MMLObject* mml, NodeIdx node, UIElement* context, UIElement* parent) {
	assert(_is_rect_fun(mml, node));

	const char* name = mml_get_name(mml, node);
	
	if(strcmp(name, "rect") == 0) {
		return mml_getval_rectf(mml, node);
	}
	else if(strcmp(name, "get_rect") == 0) {
		// TODO: Check local defs too
		const char* element_name = mml_getval_str(mml, node);

		const UIElement* el = uidesc_get_child(context, element_name);

		if(!el && parent)
			el = uidesc_get_child(parent, element_name);

		if(!el)
			el = dict_get(&ui_dict, element_name);

		if(el && (el->members & UI_EL_RECT)) {
			return el->rect;
		}
		else {
			LOG_WARNING("Unable to get_rect %s", element_name);
			return rectf_null();
		}
	}
	else if(strcmp(name, "radd") == 0) {
		uint n = 0;
		Vector2 sum = vec2(0.0f, 0.0f);
		NodeIdx child = mml_get_first_child(mml, node);
		RectF rect = _rect_fun(mml, child, context, parent);
		for(child = mml_get_next(mml, child); child != 0; child = mml_get_next(mml, child)) {
			sum = vec2_add(sum, _vec2_fun(mml, child, context, parent));
			n++;
		}
		rect.left += sum.x; rect.right += sum.x;
		rect.top += sum.y; rect.bottom += sum.y;

		return rect;
	}

	LOG_WARNING("Unable to eval rect function %s", name);
	return rectf_null();
}

static bool _is_spr_fun(MMLObject* mml, NodeIdx node) {
	const char* name = mml_get_name(mml, node);
	return strcmp(name, "spr") == 0;
}

static SprHandle _spr_fun(MMLObject* mml, NodeIdx node) {
#ifdef _DEBUG
	const char* name = mml_get_name(mml, node);
	assert(strcmp(name, "spr") == 0);
#endif

	const char* spr_name = mml_getval_str(mml, node);
	return sprsheet_get_handle(spr_name);
}

static void _rebase_ui_elements(void* min, void* max, ptrdiff_t delta) {
	DictEntry* m = ui_dict.map;
	for(uint i = 0; i < ui_dict.mask+1; ++i) {
		if(m[i].key && (m[i].data >= min) && (m[i].data < max)) {
			m[i].data += delta;
		}
	}

	for(uint i = 0; i < ui_elements.size; ++i) {
		UIElement* el = darray_get(&ui_elements, i);
		if((void*)el->list.next >= min && (void*)el->list.next < max) {
			el->list.next = (void*)el->list.next + delta;
		}
		if((void*)el->list.prev >= min && (void*)el->list.prev < max) {
			el->list.prev = (void*)el->list.next + delta;
		}
		if((void*)el->child_list.next >= min && (void*)el->child_list.next < max) {
			el->child_list.next = (void*)el->child_list.next + delta;
		}
		if((void*)el->child_list.prev >= min && (void*)el->child_list.prev < max) {
			el->child_list.prev = (void*)el->child_list.prev + delta;
		}
	}
}

static void _rebase_strs(void* min, void* max, ptrdiff_t delta) {
	DictEntry* m = ui_dict.map;
	for(uint i = 0; i < ui_dict.mask+1; ++i) {
		if(m[i].key && ((void*)m[i].key >= min) && ((void*)m[i].key < max)) {
			m[i].key += delta;
		}
	}

	for(uint i = 0; i < ui_elements.size; ++i) {
		UIElement* el = darray_get(&ui_elements, i);	
		if((void*)el->name >= min && (void*)el->name < max) {
			el->name += delta;
		}
	}
}

UIElement* _parse_def(MMLObject* mml, NodeIdx node, UIElement* parent) {
	assert(strcmp(mml_get_name(mml, node), "def") == 0);

	// Set the name, keep it in name_strings darray
	void* old_data = ui_elements.data;
	darray_append_nulls(&ui_elements, 1);
	if(old_data != ui_elements.data)
		_rebase_ui_elements(
				old_data,
				old_data + (ui_elements.size-1) * sizeof(UIElement),
				ui_elements.data - old_data
		);
	uint i = ui_elements.size-1;
	UIElement* new = darray_get(&ui_elements, i);
	const char* name = mml_getval_str(mml, node);
	uint old_size = name_strings.size;
	old_data = name_strings.data;
	darray_append_multi(&name_strings, name, strlen(name)+1);
	if(old_data != name_strings.data)
		_rebase_strs(old_data, old_data + old_size, name_strings.data - old_data);
	new->name = name_strings.data + old_size;

	// Init child list
	list_init(&new->child_list);

	// Iterate over elements
	NodeIdx element = mml_get_first_child(mml, node);
	for(; element != 0; element = mml_get_next(mml, element)) {
		const char* type = mml_get_name(mml, element);

		if(strcmp(type, "def") == 0) {
			// Add child
			UIElement* child = _parse_def(mml, element, new);
			new = darray_get(&ui_elements, i);
			list_push_back(&new->child_list, &child->list); 
		}
		else {
			// Eval a function
			if(_is_vec2_fun(mml, element)) {
				new->vec2 = _vec2_fun(mml, element, new, parent);
				new->members |= UI_EL_VEC2;
			}
			else if(_is_rect_fun(mml, element)) {
				new->rect = _rect_fun(mml, element, new, parent);
				new->members |= UI_EL_RECT;
			}
			else if(_is_spr_fun(mml, element)) {
				new->spr = _spr_fun(mml, element);
				new->members |= UI_EL_SPR;
			}
		}
	}

	return new;
}

static void _uidesc_load(MMLObject* mml) {
	NodeIdx root = mml_root(mml);

	assert(strcmp(mml_get_name(mml, root), "uidesc") == 0);

	NodeIdx child = mml_get_first_child(mml, root);
	for(; child != 0; child = mml_get_next(mml, child)) {
		UIElement* el = _parse_def(mml, child, NULL);
		dict_insert(&ui_dict, el->name, el);
	}
}

static void _init(void) {
	ui_elements = darray_create(sizeof(UIElement), 0);
	name_strings = darray_create(sizeof(char), 0);
	dict_init(&ui_dict);
}

void uidesc_init(const char* desc) {
	assert(desc);
	const char* mml_text = txtfile_read(desc);	

	_init();

	MMLObject mml;
	if(!mml_deserialize(&mml, mml_text))
		LOG_ERROR("Unable to parse uidesc %s", desc);
	MEM_FREE(mml_text);

	_uidesc_load(&mml);

	mml_free(&mml);
}

void uidesc_init_str(const char* mmldesc) {
	assert(mmldesc);

	_init();

	MMLObject mml;
	if(!mml_deserialize(&mml, mmldesc))
		LOG_ERROR("Unable to parse uidesc from string");

	_uidesc_load(&mml);

	mml_free(&mml);
}

void uidesc_close(void) {
	dict_free(&ui_dict);
	darray_free(&name_strings);
	darray_free(&ui_elements);
}

UIElement* uidesc_get(const char* name) {
	return (UIElement*)dict_get(&ui_dict, name);
}

UIElement* uidesc_get_child(UIElement* parent, const char* name) {
	assert(parent);

	if(parent->child_dict) {
		return (UIElement*)dict_get(parent->child_dict, name);
	}
	else {
		UIElement* el;
		list_for_each_entry(el, &parent->child_list, list) {
			if(strcmp(el->name, name) == 0)
				return el;
		}
	}

	return NULL;
}

