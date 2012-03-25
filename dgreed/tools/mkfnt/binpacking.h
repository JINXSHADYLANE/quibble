#include <utils.h>

typedef struct {
	uint x, y;
} Pos;

// Returns null if packing is unsuccessfull, allocates array of offsets
// otherwise. Shuffles up widths and heights!
Pos* bpack(uint w, uint h, uint n, uint* widths, uint* heights);
