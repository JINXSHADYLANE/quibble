// vim: set ft=C

#include "mml.h"

TEST_(tokenize_simple) {
	MMLObject mml;
	mml_empty(&mml);
	DArray tokens;

	const char* str = "( foo bar)(baz \t ) ";
	ASSERT_(mml_tokenize(&mml, str, &tokens));

	MMLToken* t = DARRAY_DATA_PTR(tokens, MMLToken);

	ASSERT_(tokens.size == 7);

	// (
	ASSERT_(t[0].type == TOK_BRACE_OPEN);
	// foo
	ASSERT_(t[1].type == TOK_LITERAL);
	ASSERT_(t[1].literal == &str[2]);
	ASSERT_(t[1].length == 3);
	// bar
	ASSERT_(t[2].type == TOK_LITERAL)
	ASSERT_(t[2].literal == &str[6]);
	ASSERT_(t[2].length == 3);
	// )
	ASSERT_(t[3].type == TOK_BRACE_CLOSE);
	// (
	ASSERT_(t[4].type == TOK_BRACE_OPEN);
	// baz
	ASSERT_(t[5].type == TOK_LITERAL);
	ASSERT_(t[5].literal = &str[11]);
	ASSERT_(t[5].length == 3);
	// )
	ASSERT_(t[6].type == TOK_BRACE_CLOSE);

	darray_free(&tokens);		
	
	mml_free(&mml);
}	

// Helper to check equality of two not-null-terminated strings.
// It is presumed that their length is equal (no point to check otherwise).
bool _streql(const char* str1, const char* str2, uint len) {
	while(len--) {
		if(str1[len] != str2[len])
			return false;
	}
	return true;
}	

TEST_(tokenize_qoutes) {
	// mml_tokenzine uses this only for error reporting,
	// su using same MMLObject for multiple calls is OK
	MMLObject mml;
	mml_empty(&mml);
	DArray tokens1, tokens2;

	const char* str1 = "(foo bar)(foo bar)";
	const char* str2 = "(\"foo\" bar)(\"foo\"\"bar\")";

	ASSERT_(mml_tokenize(&mml, str1, &tokens1));
	ASSERT_(mml_tokenize(&mml, str2, &tokens2));

	ASSERT_(tokens1.size == 8);
	ASSERT_(tokens1.size == tokens2.size);

	MMLToken* tok1_data = DARRAY_DATA_PTR(tokens1, MMLToken);
	MMLToken* tok2_data = DARRAY_DATA_PTR(tokens2, MMLToken);

	int i;
	for(i = 0; i < 8; ++i) {
		ASSERT_(tok1_data[i].type == tok2_data[i].type);
		if(tok1_data[i].type == TOK_LITERAL) {
			ASSERT_(tok1_data[i].length == tok2_data[i].length);
			ASSERT_(_streql(tok1_data[i].literal, tok2_data[i].literal,
				tok1_data[i].length));
		}
	}

	darray_free(&tokens1);
	darray_free(&tokens2);

	mml_free(&mml);
}	

TEST_(tokenize_qoutes_adv) {
	MMLObject mml;
	mml_empty(&mml);
	DArray tokens;

	const char* str = "\"foo bar\" \"bar\nbaz\" () \"foo( )bar\"";

	ASSERT_(mml_tokenize(&mml, str, &tokens));

	MMLToken* t = DARRAY_DATA_PTR(tokens, MMLToken);

	ASSERT_(tokens.size == 5);

	// "foo bar"
	ASSERT_(t[0].type == TOK_LITERAL);
	ASSERT_(t[0].length == 7);
	ASSERT_(_streql(t[0].literal, "foo bar", 7));
	// "bar\nbaz"
	ASSERT_(t[1].type == TOK_LITERAL);
	ASSERT_(t[1].length == 7);
	ASSERT_(_streql(t[1].literal, "bar\nbaz", 7));
	// (
	ASSERT_(t[2].type == TOK_BRACE_OPEN);
	// )
	ASSERT_(t[3].type == TOK_BRACE_CLOSE);
	// "foo( )bar"
	ASSERT_(t[4].type == TOK_LITERAL);
	ASSERT_(t[4].length == 9);
	ASSERT_(_streql(t[4].literal, "foo( )bar", 9)); 

	darray_free(&tokens);

	mml_free(&mml);
}		

