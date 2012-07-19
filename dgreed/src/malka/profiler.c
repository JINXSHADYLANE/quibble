#include "profiler.h"

#include "darray.h"
#include "datastruct.h"
#include "memory.h"

// Timing code, from luatrace

#ifdef __MACH__
#include <mach/mach.h>
#include <mach/mach_time.h>
typedef uint64_t hook_time_t;
#define CLOCK_FUNCTION mach_absolute_time
#elif __linux__
#include <time.h>
typedef long hook_time_t;
#define CLOCK_FUNCTION lclock
#ifdef CLOCK_MONOTONIC_RAW
#define LINUX_CLOCK CLOCK_MONOTONIC_RAW
#else
#define LINUX_CLOCK CLOCK_MONOTONIC
#endif
#elif _WIN32
#include <windows.h>
typedef long long hook_time_t;
#define CLOCK_FUNCTION wclock
#else
#include <time.h>
typedef clock_t hook_time_t;
#define CLOCK_FUNCTION clock
#endif


/*============================================================================*/

static unsigned long microseconds_numerator;
static lua_Number microseconds_denominator;


#ifdef __MACH__
static void get_microseconds_info(void)
{
  mach_timebase_info_data_t timebase_info;
  mach_timebase_info(&timebase_info);
  microseconds_numerator = timebase_info.numer;
  microseconds_denominator = (lua_Number)(timebase_info.denom * 1000);
}
#elif __linux__
static void get_microseconds_info(void)
{
  microseconds_numerator = 1;
  microseconds_denominator = 1000;
}

static hook_time_t lclock()
{
  struct timespec tp;
  clock_gettime(LINUX_CLOCK, &tp);
  return tp.tv_sec * 1000000000L + tp.tv_nsec;
}
#elif _WIN32
static void get_microseconds_info(void)
{
  LARGE_INTEGER frequency;
  long long f;
  QueryPerformanceFrequency(&frequency);
  f = frequency.QuadPart;
  if (f < 1000000)
  {
    microseconds_numerator = 1000000 / f;
    microseconds_denominator = 1;
  }
  else
  {
    microseconds_numerator = 1;
    microseconds_denominator = f / 1000000;
  }
}

static hook_time_t wclock()
{
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return t.QuadPart;
}
#else
static void get_microseconds_info(void)
{
  if (CLOCKS_PER_SEC < 1000000)
  {
    microseconds_numerator = 1000000 / CLOCKS_PER_SEC;
    microseconds_denominator = 1;
  }
  else
  {
    microseconds_numerator = 1;
    microseconds_denominator = CLOCKS_PER_SEC / 1000000;
  }
}
#endif

static double time_micros() {
	static bool init = false;
	if(!init) {
		get_microseconds_info();
		init = true;
	}

	return ((double)(microseconds_numerator * CLOCK_FUNCTION())) / microseconds_denominator;
}

/*============================================================================*/

typedef struct {
	const char* place;
	const char* name;
	uint n_invocations;
	int rec_depth;
	double total_time;
	int total_mem;

	double enter_time;
	uint enter_mem;
} FunctionStats;

static Dict stats;

