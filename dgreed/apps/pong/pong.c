#include <system.h>
#include <gfx_utils.h>

#define ASSETS_DIR "pong_assets/"
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

// Resources
TexHandle tex1;
SoundHandle bump;
SoundHandle music;

// Tweakables
RectF paddle_src = {0.0f, 0.0f, 16.0f, 64.0f};
RectF ball_src = {16.0f, 0.0f, 36.0f, 20.0f};
float ball_radius = 9.0f;
float paddle_width = 16.0f;
float paddle_height = 64.0f;
float paddle_acceleration = 1000.0f;
float paddle_damping = 0.95f;
float paddle_max_speed = 200.0f;
float ball_speed = 170.0f;
float ball_max_delta_angle = 30.0f;

// Game state
float paddle1_v, paddle2_v;
Vector2 paddle1_p, paddle2_p;
Vector2 ball_p, ball_v;
uint score1, score2;
uint dir = 0; // 0 - ball wil go towards player1

void resources_load(void) {
	tex1 = tex_load(ASSETS_DIR "graphics.png");
	bump = sound_load_sample(ASSETS_DIR "bump.wav");
	music = sound_load_stream(ASSETS_DIR "ggp.ogg");
}

void resources_free(void) {
	tex_free(tex1);
	sound_free(bump);
	sound_free(music);
}

void game_reset(void) {
	paddle1_v = paddle2_v = 0.0f;
	paddle1_p = vec2(paddle_width / 2.0f + 2.0f, SCREEN_HEIGHT / 2.0f);
	paddle2_p = vec2(SCREEN_WIDTH - paddle1_p.x, paddle1_p.y);

	ball_p = vec2(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
	float ball_angle = dir == 0 ? 180.0f * DEG_TO_RAD : 0.0f;
	ball_v = vec2(cos(ball_angle) * ball_speed, sin(ball_angle) * ball_speed);
}

void game_update(void) {
	float dt = time_delta() / 1000.0f;

	// Move ball
	ball_p = vec2_add(ball_p, vec2_scale(ball_v, dt));
	// Left side
	if(ball_p.x - ball_radius <= paddle1_p.x + paddle_width / 2.0f) {
		float dy = ball_p.y - paddle1_p.y;
		float norm_dy = 2.0f*dy / paddle_height;
		// Bump
		RectF paddle_rect = {
			paddle1_p.x - paddle_width/2.0f, paddle1_p.y - paddle_height/2.0f,
			paddle1_p.x + paddle_width/2.0f, paddle1_p.y + paddle_height/2.0f
		};	
		if(rectf_circle_collision(&paddle_rect, &ball_p, ball_radius)
			&& ball_v.x < 0.0f) {
			ball_v.x = -ball_v.x;
			ball_v = vec2_rotate(ball_v, norm_dy * ball_max_delta_angle);
			sound_play(bump);
		}
		// Miss
		if(ball_p.x <= ball_radius) {
			score2++;
			dir = 0;
			game_reset();
			//sound_play(miss);
		}
	}
	// Right side
	if(ball_p.x + ball_radius >= paddle2_p.x - paddle_width / 2.0f) {
		float dy = ball_p.y - paddle2_p.y;
		float norm_dy = 2.0f*dy / paddle_height;
		// Bump
		RectF paddle_rect = {
			paddle2_p.x - paddle_width/2.0f, paddle2_p.y - paddle_height/2.0f,
			paddle2_p.x + paddle_width/2.0f, paddle2_p.y + paddle_height/2.0f
		};	
		if(rectf_circle_collision(&paddle_rect, &ball_p, ball_radius)
			&& ball_v.x > 0.0f) {
			ball_v.x = -ball_v.x;
			ball_v = vec2_rotate(ball_v, -norm_dy * ball_max_delta_angle);
			sound_play(bump);
		}
		// Miss
		if(ball_p.x >= SCREEN_WIDTH - ball_radius) {
			score1++;
			dir = 1;
			game_reset();
			//sound_play(miss);
		}
	}
	// Top
	if(ball_p.y <= ball_radius && ball_v.y < 0.0f) {
		ball_v.y = -ball_v.y;
		//sound_play(bump2);
	}	
	// Bottom
	if(ball_p.y >= SCREEN_HEIGHT - ball_radius && ball_v.y > 0.0f) {
		ball_v.y = -ball_v.y;
		//sound_play(bump2);
	}

	// Move paddles
	paddle1_v -= key_pressed(KEY_UP) ? paddle_acceleration * dt : 0.0f;
	paddle1_v += key_pressed(KEY_DOWN) ? paddle_acceleration * dt : 0.0f;
	paddle1_v = clamp(-paddle_max_speed, paddle_max_speed, paddle1_v);
	paddle1_v *= paddle_damping;
	
	paddle2_v -= key_pressed(KEY_UP) ? paddle_acceleration * dt : 0.0f;
	paddle2_v += key_pressed(KEY_DOWN) ? paddle_acceleration * dt : 0.0f;
	paddle2_v = clamp(-paddle_max_speed, paddle_max_speed, paddle1_v);
	paddle2_v *= paddle_damping;

	paddle1_p.y += paddle1_v * dt;
	paddle1_p.y = clamp(paddle_height/2.0f, SCREEN_HEIGHT - paddle_height/2.0f,
		paddle1_p.y);
	
	paddle2_p.y += paddle2_v * dt;
	paddle2_p.y = clamp(paddle_height/2.0f, SCREEN_HEIGHT - paddle_height/2.0f,
		paddle2_p.y);
}

void game_draw(void) {
	gfx_draw_textured_rect(tex1, 0, &paddle_src, &paddle1_p, 0.0f, 1.0f,
		COLOR_WHITE);
	gfx_draw_textured_rect(tex1, 0, &paddle_src, &paddle2_p, 0.0f, 1.0f,
		COLOR_WHITE);
	gfx_draw_textured_rect(tex1, 0, &ball_src, &ball_p, 0.0f, 1.0f,
		COLOR_WHITE);
}
	

int dgreed_main(int argc, const char** argv) {
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong");
	sound_init();

	resources_load();
	game_reset();

	sound_play(music);
	while(system_update()) {
		game_update();
		game_draw();
		video_present();
		sound_update();
	}

	resources_free();

	sound_close();
	video_close();

	return 0;
}

