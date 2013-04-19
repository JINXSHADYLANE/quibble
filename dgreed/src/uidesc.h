#ifndef UIDESC_H
#define UIDESC_H

#include "system.h"
#include "datastruct.h"
#include "sprsheet.h"

// A little declarative UI language, helps to keep
// all the hardcoded positions and rects away from code.

/*
Example

(uidesc morka
	(def screen
		(rect 0,0,480,320)
	)

	(def title
		(add <- (middle <- (get_rect screen)) (vec2 0,-100))
		(spr title)
	)
)
 
*/

// Functions:
//
// (def name list)
// Defines UIElement, takes name and a list which can have
// other UIElements (children), spr (img or anim), vec2 and rect.
//
// (rect t,l,r,b)
// Constructs rect
//
// (vec2 x,y)
// Constructs vec2
//
// (spr name)
// Constructs spr
//
// (spr_size name)
// Returns vec2 size of spr
//
// (get_rect name)
// Constructs rect of named ui element
//
// (get_vec2 name)
// Constructs vec2 of named ui element
//
// (add <- list)
// Adds a list of vec2 elements, constructs result vec2 element
//
// (sub <- list)
// Subtracts a list of vec2 elements from the first element
//
// (x vec2)
// Returns vec2 with zeroed y component
//
// (y vec2)
// Returns vec2 with zeroed x component
//
// (radd <- list)
// First list element must be rect, others - vec2;
// constructs rect shifted by all the vec2s.
//
// (avg <- list)
// Constructs average of a list of vec2s
//
// (middle rect)
// Constructs vec2 with middle point of rect
//
// (tl|tr|bl|br rect)
// Top-left, top-right... corner of rect 

typedef enum {
	UI_EL_RECT = 1,
	UI_EL_VEC2 = 2,
	UI_EL_SPR = 4
} UIElementType;

typedef struct {
	const char* name;

	// Empty if no children
	ListHead child_list;

	// Children dict, not null if element has more than 16 children
	Dict* child_dict;

	// Empty if root element
	ListHead list;

	UIElementType members;
	SprHandle spr;	
	Vector2 vec2;
	RectF rect;
} UIElement;

void uidesc_init(const char* desc, Vector2 screen);
void uidesc_init_str(const char* mmldesc, Vector2 screen);
void uidesc_close(void);

// Returns UIElement, searches only root elements.
// NULL if not found.
UIElement* uidesc_get(const char* name);

// Returns children of UIElement, NULL if not found.
UIElement* uidesc_get_child(UIElement* parent, const char* name);

#endif
