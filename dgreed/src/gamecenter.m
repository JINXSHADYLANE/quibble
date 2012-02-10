#include "gamecenter.h"

#include "memory.h"

#import "DGreedAppDelegate.h"
#import <GameKit/GameKit.h>

#define QUEUE_STR_LEN 32
#define QUEUE_FILE "gcqueues.db"

typedef struct {
    char category[QUEUE_STR_LEN];
	int64 context;
	int64 value;
} QueuedScore;

typedef struct {
    char identifier[QUEUE_STR_LEN];
    float progress;
} QueuedAchievement;

static DGreedAppDelegate* app_delegate = nil;
static bool gamecenter_active = false;
static DArray score_queue;
static DArray achievement_queue;
static int live_requests = 0;

static NSMutableDictionary* achievement_dict;

void _set_gamecenter_app_delegate(DGreedAppDelegate* _app_delegate) {
    app_delegate = _app_delegate;
}

static bool is_gamecenter_supported(void) {
    BOOL found_local_player_class = (NSClassFromString(@"GKLocalPlayer")) != nil;
    
    NSString *req_sys_ver = @"4.1";
    NSString *curr_sys_ver = [[UIDevice currentDevice] systemVersion];
    BOOL os_version_supported = ([curr_sys_ver compare:req_sys_ver options:NSNumericSearch] != NSOrderedAscending);
    
    return os_version_supported && found_local_player_class;
}

static void _load_queues(void) {
    if(file_exists(QUEUE_FILE)) {
        FileHandle f = file_open(QUEUE_FILE);
        
        uint32 magic = file_read_uint32(f);
        if(magic != FOURCC('G', 'S', 'Q', '0'))
            LOG_ERROR("Unable to load gamecenter queues");
        
        file_read(f, &score_queue, sizeof(score_queue));
        file_read(f, &achievement_queue, sizeof(achievement_queue));

        size_t sizeof_score = score_queue.item_size;
        size_t sizeof_achievement = achievement_queue.item_size;
        
        if(sizeof(QueuedScore) != sizeof_score 
           || sizeof(QueuedAchievement) != sizeof_achievement) {
            LOG_ERROR("gamecenter queues data size mismatch");
        }
        
        size_t score_bytes = sizeof_score * score_queue.reserved;
        size_t achievement_bytes = sizeof_achievement * achievement_queue.reserved;
        score_queue.data = MEM_ALLOC(score_bytes);
        achievement_queue.data = MEM_ALLOC(achievement_bytes);
        file_read(f, score_queue.data, score_bytes);
        file_read(f, achievement_queue.data, achievement_bytes);
        
        file_close(f);
    }
    else {
        score_queue = darray_create(sizeof(QueuedScore), 0);
        achievement_queue = darray_create(sizeof(QueuedAchievement), 0);
    }
}

static void _flush_queues(void) {
    FileHandle f = file_create(QUEUE_FILE);
    
    file_write_uint32(f, FOURCC('G', 'S', 'Q', '0'));
    file_write(f, &score_queue, sizeof(score_queue));
    file_write(f, &achievement_queue, sizeof(achievement_queue));
    file_write(f, score_queue.data, sizeof(QueuedScore) * score_queue.reserved);
    file_write(f, achievement_queue.data, sizeof(QueuedAchievement) * achievement_queue.reserved);
    
    file_close(f);
}

static void _close_queues(void) {
    darray_free(&score_queue);
    darray_free(&achievement_queue);
}

static void _report_score(const char* category, int64 score, int64 context) {
    NSString* ns_category = [NSString stringWithUTF8String:category];
    //GKScore* score_reporter = [[[GKScore alloc] initWithCategory:ns_category] autorelease];
    GKScore* score_reporter = [[GKScore alloc] initWithCategory:ns_category];
    score_reporter.value = score;
    score_reporter.context = context;
    [score_reporter reportScoreWithCompletionHandler:^(NSError *error) {
        if (error != nil) {
            // Fail
            LOG_WARNING("Reporting score failed");
        }
        else {
            // Success
            // Find score queue item and remove it
            LOG_INFO("Score reported to game center successfully: %lld", score);
            QueuedScore* scores = DARRAY_DATA_PTR(score_queue, QueuedScore);
            for(uint i = 0; i < score_queue.size; ++i) {
                if(scores[i].value == score && scores[i].context == context) {
                    if(strcmp(scores[i].category, category) == 0) {
                        darray_remove_fast(&score_queue, i);
                        break;
                    }
                }
            }
        }
        live_requests--;
    }];
    live_requests++;
}

