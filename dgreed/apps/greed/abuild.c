#include "utils.h"
#include "mml.h"
#include "polygonize.h"
#include "memory.h"
#include "gfx_utils.h"
#include "ai_precalc.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

const uint width = 480;
const uint height = 320;

#define COLOR_SHADOW COLOR_RGBA(0, 0, 0, 196)
#define COLOR_EMPTY COLOR_RGBA(0, 0, 0, 0)

const char* backg_img_filename = NULL;
const char* walls_img_filename = NULL;

MMLObject arena_mml;
int shadow_shift_x, shadow_shift_y;
Color* background = NULL;
Color* walls = NULL;
Color* shadow = NULL;
Color* final_background = NULL;
Color* final_arena = NULL;
uint* collision_mask = NULL;

FileHandle precalc_file = (FileHandle)NULL;

void init(const char* mml_file) {
	char* mml_text = txtfile_read(mml_file);
	if(!mml_deserialize(&arena_mml, mml_text))
		LOG_ERROR("Failed to deserialize arena description file");
	MEM_FREE(mml_text);

	NodeIdx root = mml_root(&arena_mml);
	if(strcmp(mml_get_name(&arena_mml, root), "arena") != 0)
		LOG_ERROR("Invalid arena description file");

	NodeIdx shadow_node = mml_get_child(&arena_mml, root, "shadow_shift");
	if(!shadow_node)
		LOG_ERROR("Unable to find shadow_shift propierty");
	float shift_x, shift_y;	
	sscanf(mml_getval_str(&arena_mml, shadow_node), "%f,%f", &shift_x, &shift_y);	
	shadow_shift_x = (int)shift_x;
	shadow_shift_y = (int)shift_y;
}	

// Strips folders from path, returns only filename
char* _get_file(const char* path) {
	int idx = strlen(path)-1;
	while(idx >= 0 && (path[idx] != '\\' && path[idx] != '/')) {
		idx--;
	}
	return strclone(&(path[idx+1]));
}	

void save(const char* mml_file, const char* walls_img_file, const char*
	backg_img_file) {

	NodeIdx root = mml_root(&arena_mml);

	// Update mml
	char* walls_filename = _get_file(walls_img_file);
	char* backg_filename = _get_file(backg_img_file);
	char img_path[256];
	sprintf(img_path, "greed_assets/%s", walls_filename);
	NodeIdx walls_img_node = mml_get_child(&arena_mml, root, "walls");
	mml_setval_str(&arena_mml, walls_img_node, img_path);
	sprintf(img_path, "greed_assets/%s", backg_filename);
	NodeIdx backg_img_node = mml_get_child(&arena_mml, root, "background");
	mml_setval_str(&arena_mml, backg_img_node, img_path);

	MEM_FREE(walls_filename);
	MEM_FREE(backg_filename);

	const char* arena_name = mml_getval_str(&arena_mml, root);
	char precalc_filename[256];
	sprintf(precalc_filename, "greed_assets/%s.precalc", arena_name);
	NodeIdx precalc_node = mml_node(&arena_mml, "precalc", precalc_filename);
	mml_insert_after(&arena_mml, mml_root(&arena_mml), precalc_node, "walls");
	
	// Save mml
	char* mml_text = mml_serialize(&arena_mml);
	if(mml_text == NULL)
		LOG_ERROR("Failed to deserialize arena description file");
	txtfile_write(mml_file, mml_text);
	MEM_FREE(mml_text);

	// Save images
	stbi_write_tga(backg_img_file, 512, 512, 4, (void*)final_background);
	stbi_write_tga(walls_img_file, 512, 512, 4, (void*)final_arena);
}	

void deinit(void) {
	mml_free(&arena_mml);
	if(background)
		stbi_image_free(background);	
	if(walls)
		stbi_image_free(walls);
	if(collision_mask)
		MEM_FREE(collision_mask);
	if(shadow)
		MEM_FREE(shadow);
	if(final_background)
		MEM_FREE(final_background);
	if(final_arena)
		MEM_FREE(final_arena);
		
}

void load_images(const char* folder) {
	NodeIdx root = mml_root(&arena_mml);
	NodeIdx background_node = mml_get_child(&arena_mml, root, "background");
	NodeIdx walls_node = mml_get_child(&arena_mml, root, "walls");

	char path[256];

	int w, h, n;
	backg_img_filename = mml_getval_str(&arena_mml, background_node);
	sprintf(path, "%s%s", folder, backg_img_filename);
	background = (Color*)stbi_load(path, &w, &h, &n, 4);
	if(background == NULL)
		LOG_ERROR("Unable to load background image");
	if(w != width || h != height)
		LOG_ERROR("Bad background image size");

	walls_img_filename = mml_getval_str(&arena_mml, walls_node);
	sprintf(path, "%s%s", folder, walls_img_filename);
	walls = (Color*)stbi_load(path, &w, &h, &n, 4);
	if(background == NULL)
		LOG_ERROR("Unable to load walls image");
	if(w != width || h != height)
		LOG_ERROR("Bad walls image size");
}

