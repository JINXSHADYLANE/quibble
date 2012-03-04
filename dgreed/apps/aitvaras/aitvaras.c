#include <system.h>
#include <http.h>

#define LOBBY_ADDR "http://localhost"

void enlist_cb(HttpResponseCode retcode, const char* data, size_t len,
		const char* header, size_t header_len) {
	if(retcode == 200) {
		printf("%s\n", data);
	}
	else {
		printf("enlist failed\n");
	}
}

void remove_cb(HttpResponseCode retcode, const char* data, size_t len, 
		const char* header, size_t header_len) {
	if(retcode == 200) {
		printf("Removed!\n");
	}
	else {
		printf("remove failed\n");
	}
}

bool dgreed_init(void) {
	rand_init(413);

	video_init(1024, 768, "Aitvaras");
	sound_init();

	http_post(LOBBY_ADDR "/enlist", false, "", NULL, enlist_cb);

	return true;
}

bool dgreed_update(void) {
	sound_update();

	if(key_up(KEY_QUIT)) {
		http_post(LOBBY_ADDR "/remove", false, "", NULL, remove_cb);
		return false;
	}

	return true;
}

bool dgreed_render(void) {
	
	// Render
	
	video_present();

	return true;
}

void dgreed_close(void) {

	sound_close();
	video_close();
}

#ifndef TARGET_IOS
int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);
	
	dgreed_init();
	while(system_update()) {
		if(!dgreed_update())
			break;
		if(!dgreed_render())
			break;
	}	
	dgreed_close();

	return 0;
}	
#endif
