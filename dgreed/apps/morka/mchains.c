#include "mchains.h"

#include <mempool.h>
#include <memory.h>
#include <mml.h>

// Data

static uint n_rulesets = 0;
static uint n_symbols = 0;
static uint n_transitions = 0;
static uint n_seq_chars = 0;

static Ruleset* rulesets;
static char* symbols;
static uint* sym_advances;
static SprHandle* sym_sprs;
static char** sym_seqs;
static char* seq_stores;
static uint16* transitions;

static MemPool chain_pool;

static void _predict_size(MMLObject* mml, NodeIdx root) {
	assert(n_rulesets == 0);
	assert(n_symbols == 0);
	assert(n_transitions == 0);
	assert(n_seq_chars == 0);

	// Iterate over rulesets
	NodeIdx rs_node = mml_get_first_child(mml, root);
	for(; rs_node != 0; rs_node = mml_get_next(mml, rs_node)) {
		n_rulesets++;

		// Count symbol defs
		uint rs_symbols = 0;
		NodeIdx child = mml_get_first_child(mml, rs_node);
		for(; child != 0; child = mml_get_next(mml, child)) {
			const char* name = mml_get_name(mml, child);
			if(strcmp(name, "def") == 0) {
				rs_symbols++;
			}
			if(strcmp(name, "sdef") == 0) {
				NodeIdx seq = mml_get_first_child(mml, child);
				assert(strcmp(mml_get_name(mml, seq), "seq") == 0);
				uint l = strlen(mml_getval_str(mml, seq));
				n_seq_chars = l + 1;
				rs_symbols++;
			}
		}

		assert(rs_symbols <= 32);
		n_symbols += rs_symbols;
		n_transitions += rs_symbols*rs_symbols*rs_symbols;
	}

	assert(n_rulesets);
	assert(n_symbols);
	assert(n_transitions);
}

static byte _get_idx(char c, char* chrs, uint n_chrs) {
	for(uint i = 0; i < n_chrs; ++i) {
		if(chrs[i] == c)
			return (byte)i;
	}
	return 0xFF;
}

static uint _parse_ruleset(MMLObject* mml, NodeIdx rs_node, Ruleset* rs, uint* new_seq_chars) {
	assert(strcmp(mml_get_name(mml, rs_node), "ruleset") == 0);
	assert(strlen(mml_getval_str(mml, rs_node)) < ruleset_name_maxlen);

	strcpy(rs->name, mml_getval_str(mml, rs_node));

	uint n = 0; // symbol count
	uint seq_idx = 0;

	// Iterate first time, over all symbol defs
	NodeIdx child = mml_get_first_child(mml, rs_node);
	for(; child != 0; child = mml_get_next(mml, child)) {
		const char* type = mml_get_name(mml, child);

		if(strcmp(type, "def") == 0) {
			const char* symbol = mml_getval_str(mml, child);
			assert(strlen(symbol) == 1);
			
			NodeIdx spr_node = mml_get_first_child(mml, child);
			assert(spr_node);
			const char* spr_name = mml_get_name(mml, spr_node);
			SprHandle spr_handle = 0;
			if(strcmp(spr_name, ".empty") != 0)
				spr_handle = sprsheet_get_handle(spr_name);
			uint advance = mml_getval_uint(mml, spr_node);

			rs->symbols[n] = *symbol;
			rs->sym_advance[n] = advance;
			rs->sym_spr[n] = spr_handle;
			rs->sym_seq[n] = NULL;
			n++;
		}
		else if(strcmp(type, "sdef") == 0) {
			const char* symbol = mml_getval_str(mml, child);
			assert(strlen(symbol) == 1);
		
			NodeIdx seq_node = mml_get_first_child(mml, child);
			assert(seq_node);
			const char* seq = mml_getval_str(mml, seq_node);

			rs->symbols[n] = *symbol;
			rs->sym_seq[n] = &rs->seq_store[seq_idx];

			strcpy(rs->sym_seq[n], seq);
			seq_idx += strlen(seq) + 1;
			n++;
		}
		else if(strcmp(type, "init") == 0) {
			const char* init = mml_getval_str(mml, child);
			assert(strlen(init) == 2);
			rs->init[0] = init[0];
			rs->init[1] = init[1];
		}
	}

	*new_seq_chars = seq_idx;
	rs->n_symbols = n;
	const uint nn = n*n;
	const uint nnn = n*n*n;

	// Init probabilities to 0
	memset(rs->trans, 0, nnn*sizeof(uint16));

	// Now iterate over learn directives
	// and update transition probabilities
	child = mml_get_first_child(mml, rs_node);
	for(; child != 0; child = mml_get_next(mml, child)) {
		const char* type = mml_get_name(mml, child);

		if(strcmp(type, "learn") == 0) {
			const char* str = mml_getval_str(mml, child);
			assert(strlen(str) > 2);

			byte ctx0 = _get_idx(str[0], rs->symbols, n);
			byte ctx1 = _get_idx(str[1], rs->symbols, n);
			assert(ctx0 != 0xFF && ctx1 != 0xFF);

			for(uint i = 2; str[i]; ++i) {
				byte sym = _get_idx(str[i], rs->symbols, n);
				assert(sym != 0xFF);
				uint idx = nn * ctx0 + n * ctx1 + sym;
				rs->trans[idx]++;

				ctx0 = ctx1;
				ctx1 = sym;
			}
		}
	}

	// Sum individual probabilities to cumulative
	for(uint i = 0; i < n; ++i) {
		for(uint j = 0; j < n; ++j) {
			for(uint k = 1; k < n; ++k) {
				uint idx = nn * i + n * j + k;
				rs->trans[idx] += rs->trans[idx-1];
			}
		}
	}

	// Debug print
	/*
	for(uint i = 0; i < n; ++i) {
		for(uint j = 0; j < n; ++j) {
			for(uint k = 0; k < n; ++k) {
				printf(" %u", (uint)rs->trans[nn * i + n * j + k]);
			}
			printf("\n");
		}
		printf("\n");
	}
	*/

	return n;
}

