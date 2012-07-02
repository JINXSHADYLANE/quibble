#include "utils.h"
#include "mml.h"
#include "polygonize.h"
#include "memory.h"
#include "gfx_utils.h"
#include "ai_precalc.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#include <ctype.h>

const uint width = 480;
const uint height = 320;
const uint density_width = 30;
const uint density_height = 20;

#define COLOR_SHADOW COLOR_RGBA(0, 0, 0, 196)
#define COLOR_EMPTY COLOR_RGBA(0, 0, 0, 0)

uint chapter = 0;
const char* backg_img_filename = NULL;
const char* walls_img_filename = NULL;
const char* density_filename = NULL;

MMLObject arena_mml;
int shadow_shift_x, shadow_shift_y;
bool no_rigid_walls = false;
bool no_wall_shadows = false;
Color* background = NULL;
Color* walls = NULL;
Color* density = NULL;
Color* shadow = NULL;
Color* final_image = NULL;
uint* collision_mask = NULL;
float* density_map = NULL;

FileHandle precalc_file = (FileHandle)NULL;

// externs for modifying navmesh builder settings
extern float min_wall_distance;
extern float min_point_distance;
extern float max_edge_distance;

void init(const char* mml_file) {
	// Get chapter - second-to-last digit in filename
	// Works as long as arenas are named cx_arenay
	uint i = strlen(mml_file);
	while(i && !isdigit(mml_file[--i]));
	while(i && !isdigit(mml_file[--i]));
	assert(i);
	chapter = mml_file[i] - '0';

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

	Vector2 shift = mml_getval_vec2(&arena_mml, shadow_node);
	shadow_shift_x = (int)shift.x;
	shadow_shift_y = (int)shift.y;

	NodeIdx no_rigid_walls_node = mml_get_child(&arena_mml, root, "no_rigid_walls");
	if(no_rigid_walls_node != 0)
		no_rigid_walls = mml_getval_bool(&arena_mml, no_rigid_walls_node);

	NodeIdx no_wall_shadows_node = mml_get_child(&arena_mml, root, "no_wall_shadows");
	if(no_wall_shadows_node != 0)
		no_wall_shadows = mml_getval_bool(&arena_mml, no_wall_shadows_node);
}

// Strips folders from path, returns only filename
char* _get_file(const char* path) {
	int idx = strlen(path)-1;
	while(idx >= 0 && (path[idx] != '\\' && path[idx] != '/')) {
		idx--;
	}
	return strclone(&(path[idx+1]));
}

void save(const char* mml_file, const char* walls_img_file)
{
	NodeIdx root = mml_root(&arena_mml);

	// Update mml
	char* walls_filename = _get_file(walls_img_file);
	char img_path[256];
	sprintf(img_path, "greed_assets/%s", walls_filename);
	NodeIdx walls_img_node = mml_get_child(&arena_mml, root, "walls");
	mml_setval_str(&arena_mml, walls_img_node, img_path);
	MEM_FREE(walls_filename);

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
	gfx_save_tga(walls_img_file,(void*)final_image, 512, 1024);
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
	if(final_image)
		MEM_FREE(final_image);
}

void gen_density_map(void) {
	density_map = (float*)MEM_ALLOC(density_width * density_height * sizeof(float));
	for(uint i = 0; i < density_width*density_height; ++i) {
		int r __attribute__((unused)), g, b, a  __attribute__((unused));
		COLOR_DECONSTRUCT(density[i], r, g, b, a);
		float d = (float)((g+b)/2) / 255.0f;
		assert(d >= 0.0f && d <= 1.0f);
		density_map[i] = d;
	}
}

