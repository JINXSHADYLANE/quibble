#include "vfs.h"
#include "memory.h"

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#endif

static void* _map_file(const char* filename, size_t* size) {
#ifndef _WIN32
	struct stat sb;
	int fd = open(filename, O_RDONLY);
	fstat(fd, &sb);
	void* p = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	*size = sb.st_size;
	return p;
#else
	FileHandle f = file_open(filename);
	*size = file_size(f);
	void* p = MEM_ALLOC(*size);
	file_read(f, p, *size);
	file_close(f);
	return p;
#endif
}

static void _unmap_file(void* mapped_file, size_t size) {
#ifndef _WIN32
	munmap(mapped_file, size);
#else
	MEM_FREE(mapped_file);
#endif
}

void vfs_open(VfsBlob* blob, const char* container) {
	assert(blob);

	size_t size;
	blob->blob = _map_file(container, &size);
	const VfsHeader* hdr = blob->blob;
	if(hdr->magic != FOURCC('Q', 'B', 'F', 'S')) {
		LOG_ERROR("Invalid vfs file %s", container);
	}

	size_t names_size = hdr->offsets_pos - hdr->names_pos;
	size_t offsets_size = hdr->lengths_pos - hdr->offsets_pos;
	size_t lengths_size = offsets_size;

	blob->size = size;
	blob->n_files = hdr->n_files;
	blob->file_names = MEM_ALLOC(names_size + offsets_size + lengths_size);
	blob->file_offsets = (void*)blob->file_names + names_size;
	blob->file_lengths = (void*)blob->file_offsets + offsets_size;
	blob->next = NULL;

	memcpy(blob->file_names, blob->blob + hdr->names_pos, names_size);
	memcpy(blob->file_offsets, blob->blob + hdr->offsets_pos, offsets_size);
	memcpy(blob->file_lengths, blob->blob + hdr->lengths_pos, lengths_size);
}

void vfs_close(VfsBlob* blob) {
	MEM_FREE(blob->file_names);
	_unmap_file(blob->blob, blob->size);
}

const void* vfs_get(VfsBlob* blob, const char* filename, size_t* size) {
	assert(blob);
	assert(blob->size);
	assert(blob->n_files);
	assert(filename);

	// Linear search through filenames
	uint i = 0;
	const char* name = blob->file_names;
	while(i < blob->n_files && strcmp(name, filename) != 0) {
		while(*(name++) != '\0');
		i++;
	}

	// Not found, try next blob in chain
	if(i >= blob->n_files || strcmp(name, filename) != 0)
		if(blob->next)
			return vfs_get(blob->next, filename, size);

	// Return size and ptr to file
	if(size)
		*size = blob->file_lengths[i];
	return blob->blob + blob->file_offsets[i];
}

