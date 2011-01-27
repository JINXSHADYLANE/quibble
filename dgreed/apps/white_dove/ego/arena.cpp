#include "arena.h"
#include "globals.h"
using namespace ego;

Arena::Arena(Video* video, Texture* tex)
	: video(video), tex(tex)
{
}

Arena::~Arena()
{
	delete tex;
	delete video;
}

void Arena::draw()
{
//	video->drawRect(tex, 0, RectF(0.0f, 0.0f, 100.0f, 100.0f), 
//		RectF(0.0f, 0.0f, 100.0f, 100.0f));
		
}

