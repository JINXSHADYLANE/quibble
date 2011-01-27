// White Dove

#include "game.h"
using namespace ego;

Game::Game(int w, int h)
{
	Log::init("white_dove.log");

	video = Video::init(w, h, "White Dove");
	input = Input::init();

	background = video->loadTexture("white_dove_assets/fonas.png");
	//arena = new Arena(video, background);
	
}

Game::~Game()
{
	//delete arena;
	delete background;

	delete input;
	delete video;
	Log::close();
}

bool Game::update()
{
	bool run = input->process();

	if (input->keyUp(Input::KEY_QUIT))
		run = false;
	
	return run;
}

void Game::draw()
{
//	arena->draw();
}

