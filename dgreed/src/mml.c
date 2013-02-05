#include <assert.h>
#include <string.h>

#include "mml.h"
#include "memory.h"

#define LINE_END_CHAR '\n'
#define ESCAPE_CHAR '\\'
#define QUOTE_CHAR '"'
#define COMMENT_CHAR '#'
#define BRACE_OPEN_CHAR '('
#define BRACE_CLOSE_CHAR ')'

bool _is_whitespace(char c) {
	switch(c) {
		case ' ':
		case '\t':
		case '\n':
		case '\f':
		case '\r':
			return true;
		default:
			return false;
	}
}	

// Allocates new string in string pool
StrIdx _alloc_str(MMLObject* mml, uint length) {
	assert(mml);
	assert(length);
	
	uint new_size, req_size = mml->str_pool.size + length;
	if(req_size <= mml->str_pool.reserved)
		goto end;

	new_size = mml->str_pool.size;
	do {
		new_size = (new_size * 3) / 2;
	} while(new_size <= req_size);

	darray_reserve(&(mml->str_pool), new_size);

end:
	mml->str_pool.size += length;
	return mml->str_pool.size - length;
}	

// Allocates new node in node pool
NodeIdx _alloc_node(MMLObject* mml) {
	assert(mml);

	if(mml->node_pool.size + 1 <= mml->node_pool.reserved)
		goto end;

	uint new_size = (mml->node_pool.size * 3) / 2;
	darray_reserve(&(mml->node_pool), new_size);

end:
	return mml->node_pool.size++;
}

void mml_empty(MMLObject* mml) {
	assert(mml);

	mml->node_pool = darray_create(sizeof(MMLNode), 0);
	mml->str_pool = darray_create(sizeof(char), 0);

	// This code produces warning in release mode:
	//NodeIdx root = mml_node(mml, "root", "_");
	//assert(root == 0);
	mml_node(mml, "root", "_");
}	

// TODO: Use some magic here to allocate right size pools
bool mml_deserialize(MMLObject* mml, const char* string) {
	assert(mml);
	assert(string);

	mml->node_pool = darray_create(sizeof(MMLNode), 0);
	mml->str_pool = darray_create(sizeof(char), 0);

	// Tokenize
	DArray tokens;
	bool result = mml_tokenize(mml, string, &tokens);
	if(!result) {
		darray_free(&tokens);
		mml_free(mml);
		return false;
	}	

	// Parse
	result = mml_parse(mml, &tokens);
	if(!result) {
		darray_free(&tokens);
		mml_free(mml);
		return false;
	}	

	darray_free(&tokens);

	return true;
}	

void mml_free(MMLObject* mml) {
	assert(mml);

	darray_free(&(mml->node_pool));
	darray_free(&(mml->str_pool));
}

void _serialize(MMLObject* mml, DArray* out, NodeIdx node, uint padding) {
	assert(mml);
	assert(out);

	MMLNode* node_ptr = mml_get_nodeptr(mml, node);
	
	uint i = padding;
	while(i--) {
		darray_append(out, " ");
	}	

	const char* name = mml_get_str(mml, node_ptr->name_start);
	const char* value = mml_get_str(mml, node_ptr->value_start);

	darray_append_multi(out, "( ", 2);
	mml_insert_escapes(name, out);
	darray_append(out, " ");
	mml_insert_escapes(value, out);

	if(node_ptr->first_child_idx == 0) {
		// node has no children, print it on a single line
		darray_append_multi(out, " )\n", 3);
		return;
	}
	else {
		// node has children, recurse
		darray_append(out, "\n");
		NodeIdx child = node_ptr->first_child_idx;
		while(child) {
			_serialize(mml, out, child, padding + 4);
			child = mml_get_next(mml, child);
		}	
		i = padding;
		while(i--) {
			darray_append(out, " ");
		}	
		darray_append_multi(out, ")\n", 2);
	}	
}	

void _serialize_compact(MMLObject* mml, DArray* out, NodeIdx node) {
	assert(mml);
	assert(out);

	MMLNode* node_ptr = mml_get_nodeptr(mml, node);
	
	const char* name = mml_get_str(mml, node_ptr->name_start);
	const char* value = mml_get_str(mml, node_ptr->value_start);

	darray_append(out, "(");
	mml_insert_escapes(name, out);
	darray_append(out, " ");
	mml_insert_escapes(value, out);

	NodeIdx child = node_ptr->first_child_idx;
	while(child) {
		_serialize_compact(mml, out, child);
		child = mml_get_next(mml, child);
	}	
	darray_append(out, ")");
}	

