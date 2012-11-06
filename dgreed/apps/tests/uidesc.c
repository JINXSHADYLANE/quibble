
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

TEST_(get_rect_nested) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(def el2"
		"			(rect 5,6,7,8)"
		"		)"
		"		(get_rect el2)"
		"		(vec2 1,2)"
		"	)"
		"	(def el2"
		"		(vec2 3,4)"
		"		(get_rect el1)"
		"	)"
		")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	UIElement* el12 = uidesc_get_child(el1, "el2");
	ASSERT_(el12);
	UIElement* el2 = uidesc_get("el2");
	ASSERT_(el2);
	ASSERT_(el12 != el2);

	ASSERT_(el1->members  == (UI_EL_VEC2 | UI_EL_RECT));
	ASSERT_(el12->members == UI_EL_RECT);
	ASSERT_(el2->members  == (UI_EL_VEC2 | UI_EL_RECT));

	ASSERT_(el1->vec2.x == 1.0f);
	ASSERT_(el1->vec2.y == 2.0f);
	ASSERT_(el1->rect.left == 5.0f);
	ASSERT_(el1->rect.top == 6.0f);
	ASSERT_(el1->rect.right == 7.0f);
	ASSERT_(el1->rect.bottom == 8.0f);

	ASSERT_(el2->vec2.x == 3.0f);
	ASSERT_(el2->vec2.y == 4.0f);
	ASSERT_(el2->rect.left == 5.0f);
	ASSERT_(el2->rect.top == 6.0f);
	ASSERT_(el2->rect.right == 7.0f);
	ASSERT_(el2->rect.bottom == 8.0f);

	uidesc_close();
}

TEST_(add) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(add <- (vec2 2,3) (vec2 4,-1))"	
		"	)"
		"	(def el2"
		"		(add <- (get_vec2 el1) (add <- (vec2 -5,0) (vec2 10,10)))"
		"	)"
		")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	UIElement* el2 = uidesc_get("el2");
	ASSERT_(el2);

	ASSERT_(el1->members == UI_EL_VEC2);
	ASSERT_(el2->members == UI_EL_VEC2);

	ASSERT_(el1->vec2.x == 6.0f);
	ASSERT_(el1->vec2.y == 2.0f);
	ASSERT_(el2->vec2.x == 11.0f);
	ASSERT_(el2->vec2.y == 12.0f);

	uidesc_close();
}

TEST_(avg) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(avg <- (vec2 1,1))"	
		"	)"
		"	(def el2"
		"		(avg <- (vec2 1,-5) (vec2 2,0) (vec2 3,5))"
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
	ASSERT_(el1->vec2.y == 1.0f);
	ASSERT_(el2->vec2.x == 2.0f);
	ASSERT_(el2->vec2.y == 0.0f);

	uidesc_close();
}

TEST_(middle) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(middle <- (rect 0,10,100,40))"	
		"	)"
		")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);

	ASSERT_(el1->members == UI_EL_VEC2);

	ASSERT_(el1->vec2.x == 50.0f);
	ASSERT_(el1->vec2.y == 25.0f);

	uidesc_close();
}

TEST_(tl_tr_bl_br) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(def tl"
		"			(tl <- (rect -1,-4,15,23))"
		"		)"
		"		(def tr"
		"			(tr <- (rect -1,-4,15,23))"
		"		)"
		"		(def bl"
		"			(bl <- (rect -1,-4,15,23))"
		"		)"
		"		(def br"
		"			(br <- (rect -1,-4,15,23))"
		"		)"
		"	)"
		")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);

	UIElement* tl = uidesc_get_child(el1, "tl");
	UIElement* tr = uidesc_get_child(el1, "tr");
	UIElement* bl = uidesc_get_child(el1, "bl");
	UIElement* br = uidesc_get_child(el1, "br");

	ASSERT_(tl && tr && bl && br);
	
	ASSERT_(tl->vec2.x == -1.0f);	
	ASSERT_(tl->vec2.y == -4.0f);	

	ASSERT_(tr->vec2.x == 15.0f);	
	ASSERT_(tr->vec2.y == -4.0f);	

	ASSERT_(bl->vec2.x == -1.0f);	
	ASSERT_(bl->vec2.y == 23.0f);	

	ASSERT_(br->vec2.x == 15.0f);	
	ASSERT_(br->vec2.y == 23.0f);	

	uidesc_close();
}

TEST_(radd) {
	uidesc_init_str(
		"(uidesc def"
		"	(def el1"
		"		(radd <- (rect 0,0,10,100) (vec2 2,3) (vec2 4,-1))"	
		"	)"
		"	(def el2"
		"		(radd <- (get_rect el1) (add <- (vec2 -5,0) (vec2 10,10)))"
		"	)"
		")"
	);

	UIElement* el1 = uidesc_get("el1");
	ASSERT_(el1);
	UIElement* el2 = uidesc_get("el2");
	ASSERT_(el2);

	ASSERT_(el1->members == UI_EL_RECT);
	ASSERT_(el2->members == UI_EL_RECT);

	ASSERT_(el1->rect.left == 6.0f);
	ASSERT_(el1->rect.top == 2.0f);
	ASSERT_(el1->rect.right == 16.0f);
	ASSERT_(el1->rect.bottom == 102.0f);

	ASSERT_(el2->rect.left == 11.0f);
	ASSERT_(el2->rect.top == 12.0f);
	ASSERT_(el2->rect.right == 21.0f);
	ASSERT_(el2->rect.bottom == 112.0f);

	uidesc_close();
}

