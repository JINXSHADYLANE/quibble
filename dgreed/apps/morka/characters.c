#include "characters.h"

CharacterDefaultParams default_characters[character_count] = {

	{
	"Dash",			// Character name
	"rabbit_icon",	// Shop icon sprite
	"bubble_obj",	// Shield in-game sprite
	"rabbit",		// Character in-game sprite
	"rabbit",		// Animation
	"Runs faster",	// Shop description string
	0,				// Purchase cost in coins
	550.0f,			// Running speed
	100.0f,			// Jump horizontal speed
	400.0f,			// Jump vertical speed
	0.0f,			// Sprite horizontal offset from hitbox
	{37.0f,0.0f},	// Shield offset
	{1.0f,1.0f}		// Shield scale
	},

	{
	"Skippy",
	"squirrel_icon",
	"bubble_obj",	
	"squirrel",
	"squirrel",
	"Jumps higher",
	200,	
	500.0f,
	120.0f,
	420.0f,
	-30.0f,
	{6.0f,20.0f},
	{1.25f,1.1f}				
	},

	{
	"Stinky",
	"skunk_icon",
	"bubble_obj",	
	"skunk",
	"squirrel",
	"Slows everyone",
	400,	
	510.0f,
	110.0f,
	410.0f,
	-30.0f,
	{6.0f,20.0f},
	{1.25f,1.1f}		
	},

	{
	"Richman",
	"hare_icon",
	"bubble_obj",	
	"golden_rabbit",
	"rabbit",
	"Attracts coins",
	800,	
	490.0f,
	110.0f,
	420.0f,
	0.0f,
	{37.0f,0.0f},
	{1.0f,1.0f}					
	}			
};