TEST_(tokenize_comments) {
	MMLObject mml;
	mml_empty(&mml);
	DArray tokens1, tokens2;

	const char* str1 = "(foo bar)(foo bar (bar baz))(foobar)";
	const char* str2 = " #comment1\n(# comment2\nfoo bar)\n"
					   "( foo bar (bar baz))(foobar)\n#  comment3\n";

	ASSERT_(mml_tokenize(&mml, str1, &tokens1));
	ASSERT_(mml_tokenize(&mml, str2, &tokens2));

	MMLToken* tok1_data = DARRAY_DATA_PTR(tokens1, MMLToken);
	MMLToken* tok2_data = DARRAY_DATA_PTR(tokens2, MMLToken);

	ASSERT_(tokens1.size == tokens2.size);
	ASSERT_(tokens1.size == 15);

	int i;
	for(i = 0; i < 15; ++i) {
		ASSERT_(tok1_data[i].type == tok2_data[i].type);
		if(tok1_data[i].type == TOK_LITERAL) {
			ASSERT_(tok1_data[i].length == tok2_data[i].length);
			ASSERT_(_streql(tok1_data[i].literal, tok2_data[i].literal,
				tok1_data[i].length));
		}
	}		

	darray_free(&tokens1);
	darray_free(&tokens2);

	mml_free(&mml);
}	

TEST_(tokenize_comments_adv) {
	MMLObject mml;
	mml_empty(&mml);
	DArray tokens;

	const char* str = "(\"# comment in quoted literal\" \"foobar\")";

	ASSERT_(mml_tokenize(&mml, str, &tokens));

	MMLToken* t = DARRAY_DATA_PTR(tokens, MMLToken);

	ASSERT_(tokens.size == 4);

	// (
	ASSERT_(t[0].type == TOK_BRACE_OPEN);
	// "# comment in quoted literal"
	ASSERT_(t[1].type == TOK_LITERAL);
	ASSERT_(t[1].length == 27);
	ASSERT_(_streql(t[1].literal, "# comment in quoted literal", 27));
	// "foobar"
	ASSERT_(t[2].type == TOK_LITERAL);
	ASSERT_(t[2].length == 6);
	ASSERT_(_streql(t[2].literal, "foobar", 6));
	// )
	ASSERT_(t[3].type == TOK_BRACE_CLOSE);

	darray_free(&tokens);

	mml_free(&mml);
}	

TEST_(remove_escapes) {
	char out[64];

	const char* str1 = "simple string, no escapes";
	ASSERT_(mml_remove_escapes(str1, 25, out) == 25);
	ASSERT_(_streql(str1, out, 25));

	const char* str2 = "\\\" \\\\ \\n \\r \\t \\b \\a \\ ";
	const char* str2_correct = "\" \\ \n \r \t \b a  ";
	ASSERT_(mml_remove_escapes(str2, 23, out) == 15);
	ASSERT_(_streql(str2_correct, out, 15)); 
}	

TEST_(tokenizer_errors) {
	MMLObject mml;
	mml_empty(&mml);

	DArray tokens1, tokens2, tokens3;

	const char* str1 = "(foo bar) baz";
	ASSERT_(mml_tokenize(&mml, str1, &tokens1) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"TOKENIZER: Unexpected literal in the end") == 0);
	
	const char* str2 = "(foo \"bar)";
	ASSERT_(mml_tokenize(&mml, str2, &tokens2) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"TOKENIZER: Open qouted literal in the end") == 0);

	const char* str3 = "(foo bar) #baz";
	ASSERT_(mml_tokenize(&mml, str3, &tokens3) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"TOKENIZER: No newline after last comment") == 0);

	darray_free(&tokens1);
	darray_free(&tokens2);
	darray_free(&tokens3);

	mml_free(&mml);
}	

