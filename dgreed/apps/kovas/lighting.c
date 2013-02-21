#include "lighting.h"

#include <system.h>

const float light_tile_size = 16.0f;

static TexHandle light_tex; 
static RectF light_screen;

void lighting_init(RectF screen) {
	light_tex = tex_load("bulb_assets/dark.png");
	light_screen = screen;
}

void lighting_close(void) {
	tex_free(light_tex);
}

Color _sample_lights(Vector2 pos, Light* lights, uint n_lights) {
	float lum = 0.0f;
	for(uint i = 0; i < n_lights; ++i) {
		float max_dist_sq = lights[i].radius * lights[i].radius;
		float dist_sq = vec2_length_sq(vec2_sub(pos, lights[i].pos));
		if(dist_sq <= max_dist_sq) {
			lum += (1.0f - sqrtf(dist_sq / max_dist_sq)) * lights[i].alpha;
		}
	}
	lum = clamp(0.0f, 1.0f, lum);
	byte alpha = (byte)lrintf(lum * 255.0f);
	return COLOR_RGBA(255, 255, 255, 255 - alpha);
}

void lighting_render(uint layer, DArray* lights) {
	assert(lights);
	assert(lights->item_size == sizeof(Light));

	float w = rectf_width(&light_screen);
	float h = rectf_height(&light_screen);

	uint x_tiles = (uint)ceilf(w / light_tile_size);
	uint y_tiles = (uint)ceilf(h / light_tile_size);

	Light* lights_array = DARRAY_DATA_PTR(*lights, Light);

	// Draw, joining horizontal runs of same color into spans
	for(uint y = 0; y < y_tiles; ++y) {
		RectF dest = rectf(
			light_screen.left, light_screen.top + y * light_tile_size, 
			0.0f, 0.0f
		);
		dest.right = dest.left; dest.bottom = dest.top + light_tile_size;
		Color last_color = COLOR_WHITE;
		for(uint x = 0; x < x_tiles; ++x) {
			Vector2 p = vec2(
				light_screen.left + x * light_tile_size + light_tile_size / 2.0f,
				light_screen.top + y * light_tile_size + light_tile_size / 2.0f
			);	
			Color color = _sample_lights(p, lights_array, lights->size); 		
			if(color == last_color || x == 0) {
				// Append to old span	
				dest.right += light_tile_size;
			}
			else {
				// Draw old span
				video_draw_rect(light_tex, layer, NULL, &dest, last_color);
				// Start new one
				dest.left = dest.right;
				dest.right = dest.left + light_tile_size;
			}
			last_color = color;
		}
		// Draw last span
		video_draw_rect(light_tex, layer, NULL, &dest, last_color);
	}
}
