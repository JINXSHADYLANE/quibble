// Gyvis - existential snake

#include "ego.hpp"
using namespace ego;

#include <list>
#include <cmath>
#include <ctime>

namespace globals {
	const float title_length = 5500.0f;
	const float fadeout_length = 7500.0f;
	const float move_interval = 128.0f;
	const int start_length = 3;
	const string asset_dir = "gyvis_assets/";

	const RectF head_src(0.0f, 0.0f, 24.0f, 24.0f);
	const RectF body_src(24.0f, 0.0f, 24.0f + 24.0f, 24.0f);
	const RectF tail_src(48.0f, 0.0f, 48.0f + 24.0f, 24.0f);
	const RectF bend_src(72.0f, 0.0f, 72.0f + 24.0f, 24.0f);
	const RectF food_src(96.0f, 0.0f, 96.0f + 24.0f, 24.0f);
	const RectF void_src(1.0f, 25.0f, 23.0f, 25.0f + 22.0f);
	const RectF text_src(0.0f, 54.0f, 128.0f, 54.0f + 42.0f);

	Video* video;
	Audio* audio;
	Input* input;
}

using namespace globals;

struct Pos {
	int x, y;

	bool operator ==(const Pos& p) {
		return x == p.x && y == p.y;
	}	
};	

enum Dir {
	dir_right = 0,
	dir_down,
	dir_left,
	dir_up,
	dir_invalid
};

class Arena {
public:
	Arena(int w, int h, Texture* tex)
		: tex(tex) {
		screen_width = float(w);
		screen_height = float(h);
		tile_width = tile_height = 24;	
		x_tiles = w / tile_width;
		y_tiles = h / tile_height;
		width = x_tiles * tile_width;
		height = y_tiles * tile_width;
		x = (w - width) / 2;
		y = (h - height) / 2;
	}

	int getWidth() {
		return x_tiles;
	}

	int getHeight() {
		return y_tiles;
	}

	Pos dropFood() {
		food.x = Random::_int(0, width / tile_width);
		food.y = Random::_int(0, height / tile_height);
		return food;
	};

	Pos getFood() {
		return food;
	}

	RectF tileRect(int x, int y) {
		float fx = float(this->x), fy = float(this->y);
		float ftw = float(tile_width), fth = float(tile_height);
		RectF result(fx + float(x) * ftw, fy + float(y) * fth, 0.0f, 0.0f);
		result.right = result.left + ftw;
		result.bottom = result.top + fth;
		return result;
	}

	void draw() {
		// Empty background
		video->drawRect(tex, 0, void_src, 
			RectF(0.0f, 0.0f, screen_width, screen_height)); 

		// Arena bounds
		float fx = float(x), fy = float(y);
		float fw = float(width), fh = float(height);
		if(x > 0) {
			video->drawLine(1, Vector2(fx, fy), Vector2(fx, fy+fh),
				Color::black);
			video->drawLine(1, Vector2(fx+fw, fy), Vector2(fx+fw, fy+fh),
				Color::black);
		}
		if(y > 0) {
			video->drawLine(1, Vector2(fx, fy), Vector2(fx+fw, fy),
				Color::black);
			video->drawLine(1, Vector2(fx, fy+fh), Vector2(fx+fw, fy+fh),
				Color::black);
		}

		// Food
		video->drawRect(tex, 2, food_src, tileRect(food.x, food.y));
	}

	void drawFadeout(float t) {
		video->drawRect(tex, 3, void_src,
			RectF(0.0f, 0.0f, screen_width, screen_height), 0.0f,
			Color::lerp(Color(0.0f, 0.0f, 0.0f, 0.0f), Color::black, 
			std::min(t*1.7f, 1.0f)));
	}

private:
	Texture* tex;

	float screen_width, screen_height; 
	int x_tiles, y_tiles;
	int x, y, width, height;
	int tile_width, tile_height;
	
	Pos food;
};

class Snake {
public:
	Snake(Arena* arena, Texture* tex, Sound* food_snd)
		: arena(arena), tex(tex), food_snd(food_snd),
		  dir(dir_right), new_dir(dir_right)  {
		reset();
	}	

	void reset() {
		if(!body.empty())
			body.clear();
		dir = new_dir = dir_right;
		Pos p;
		p.x = arena->getWidth() / 2;
		p.y = arena->getHeight() / 2;
		p.x -= start_length / 2;
		for(int i = 0; i < start_length; ++i) {
			body.push_back(p);
			p.x++;
		}
		while(isOccupied(arena->dropFood()));
	}

	bool isOccupied(Pos p) {
		std::list<Pos>::iterator itr;
		for(itr = body.begin(); itr != body.end(); ++itr) {
			if(itr->x == p.x && itr->y == p.y)
				return true;
		}
		return false;
	}	

	float setDir(Dir d) {
		// Prevent setting opposite dir
		if(std::abs(dir - d) == 2)
			return 0.0f;

		if(new_dir != d) {
			// Make next move immediately after changing dir,
			// this make game feel more responsive.
			new_dir = d;
			move();
			return Time::ms();
		}	
		return 0.0f;
	}	