char* mml_serialize(MMLObject* mml) {
	DArray out;
	out = darray_create(sizeof(char), 0);

	_serialize(mml, &out, 0, 0);
	darray_append(&out, "\0");

	// This is a hack, but saves one alloc/free pair.
	// Freeing out.data is mostly the same as calling darray_free.
	return (char*)out.data;
}	

char* mml_serialize_compact(MMLObject* mml) {
	DArray out;
	out = darray_create(sizeof(char), 0);

	_serialize_compact(mml, &out, 0);
	darray_append(&out, "\0");

	// Look at mml_serialize for explanation
	return (char*)out.data;
}	

NodeIdx mml_root(MMLObject* mml) {
	// Root is always at index 0
	return 0;
}	

NodeIdx mml_node(MMLObject* mml, const char* name, const char* value) {
	assert(mml);
	assert(name);
	assert(value);

	// Allocate space for strings, +1 is for null char
	uint name_len = strlen(name);
	uint value_len = strlen(value);
	StrIdx name_idx = _alloc_str(mml, name_len+1);
	StrIdx value_idx = _alloc_str(mml, value_len+1);

	// Copy strings
	strcpy(mml_get_str(mml, name_idx), name);
	strcpy(mml_get_str(mml, value_idx), value);

	// Allocate and construct node
	NodeIdx new_node_idx = _alloc_node(mml);
	MMLNode* new_node = mml_get_nodeptr(mml, new_node_idx);
	new_node->name_start = name_idx;
	new_node->value_start = value_idx;
	new_node->first_child_idx = new_node->next_idx = 0;

	return new_node_idx;
}	

const char* mml_get_name(MMLObject* mml, NodeIdx node) {
	assert(mml);
	assert(node < mml->node_pool.size);
	
	MMLNode* node_ptr = mml_get_nodeptr(mml, node);
	return mml_get_str(mml, node_ptr->name_start); 
}

StrIdx _set_str(MMLObject* mml, StrIdx str_idx, const char* new_str) {
	assert(mml);
	assert(new_str);
	assert(str_idx < mml->str_pool.size);

	char* old_str = mml_get_str(mml, str_idx);
	uint old_len = strlen(old_str);
	uint new_len = strlen(new_str);

	if(old_len >= new_len) {
		// New string is not longer than old,
		// copy it over.
		strcpy(old_str, new_str);
		return str_idx;
	}
	else {
		// New string is longer, allocate new string
		StrIdx new_str_idx = _alloc_str(mml, new_len+1);
		strcpy(mml_get_str(mml, new_str_idx), new_str);
		return new_str_idx;
	}
}	

void mml_set_name(MMLObject* mml, NodeIdx node, const char* name) {
	assert(mml);
	assert(name);
	assert(node < mml->node_pool.size);

	MMLNode* node_ptr = mml_get_nodeptr(mml, node);
	node_ptr->name_start = _set_str(mml, node_ptr->name_start, name);
}

NodeIdx mml_get_first_child(MMLObject* mml, NodeIdx parent) {
	assert(mml);
	assert(parent < mml->node_pool.size);

	MMLNode* parent_ptr = mml_get_nodeptr(mml, parent);
	return parent_ptr->first_child_idx;
}	

NodeIdx mml_get_next(MMLObject* mml, NodeIdx parent) {
	assert(mml);
	assert(parent < mml->node_pool.size);

	MMLNode* parent_ptr = mml_get_nodeptr(mml, parent);
	return parent_ptr->next_idx;
}

uint mml_count_children(MMLObject* mml, NodeIdx parent) {
	uint count = 0;

	NodeIdx child = mml_get_first_child(mml, parent);
	while(child) {
		count++;
		child = mml_get_next(mml, child);
	}
	return count;
}	

NodeIdx mml_get_child(MMLObject* mml, NodeIdx parent, const char* name) {
	assert(mml);
	assert(name);
	assert(parent < mml->node_pool.size);

	NodeIdx child = mml_get_first_child(mml, parent);
	while(child) {
		if(strcmp(name, mml_get_name(mml, child)) == 0)
			return child;
		child = mml_get_next(mml, child);	
	}
	return 0;
}	