void make_collision_mask(void) {
	assert(walls);

	collision_mask = (uint*)MEM_ALLOC(width * height * sizeof(uint));
	memset(collision_mask, RASTER_EMPTY, width * height * sizeof(uint));

	for(uint y = 0; y < height; ++y) {
		for(uint x = 0; x < width; ++x) {
			size_t idx = IDX_2D(x, y, width);

			byte a = (endian_swap4(walls[idx]) & 0xFF000000) >> 24;
			if(a > 128)
				collision_mask[idx] = RASTER_SOLID;
		}
	}
}	

void make_shadow(void) {
	shadow = (Color*)MEM_ALLOC(sizeof(Color) * width * height);

	for(uint y = 0; y < height; ++y) {
		for(uint x = 0; x < width; ++x) {
			size_t idx = IDX_2D(x, y, width);
			if(collision_mask[idx] == RASTER_SOLID)
				shadow[idx] = endian_swap4(COLOR_SHADOW);
			else
				shadow[idx] = endian_swap4(COLOR_EMPTY);
		}
	}

	gfx_blur(shadow, width, height);
	gfx_blur(shadow, width, height);
	gfx_blur(shadow, width, height);

	Color* big_shadow = shadow;
	shadow = gfx_downscale(big_shadow, width, height);
	MEM_FREE(big_shadow);
}

void blend_images(void) {
	final_background = (Color*)MEM_ALLOC(sizeof(Color) * 512 * 512);
	memset(final_background, 0, sizeof(Color) * 512 * 512);

	final_arena = (Color*)MEM_ALLOC(sizeof(Color) * 512 * 512);
	memset(final_arena, 0, sizeof(Color) * 512 * 512);

	gfx_blit(final_background, 512, 512, background, width, height, 0, 0);

	gfx_blit(final_arena, 512, 512, walls, width, height, 0, 0);
	gfx_blit(final_arena, 512, 512, shadow, width/2, height/2, 0, height+1);
}	

void gen_precalc_data(void) {
	DArray segments = darray_create(sizeof(Segment), 0);
	DArray triangles = 
		poly_triangulate_raster(collision_mask, width, height, &segments);

	DArray platforms = darray_create(sizeof(Vector2), 0);	
	NodeIdx root = mml_root(&arena_mml);
	NodeIdx platforms_node = mml_get_child(&arena_mml, root, "platforms");
	for(NodeIdx platform = mml_get_first_child(&arena_mml, platforms_node);
		platform != 0;
		platform = mml_get_next(&arena_mml, platform)) {

		assert(strcmp(mml_get_name(&arena_mml, platform), "p") == 0);
		
		Vector2	p = mml_getval_vec2(&arena_mml, platform);

		darray_append(&platforms, (void*)&p);
	}	

	NavMesh	nav_mesh = ai_precalc_navmesh(segments, platforms,
		(float)width, (float)height);

	Triangle* tris = NULL;
	if(triangles.size != 0)
		tris = DARRAY_DATA_PTR(triangles, Triangle);
	
	file_write_uint32(precalc_file, triangles.size);	

	for(uint i = 0; i < triangles.size; ++i) {
		file_write_float(precalc_file, tris[i].p1.x);
		file_write_float(precalc_file, tris[i].p1.y);
		file_write_float(precalc_file, tris[i].p2.x);
		file_write_float(precalc_file, tris[i].p2.y);
		file_write_float(precalc_file, tris[i].p3.x);
		file_write_float(precalc_file, tris[i].p3.y);
	}

	ai_save_navmesh(precalc_file, &nav_mesh);

	darray_free(&triangles);
	darray_free(&segments);
}	

int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);
	log_init("abuild.log", LOG_LEVEL_INFO);
	if(params_count() < 2) {
		printf("Provide source and target files\n");
		log_close();
		exit(0);
	}

	init(params_get(0));

	char* folder = path_get_folder(params_get(0));	
	load_images(folder);
	MEM_FREE(folder);

	make_collision_mask();
	make_shadow();
	blend_images();
	
	// Figure out where to save final img:
	char* mml_path = path_get_folder(params_get(1)); 
	// Glue everything together
	char final_walls_name[256];
	char final_backg_name[256];
	
	char* file = path_change_ext(walls_img_filename, "tga");
	sprintf(final_walls_name, "%s%s", mml_path, file);
	MEM_FREE(file);

	file = path_change_ext(backg_img_filename, "tga");
	sprintf(final_backg_name, "%s%s", mml_path, file);
	MEM_FREE(file);
	
	// Save collision and ai precalc data in binary file
	char* precalc_filename = path_change_ext(params_get(1), "precalc");
	precalc_file = file_create(precalc_filename);

	gen_precalc_data();

	file_close(precalc_file);
	MEM_FREE(mml_path);
	MEM_FREE(precalc_filename);

	save(params_get(1), final_walls_name, final_backg_name);
	deinit();
	log_close();

	return 0;
}

