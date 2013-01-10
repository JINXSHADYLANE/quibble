#ifndef MML_H
#define MML_H

#include "utils.h"
#include "darray.h"

// MML - Minimalistic markup language
// Amazingly simple and blinding fast alternative to XML.

// Public interface:

// Beware - all pointers to MMLNodes or their names/values
// may be invalidated by these functions:
// mml_node, 
// mml_set_name (invalidates names/values and only if new name is longer),
// mml_cleanup,
// mml_setval_*** (same as mml_set_name)
//
// Storing indexes to node_pool/str_pool arrays is always safe.

#define MML_ERROR_SIZE 128

typedef uint NodeIdx;
typedef uint StrIdx;

typedef struct {
	// Indexes to str_pool 
	StrIdx name_start;
	StrIdx value_start;

	// Indexes to node_pool.
	// Root node always has index 0, so here it means invalid index.
	NodeIdx first_child_idx;
	NodeIdx next_idx;
} MMLNode;	

typedef struct {
	DArray node_pool;
	DArray str_pool;
	char last_error[MML_ERROR_SIZE];
} MMLObject;	

// Creates new, empty MMLObject (it has single node - root) 
void mml_empty(MMLObject* mml);
// Parses string and constructs MMLObject representing it.
// Returns false on error, mml_last_error returns its description.
bool mml_deserialize(MMLObject* mml, const char* string);
// Frees all memory used by MMLObject
void mml_free(MMLObject* mml);

// Converts MMLObject to string, which can later be parsed
// to equivavalent MMLObject by mml_deserialize.
char* mml_serialize(MMLObject* mml);
// Mostly the same as mml_serialize, returns not nicely formatted but
// shortest possible string.
char* mml_serialize_compact(MMLObject* mml);

// Returns root node.
NodeIdx mml_root(MMLObject* mml);
// Constructs new node. It is not attached anywhere!
NodeIdx mml_node(MMLObject* mml, const char* name, const char* value);
// Returns name of node.
const char* mml_get_name(MMLObject* mml, NodeIdx node);
// Sets new name of node.
void mml_set_name(MMLObject* mml, NodeIdx node, const char* name);
// Returns first child of node, 0 if node has no children.
NodeIdx mml_get_first_child(MMLObject* mml, NodeIdx parent);
// Returns next node in children list, 0 if this is last/only node.
NodeIdx mml_get_next(MMLObject* mml, NodeIdx node);

// Returns number of children a parant node has.
// Children of children 
uint mml_count_children(MMLObject* mml, NodeIdx parent);
// Returns first child of parent node which has provided name,
// 0 if no such child exists. 
// (don't mix with root node, it also has index 0)
NodeIdx mml_get_child(MMLObject* mml, NodeIdx parent, const char* name);
// Returns first sibling of a node, which has provided name.
// Only nodes coming after starting node are checked.
// Returns 0 if no such sibling exists.
NodeIdx mml_get_sibling(MMLObject* mml, NodeIdx node, const char* name);
// Removes first child of parent node which has provided name.
// Returns true on success, false if no such child exists.
bool mml_remove_child(MMLObject* mml, NodeIdx parent, const char* name);
// Inserts new child node after the node which has provided name.
// Returns true on success, false otherwise.
// Don't insert nodes which are already attached to the tree!
bool mml_insert_after(MMLObject* mml, NodeIdx parent, NodeIdx new, 
	const char* name);
// Appends new node to the end of children list.
// Always succeeds, even if parent node is not attached to tree.
void mml_append(MMLObject* mml, NodeIdx parent, NodeIdx new);

// Removes all unattached nodes from pool, reorders them to improve
// locality-of-reference when iterating over children lists.
// This is costly operation, call only when tree will no longer change.
void mml_cleanup(MMLObject* mml);

// Converts node values to some common types.
// Returns unpredictable nonsense if value string 
// cannot be converted to specific type. 
const char* mml_getval_str(MMLObject* mml, NodeIdx node);
uint mml_getval_uint(MMLObject* mml, NodeIdx node);
int mml_getval_int(MMLObject* mml, NodeIdx node);
bool mml_getval_bool(MMLObject* mml, NodeIdx node);
float mml_getval_float(MMLObject* mml, NodeIdx node);
Vector2 mml_getval_vec2(MMLObject* mml, NodeIdx node);
RectF mml_getval_rectf(MMLObject* mml, NodeIdx node);
Color mml_getval_color(MMLObject* mml, NodeIdx node);

// Sets node values from some common types
void mml_setval_str(MMLObject* mml, NodeIdx node, const char* val);
void mml_setval_uint(MMLObject* mml, NodeIdx node, uint val);
void mml_setval_int(MMLObject* mml, NodeIdx node, int val);
void mml_setval_bool(MMLObject* mml, NodeIdx node, bool val);
void mml_setval_float(MMLObject* mml, NodeIdx node, float val);
void mml_setval_vec2(MMLObject* mml, NodeIdx node, Vector2 val);
void mml_setval_rectf(MMLObject* mml, NodeIdx node, RectF val);
void mml_setval_color(MMLObject* mml, NodeIdx node, Color val);

// Returns reason why last failure happened. 
const char* mml_last_error(MMLObject* mml);

// Converts node index to pointer. Beware of invalidation!
MMLNode* mml_get_nodeptr(MMLObject* mml, NodeIdx node_idx);
// Converts string index to pointer. Beware of invalidation!
char* mml_get_str(MMLObject* mml, StrIdx str_start);

// Private interface (visible only for testing):

typedef enum {
	TOK_BRACE_OPEN,
	TOK_BRACE_CLOSE,
	TOK_LITERAL
} MMLTokenType;	

typedef struct {
	MMLTokenType type;
	// These make sense only if type is TOK_LITERAL
	const char* literal;
	uint length;
} MMLToken;	

// Converts string to array of tokens. 
// Newly created array must be freed elsewhere!
bool mml_tokenize(MMLObject* mml, const char* string, DArray* tokens);
// Constructs mml object from string and array of tokens.
bool mml_parse(MMLObject* mml, DArray* tokens);

// Filters out escape characters, returns new string length.
// 'out' string must be able to hold at least 'length' characters.
uint mml_remove_escapes(const char* in, uint length, char* out);

// Inserts escape charactes.
// Since output string size is unpredictable, it is appended to darray
// Qouting is also done here if needed.
uint mml_insert_escapes(const char* in, DArray* out);

#endif