NodeIdx mml_get_sibling(MMLObject* mml, NodeIdx node, const char* name) {
	assert(mml);
	assert(name);
	assert(node);
	assert(node < mml->node_pool.size);

	node = mml_get_next(mml, node);
	while(node) {
		if(strcmp(name, mml_get_name(mml, node)) == 0)
			return node;
		node = mml_get_next(mml, node);	
	}
	return 0;
}

bool mml_remove_child(MMLObject* mml, NodeIdx parent, const char* name) {
	assert(mml);
	assert(name);
	assert(parent < mml->node_pool.size);

	NodeIdx last_child = 0;
	NodeIdx child = mml_get_first_child(mml, parent);
	while(child) {
		if(strcmp(name, mml_get_name(mml, child)) == 0) {
			MMLNode* child_ptr = mml_get_nodeptr(mml, child);
			if(last_child == 0) {
				// This is first child of parent node
				MMLNode* parent_ptr = mml_get_nodeptr(mml, parent);
				parent_ptr->first_child_idx = child_ptr->next_idx; 
				return true;
			}
			else {
				// This is not first child in children list
				MMLNode* last_child_ptr = mml_get_nodeptr(mml, last_child);
				last_child_ptr->next_idx = child_ptr->next_idx;
				return true;
			}
		}	
		last_child = child;
		child = mml_get_next(mml, child);
	}
	return false;
}

bool mml_insert_after(MMLObject* mml, NodeIdx parent, NodeIdx new,
	const char* name) {
	assert(mml);
	assert(name);
	assert(parent < mml->node_pool.size);
	assert(new < mml->node_pool.size);

	NodeIdx child = mml_get_first_child(mml, parent);
	while(child) {
		if(strcmp(name, mml_get_name(mml, child)) == 0) {
			MMLNode* child_ptr = mml_get_nodeptr(mml, child);
			MMLNode* new_ptr = mml_get_nodeptr(mml, new);
			new_ptr->next_idx = child_ptr->next_idx;
			child_ptr->next_idx = new;
			return true;
		}	
		child = mml_get_next(mml, child);	
	}
	return false;
}		

void mml_append(MMLObject* mml, NodeIdx parent, NodeIdx new) {
	assert(mml);
	assert(parent < mml->node_pool.size);
	assert(new < mml->node_pool.size);

	NodeIdx last_child = 0;
	NodeIdx child = mml_get_first_child(mml, parent);

	// Iterate till the end of list
	while(child) {
		last_child = child;
		child = mml_get_next(mml, child);
	}	

	if(last_child == 0) {
		// Parent has no children
		MMLNode* parent_ptr = mml_get_nodeptr(mml, parent);
		parent_ptr->first_child_idx = new;
	}
	else {
		// Parent has children list, attach to the end of it
		MMLNode* last_child_ptr = mml_get_nodeptr(mml, last_child);
		last_child_ptr->next_idx = new;
	}	
}	

void mml_cleanup(MMLObject* mml) {
	// TODO: implement this
}	

const char* mml_getval_str(MMLObject* mml, NodeIdx node) {
	assert(mml);
	assert(node < mml->node_pool.size);

	MMLNode* node_ptr = mml_get_nodeptr(mml, node);
	return mml_get_str(mml, node_ptr->value_start);
}	

uint mml_getval_uint(MMLObject* mml, NodeIdx node) {
	uint result;
	sscanf(mml_getval_str(mml, node), "%u", &result);
	return result;
}	

int mml_getval_int(MMLObject* mml, NodeIdx node) {
	int result;
	sscanf(mml_getval_str(mml, node), "%d", &result);
	return result;
}

// TODO: update this be case-insensitive and support 0&1
bool mml_getval_bool(MMLObject* mml, NodeIdx node) {
	const char* value = mml_getval_str(mml, node);
	if(strcmp(value, "true") == 0)
		return true;
	return false;	
}	

float mml_getval_float(MMLObject* mml, NodeIdx node) {
	float result;
	sscanf(mml_getval_str(mml, node), "%f", &result);
	return result;
}	

Vector2 mml_getval_vec2(MMLObject* mml, NodeIdx node) {
	Vector2 result;
	sscanf(mml_getval_str(mml, node), "%f,%f", &result.x, &result.y);
	return result;
}