static void _process_score_queue(void) {
    if(score_queue.size && live_requests == 0) {
        QueuedScore* scores = DARRAY_DATA_PTR(score_queue, QueuedScore);
        for(uint i = 0; i < score_queue.size; ++i) {
            _report_score(scores[i].category, scores[i].value, scores[i].context);
        }
    }
}

static void _get_achievements(void) {
    [GKAchievement loadAchievementsWithCompletionHandler:^(NSArray *achievements, NSError *error) {
        if(error == nil) {
            for(GKAchievement* achievement in achievements)
                [achievement_dict setObject: achievement forKey: achievement.identifier];
        }
        live_requests--;
    }];
    live_requests++;
}

static GKAchievement* _get_achievement(const char* identifier) {
    NSString* ns_ident = [NSString stringWithUTF8String:identifier];
    GKAchievement *achievement = [achievement_dict objectForKey:ns_ident];
    if (achievement == nil) {
        achievement = [[[GKAchievement alloc] initWithIdentifier:ns_ident] autorelease];
        [achievement_dict setObject:achievement forKey:achievement.identifier];
    }
    return [[achievement retain] autorelease];
}

static void _report_achievement(const char* identifier, float progress) {
    GKAchievement *achievement = _get_achievement(identifier);
    if(achievement) {
        achievement.percentComplete = progress;
        [achievement reportAchievementWithCompletionHandler:^(NSError *error) {
            if (error != nil) {
                // Fail
                LOG_WARNING("Reporting achievement failed");
            }
            else {
                // Success
                // Find achievement queue item and remove it
                QueuedAchievement* achievements = DARRAY_DATA_PTR(achievement_queue, QueuedAchievement);
                for(uint i = 0; i < achievement_queue.size; ++i) {
                    if(achievements[i].progress == progress 
                       && strcmp(achievements[i].identifier, identifier) == 0) {
                        darray_remove_fast(&achievement_queue, i);
                        break;
                    }
                }
            }
            live_requests--;
        }];
        live_requests++;
    }
}

static void _process_achievement_queue(void) {
    if(achievement_queue.size && live_requests == 0) {
        QueuedAchievement* achievements = DARRAY_DATA_PTR(achievement_queue, QueuedAchievement);
        for(uint i = 0; i < achievement_queue.size; ++i) {
            _report_achievement(achievements[i].identifier, achievements[i].progress);
        }
    }
}

void gamecenter_init(void) {
    assert(!gamecenter_active);
    
    if(is_gamecenter_supported()) {
        // Load request queues
        _load_queues();
        
        achievement_dict = [[NSMutableDictionary alloc] init];
        
        GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];
        [localPlayer authenticateWithCompletionHandler:^(NSError *error) {
            if (localPlayer.isAuthenticated) {
                LOG_INFO("GameCenter authenticated");
                gamecenter_active = true;
                
                _get_achievements();
                _process_score_queue();
                _process_achievement_queue();
            }
        }];
    }
}

void gamecenter_close(void) {
    if(gamecenter_active) {
        [achievement_dict release];
        _close_queues();
    }
}

bool gamecenter_is_active(void) {
    return gamecenter_active;
}

void gamecenter_report_score(GameCenterScore* score) {
    assert(strlen(score->category) < QUEUE_STR_LEN);
    QueuedScore qscore;
    strcpy(qscore.category, score->category);
    qscore.context = score->context;
    qscore.value = score->value;
    
    darray_append(&score_queue, &qscore);
    
    _process_score_queue();
}

static GKLeaderboardPlayerScope _dgreed_to_gk_player_scope(PlayerScope ps) {
    switch(ps) {
        case PS_GLOBAL:
            return GKLeaderboardPlayerScopeGlobal;
        case PS_FRIENDS:
            return GKLeaderboardPlayerScopeFriendsOnly;
    }
    return GKLeaderboardPlayerScopeGlobal;
}

static GKLeaderboardTimeScope _dgreed_to_gk_time_scope(TimeScope ts) {
    switch(ts) {
        case TS_DAY:
            return GKLeaderboardTimeScopeToday;
        case TS_WEEK:
            return GKLeaderboardTimeScopeWeek;
        case TS_ALL:
            return GKLeaderboardTimeScopeAllTime;
    }
    return GKLeaderboardTimeScopeAllTime;
}

