#ifndef MENUS_H
#define MENUS_H

typedef enum {
	MENU_MAIN,
	MENU_SETTINGS,
	MENU_CHAPTER,
	MENU_ARENA,
	MENU_PAUSE,
	MENU_GAMEOVER,
	MENU_GAME
} MenuState;	

extern MenuState menu_state;
extern MenuState menu_transition;

void menus_init(void);
void menus_close(void);
void menus_update(void);
void menus_render(void);

#endif