RectF mml_getval_rectf(MMLObject* mml, NodeIdx node) {
	RectF result;
	sscanf(mml_getval_str(mml, node), "%f,%f,%f,%f",
		&result.left, &result.top, &result.right, &result.bottom
	);
	return result;
}

Color mml_getval_color(MMLObject* mml, NodeIdx node) {
	uint r, g, b, a;
	sscanf(mml_getval_str(mml, node), "%u,%u,%u,%u", &r, &g, &b, &a);
	return COLOR_RGBA(r, g, b, a);
}

void mml_setval_str(MMLObject* mml, NodeIdx node, const char* val) {
	assert(mml);
	assert(node < mml->node_pool.size);

	MMLNode* node_ptr = mml_get_nodeptr(mml, node);
	node_ptr->value_start = _set_str(mml, node_ptr->value_start, val);
}	

void mml_setval_uint(MMLObject* mml, NodeIdx node, uint val) {
	char str_val[16];
	sprintf(str_val, "%u", val);
	mml_setval_str(mml, node, str_val);
}	

void mml_setval_int(MMLObject* mml, NodeIdx node, int val) {
	char str_val[16];
	sprintf(str_val, "%d", val);
	mml_setval_str(mml, node, str_val);
}

// TODO: update according to mml_getval_bool
void mml_setval_bool(MMLObject* mml, NodeIdx node, bool val) {
	const char* t = "true";
	const char* f = "false";
	const char* str_val = val ? t : f;
	mml_setval_str(mml, node, str_val);
}	

void mml_setval_float(MMLObject* mml, NodeIdx node, float val) {
	char str_val[32];
	sprintf(str_val, "%f", val);
	mml_setval_str(mml, node, str_val);
}	

void mml_setval_vec2(MMLObject* mml, NodeIdx node, Vector2 val) {
	char str_val[64];
	sprintf(str_val, "%f,%f", val.x, val.y);
	mml_setval_str(mml, node, str_val);
}

void mml_setval_rectf(MMLObject* mml, NodeIdx node, RectF val) {
	char str_val[128];
	sprintf(str_val, "%f,%f,%f,%f", 
		val.left, val.top, val.right, val.bottom
	);
	mml_setval_str(mml, node, str_val);
}

void mml_setval_color(MMLObject* mml, NodeIdx node, Color val) {
	byte r, g, b, a;
	char str_val[64];
	COLOR_DECONSTRUCT(val, r, g, b, a);
	sprintf(str_val, "%hhu,%hhu,%hhu,%hhu", r, g, b, a);
	mml_setval_str(mml, node, str_val);
}
		
const char* mml_last_error(MMLObject* mml) {
	assert(mml);
	return mml->last_error;
}	

MMLNode* mml_get_nodeptr(MMLObject* mml, NodeIdx node_idx) {
	assert(mml);
	assert(node_idx < mml->node_pool.size);

	MMLNode* nodes = DARRAY_DATA_PTR(mml->node_pool, MMLNode);
	assert(nodes);

	return &nodes[node_idx];
}	

char* mml_get_str(MMLObject* mml, StrIdx str_start) {
	assert(mml);
	assert(str_start < mml->str_pool.size);

	char* strs = DARRAY_DATA_PTR(mml->str_pool, char);
	assert(strs);

	return &strs[str_start];
}

