#include "anim.h"

#include "datastruct.h"
#include "memory.h"
#include "mempool.h"
#include "mml.h"

// Data buffer sizes, _predict_sizes() will calculate these
static uint str_blob_size = 0;
static uint n_seqs = 0;
static uint n_frames = 0;
static uint n_descs = 0;

// Data
static char* str_blob;
static AnimDesc* descs;
static AnimSeq* seqs;
static byte* frames;
static MemPool anim_pool;
static Dict anim_dict; // Both descs and seqs are stored here

extern bool _is_whitespace(char c);

static uint _count_frames(const char* list) {
	uint count = 0;
	bool last_was_whitespace = true;

	// Simply count every transition from whitespace
	// to non-whitespace, it must be equal to number
	// of frames.
	for(const char* c = list; *c != 0; ++c) {
		if(_is_whitespace(*c)) {
			if(!last_was_whitespace) {
				last_was_whitespace = true;
			}
		}
		else {
			if(last_was_whitespace) {
				last_was_whitespace = false;
				count++;
			}
		}
	}
	return count;
}

static void _predict_sizes(MMLObject* mml, NodeIdx root) {
	assert(str_blob_size == 0);
	assert(n_seqs == 0);
	assert(n_descs == 0);
	assert(n_frames == 0);

	// Itarate over anims
	NodeIdx anim = mml_get_first_child(mml, root);
	for(; anim != 0; anim = mml_get_next(mml, anim)) {
		// Each anim has one desc
		n_descs++;

		// Count space for anim name in str blob
		uint name_len = strlen(mml_getval_str(mml, anim)) + 1;
		str_blob_size += name_len;

		// Iterate over children seqs
		NodeIdx child = mml_get_first_child(mml, anim);
		for(; child != 0; child = mml_get_next(mml, child)) {
			if(strcmp(mml_get_name(mml, child), "seq") == 0) {

				// Size of seq depends on number of frames
				NodeIdx frames_node = mml_get_child(mml, child, "frames");
				assert(frames_node);
				n_frames += _count_frames(mml_getval_str(mml, frames_node));
				n_seqs++;

				// Seq will also be in anim_dict with key "animname$seqname",
				// so count that towards str blob size too
				str_blob_size += strlen(mml_getval_str(mml, child)) + name_len + 1;
			}
		}
	}

	assert(str_blob_size);
	assert(n_seqs);
	assert(n_descs);
	assert(n_frames);
}

static uint _parse_seq(MMLObject* mml, NodeIdx seq_node,
	AnimSeq* seq, byte* frame_dest) {

	seq->frames = frame_dest;
	seq->n_frames = 0;
	seq->play_seq = NULL;

	NodeIdx child = mml_get_first_child(mml, seq_node);
	for(; child != 0; child = mml_get_next(mml, child)) {
		const char* type = mml_get_name(mml, child);
		const char* content = mml_getval_str(mml, child);

		if(strcmp(type, "frames") == 0) {
			uint num; 
			while(*content && sscanf(content, "%u", &num) > 0) {
				seq->frames[seq->n_frames++] = num;

				// Skip to next number
				while(*content && _is_whitespace(*content))
					content++;
				while(*content && !_is_whitespace(*content))
					content++;
			}
		}
		else if(strcmp(type, "on_finish") == 0) {
			if(strcmp(content, "stop") == 0) {
				seq->on_finish = AS_STOP;
			}
			else if(strcmp(content, "loop") == 0) {
				seq->on_finish = AS_LOOP;
			}
			else if(strfind("play.", content) == 0) {
				// '.' is 4th char, seq name starts after.
				// Force it into AnimSeq* place, it will
				// get resolved to proper pointer later,
				// when all seqs are parsed.
				seq->play_seq = (void*)&content[5];
				seq->on_finish = AS_PLAY;
			}
		}
	}

	return seq->n_frames;
}