TEST_(parser_simple) {
	const char* str = "(foo bar (bar baz))";

	MMLObject mml;
	ASSERT_(mml_deserialize(&mml, str));

	NodeIdx root_idx = mml_root(&mml);
	ASSERT_(strcmp(mml_get_name(&mml, root_idx), "foo") == 0);
	ASSERT_(strcmp(mml_getval_str(&mml, root_idx), "bar") == 0);

	NodeIdx node1_idx = mml_get_first_child(&mml, root_idx);
	ASSERT_(node1_idx);
	ASSERT_(strcmp(mml_get_name(&mml, node1_idx), "bar") == 0);
	ASSERT_(strcmp(mml_getval_str(&mml, node1_idx), "baz") == 0);
	ASSERT_(mml_get_next(&mml, node1_idx) == 0);
	ASSERT_(mml_get_first_child(&mml, node1_idx) == 0);

	mml_free(&mml);
}	

TEST_(parser_adv) {
	const char* str = 
	"(root _ \n"
	"	(node1 3\n"
	"		(i -2)\n"
	"		(u 65537)\n"
	"		(b true)\n"
	"		(f 3.14)\n"
	"	) # type1 end\n"
	"	(node2 \"foo bar baz\")\n"
	"	(node3 \"multiline\n"
	"literal\") # multiline test\n"
	")\n";

	MMLObject mml;
	ASSERT_(mml_deserialize(&mml, str));

	NodeIdx root_idx = mml_root(&mml);
	ASSERT_(strcmp(mml_get_name(&mml, root_idx), "root") == 0);
	ASSERT_(strcmp(mml_getval_str(&mml, root_idx), "_") == 0);

	NodeIdx node1_idx = mml_get_first_child(&mml, root_idx);
	ASSERT_(node1_idx);
	ASSERT_(strcmp(mml_get_name(&mml, node1_idx), "node1") == 0);
	ASSERT_(strcmp(mml_getval_str(&mml, node1_idx), "3") == 0);
	ASSERT_(mml_getval_uint(&mml, node1_idx) == 3);

	NodeIdx i_idx = mml_get_first_child(&mml, node1_idx);
	ASSERT_(i_idx);
	ASSERT_(strcmp(mml_get_name(&mml, i_idx), "i") == 0);
	ASSERT_(mml_getval_int(&mml, i_idx) == -2);
	ASSERT_(mml_get_first_child(&mml, i_idx) == 0);

	NodeIdx u_idx = mml_get_next(&mml, i_idx);
	ASSERT_(u_idx);
	ASSERT_(strcmp(mml_get_name(&mml, u_idx), "u") == 0);
	ASSERT_(mml_getval_uint(&mml, u_idx) == 65537);
	ASSERT_(mml_get_first_child(&mml, u_idx) == 0);
	
	NodeIdx b_idx = mml_get_next(&mml, u_idx);
	ASSERT_(b_idx);
	ASSERT_(strcmp(mml_get_name(&mml, b_idx), "b") == 0);
	ASSERT_(mml_getval_bool(&mml, b_idx) == true);
	ASSERT_(mml_get_first_child(&mml, b_idx) == 0);
	
	NodeIdx f_idx = mml_get_next(&mml, b_idx);
	ASSERT_(f_idx);
	ASSERT_(strcmp(mml_get_name(&mml, f_idx), "f") == 0);
	ASSERT_(abs(mml_getval_int(&mml, f_idx) - 3.14f) < 0.0001f);
	ASSERT_(mml_get_first_child(&mml, f_idx) == 0);
	ASSERT_(mml_get_next(&mml, f_idx) == 0);

	NodeIdx node2_idx = mml_get_next(&mml, node1_idx);
	ASSERT_(node2_idx);
	ASSERT_(strcmp(mml_get_name(&mml, node2_idx), "node2") == 0);
	ASSERT_(strcmp(mml_getval_str(&mml, node2_idx), "foo bar baz") == 0);
	ASSERT_(mml_get_first_child(&mml, node2_idx) == 0);

	NodeIdx node3_idx = mml_get_next(&mml, node2_idx);
	ASSERT_(node3_idx);
	ASSERT_(strcmp(mml_get_name(&mml, node3_idx), "node3") == 0);
	ASSERT_(strcmp(mml_getval_str(&mml, node3_idx), "multiline\nliteral") == 0);
	ASSERT_(mml_get_first_child(&mml, node3_idx) == 0);
	ASSERT_(mml_get_next(&mml, node3_idx) == 0);
	
	mml_free(&mml);
}	