bool mml_tokenize(MMLObject* mml, const char* string, DArray* tokens) {
	assert(string);
	assert(tokens);

	*tokens = darray_create(sizeof(MMLToken), 300);
    
	bool in_comment = false;
	bool in_qliteral = false;
	bool in_literal = false;
	uint literal_start = 0;

	// This is not a typo
	uint i = -1;
	do {
		i++;
		if(in_comment) {
			// In comment, skip everything till line end
			if(string[i] == LINE_END_CHAR) {
				in_comment = false;
				continue;
			}
			continue;
		}	

		if(in_qliteral) {
			// Everything till next non-escaped quote is literal
			if(string[i] == QUOTE_CHAR && string[i-1] != ESCAPE_CHAR) {
				// Found endquote, construct literal token 
				MMLToken new_token = 
					{TOK_LITERAL, &string[literal_start], i - literal_start};
				darray_append(tokens, &new_token);	
				in_qliteral = false;
				continue;
			}	
			continue;
		}	

		if(string[i] == COMMENT_CHAR) {
			// Comment start, end current literal
			if(in_literal) {
				in_literal = false;
				MMLToken new_token =
					{TOK_LITERAL, &string[literal_start], i - literal_start};
				darray_append(tokens, &new_token);	
			}
			in_comment = true;
			continue;
		}	

		if(string[i] == QUOTE_CHAR) {
			// Quoted literal start
			in_qliteral = true;
			literal_start = i+1;
			continue;
		}	

		if(_is_whitespace(string[i])) {
			// Whitespace, end current literal if such exists
			if(in_literal) {
				in_literal = false;
				MMLToken new_token =
					{TOK_LITERAL, &string[literal_start], i - literal_start};
				darray_append(tokens, &new_token);	
			}
			continue;
		}	

		if(string[i] == BRACE_OPEN_CHAR || string[i] == BRACE_CLOSE_CHAR) {
			// Brace, end current literal if such exists
			if(in_literal) {
				in_literal = false;
				MMLToken new_token =
					{TOK_LITERAL, &string[literal_start], i - literal_start};
				darray_append(tokens, &new_token);	
			}

			MMLToken new_token = {0};
			if(string[i] == BRACE_OPEN_CHAR)
				new_token.type = TOK_BRACE_OPEN;
			else
				new_token.type = TOK_BRACE_CLOSE;
			darray_append(tokens, &new_token);

			continue;
		}	

		if(!in_literal) {
			// Different char - start new literal
			in_literal = true;
			literal_start = i;
		}	
	} while(string[i+1]);

	if(in_literal) {
		sprintf(mml->last_error, "TOKENIZER: Unexpected literal in the end");
		return false;
	}
	if(in_qliteral) {
		sprintf(mml->last_error, "TOKENIZER: Open qouted literal in the end");
		return false;
	}	
	if(in_comment) {
		sprintf(mml->last_error, "TOKENIZER: No newline after last comment");
		return false;
	}	

	return true;
}

bool _parse(MMLObject* mml, MMLToken* tokens, uint start, uint end, 
	NodeIdx* result) {
	// Token at end index does not belong to this node
	uint size = end - start;

	if(size < 4) {
		sprintf(mml->last_error, "PARSER: Node must contain at least 4 tokens");
		return false;
	}	
	if(tokens[start].type != TOK_BRACE_OPEN || 
		tokens[end-1].type != TOK_BRACE_CLOSE) {
		sprintf(mml->last_error, 
			"PARSER: Node must begin and end with proper braces");
		return false;
	}	
	if(tokens[start+1].type != TOK_LITERAL || 
		tokens[start+2].type != TOK_LITERAL) {
		sprintf(mml->last_error,
			"PARSER: Node must have name and value");
		return false;
	}	

	// +1 length is for null char
	StrIdx name_str_idx = _alloc_str(mml, tokens[start+1].length + 1);
	StrIdx val_str_idx = _alloc_str(mml, tokens[start+2].length + 1);
	char* name_str = mml_get_str(mml, name_str_idx);
	char* val_str = mml_get_str(mml, val_str_idx);

	// Store escape-filtered strings
	uint name_len = mml_remove_escapes(tokens[start+1].literal,
		tokens[start+1].length, name_str);
	uint val_len = mml_remove_escapes(tokens[start+2].literal,
		tokens[start+2].length, val_str);

	// Put null chars
	name_str[name_len] = '\0';
	val_str[val_len] = '\0';	

	// Construct new node
	NodeIdx new_node_idx = _alloc_node(mml);  
	MMLNode* new_node = mml_get_nodeptr(mml, new_node_idx); 
	new_node->name_start = name_str_idx;
	new_node->value_start = val_str_idx;
	new_node->first_child_idx = new_node->next_idx = 0;

	NodeIdx last_child_idx = 0;

	uint i;
	for(i = start+3; i < end-1; ++i) {
		if(tokens[i].type == TOK_LITERAL) {
			sprintf(mml->last_error, "PARSER: Unexpected literal");
			return false;
		}
		if(tokens[i].type == TOK_BRACE_CLOSE) {
			sprintf(mml->last_error, "PARSER: Unexpected closing brace");
			return false;
		}	

		// Determine length of child
		uint depth = 1, j = i;
		while(depth) {
			j++;
			if(tokens[j].type == TOK_BRACE_OPEN)
				depth++;
			if(tokens[j].type == TOK_BRACE_CLOSE)
				depth--;
			if(j >= end-1) {
				sprintf(mml->last_error, "PARSER: Misplaced brace");
				return false;
			}	
		}	
		
		// Recursively parse child
		NodeIdx child_idx = 0;
		bool result = _parse(mml, tokens, i, j+1, &child_idx);
		i = j;

		if(!result)
			return false;

		assert(child_idx);	

		// Attach child to this node
		if(last_child_idx == 0) {
			// This is first child
			MMLNode* new_node = mml_get_nodeptr(mml, new_node_idx);
			new_node->first_child_idx = child_idx;
			last_child_idx = child_idx;
		}
		else {
			// This is not first child, attach it to linked list
			MMLNode* last_child = mml_get_nodeptr(mml, last_child_idx);
			last_child->next_idx = child_idx;
			last_child_idx = child_idx;
		}	
	}
	*result = new_node_idx;

	return true;
}	