static void _anim_load_desc(const char* desc) {
	const char* mml_text = txtfile_read(desc);

	MMLObject mml;
	if(!mml_deserialize(&mml, mml_text))
		LOG_ERROR("Unable to parse anim desc %s", desc);
	MEM_FREE(mml_text);

	NodeIdx root = mml_root(&mml);
	if(strcmp(mml_get_name(&mml, root), "anims") != 0)
		LOG_ERROR("Invalid anim desc %s", desc);

	_predict_sizes(&mml, root);

	// Alloc all three buffers as a single piece of memory
	size_t total_size = str_blob_size;
	total_size += n_descs * sizeof(AnimDesc);
	total_size += n_seqs * sizeof(AnimSeq);
	total_size += n_frames;
	str_blob = MEM_ALLOC(total_size);
	descs = (void*)str_blob + str_blob_size;
	seqs = (void*)descs + n_descs * sizeof(AnimDesc);
	frames = (void*)seqs + n_seqs * sizeof(AnimSeq);

	/*
	// ... or alloc normal way
	str_blob = MEM_ALLOC(str_blob_size);
	seqs_blob = MEM_ALLOC(seqs_blob_size);
	descs = MEM_ALLOC(descs_size * sizeof(AnimDesc));
	*/

	// Iterate over anims
	NodeIdx anim = mml_get_first_child(&mml, root);
	char* str_dest = str_blob;
	AnimSeq* seq_dest = seqs;
	byte* frame_dest = frames;
	for(uint i = 0; anim != 0; ++i, anim = mml_get_next(&mml, anim)) {
		// Get ptr to to-be-filled desc, fill initial data
		AnimDesc* desc = &descs[i];
		desc->seqs = seq_dest;
		desc->n_seqs = 0;

		// Store start seq name here, to resolve after parsing all seqs
		const char* start_seq = NULL;

		// Put name into str blob
		strcpy(str_dest, mml_getval_str(&mml, anim));
		const char* anim_name = str_dest;
	
		// Pair name and desc together in anim_dict
		bool unique = dict_insert(&anim_dict, str_dest, desc);
		if(!unique)
			LOG_ERROR("Anim %s already exists.", str_dest);

		// Advance str pointer
		str_dest += strlen(str_dest) + 1;
		assert(str_dest <= str_blob + str_blob_size);

		NodeIdx child = mml_get_first_child(&mml, anim);
		for(; child != 0; child = mml_get_next(&mml, child)) {
			const char* type = mml_get_name(&mml, child);

			if(strcmp(type, "seq") == 0) {
				// Parse seq
				uint new_frames = _parse_seq(&mml, child, seq_dest, frame_dest);

				// Pair animname$seqname and seq in anim_dict
				sprintf(str_dest, "%s$%s", 
						anim_name, mml_getval_str(&mml, child)
				); 
				bool unique = dict_insert(&anim_dict, str_dest, seq_dest);
				if(!unique)
					LOG_ERROR("Seq %s already exists.", str_dest);
				str_dest += strlen(str_dest) + 1;
				assert(str_dest <= str_blob + str_blob_size);

				// Update seq and frame write cursors
				seq_dest++;
				assert((void*)seq_dest <= (void*)frames);
				frame_dest += new_frames;
				assert(frame_dest - frames <= n_frames);
				desc->n_seqs++;
			}
			else if(strcmp(type, "start_seq") == 0) {
				// Setup start_seq ptr later, when all seqs
				// will be parsed
				start_seq = mml_getval_str(&mml, child);
			}
			else if(strcmp(type, "fps") == 0) {
				desc->fps = mml_getval_uint(&mml, child);
			}
		}

		// Resolve start seq
		char seq_key[128];
		if(start_seq) {
			assert(strlen(anim_name) + strlen(start_seq) + 2 < 128);
			sprintf(seq_key, "%s$%s", anim_name, start_seq);
			desc->start_seq = (void*)dict_get(&anim_dict, seq_key);
			if(!desc->start_seq)
				LOG_ERROR("No such seq %s to start in", seq_key);
		}

		// Resolve 'play' seqs
		for(uint i = 0; i < desc->n_seqs; ++i) {
			AnimSeq* seq = &desc->seqs[i];
			if(seq->on_finish == AS_PLAY) {
				assert(seq->play_seq);
				const char* seq_name = (const char*)seq->play_seq;
				assert(strlen(anim_name) + strlen(seq_name) + 2 < 128);
				sprintf(seq_key, "%s$%s", anim_name, seq_name);
				seq->play_seq = (void*)dict_get(&anim_dict, seq_key);
				if(!seq->play_seq)
					LOG_ERROR("No such seq %s to play", seq_key);
			}
		}
	}
	mml_free(&mml);
}