TEST_(parser_stress) {
	char str[9 * 1000 + 1];

	// Procedurally construct mml string
	uint i;
	for(i = 0; i < 1000; ++i) {
		sprintf(&str[i*8], "(%03u %03u", i, 999 - i);
		str[8999 - i] = ')';
	}
	str[9000] = '\0';

	MMLObject mml; 
	ASSERT_(mml_deserialize(&mml, str));	

	NodeIdx idx = mml_root(&mml);
	char num1[4];
	for(i = 0; i < 1000; ++i) {
		// root node has idx 0, everything else must differ
		if(i != 0) {
			ASSERT_(idx);
		}	
		sprintf(num1, "%03u", i);
		ASSERT_(strcmp(mml_get_name(&mml, idx), num1) == 0);
		ASSERT_(mml_getval_uint(&mml, idx) == 999 - i);
		ASSERT_(mml_get_next(&mml, idx) == 0);

		idx = mml_get_first_child(&mml, idx);
	}	
	ASSERT_(idx == 0);

	mml_free(&mml);
}	

TEST_(find_child) {
	const char* str = 
	"(root _\n"
	"	(jurgis 0)\n"
	"	(antanas 1)\n"
	"	(aloyzas 2)\n"
	"	(martynas 3)\n"
	"	(antanas 4)\n"
	")";

	MMLObject mml; 
	ASSERT_(mml_deserialize(&mml, str));	

	NodeIdx root_idx = mml_root(&mml);

	NodeIdx idx = mml_get_child(&mml, root_idx, "martynas");
	ASSERT_(idx);
	ASSERT_(strcmp(mml_get_name(&mml, idx), "martynas") == 0);
	ASSERT_(mml_getval_uint(&mml, idx) == 3);
	ASSERT_(mml_get_first_child(&mml, idx) == 0);

	idx = mml_get_child(&mml, root_idx, "jurgis");
	ASSERT_(idx);
	ASSERT_(strcmp(mml_get_name(&mml, idx), "jurgis") == 0);
	ASSERT_(mml_getval_uint(&mml, idx) == 0);
	ASSERT_(mml_get_first_child(&mml, idx) == 0);

	idx = mml_get_child(&mml, root_idx, "onute");
	ASSERT_(idx == 0);

	idx = mml_get_child(&mml, root_idx, "antanas");
	ASSERT_(idx);
	ASSERT_(strcmp(mml_get_name(&mml, idx), "antanas") == 0);
	ASSERT_(mml_getval_uint(&mml, idx) == 1);
	ASSERT_(mml_get_first_child(&mml, idx) == 0);

	mml_free(&mml);
}	

TEST_(parser_errors) {
	MMLObject mml;

	const char* str1 = "(foobar)";
	ASSERT_(mml_deserialize(&mml, str1) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"PARSER: Node must contain at least 4 tokens") == 0);

	const char* str2 = "(foo bar(";	
	ASSERT_(mml_deserialize(&mml, str2) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"PARSER: Node must begin and end with proper braces") == 0);

	const char* str3 = "(foo ) )";	
	ASSERT_(mml_deserialize(&mml, str3) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"PARSER: Node must have name and value") == 0);

	const char*	str4 = "(baz baz baz)";
	ASSERT_(mml_deserialize(&mml, str4) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"PARSER: Unexpected literal") == 0);

	const char* str5 = "(foo bar ) )";	
	ASSERT_(mml_deserialize(&mml, str5) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"PARSER: Unexpected closing brace") == 0);

	const char* str6 = "(foo bar (bar baz )";	
	ASSERT_(mml_deserialize(&mml, str6) == false);
	ASSERT_(strcmp(mml_last_error(&mml),
		"PARSER: Misplaced brace") == 0);

}

