#include "achievements.h"

#include <utils.h>
#include <keyval.h>

#ifdef TARGET_IOS
#include <gamecenter.h>
#endif

void achievements_progress(const char* name, float progress) {
	char key_name[64];
	sprintf(key_name, "ac_%s", name);

	if(!keyval_get_bool(key_name, false)) {
	
		#ifdef TARGET_IOS
		if(gamecenter_is_active()) {
			gamecenter_report_achievement(name, progress * 100.0f);
			if(progress == 1.0f) {
				keyval_set_bool(key_name, true);
			}
		}
		#endif
	}
}