	bool move() {
		Pos head = *(--body.end());
		switch(dir) {
			case dir_right:
				head.x++;
				break;
			case dir_down:
				head.y++;
				break;
			case dir_left:
				head.x--;
				break;
			case dir_up:
				head.y--;
				break;
			default:
				break;
		}
		int arena_w = arena->getWidth();
		int arena_h = arena->getHeight();
		head.x = (head.x + arena_w) % arena_w;
		head.y = (head.y + arena_h) % arena_h;

		// Grow, drop next food if food is eaten
		Pos food = arena->getFood();
		bool grow = head == food || isOccupied(food);
		if(!grow)
			body.pop_front();
		else {
			food_snd->play();
			while(isOccupied(arena->dropFood()));
		}	

		if(isOccupied(head))
			return false;

		body.push_back(head);

		dir = new_dir;
		return true;
	}

	void draw() {
		Dir last_dir = dir_invalid;
		std::list<Pos>::iterator itr, next_itr;
		for(itr = body.begin(); itr != body.end(); ++itr) {
			Pos cur = *itr;
			Pos next = ++(next_itr = itr) != body.end() ? *next_itr : cur;

			// Determine current direction, taking wrap-around into account
			Dir dir = last_dir;
			if(cur.x != next.x) {
				dir = cur.x > next.x ? dir_left : dir_right;
				if(std::abs(cur.x - next.x) > 1)
					dir = (dir == dir_left) ? dir_right : dir_left;
			}		
			if(cur.y != next.y) {
				dir = cur.y > next.y ? dir_up : dir_down;
				if(std::abs(cur.y - next.y) > 1)
					dir = (dir == dir_up) ? dir_down : dir_up;
			}		

			RectF dest = arena->tileRect(cur.x, cur.y);
			float rot = (float(dir+2)) * pi / 2.0f;

			// Rotation of bend tile is trickier
			float bend_rot = (dir - last_dir == 1 || dir - last_dir == -3) ? 
				rot + pi / 2.0f : rot;

			// Tail
			if(last_dir == dir_invalid)
				video->drawRect(tex, 2, tail_src, dest, rot);
			else if(last_dir == dir) {
				// Body
				if(cur.x != next.x || cur.y != next.y)
					video->drawRect(tex, 2, body_src, dest, rot);
				// Head
				else
					video->drawRect(tex, 2, head_src, dest, rot);
			}		
			// Bend
			else 
				video->drawRect(tex, 2, bend_src, dest, bend_rot);


			last_dir = dir;
		}
	}

private:
	Arena* arena;
	Texture* tex;
	Sound* food_snd;
	
	Dir dir, new_dir;
	std::list<Pos> body;
};

class Game {
public:
	Game(int w, int h)
		: last_move(0.0f), game_over(false) {
		Random::init(time(NULL));
		Log::init("gyvis.log");
		video = Video::init(w, h, "gyvis");	
		audio = Audio::init();
		input = Input::init();

		atlas = video->loadTexture(asset_dir + "snake128.png");
		food_snd = audio->loadSample(asset_dir + "beat.wav");
		game_over_snd = audio->loadSample(asset_dir + "death.wav");

		arena = new Arena(w, h, atlas);
		arena->dropFood();
		snake = new Snake(arena, atlas, food_snd);
		
		music = audio->loadStream(asset_dir + "Feather Waltz.ogg");
		music->play();
	}

	~Game() {
		delete snake;
		delete arena;

		delete atlas;
		delete food_snd;
		delete game_over_snd;
		delete music;

		delete input;
		delete audio;
		delete video;
		Log::close();
	}

	bool update() {
		static bool first_time = true;
		if(first_time) {
			first_time = false;
			start_t = Time::ms();
		}

		bool cont = input->process();
		audio->update();

		if(!game_over) {
			float move_t = 0.0f;
			if(input->keyDown(Input::KEY_UP))
				move_t = snake->setDir(dir_up);
			if(input->keyDown(Input::KEY_DOWN))
				move_t = snake->setDir(dir_down);
			if(input->keyDown(Input::KEY_LEFT))
				move_t = snake->setDir(dir_left);
			if(input->keyDown(Input::KEY_RIGHT))
				move_t = snake->setDir(dir_right);
			
			if(move_t != 0.0f)
				last_move = move_t;

			if(Time::ms() > last_move + move_interval) {
				last_move = Time::ms();
				game_over = !snake->move();
			}

			if(game_over)
				game_over_snd->play();
		}
		else {
			if(Time::ms() > last_move + fadeout_length) {
				snake->reset();
				game_over = false;
			}
		}

		if(input->keyUp(Input::KEY_QUIT))
			cont = false;

		return cont;
	}

	void draw() {
		arena->draw();
		snake->draw();

		if(game_over) {
			arena->drawFadeout((Time::ms() - last_move) / fadeout_length);
		}

		// Show title text with nice animation
		if(Time::ms() < start_t + title_length) {
			float t = (Time::ms() - start_t) / title_length;
			float ct = t > 0.5f ? 1.0f - (t - 0.5f) * 2.0f : t * 2.0f;
			video->drawRect(atlas, 3, text_src, 
				Vector2(lerp(320.0f, 330.0f, t), 16.0f), 0.0f,
				Color::lerp(Color(1.0f, 1.0f, 1.0f, 0.0f), Color::white, ct));
		}

		video->present();
	}

private:
	Sound* music;
	Texture* atlas;
	Sound* food_snd;
	Sound* game_over_snd;

	Arena* arena;
	Snake* snake;

	float last_move, start_t;

	bool game_over;
};

extern "C" {
	int dgreed_main(int argc, const char** argv) {
		Game game(480, 264);
		while(game.update()) {
			game.draw();
		}	
		return 0;
	}
}

