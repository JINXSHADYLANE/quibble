#ifndef GAME_H
#define GAME_H

#include "ego.hpp"
#include "arena.h"
using namespace ego;

class Game
{
public:
	Game(int w, int h);
	~Game();

	bool update();
	void draw();

private:
	Video* video;
	Input* input;

	Texture* background;
	Arena* arena;
	
};

#endif // GAME_H
