#include <stdio.h>
#include <stdlib.h>
#include "vfs.h"

#define MAX_FILES 2048
#define MAX_FILENAME 128

int process_manifest(const char* manifest, const char* output) {
	uint32 n_files = 0;
	uint32 sizes[MAX_FILES] = {0};
	uint32 offsets[MAX_FILES] = {0};
	uint32 total_size = 0;
	uint32 names_strlen = 0;

	FILE* f = fopen(manifest, "r");
	char filename[MAX_FILENAME] = {0};

	// 1st pass - count files, sizes, amount of memory for filenames
	while(fscanf(f, "%s\n", filename) > 0) {
		FILE* h = fopen(filename, "rb");
		if(h) {
			names_strlen += strlen(filename) + 1;
			fseek(h, 0, SEEK_END);
			sizes[n_files++] = ftell(h);
			total_size += ftell(h);
			fclose(h);
		}
		else {
			fprintf(stderr, "warning: can't read file %s\n", filename);
			return -1;
		}
	}

	names_strlen = align_padding(names_strlen, 8);

	// Prep for outputting vfs blob
	assert(sizeof(VfsHeader) == 32);
	VfsHeader hdr = {
		.magic = FOURCC('Q', 'B', 'F', 'S'),
		.size = 32 + names_strlen + 8 * n_files + total_size,
		.n_files = n_files,
		.names_pos = 32,
		.offsets_pos = 32 + names_strlen,
		.lengths_pos = 32 + names_strlen + 4 * n_files,
		.data_pos = 32 + names_strlen + 8 * n_files,
		.padding = 0
	};

	printf("number of files: %d\n", n_files);
	printf("sum of sizes: %d\n", total_size);
	printf("total blob size: %d (%dk)\n", hdr.size, hdr.size / 1024);

	FILE* out = fopen(output, "wb");
	fwrite(&hdr, 1, sizeof(VfsHeader), out);

	// 2nd pass - write names
	fseek(f, 0, SEEK_SET);
	while(fscanf(f, "%s\n", filename) > 0) {
		fwrite(filename, 1, strlen(filename)+1, out);
	}

	// Write offsets & lengths
	offsets[0] = hdr.data_pos;
	for(uint i = 1; i < n_files; ++i) {
		offsets[i] = offsets[i-1] + sizes[i-1];
	}
	fwrite(offsets, 1, n_files * 4, out);
	fwrite(sizes, 1, n_files * 4, out);

	// 3rd pass - write data
	uint i = 0;
	fseek(f, 0, SEEK_SET);
	while(fscanf(f, "%s\n", filename) > 0) {
		FILE* h = fopen(filename, "rb");
		void* buffer = malloc(sizes[i]);
		fread(buffer, sizes[i], 1, h);
		fwrite(buffer, sizes[i], 1, out);
		fclose(h);
		free(buffer);
		i++;
	}

	assert(ftell(out) == hdr.size);

	printf("done\n");

	fclose(out);

	fclose(f);

	return 0;
}

int main(int argc, const char** argv) {
	if(argc != 3) {
		printf("mkvfs: packages files into vfs blobs\n");
		printf("usage: mkvfs manifest output\n");
		return 0;
	}

	return process_manifest(argv[1], argv[2]);
}

