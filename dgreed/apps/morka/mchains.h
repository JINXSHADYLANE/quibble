#ifndef MCHAINS_H
#define MCHAINS_H

#include <sprsheet.h>

// Generates 2nd order Markov chains for 
// various procedural generation purposes.

#define ruleset_name_maxlen 18
#define chain_buffer_size 20

typedef union {
	SprHandle spr;
	void* ruleset; // void ptr to break circular struct dependency
} SymDesc;

// 48 bytes
typedef struct {
	char name[ruleset_name_maxlen];
	char init[2];

	uint n_symbols;
	char* symbols;
	int* sym_advance;
	SymDesc* sym_desc;
	char** sym_seq;

	char* seq_store;

	// 3d array of 2nd order cumulative transition probabilities.
	uint16* trans;
} Ruleset;

// 32 bytes
typedef struct {
	Ruleset* rules;
	int cursor;
	char* seq_cursor;
	char buffer[chain_buffer_size];
} Chain;

void mchains_init(const char* desc);
void mchains_close(void);

Chain* mchains_new(const char* ruleset);
Chain* mchains_new_ex(Ruleset* ruleset);
void mchains_del(Chain* c);
void mchains_reset(Chain* c);
char mchains_next(Chain* c, RndContext* rnd);

void mchains_symbol_info(Chain* c, char symbol, int* advance, SprHandle* spr);
void mchains_symbol_info_ex(Chain* c, char symbol, int* advance, SymDesc* desc);

#endif