static GameCenterScore _gk_to_dgreed_score(GKScore* score) {
    GameCenterScore s = {
        .category = [score.category UTF8String],
        .context = score.context,
        .value = score.value,
        .player = [score.playerID UTF8String]
    };
    
    return s;
}

static GameCenterAchievement _gk_to_dgreed_achievement(GKAchievement* achievement, GKAchievementDescription* description) {
    NSString* desc = achievement.isCompleted ? description.achievedDescription : description.unachievedDescription;
    GameCenterAchievement a = {
        .identifier = [achievement.identifier UTF8String],
        .description = [desc UTF8String],
        .title = [description.title UTF8String],
        .percent = achievement.percentComplete,
        .completed = achievement.completed
    };
    
    return a;
}

void gamecenter_get_scores(uint start, uint len, TimeScope ts, 
                           PlayerScope ps, GameCenterCallback callback) {
    
    GKLeaderboard *leaderboard_request = [[[GKLeaderboard alloc] init] autorelease];
    if(leaderboard_request != nil) {
        leaderboard_request.playerScope = _dgreed_to_gk_player_scope(ps);
        leaderboard_request.timeScope = _dgreed_to_gk_time_scope(ts);
        leaderboard_request.range = NSMakeRange(start,start+len);
        [leaderboard_request loadScoresWithCompletionHandler: ^(NSArray *scores, NSError *error) {
            if(error != nil) {
                // error
                (*callback)(NULL);
            }
            if(scores != nil) {
                DArray cb_data = darray_create(sizeof(GameCenterScore), [scores count]);
                for(GKScore* score in scores) {
                    GameCenterScore s = _gk_to_dgreed_score(score);
                    darray_append(&cb_data, &s);
                }
                
                // Invoke callback
                (*callback)(&cb_data);
                
                darray_free(&cb_data);
            }
        }];
    }
}

void gamecenter_show_leaderboard(const char* category, TimeScope ts) {
    GKLeaderboardViewController *leaderboardController = [[GKLeaderboardViewController alloc] init];
    if(leaderboardController != nil) {
        leaderboardController.category = [NSString stringWithUTF8String:category];
        leaderboardController.timeScope = _dgreed_to_gk_time_scope(ts);
        leaderboardController.leaderboardDelegate = app_delegate;
        [app_delegate.window bringSubviewToFront:app_delegate.controller.view];
        [app_delegate.controller presentModalViewController: leaderboardController animated: YES];
    }
    [leaderboardController release];
}

void gamecenter_report_achievement(const char* identifier, float percent) {
    assert(strlen(identifier) < QUEUE_STR_LEN);
    QueuedAchievement qachievement;
    strcpy(qachievement.identifier, identifier);
    qachievement.progress = percent;
    
    darray_append(&achievement_queue, &qachievement);
    
    _process_achievement_queue();
}

void gamecenter_get_achievements(GameCenterCallback callback) {
    [GKAchievementDescription loadAchievementDescriptionsWithCompletionHandler:
     ^(NSArray *descriptions, NSError *error) {
         if(error != nil) {
             // error
             (*callback)(NULL);
         }
         if(descriptions != nil) {
             DArray cb_data = darray_create(sizeof(GameCenterAchievement), [descriptions count]);
             for(GKAchievementDescription* desc in descriptions) {
                 GKAchievement* achievement = _get_achievement([desc.identifier UTF8String]);
                 GameCenterAchievement a = _gk_to_dgreed_achievement(achievement, desc);
                 darray_append(&cb_data, &a);
             }
             
             // Invoke callback
             (*callback)(&cb_data);
             
             darray_free(&cb_data);
         }
    }];
}

void gamecenter_show_achievements(void) {
    GKAchievementViewController *achievements = [[GKAchievementViewController alloc] init];
    if (achievements != nil) {
        achievements.achievementDelegate = app_delegate;
        [app_delegate.window bringSubviewToFront:app_delegate.controller.view];
        [app_delegate.controller presentModalViewController: achievements animated: YES];
    }
    [achievements release];
}

void gamecenter_app_suspend(void) {
    if(gamecenter_active)
        _flush_queues();
}
