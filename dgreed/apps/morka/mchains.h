#ifndef MCHAINS_H
#define MCHAINS_H

#include <sprsheet.h>

// Generates 2nd order Markov chains for 
// various procedural generation purposes.

#define ruleset_name_maxlen 10
#define chain_buffer_size 24

typedef struct {
	char name[ruleset_name_maxlen];

	uint n_symbols;
	char* symbols;
	uint* sym_advance;
	SprHandle* sym_spr;

	char init[2];

	// 3d array of 2nd order cumulative transition probabilities.
	uint16* trans;
} Ruleset;

typedef struct {
	Ruleset* rules;
	int cursor;
	char buffer[chain_buffer_size];
} Chain;

void mchains_init(const char* desc);
void mchains_close(void);

Chain* mchains_new(const char* ruleset);
void mchains_del(Chain* c);
char mchains_next(Chain* c, RndContext* rnd);

void mchain_symbol_info(Chain* c, char symbol, uint* advance, SprHandle* spr);

#endif

