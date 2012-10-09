#ifndef MEMORY_H
#define MEMORY_H

#ifndef NDEBUG
#ifndef TRACK_MEMORY
#define TRACK_MEMORY
#endif
#endif

// Hack to play nice with SDL
#ifdef MEM_FREE
#undef MEM_FREE
#endif

typedef struct {               
   unsigned int n_allocations;   
   unsigned int n_deallocations; 
   size_t bytes_allocated; 
   size_t peak_bytes_allocated;
} MemoryStats;

#ifdef TRACK_MEMORY

#define MEM_ALLOC(size) mem_alloc(size, __FILE__, __LINE__)
#define MEM_CALLOC(num, size) mem_calloc(num, size, __FILE__, __LINE__)
#define MEM_REALLOC(ptr, size) mem_realloc(ptr, size, __FILE__, __LINE__)
#define MEM_FREE(ptr) mem_free(ptr)

void* mem_alloc(size_t size, const char* file, int line);
void* mem_calloc(size_t num, size_t size, const char* file, int line);
void* mem_realloc(void* p, size_t size, const char* file, int line);
void mem_free(const void* ptr);
void mem_stats(MemoryStats* mstats);
void mem_dump(const char* path);
 
#else

#define MEM_ALLOC(size) malloc(size)
#define MEM_CALLOC(num, size) calloc(num, size)
#define MEM_REALLOC(ptr, size) realloc(ptr, size)
#define MEM_FREE(ptr) free((void*)ptr)

#endif

#endif

