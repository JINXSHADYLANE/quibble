#ifndef VFS_H
#define VFS_H

#include "utils.h"

// Very minimal read-only virtual filesystem for keeping lots of
// virtual files inside one real "blob" file. Originally made for
// use as Android APK expansion file format.
// Allows to change contents without modifying blobs by chaining
// additional "mod" blobs.

// 32 bytes
typedef struct {
	uint32 magic;
	uint32 size;
	uint32 n_files;
	uint32 names_pos;
	uint32 offsets_pos;
	uint32 lengths_pos;
	uint32 data_pos;
	uint32 padding;
} VfsHeader;

typedef struct VfsBlob {
	// mmaped file
	const void* blob;
	uint32 size;

	// file info table
	uint32 n_files;
	const char* file_names;
	const uint32* file_offsets;
	const uint32* file_lengths;

	// next blob in chain
	struct VfsBlob* next;
} VfsBlob;

void vfs_open(VfsBlob* blob, const char* container);
void vfs_close(VfsBlob* blob);

const void* vfs_get(VfsBlob* blob, const char* filename, size_t* size);

#endif