static void _mchains_load_desc(const char* desc) {
	const char* mml_text = txtfile_read(desc);

	MMLObject mml;
	if(!mml_deserialize(&mml, mml_text))
		LOG_ERROR("Unable to parse mchains desc %s", desc);
	MEM_FREE(mml_text);

	NodeIdx root = mml_root(&mml);
	if(strcmp(mml_get_name(&mml, root), "mchains") != 0)
		LOG_ERROR("Invalid mchains desc %s", desc);

	_predict_size(&mml, root);

	// Alloc all buffers in a single bite
	size_t total_size = n_rulesets * sizeof(Ruleset);
	total_size += n_symbols * (1 + sizeof(uint) + sizeof(SprHandle) + sizeof(char*));
	total_size += n_seq_chars;
	total_size += n_transitions * sizeof(uint16);
	rulesets = MEM_ALLOC(total_size);

	symbols = (void*)rulesets + n_rulesets * sizeof(Ruleset);
	sym_advances = (void*)symbols + n_symbols;
	sym_sprs = (void*)sym_advances + n_symbols * sizeof(uint);
	sym_seqs = (void*)sym_sprs + n_symbols * sizeof(SprHandle);
	seq_stores = (void*)sym_seqs + n_symbols * sizeof(char*);
	transitions = (void*)seq_stores + n_seq_chars;
	
	// Iterate over rulesets
	uint symbol_count = 0, seq_char_count = 0;	
	uint16* transition_dest = transitions;
	NodeIdx rs_node = mml_get_first_child(&mml, root);
	for(uint i = 0; rs_node != 0; ++i, rs_node = mml_get_next(&mml, rs_node)) {
		Ruleset* rs = &rulesets[i];
		rs->symbols = &symbols[symbol_count];
		rs->sym_advance = &sym_advances[symbol_count];
		rs->sym_spr = &sym_sprs[symbol_count];
		rs->sym_seq = &sym_seqs[symbol_count];
		rs->seq_store = &seq_stores[seq_char_count];
		rs->trans = transition_dest;

		uint new_seq_chars;
		uint new_symbols = _parse_ruleset(&mml, rs_node, rs, &new_seq_chars);
		seq_char_count += new_seq_chars;
		symbol_count += new_symbols;
		transition_dest += new_symbols * new_symbols * new_symbols;

		assert(symbol_count <= n_symbols);
		assert((void*)transition_dest <= (void*)rulesets + total_size);
	}

	mml_free(&mml);
}

