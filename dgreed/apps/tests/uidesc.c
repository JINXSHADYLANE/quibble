
#include "uidesc.h"

TEST_(empty) {
	uidesc_init_str(
			"(uidesc empty)"
	);

	ASSERT_(uidesc_get("screen") == NULL);

	uidesc_close();
}

TEST_(def) {
	uidesc_init_str(
			"(uidesc def"
			"	(def screen"
			"	)"
			")"
	);

	UIElement* screen = uidesc_get("screen");
	ASSERT_(screen);
	ASSERT_(strcmp(screen->name, "screen") == 0);
	ASSERT_(screen->members == 0);

	ASSERT_(uidesc_get("el1") == NULL);

	uidesc_close();
}

TEST_(def_vec2) {
	uidesc_init_str(
			"(uidesc def"
			"	(def el1"
			"		(vec2 32,17)"
			"	)"
			")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	ASSERT_(strcmp(el1->name, "el1") == 0);
	ASSERT_(el1->members == UI_EL_VEC2);
	ASSERT_(el1->vec2.x == 32.0f);
	ASSERT_(el1->vec2.y == 17.0f);

	uidesc_close();
}

TEST_(def_rect) {
	uidesc_init_str(
			"(uidesc def"
			"	(def el1"
			"		(vec2 -32,-17)"
			"		(rect 1,2,3,4)"
			"	)"
			")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	ASSERT_(strcmp(el1->name, "el1") == 0);
	ASSERT_(el1->members == (UI_EL_VEC2 | UI_EL_RECT));
	ASSERT_(el1->vec2.x == -32.0f);
	ASSERT_(el1->vec2.y == -17.0f);
	ASSERT_(el1->rect.left == 1.0f);
	ASSERT_(el1->rect.top == 2.0f);
	ASSERT_(el1->rect.right == 3.0f);
	ASSERT_(el1->rect.bottom == 4.0f);

	uidesc_close();
}

TEST_(def_nested) {
	uidesc_init_str(
			"(uidesc def"
			"	(def el1"
			"		(def el2"
			"			(vec2 100,101)"
			"		)"
			"		(rect 1,2,3,4)"
			"	)"
			")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	UIElement* el2 = uidesc_get("el2");
	ASSERT_(el2 == NULL);
	el2 = uidesc_get_child(el1, "el2");
	ASSERT_(el2);

	ASSERT_(el1->members == UI_EL_RECT);
	ASSERT_(el2->members == UI_EL_VEC2);

	uidesc_close();
}

TEST_(get_vec2) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(vec2 1,2)"
		"	)"
		"	(def el2"
		"		(get_vec2 el1)"
		"	)"
		")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	UIElement* el2 = uidesc_get("el2");
	ASSERT_(el2);

	ASSERT_(el1->members == UI_EL_VEC2);
	ASSERT_(el2->members == UI_EL_VEC2);

	ASSERT_(el1->vec2.x == 1.0f);
	ASSERT_(el1->vec2.y == 2.0f);
	ASSERT_(el2->vec2.x == 1.0f);
	ASSERT_(el2->vec2.y == 2.0f);

	uidesc_close();
}

