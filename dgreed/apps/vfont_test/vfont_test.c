#include <system.h>
#include <locale.h>
#include "vfont.h"
#include "utils.h"
int dgreed_main(int argc, const char** argv) {
	const char* string = "VAV T. ąčęėĮŠų 覉 硖";
	const char* string1 = "AVASDGHJg T.";
	const char* font_name2 = "vfont_test_assets/DejaVuSerif.ttf";
	const char* font_name1 = "vfont_test_assets/DejaVuSans.ttf";
	Vector2 vector;
	vector.x = 0;
	vector.y = 10;

	video_init(800, 600, "vfont_test");
	vfont_init();
 
	int i=1;
	int size = 15;
	while(system_update()) {
	   vfont_select(font_name2, 39);
	   vfont_draw(string, 0, vector, COLOR_RGBA(255,0,0,255));
	    vector.y+=45;
	   vfont_select(font_name1,size);
	   vfont_draw(string1, 0, vector, COLOR_WHITE);
	    vector.y=10;
	    size+=i;
	    if(size > 70) i*=-1;
	    if(size < 4) i*=-1;
	   video_present();
	}
	vfont_close();
	video_close();
	return 0;
}

