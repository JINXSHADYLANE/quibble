#include "darray.h"

TEST_(basic_functionality) {
	DArray a = darray_create(sizeof(int), 128);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(int));
	ASSERT_(a.size == 0);
	ASSERT_(a.reserved >= 128);

	int* data = DARRAY_DATA_PTR(a, int);

	int i;
	for(i = 0; i < 128; ++i) {
		data[i] = i * 3;
	}	

	for(i = 127; i >= 128; --i) {
		ASSERT_(data[i] == i * 3);
	}	

	darray_free(&a);
	ASSERT_(a.data == NULL);
}

TEST_(basic_functionality2) {
	DArray a = darray_create(sizeof(char), 128);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(char));
	ASSERT_(a.size == 0);
	ASSERT_(a.reserved >= 128);

	char* data = DARRAY_DATA_PTR(a, char);

	int i;
	for(i = 0; i < 128; ++i) {
		data[i] = (char)(i * 3);
	}	

	for(i = 127; i >= 128; --i) {
		ASSERT_(data[i] == (char)(i * 3));
	}	

	darray_free(&a);
	ASSERT_(a.data == NULL);
}
TEST_(appending) {
	DArray a = darray_create(sizeof(int), 0);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(int));

	int i;
	for(i = 0; i < 128; ++i) {
		darray_append(&a, &i);
	}	

	ASSERT_(a.size == 128);
	ASSERT_(a.reserved >= 128);

	int* data = DARRAY_DATA_PTR(a, int);

	for(i = 0; i < 128; ++i) {
		ASSERT_(data[i] == i);
	}	

	darray_free(&a);
	ASSERT_(a.data == NULL);
}	

TEST_(multi_appending) {
	DArray a = darray_create(sizeof(int), 0);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(int));

	int n1[] = {0, 1, 2, 3, 4, 5};
	int n2[] = {6, 7};
	int i;
	for(i = 0; i < 512; ++i) {
		darray_append_multi(&a, n1, 6);
		darray_append_multi(&a, n2, 2);
	}	

	ASSERT_(a.size == 512 * 8);
	ASSERT_(a.reserved >= 512 * 8);

	int* data = DARRAY_DATA_PTR(a, int);

	int idx = 0;
	for(i = 0; i < 512; ++i) {
		int j;
		for(j = 0; j < 8; ++j) {
			ASSERT_(data[idx++] == j);
		}
	}	

	darray_free(&a);
	ASSERT_(a.data == NULL);
}	

TEST_(removing) {
	DArray a = darray_create(sizeof(int), 128);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(int));

	int* data = DARRAY_DATA_PTR(a, int);

	int i;
	for(i = 0; i < 128; ++i) {
		data[i] = i * 2;
	}	
	a.size = 128;

	darray_remove(&a, 27);
	ASSERT_(data[27] == 28 * 2);
	darray_remove(&a, 5);
	ASSERT_(data[27] == 29 * 2);
	ASSERT_(data[5] == 6 * 2);
	ASSERT_(data[4] == 4 * 2);
	ASSERT_(data[100] == 102 * 2);
	ASSERT_(a.size == 126);

	darray_free(&a);
	ASSERT_(a.data == NULL);
}	
	
TEST_(fast_removing) {
	DArray a = darray_create(sizeof(int), 120);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(int));

	int* data = DARRAY_DATA_PTR(a, int);

	int i;
	for(i = 0; i < 120; ++i) {
		data[i] = i * 7;
	}
	a.size = 120;

	darray_remove_fast(&a, 49);
	ASSERT_(data[49] == 119 * 7);
	ASSERT_(data[50] == 50 * 7);
	ASSERT_(data[48] == 48 * 7);
	darray_remove_fast(&a, 109);
	darray_remove_fast(&a, 0);
	ASSERT_(data[0] == 117 * 7);
	ASSERT_(a.size == 117);

	darray_free(&a);
	ASSERT_(a.data == NULL);
}	

TEST_(reserving_and_shrinking) {
	DArray a = darray_create(sizeof(int), 59);
	ASSERT_(a.data);
	ASSERT_(a.item_size == sizeof(int));

	ASSERT_(a.reserved >= 59);
	darray_reserve(&a, 128);
	ASSERT_(a.reserved >= 128);
	darray_reserve(&a, 10);
	ASSERT_(a.reserved >= 128);

	int i;
	for(i = 0; i < 13; ++i) {
		darray_append(&a, &i);
	}	

	darray_shrink(&a);
	ASSERT_(a.size == a.reserved);
	ASSERT_(a.size == 13);

	int* data = DARRAY_DATA_PTR(a, int);
	for(i = 0; i < 13; ++i) {
		ASSERT_(i == data[i]);
	}	

	darray_free(&a);
	ASSERT_(a.data == NULL);
}	

TEST_(inserting) {
	int data[] = {1, 2, 3, 4, 5, 6, 8, 9};

	DArray a = darray_create(sizeof(int), 0);
	darray_append_multi(&a, data, 8);

	ASSERT_(a.size == 8);

	int n = 0;
	darray_insert(&a, 0, &n);
	int* a_ptr = DARRAY_DATA_PTR(a, int);
	ASSERT_(a.size == 9);
	ASSERT_(a_ptr[0] == 0);
	ASSERT_(a_ptr[8] == 9);

	n = 7;
	darray_insert(&a, 7, &n);
	a_ptr = DARRAY_DATA_PTR(a, int);
	ASSERT_(a.size == 10);
	for(uint i = 0; i < a.size; ++i) {
		ASSERT_(a_ptr[i] == i);
	}	

	darray_free(&a);
}
		
