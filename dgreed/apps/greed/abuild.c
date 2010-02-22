#include "utils.h"
#include "mml.h"
#include "polygonize.h"
#include "memory.h"
#include "gfx_utils.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

const uint width = 480;
const uint height = 320;

#define COLOR_SHADOW COLOR_RGBA(0, 0, 0, 196)
#define COLOR_EMPTY COLOR_RGBA(0, 0, 0, 0)

MMLObject arena_mml;
int shadow_shift_x, shadow_shift_y;
Color* background = NULL;
Color* walls = NULL;
Color* shadow = NULL;
Color* final_img = NULL;
uint* collision_mask = NULL;

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

void save(const char* mml_file, const char* img_file) {
	// Update mml
	char* filename = _get_file(img_file);
	char img_path[256];
	sprintf(img_path, "greed_assets/%s", filename);
	NodeIdx img_node = mml_node(&arena_mml, "img", img_path);
	MEM_FREE(filename);

	mml_insert_after(&arena_mml, mml_root(&arena_mml), img_node, "walls");
	
	// Save mml
	char* mml_text = mml_serialize(&arena_mml);
	if(mml_text == NULL)
		LOG_ERROR("Failed to deserialize arena description file");
	txtfile_write(mml_file, mml_text);
	MEM_FREE(mml_text);

	// Save image
	stbi_write_tga(img_file, 512, 512, 4, (void*)final_img);
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
	if(final_img)
		MEM_FREE(final_img);
}

void load_images(const char* folder) {
	NodeIdx root = mml_root(&arena_mml);
	NodeIdx background_node = mml_get_child(&arena_mml, root, "background");
	NodeIdx walls_node = mml_get_child(&arena_mml, root, "walls");

	char path[256];

	int w, h, n;
	sprintf(path, "%s%s", folder, mml_getval_str(&arena_mml, background_node));
	background = (Color*)stbi_load(path, &w, &h, &n, 4);
	if(background == NULL)
		LOG_ERROR("Unable to load background image");
	if(w != width || h != height)
		LOG_ERROR("Bad background image size");

	sprintf(path, "%s%s", folder, mml_getval_str(&arena_mml, walls_node));
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
				shadow[idx] = COLOR_SHADOW;
			else
				shadow[idx] = COLOR_EMPTY;
		}
	}

	gfx_blur(shadow, width, height);
	gfx_blur(shadow, width, height);
	gfx_blur(shadow, width, height);
}

void blend_images(void) {
	final_img = (Color*)MEM_ALLOC(sizeof(Color) * 512 * 512);
	memset(final_img, 0, sizeof(Color) * 512 * 512);

	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			size_t idx = IDX_2D(x, y, 512);

			Color backg = gfx_sample_img(background, width, height, x, y);
			Color shadw = gfx_sample_img(shadow, width, height,
				x - shadow_shift_x, y - shadow_shift_y);
			Color wall	= gfx_sample_img(walls, width, height, x, y);

			Color blended = gfx_blend(gfx_blend(backg, shadw), wall);
			final_img[idx] = blended;
		}
	}
}	

void gen_collision_data(void) {
	DArray triangles = 
		poly_triangulate_raster(collision_mask, width, height, NULL);
	Triangle* tris = DARRAY_DATA_PTR(triangles, Triangle);

	NodeIdx collision_node = mml_node(&arena_mml, "collision" , "_");

	for(uint i = 0; i < triangles.size; ++i) {
		char tri_desc[1024];
		sprintf(tri_desc, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
			tris[i].p1.x, tris[i].p1.y,
			tris[i].p2.x, tris[i].p2.y,
			tris[i].p3.x, tris[i].p3.y);
		NodeIdx tri_node = mml_node(&arena_mml, "t", tri_desc);
		mml_append(&arena_mml, collision_node, tri_node);
	}

	mml_append(&arena_mml, mml_root(&arena_mml), collision_node);
}	

// Returns new string with removed file name from path
char* _get_folder(const char* path) {
	char* result = strclone(path);
	int idx = strlen(result)-1;
	while(idx >= 0 && (result[idx] != '\\' && result[idx] != '/')) {
		result[idx] = '\0';
		idx--;
	}
	return result;
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

	char* folder = _get_folder(params_get(0));	
	load_images(folder);
	MEM_FREE(folder);

	make_collision_mask();
	make_shadow();
	blend_images();
	gen_collision_data();

	// Figure out where to save final img:
	char* mml_path = _get_folder(params_get(1)); 
	// Get arena name
	const char* arena_name = mml_getval_str(&arena_mml, mml_root(&arena_mml));
	// Glue everything together
	char final_img_name[256];
	sprintf(final_img_name, "%s%s.tga", mml_path, arena_name);
	MEM_FREE(mml_path);

	save(params_get(1), final_img_name);
	deinit();
	log_close();

	return 0;
}

