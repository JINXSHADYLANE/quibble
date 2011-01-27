// White Dove - quest game

#include "game.h"
#include "globals.h"

extern "C"
{
	int dgreed_main(int argc, const char** argv)
	{
		Game white_dove(SCREEN_WIDTH, SCREEN_HEIGHT);
		while (white_dove.update())
		{
			white_dove.draw();
		}
		return 0;
	}
}

