#ifndef ARENA_H
#define ARENA_H

#include "ego.hpp"
using namespace ego;

class Arena
{
public:
	Arena(Video* video, Texture* tex);
	~Arena();

	void draw();

private:
	Texture* tex;
	Video* video;
};

#endif // ARENA_H
