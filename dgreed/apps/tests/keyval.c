#include "keyval.h"

SETUP_ {
	keyval_init("test.db");
}

TEARDOWN_ {
	keyval_close();
}

TEST_(defaults) {
	ASSERT_(strcmp(keyval_get("unused1", "default"), "default") == 0);
	ASSERT_(42 == keyval_get_int("unused2", 42));
	ASSERT_(true == keyval_get_bool("unused3", true));
	ASSERT_(3.1415f == keyval_get_float("unused4", 3.1415f));
}

TEST_(set_get) {
	keyval_set("prog", "tests");
	keyval_set("lastrun", "now");
	keyval_set_int("foo", 13);
	keyval_set_bool("bar", false);
	keyval_set_float("baz", 18.0f);

	ASSERT_(strcmp(keyval_get("prog", ""), "tests") == 0);
	ASSERT_(strcmp(keyval_get("lastrun", ""), "now") == 0);
	ASSERT_(13 == keyval_get_int("foo", 0));
	ASSERT_(false == keyval_get_bool("bar", true));
	ASSERT_(18.0f == keyval_get_float("baz", 0.0f));
}

TEST_(stress) {
	for(uint i = 0; i < 350; ++i) {
		char key[16];
		sprintf(key, "key[%u]", i);
		keyval_set_int(key, (i+1)*3);
	}

	for(uint i = 0; i < 350; ++i) {
		char key[16];
		sprintf(key, "key[%u]", i);
		
		ASSERT_(keyval_get_int(key, 0) == ((i+1) * 3));
	}
}

TEST_(gc) {
	keyval_gc();

	for(uint i = 0; i < 350; ++i) {
		char key[16];
		sprintf(key, "key[%u]", i);
		
		ASSERT_(keyval_get_int(key, 0) == ((i+1) * 3));
	}

	ASSERT_(strcmp(keyval_get("prog", ""), "tests") == 0);
	ASSERT_(strcmp(keyval_get("lastrun", ""), "now") == 0);
	ASSERT_(13 == keyval_get_int("foo", 0));
	ASSERT_(false == keyval_get_bool("bar", true));
	ASSERT_(18.0f == keyval_get_float("baz", 0.0f));
}

TEST_(wipe) {
	keyval_set("n", "north");
	keyval_set("s", "south");
	keyval_set("e", "east");
	keyval_set("w", "west");

	ASSERT_(strcmp(keyval_get("n", ""), "north") == 0);
	ASSERT_(strcmp(keyval_get("s", ""), "south") == 0);
	ASSERT_(strcmp(keyval_get("e", ""), "east") == 0);
	ASSERT_(strcmp(keyval_get("w", ""), "west") == 0);

	keyval_wipe();

	ASSERT_(strcmp(keyval_get("n", ""), "") == 0);
	ASSERT_(strcmp(keyval_get("s", ""), "") == 0);
	ASSERT_(strcmp(keyval_get("e", ""), "") == 0);
	ASSERT_(strcmp(keyval_get("w", ""), "") == 0);
}
