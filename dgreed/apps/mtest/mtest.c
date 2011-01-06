#include <malka/malka.h>

int dgreed_main(int argc, const char** argv) {
	if(argc != 2) {
		printf("Provide lua file name\n");
		return 1;
	}
	return malka_run(argv[1]);	
}

