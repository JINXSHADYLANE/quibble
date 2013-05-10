#include <vfs.h>

TEST_(open) {
	VfsBlob blob;
	// Test with data file constructed for this purpose
	vfs_open(&blob, "test.vfs");

	ASSERT_(blob.size == 120);
	ASSERT_(blob.n_files == 4);
	ASSERT_(blob.file_lengths[0] == 12);
	ASSERT_(blob.file_lengths[1] == 2);
	ASSERT_(blob.file_lengths[2] == 4);
	ASSERT_(blob.file_lengths[3] == 14);
	ASSERT_(blob.next == NULL);

	vfs_close(&blob);
}

TEST_(get) {
	VfsBlob blob;
	// Test with data file constructed for this purpose
	vfs_open(&blob, "test.vfs");

	size_t s;

	// a.txt
	const byte* a = vfs_get(&blob, "a.txt", &s);
	ASSERT_(s == 12);
	for(uint i = 0; i < s-1; ++i)
		ASSERT_(a[i] == '1');

	// b.txt
	const byte* b = vfs_get(&blob, "b.txt", &s);
	ASSERT_(s == 2);
	ASSERT_(b[0] == '2');

	// Non-existing
	ASSERT_(vfs_get(&blob, "e.txt", &s) == NULL);
	ASSERT_(s == 2);

	// c.txt
	const byte* c = vfs_get(&blob, "c.txt", &s);
	ASSERT_(s == 4);
	for(uint i = 0; i < s-1; ++i)
		ASSERT_(c[i] == '3');

	// d.txt
	const byte* d = vfs_get(&blob, "d.txt", &s);
	ASSERT_(s == 14);
	ASSERT_(d[0] == '4');

	vfs_close(&blob);
}