static void _hook(lua_State* l, lua_Debug* d) {
	lua_getinfo(l, "nS", d);

	// Just skip all C functions, native profiler can measure these
	if(strcmp(d->short_src, "[C]") == 0)
		return;

	// Also skip weird untraceable functions
	if(d->linedefined == 0)
		return;

	// We're either entering or leaving
	bool enter = d->event == LUA_HOOKCALL;

	// Construct function signature which we'll use to track it
	char sig[256];
	sprintf(sig, "%s:%d: %s", d->source, d->linedefined, d->name);

	// Sample current time and memory
	uint mem = lua_gc(l, LUA_GCCOUNT, 0);
	double t = time_micros();

	// Get FunctionStats
	DictEntry* e = dict_entry(&stats, sig);
	FunctionStats* s = NULL;
	if(!e) {
		// Encountered for the first time
		assert(enter);

		char place[256];
		sprintf(place, "%s:%d", d->source, d->linedefined);

		FunctionStats fs = {
			.place = strclone(place),
			.name = d->name ? strclone(d->name) : NULL,
			.n_invocations = 0,
			.rec_depth = 0,
			.total_time = 0.0,
			.total_mem = 0,
			.enter_time = 0.0,
			.enter_mem = 0 
		};
		s = MEM_ALLOC(sizeof(FunctionStats));
		memcpy(s, &fs, sizeof(FunctionStats));

		dict_set(&stats, strclone(sig), s);
	}
	else {
		// Already encountered before
		s = (FunctionStats*)e->data;
	}

	if(enter) {
		// Entering function, update invocations
		// and mark enter time and mem

		// If this is recursive function - measure
		// only root invocation
		if(s->rec_depth++ == 0) { 
			s->enter_time = t;
			s->enter_mem = mem;
		}

		s->n_invocations++;
	}
	else {
		// Leaving function,
		// update total time and mem counts
		
		assert(s->rec_depth > 0);
		if(s->rec_depth-- == 1) {
			double dt = t - s->enter_time;
			int dmem = mem - s->enter_mem;

			s->total_time += dt;
			s->total_mem += dmem;
		}
	}
}

void profiler_init(lua_State* l) {
	assert(l);

	dict_init(&stats);

	lua_sethook(l, _hook, LUA_MASKCALL | LUA_MASKRET, 0);
}

void profiler_close(lua_State* l) {
	lua_sethook(l, _hook, 0, 0);

	profiler_results();

	for(uint i = 0; i < stats.mask+1; ++i) {
		DictEntry* e = &stats.map[i];
		if(e->key && e->data) {
			const FunctionStats* s = e->data;
			if(s->place)
				MEM_FREE(s->place);
			if(s->name)
				MEM_FREE(s->name);
			MEM_FREE(e->key); 
			MEM_FREE(e->data);
		}
	}

	dict_free(&stats);
}

static int mem_pred(const void* a, const void* b) {
	const FunctionStats** as = a;
	const FunctionStats** bs = b;
	return (*as)->total_mem - (*bs)->total_mem;
}

static int time_pred(const void* a, const void* b) {
	const FunctionStats** as = a;
	const FunctionStats** bs = b;
	return (*as)->total_time - (*bs)->total_time;
}

static int inv_pred(const void* a, const void* b) {
	const FunctionStats** as = a;
	const FunctionStats** bs = b;
	return (*as)->n_invocations - (*bs)->n_invocations;
}

const char* profiler_results(void) {
	DArray funcs = darray_create(sizeof(FunctionStats*), 0);

	// Push all functions into the darray
	for(uint i = 0; i < stats.mask+1; ++i) {
		DictEntry* e = &stats.map[i];
		if(e->key && e->data) {
			const FunctionStats* s = e->data;
			darray_append(&funcs, &s);
		}
	}

	// Sort by total time
	qsort(funcs.data, funcs.size, sizeof(FunctionStats*), time_pred);

	// Output top 20
	printf("Top 20 time:\n");
	for(uint i = 0; i < 20; ++i) {
		FunctionStats* s = *(FunctionStats**)darray_get(&funcs, funcs.size - i - 1);
		printf("%s %s \t\t %fms\n", s->place, s->name, s->total_time / 1000.0);
	}
	printf("-----\n");

	// Sort by total mem 
	qsort(funcs.data, funcs.size, sizeof(FunctionStats*), mem_pred);

	// Output top 20
	printf("Top 20 mem:\n");
	for(uint i = 0; i < 20; ++i) {
		FunctionStats* s = *(FunctionStats**)darray_get(&funcs, funcs.size - i - 1);
		printf("%s %s \t\t %d\n", s->place, s->name, s->total_mem);
	}
	printf("-----\n");

	// Sort by total invoke 
	qsort(funcs.data, funcs.size, sizeof(FunctionStats*), inv_pred);

	// Output top 20
	printf("Top 20 calls:\n");
	for(uint i = 0; i < 20; ++i) {
		FunctionStats* s = *(FunctionStats**)darray_get(&funcs, funcs.size - i - 1);
		printf("%s %s \t\t %d\n", s->place, s->name, s->n_invocations);
	}
	printf("-----\n");

	darray_free(&funcs);

	return NULL;
}