TEST_(append_and_insert) {
	MMLObject mml;

	mml_empty(&mml);

	NodeIdx root_idx = mml_root(&mml);	

	NodeIdx node1_idx = mml_node(&mml, "node1", "");
	NodeIdx node2_idx = mml_node(&mml, "node2", "");
	NodeIdx node3_idx = mml_node(&mml, "node3", "");
	NodeIdx node4_idx = mml_node(&mml, "node4", "");

	mml_append(&mml, root_idx, node1_idx);
	mml_append(&mml, root_idx, node3_idx);
	ASSERT_(mml_insert_after(&mml, root_idx, node2_idx, "node1"));
	ASSERT_(mml_insert_after(&mml, root_idx, node4_idx, "node3"));

	ASSERT_(mml_get_first_child(&mml, root_idx) == node1_idx);
	ASSERT_(mml_get_next(&mml, node1_idx) == node2_idx);
	ASSERT_(mml_get_next(&mml, node2_idx) == node3_idx);
	ASSERT_(mml_get_next(&mml, node3_idx) == node4_idx);
	ASSERT_(mml_get_next(&mml, node4_idx) == 0);

	mml_free(&mml);
}	

TEST_(remove) {
	const char* str = 
	"(root _\n"
	"	(pabudo 0)\n"
	"	(ryta 1)\n"
	"	(kukutis 2)\n"
	"	(ir 3)\n"
	"	(mato 4)\n"
	"	(jis 5)\n"
	"	(pats 6)\n"
	"	(guli 7)\n"
	"	(salia 8)\n"
	"	(nebegyvas 9)\n"
	")\n";

	MMLObject mml;
	ASSERT_(mml_deserialize(&mml, str));

	NodeIdx root_idx = mml_root(&mml);

	ASSERT_(mml_remove_child(&mml, root_idx, "pats")); 
	ASSERT_(mml_remove_child(&mml, root_idx, "salia")); 
	ASSERT_(mml_remove_child(&mml, root_idx, "pabudo")); 
	ASSERT_(mml_remove_child(&mml, root_idx, "kukutis")); 
	ASSERT_(mml_remove_child(&mml, root_idx, "mato")); 
	ASSERT_(mml_remove_child(&mml, root_idx, "troba") == false);

	uint i = 5, n = 1;
	NodeIdx child = mml_get_first_child(&mml, root_idx);
	while(i--) {
		ASSERT_(child);
		ASSERT_(mml_getval_uint(&mml, child) == n);
		n += 2;
		child = mml_get_next(&mml, child);
	}	
	ASSERT_(child == 0);

	mml_free(&mml);
}	

TEST_(insert_escapes) {
	DArray out;
	out = darray_create(sizeof(char), 0);
	char* out_str = DARRAY_DATA_PTR(out, char);

	ASSERT_(mml_insert_escapes("simple", &out) == 6);
	ASSERT_(_streql(out_str, "simple", 6));
	out.size = 0;

	ASSERT_(mml_insert_escapes("\n \r \t \b \" \\", &out) == 19);
	ASSERT_(_streql(out_str, "\"\\n \\r \\t \\b \\\" \\\\\"", 19));
	out.size = 0;

	ASSERT_(mml_insert_escapes("foo bar", &out) == 9);
	ASSERT_(_streql(out_str, "\"foo bar\"", 9));

	darray_free(&out);
}	

