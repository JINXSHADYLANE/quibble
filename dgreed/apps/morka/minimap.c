#include "minimap.h"

static uint level_distance = 0;
DArray minimap_pointers;

void minimap_init(){
	minimap_pointers = darray_create(sizeof(ObjRabbit*), 0);
}

void minimap_close(void){
	darray_free(&minimap_pointers);
}

void minimap_track(ObjRabbit* rabbit){
	darray_append(&minimap_pointers, rabbit);
}

void minimap_draw(){
	// TODO
}

void minimap_reset(uint distance){
	minimap_pointers.size = 0;
	level_distance = distance;
}