void load_images(const char* folder) {
	NodeIdx root = mml_root(&arena_mml);
	NodeIdx background_node = mml_get_child(&arena_mml, root, "background");
	NodeIdx walls_node = mml_get_child(&arena_mml, root, "walls");
	NodeIdx density_node = mml_get_child(&arena_mml, root, "density");

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

	if(density_node) {
		density_filename = mml_getval_str(&arena_mml, density_node);
		sprintf(path, "%s%s", folder, density_filename);
		density = (Color*)stbi_load(path, &w, &h, &n, 4);

		// Rescale density map to correct size
		int	scale = 1;
		while(w/scale > density_width && h/scale > density_height)
			scale++;
		if(w/scale != density_width || h/scale != density_height)
			LOG_ERROR("Unable to downscale density map to 30x20");

		Color* img = MEM_ALLOC(w * h * 4);
		memcpy(img, density, w * h * 4);
		stbi_image_free(density);
		density = img;
		while(scale/=2) {
			Color* new_density = gfx_downscale(density, w, h);
			w /= 2; h /= 2;
			MEM_FREE(density);
			density = new_density;
		}

		assert(w == density_width && h == density_height);

		gen_density_map();
	}
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
	if(no_wall_shadows) {
		memset(shadow, 0, sizeof(Color) * width * height);
		return;
	}

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
}

void blend_images(void) {
	const int w = 512;
	const int h = 1024;

	final_image = (Color*)MEM_ALLOC(sizeof(Color) * w * h);
	memset(final_image, 0, sizeof(Color) * w * h);

	// First - all layers blended together
	gfx_fill(final_image, w, h, COLOR_WHITE, 0, 0, width, height);
	gfx_blit(final_image, w, h, background, width, height, 0, 0);
	gfx_blit_ex(final_image, w, h, shadow, width, height, 0, 0,
		shadow_shift_x, shadow_shift_y);
	gfx_blit(final_image, w, h, walls, width, height, 0, 0);

	// Second - background and shadow only
	gfx_fill(final_image, w, h, COLOR_WHITE, 0, height+2, width, height*2 + 2);
	gfx_blit(final_image, w, h, background, width, height, 0, height+2);
	gfx_blit_ex(final_image, w, h, shadow, width, height, 0, height+2,
		shadow_shift_x, shadow_shift_y);

	// Third - walls only (perform simple byte-by-byte copy)
	for(uint dy = 0; dy < height; ++dy) {
		for(uint dx = 0; dx < width; ++dx) {
			uint y = (height+2) * 2 + dy;
			size_t dest_idx = IDX_2D(dx, y, w);
			size_t src_idx = IDX_2D(dx, dy, width);
			final_image[dest_idx] = walls[src_idx];
		}
	}
}
void gen_precalc_data(void) {
	DArray segments = darray_create(sizeof(Segment), 0);
	DArray triangles;
	if(!no_rigid_walls)
		triangles = poly_triangulate_raster(collision_mask, width, height, &segments);
	else
		triangles = darray_create(sizeof(Triangle), 0);

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

	// Set navmesh settings, special for fourth chapter
	if(chapter == 4) {
		min_wall_distance = 21.0f;
		min_point_distance = 19.0f;
		max_edge_distance = 40.0f;
	}

	NavMesh	nav_mesh = ai_precalc_navmesh(segments, platforms,
		(float)width, (float)height);

	darray_free(&platforms);

	// Collision tris
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

	// Density map
	if(density_map != NULL) {
		size_t density_size = density_width * density_height;
		file_write_uint32(precalc_file, (uint32)density_size);
		for(uint i = 0; i < density_width*density_height; ++i)
			file_write_float(precalc_file, density_map[i]);

		MEM_FREE(density_map);
	}
	else
		file_write_uint32(precalc_file, 0);

	// Navmesh
	ai_save_navmesh(precalc_file, &nav_mesh);

	darray_free(&triangles);
	darray_free(&segments);
}

int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);
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

	char* file = path_change_ext(walls_img_filename, "tga");
	sprintf(final_walls_name, "%s%s", mml_path, file);
	MEM_FREE(file);

	// Save collision and ai precalc data in binary file
	char* precalc_filename = path_change_ext(params_get(1), "precalc");
	precalc_file = file_create(precalc_filename);

	gen_precalc_data();

	file_close(precalc_file);
	MEM_FREE(mml_path);
	MEM_FREE(precalc_filename);

	save(params_get(1), final_walls_name);
	deinit();

	return 0;
}