TEST_(serialize_simple) {
	
	MMLObject mml;
	mml_empty(&mml);

	NodeIdx root_idx = mml_root(&mml);

	NodeIdx node1_idx = mml_node(&mml, "nieko", "0");
	NodeIdx node2_idx = mml_node(&mml, "broliai", "1");
	NodeIdx node3_idx = mml_node(&mml, "are", "2");
	NodeIdx node4_idx = mml_node(&mml, "rudenio", "3");
	NodeIdx node5_idx = mml_node(&mml, "zemele", "4");

	mml_append(&mml, root_idx, node1_idx);
	mml_append(&mml, root_idx, node2_idx);
	mml_append(&mml, root_idx, node3_idx);
	mml_append(&mml, root_idx, node4_idx);
	mml_append(&mml, root_idx, node5_idx);

	char* out = NULL;
	const char* correct_out = 
	"( root _\n"
	"    ( nieko 0 )\n"
	"    ( broliai 1 )\n"
	"    ( are 2 )\n"
	"    ( rudenio 3 )\n"
	"    ( zemele 4 )\n"
	")\n";

	out = mml_serialize(&mml);
	ASSERT_(out);
	ASSERT_(strcmp(out, correct_out) == 0);

	MEM_FREE(out);

	mml_free(&mml);
}

TEST_(serialize_adv) {
	MMLObject mml;
	mml_empty(&mml);

	NodeIdx root_idx = mml_root(&mml);

	NodeIdx node1_idx = mml_node(&mml, "node1", "foo bar");
	NodeIdx node2_idx = mml_node(&mml, "node2", "\"foobar\"");
	NodeIdx node3_idx = mml_node(&mml, "node3", "\tfoo\n"
												"\tbar");
	NodeIdx node4_idx = mml_node(&mml, "foo", "bar");
	NodeIdx node5_idx = mml_node(&mml, "bar", "baz");

	mml_append(&mml, node3_idx, node4_idx);
	mml_append(&mml, node3_idx, node5_idx);

	mml_append(&mml, root_idx, node1_idx);
	mml_append(&mml, root_idx, node2_idx);
	mml_append(&mml, root_idx, node3_idx);

	char* out = NULL;
	const char* correct_out =
	"( root _\n"
	"    ( node1 \"foo bar\" )\n"
	"    ( node2 \"\\\"foobar\\\"\" )\n"
	"    ( node3 \"\\tfoo\\n\\tbar\"\n"
	"        ( foo bar )\n"
	"        ( bar baz )\n"
	"    )\n"
	")\n";

	out = mml_serialize(&mml);
	ASSERT_(out);
	ASSERT_(strcmp(out, correct_out) == 0);

	MEM_FREE(out);

	mml_free(&mml);
}	

TEST_(serialize_compact) {
	MMLObject mml;
	mml_empty(&mml);

	NodeIdx root_idx = mml_root(&mml);

	NodeIdx node1_idx = mml_node(&mml, "foo", "bar");
	NodeIdx node2_idx = mml_node(&mml, "bar", "foo");
	NodeIdx node3_idx = mml_node(&mml, "bar", "baz");
	NodeIdx node4_idx = mml_node(&mml, "baz", "bar");

	mml_append(&mml, node2_idx, node4_idx);
	mml_append(&mml, root_idx, node1_idx);
	mml_append(&mml, root_idx, node2_idx);
	mml_append(&mml, root_idx, node3_idx);

	char* out = NULL;
	const char* correct_out = "(root _(foo bar)(bar foo(baz bar))(bar baz))";
	out = mml_serialize_compact(&mml);
	ASSERT_(out);
	ASSERT_(strcmp(out, correct_out) == 0);

	MEM_FREE(out);

	mml_free(&mml);
}	