void anim_init(const char* desc) {
	assert(desc);
	
	str_blob = NULL;
	descs = NULL;
	seqs = NULL;
	frames = NULL;
	mempool_init_ex(&anim_pool, sizeof(Anim), 1024);
	dict_init(&anim_dict);

	_anim_load_desc(desc);
}

void anim_close(void) {
	dict_free(&anim_dict);
	mempool_drain(&anim_pool);

	if(str_blob)
		MEM_FREE(str_blob);
	
	/*
	if(frames)
		MEM_FREE(frames);
	if(seqs)
		MEM_FREE(seqs);
	if(descs)
		MEM_FREE(descs);
	if(str_blob)
		MEM_FREE(str_blob);
	*/
}

Anim* anim_new(const char* name) {
	DictEntry* ent = dict_entry(&anim_dict, name);
	assert(ent);
	const AnimDesc* desc = ent->data;
	assert(desc);

	Anim* new = mempool_alloc(&anim_pool);
	new->name = ent->key;
	new->desc = desc;
	new->seq = desc->start_seq;
	new->play_t = time_s();

	return new;
}

void anim_del(Anim* anim) {
	assert(anim);

	mempool_free(&anim_pool, anim);
}

void anim_play(Anim* anim, const char* seq) {
	// Find seq
	char key[64];
	assert(strlen(anim->name) + strlen(seq) + 2 < 64);
	sprintf(key, "%s$%s", anim->name, seq);
	const AnimSeq* s = dict_get(&anim_dict, key);
	assert(s);

	// Set seq, start playing at the beginning
	anim->seq = s;
	anim->play_t = time_s();
}

uint anim_frame(Anim* anim) {
	assert(anim);

	const AnimDesc* desc = anim->desc;
	const AnimSeq* seq = anim->seq;

	float fps = (float)desc->fps;
	float dt = time_s() - anim->play_t;
	float fframe = dt * fps;
	uint iframe = lrintf(fframe - 0.5f);

	if(seq->on_finish == AS_LOOP) {
		return seq->frames[iframe % seq->n_frames];
	}
	else if(seq->on_finish == AS_STOP) {
		return seq->frames[MIN(seq->n_frames-1, iframe)];
	}
	else {
		assert(seq->on_finish == AS_PLAY);

		uint n_frames = seq->n_frames;
		if(iframe < n_frames) {
			return seq->frames[iframe];
		}
		else {
			float s_per_frame = 1.0f / fps;
			anim->seq = seq->play_seq;
			anim->play_t += s_per_frame * (float)n_frames;	
			return anim_frame(anim);
		}
	}
}

void anim_draw(Anim* anim, const char* spr, uint layer, Vector2 dest,
		float rot, float scale, Color tint) {

	spr_draw_anim_cntr(spr, anim_frame(anim), layer, dest, rot, scale, tint);
}

void anim_draw_h(Anim* anim, SprHandle spr, uint layer, Vector2 dest,
		float rot, float scale, Color tint) {

	spr_draw_anim_cntr_h(spr, anim_frame(anim), layer, dest, rot, scale, tint);
}