void mchains_init(const char* desc) {
	assert(desc);

	rulesets = NULL;
	symbols = NULL;
	sym_advances = NULL;
	sym_sprs = NULL;
	transitions = NULL;
	mempool_init_ex(&chain_pool, sizeof(Chain), 512);

	_mchains_load_desc(desc);
}

void mchains_close(void) {
	if(rulesets) {
		MEM_FREE(rulesets);
	}

	mempool_drain(&chain_pool);
}

Chain* mchains_new(const char* ruleset) {
	assert(ruleset);

	// Find ruleset
	Ruleset* rules = NULL;
	for(uint i = 0; i < n_rulesets; ++i) {
		if(strcmp(rulesets[i].name, ruleset) == 0) {
			rules = &rulesets[i];
			break;
		}
	}
	assert(rules);

	// Alloc chain
	Chain* new = mempool_alloc(&chain_pool);
	new->rules = rules;
	new->cursor = 1;
	new->seq_cursor = NULL;
	new->buffer[1] = rules->init[0];
	new->buffer[0] = rules->init[1];

	return new;
}

void mchains_del(Chain* c) {
	mempool_free(&chain_pool, c);
}

char mchains_next(Chain* c, RndContext* rnd) {
	if(c->seq_cursor) {
		// Output sequence symbols
		char seq_char = *(c->seq_cursor++);	
		if(seq_char == '\0') {
			c->seq_cursor = NULL;
		}
		else {
			return seq_char;
		}
	}

	uint n = c->rules->n_symbols;
	if(c->cursor < 0) {
		// Pre-gen symbols to buffer

		// Buffer is empty, fill it backwards
		byte ctx0 = _get_idx(c->buffer[1], c->rules->symbols, n);
		byte ctx1 = _get_idx(c->buffer[0], c->rules->symbols, n);

		uint16* trans = c->rules->trans;
		const uint nn = n * n;

		int i = c->cursor = chain_buffer_size-1;
		while(i >= 0) {
			// Generate random number in range (0, symbol_probabilities_sum)
			uint base_idx = nn * ctx0 + n * ctx1;
			int max_r = trans[base_idx + n-1]; 
#ifdef _DEBUG
			if(max_r <= 0) {
				printf("Unable to follow context %c%c in chain %s\n", 
						c->rules->symbols[ctx0],
						c->rules->symbols[ctx1],
						c->rules->name
				);
				LOG_ERROR("mchains error");
			}
#endif
			int r = rand_int_ex(rnd, 0, max_r);

			// Linear search for the first cumulative probability that is 
			// higher than generated number. (No point in doing binary 
			// search here, we have < 10 symbols most of the time).
			byte sym = 0;
			for(int j = base_idx + n-2; j >= base_idx; --j) {
				if(trans[j] <= r) {
					sym = (j - base_idx) + 1;
					break;
				}
			}

			// If linear search fell through, it means the very first symbol
			// must be chosen, which is default sym value.
			ctx0 = ctx1;
			ctx1 = sym;
			c->buffer[i] = c->rules->symbols[sym];

			i--;
		}
	}
	
	// Output pre-gen'ed symbols or switch to seq mode
	char sym = c->buffer[c->cursor--];
	byte i = _get_idx(sym, c->rules->symbols, n);
	c->seq_cursor = c->rules->sym_seq[i];
	if(c->seq_cursor != NULL) 
		return mchains_next(c, rnd);
	else
		return sym;
}

void mchains_symbol_info(Chain* c, char symbol, uint* advance, SprHandle* spr) {
	Ruleset* rs = c->rules;
	uint i = _get_idx(symbol, rs->symbols, rs->n_symbols);
	*advance = rs->sym_advance[i];
	*spr = rs->sym_spr[i];
}


