#ifndef GAMECENTER_H
#define GAMECENTER_H

// iOS GameCenter API wrapper
// Does not work on non-iOS builds!

#ifndef TARGET_IOS
#error GameCenter works only on iOS
#endif

#include <utils.h>
#include <darray.h>

typedef enum {
	TS_DAY = 0,
	TS_WEEK,
	TS_ALL
} TimeScope;

typedef enum {
	PS_GLOBAL = 0,
	PS_FRIENDS
} PlayerScope;

typedef struct {
	const char* category;
	int64 context;
	int64 value;
	const char* player;
} GameCenterScore;

typedef struct {
	const char* identifier;
	const char* description;
	const char* title;
	float percent;
	bool completed;
} GameCenterAchievement;

typedef void (*GameCenterCallback)(DArray* data);

void gamecenter_init(void);
void gamecenter_close(void);
bool gamecenter_is_active(void);

void gamecenter_report_score(GameCenterScore* score);
void gamecenter_get_scores(uint start, uint len, 
		TimeScope ts, PlayerScope ps, GameCenterCallback callback);
void gamecenter_show_leaderboard(const char* category, TimeScope ts);

void gamecenter_report_achievement(const char* identifier, float percent);
void gamecenter_get_achievements(GameCenterCallback callback);
void gamecenter_show_achievements(void);

void gamecenter_app_suspend(void);

#endif