bool mml_parse(MMLObject* mml, DArray* tokens) {
	assert(mml);
	assert(tokens);

	MMLToken* tokens_data = DARRAY_DATA_PTR(*tokens, MMLToken);

	NodeIdx root_idx;
	bool result = _parse(mml, tokens_data, 0, tokens->size, &root_idx);

	if(!result)
		return false;

	assert(root_idx == 0);
	return true;
}

uint mml_remove_escapes(const char* in, uint length, char* out) {
	uint i, write_idx = 0;
	for(i = 0; i < length; ++i) {
		if(in[i] == ESCAPE_CHAR) {
			// Last char should not be escape
			assert(i < length - 1);
			switch(in[i+1]) {
				case 'n':
					out[write_idx++] = '\n';
					break;
				case 'r':
					out[write_idx++] = '\r';
					break;
				case 't':
					out[write_idx++] = '\t';
					break;
				case 'b':
					out[write_idx++] = '\b';
					break;
				default:
					out[write_idx++] = in[i+1];
			}	
			i++;
			continue;
		}	
		out[write_idx++] = in[i];
	}
	return write_idx;
}	

// Returns true if string needs escaping.
// Also returns length in 'len' parameter.
// Also returns if string needs qouting.
bool _needs_escaping(const char* in, uint* len, bool* qout) {
	assert(in);
	assert(len);
	assert(qout);

	*qout = false;
	bool escape = false;

	uint i = 0;
	while(in[i]) {
		switch(in[i]) {
			case '\n':
			case '\r':
			case '\t':
			case '\b':
			case '"':
			case '\\':
				escape = true;
			case '(':
			case ')':
			case ' ':
			case '#':
				*qout = true;
		}
		i++;
	}	
	*len = i;

	// Quote empty strings
	if(!i)
		*qout = true;

	return escape;
}	

uint mml_insert_escapes(const char* in, DArray* out) {
	assert(in);
	assert(out);
	assert(out->item_size == 1);

	bool qoute = false;
	uint length;
	if(!_needs_escaping(in, &length, &qoute)) {
		if(qoute)
			darray_append(out, "\"");

        if(length)
            darray_append_multi(out, (void*)in, length);

		if(qoute)
			darray_append(out, "\"");
		return length + (qoute ? 2 : 0);
	}

	length = 0;

	if(qoute) {
		darray_append(out, "\"");
		length++;
	}	

	uint i = 0;
	while(in[i]) {
		switch(in[i]) {
			case '\n':
				darray_append_multi(out, "\\n", 2);
				length++; break;
			case '\r':
				darray_append_multi(out, "\\r", 2);
				length++; break;
			case '\t':
				darray_append_multi(out, "\\t", 2);
				length++; break;
			case '\b':
				darray_append_multi(out, "\\b", 2);
				length++; break;
			case '"':
				darray_append_multi(out, "\\\"", 2);
				length++; break;
			case '\\':
				darray_append_multi(out, "\\\\", 2);
				length++; break;
			default:
				darray_append(out, (void*)&in[i]);
		}	
		length++; i++;
	}	

	if(qoute) {
		darray_append(out, "\"");
		length++;
	}	
		
	return length;
}