TEST_(get_sibling) {
	const char* str = 
	"(root _\n"
	"	(peleseis 0)\n"
	"	(ir 1)\n"
	"	(kerpem 2)\n"
	"	(apaugus 3)\n"
	"	(aukstai 4)\n"
	"	(traku 5)\n"
	"	(stai 6)\n"
	"	(garbinga 7)\n"
	"	(pilis 8)\n"
	")\n";

	MMLObject mml;
	ASSERT_(mml_deserialize(&mml, str));

	NodeIdx root = mml_root(&mml);

	NodeIdx node1 = mml_get_first_child(&mml, root);
	ASSERT_(node1);
	ASSERT_(mml_getval_uint(&mml, node1) == 0);

	NodeIdx node2 = mml_get_sibling(&mml, node1, "kerpem");
	ASSERT_(node2);
	ASSERT_(mml_getval_uint(&mml, node2) == 2);

	ASSERT_(mml_get_sibling(&mml, node2, "ir") == 0);

	ASSERT_(mml_get_sibling(&mml, node2, "traku") ==
		mml_get_sibling(&mml, node1, "traku"));

	node1 = mml_get_sibling(&mml, node2, "garbinga");	
	ASSERT_(node1);
	ASSERT_(mml_getval_uint(&mml, node1) == 7);

	node2 = mml_get_sibling(&mml, node1, "pilis");
	ASSERT_(node2);
	ASSERT_(mml_getval_uint(&mml, node2) == 8);

	ASSERT_(mml_get_sibling(&mml, node2, "pilis") == 0);
	ASSERT_(mml_get_sibling(&mml, node2, "valdovus") == 0);

	mml_free(&mml);
}	

TEST_(count_children) {
	const char* str = 
	"(root _\n"
	"	(kai 0)\n"
	"	(sirpsta 1\n"
	"		(vysnios 1.0 (_ _))\n"
	"		(suvalkijoj 1.1\n"
	"			(raudonos 1.1.0)\n"
	"			(kad 1.1.1)\n"
	"			(pravirkt 1.1.2)\n"
	"			(gali 1.1.3)\n"
	"		)\n"
	"	)\n"
	")\n";

	MMLObject mml;
	ASSERT_(mml_deserialize(&mml, str));

	NodeIdx root = mml_root(&mml);
	NodeIdx kai = mml_get_child(&mml, root, "kai");
	ASSERT_(kai);
	NodeIdx sirpsta = mml_get_sibling(&mml, kai, "sirpsta");
	ASSERT_(sirpsta);
	NodeIdx vysnios = mml_get_child(&mml, sirpsta, "vysnios");
	ASSERT_(vysnios);
	NodeIdx suvalkijoj = mml_get_child(&mml, sirpsta, "suvalkijoj");
	ASSERT_(suvalkijoj);

	ASSERT_(mml_count_children(&mml, root) == 2);
	ASSERT_(mml_count_children(&mml, kai) == 0);
	ASSERT_(mml_count_children(&mml, sirpsta) == 2);
	ASSERT_(mml_count_children(&mml, suvalkijoj) == 4);
	ASSERT_(mml_count_children(&mml, vysnios) == 1);

	mml_free(&mml);
}

TEST_(long_string) {
	MMLObject mml;
	mml_empty(&mml);

	char str[1000];
	for(uint i = 0; i < 999; ++i)
		str[i] = 'a';
	str[999] = '\0';

	NodeIdx root_idx = mml_root(&mml);

	NodeIdx node1_idx = mml_node(&mml, "foo", str);
	NodeIdx node2_idx = mml_node(&mml, "bar", "foo");
	NodeIdx node3_idx = mml_node(&mml, "bar", str);
	NodeIdx node4_idx = mml_node(&mml, "baz", "bar");

	mml_append(&mml, node2_idx, node4_idx);
	mml_append(&mml, root_idx, node1_idx);
	mml_append(&mml, root_idx, node2_idx);
	mml_append(&mml, root_idx, node3_idx);

	char* out = NULL;
	char correct_out[3000];
	sprintf(correct_out, "(root _(foo %s)(bar foo(baz bar))(bar %s))", str, str);
	out = mml_serialize_compact(&mml);
	ASSERT_(out);
	ASSERT_(strcmp(out, correct_out) == 0);

	MEM_FREE(out);

	mml_free(&mml);
}

