#include "puzzles.h"
#include "common.h"
#include <memory.h>
#include <mml.h>

uint puzzle_count = 0;
PuzzleDesc* puzzle_descs = NULL; 

void puzzles_init(void) {
	char* desc_text = txtfile_read(PUZZLE_DESCS);
	if(!desc_text)
		LOG_ERROR("Unable to read puzzle description file");

	MMLObject desc;
	if(!mml_deserialize(&desc, desc_text))
		LOG_ERROR("Unable to deserialize particle description file");

	MEM_FREE(desc_text);	

	NodeIdx root = mml_root(&desc);

	if(strcmp(mml_get_name(&desc, root), "puzzles") != 0)
		LOG_ERROR("Invalid particle description file");
	
	puzzle_count = mml_count_children(&desc, root);
	puzzle_descs = (PuzzleDesc*)MEM_ALLOC(sizeof(PuzzleDesc) * puzzle_count);

	NodeIdx puzzle_desc = mml_get_first_child(&desc, root);
	uint i = 0;
	while(puzzle_desc) {
		if(strcmp(mml_get_name(&desc, puzzle_desc), "puzzle") != 0)
			LOG_ERROR("Bad puzzle description");
		
		// Name
		puzzle_descs[i].name = strclone(mml_getval_str(&desc, puzzle_desc));	
		
		// Tile size
		NodeIdx tile_size_node = mml_get_child(&desc, puzzle_desc, "tile_size");
		if(!tile_size_node)
			LOG_ERROR("Puzzle description is missing tile_size");
		const char* val = mml_getval_str(&desc, tile_size_node);
		float w, h;
		sscanf(val, "%f,%f", &w, &h);
		puzzle_descs[i].tile_size = vec2(w, h);
		
		// Texture
		NodeIdx texture_node = mml_get_child(&desc, puzzle_desc, "texture");
		if(!texture_node)	
			LOG_ERROR("Puzzle description is missing texture");	
		char path[64];
		strcpy(path, ASSETS_DIR);
		strcat(path, mml_getval_str(&desc, texture_node));
		puzzle_descs[i].image = tex_load(path);

		// Solved state
		uint tile_index, width = 0, height, j = 0, k;	
		NodeIdx solved_state_node = mml_get_child(&desc, puzzle_desc, "solved_state");
		if(!solved_state_node)
			LOG_ERROR("Puzzle description is missing solved_state");
		height = mml_count_children(&desc, solved_state_node);	
		if(height > 0) {

			// Some tricky code ahead:
			// To allocate exactly as much memory for solved state as needed
			// we must know puzzle width and height which can only be determined
			// by iterating trough tile indices once. Once there is a place where
			// to put solved state we must iterate again, read indices and put
			// them into newly allocated space.
			// Exactly same code can be used for both passes if we allow one
			// evil goto...

			puzzle_descs[i].solved = NULL;
			NodeIdx row_node;
		again:	
			row_node = mml_get_first_child(&desc, solved_state_node);
			j = 0;
			while(row_node) {
				// TODO: Find a way to avoid using strclone
				char* row_str = strclone(mml_getval_str(&desc, row_node));
				char* token = strtok(row_str, " ");
				k = 0;
				while(token) {
					sscanf(token, "%u", &tile_index);
					if(puzzle_descs[i].solved)
						puzzle_descs[i].solved[IDX_2D(k, j, width)] = tile_index;
					token = strtok(NULL, " ");
					k++;
				}
				assert(k > 0);
				MEM_FREE(row_str);
				if(width == 0)
					width = k;
				if(k != width)
					LOG_ERROR("Row width mismatch in solved state description");
				
				row_node = mml_get_next(&desc, row_node);
				j++;
			}
			assert(j == height);

			if(!puzzle_descs[i].solved) {
				puzzle_descs[i].solved = 
					(uint*)MEM_ALLOC(sizeof(uint) * width * height);
				goto again;	
			}
		}
		else { 
			// No solved state was provided, generate one which mimicks puzzle image 
			uint image_width, image_height;
			tex_size(puzzle_descs[i].image, &image_width, &image_height);
			width = image_width / (int)w;
			height = image_height / (int)h;

			assert(image_width % (int)w == 0);
			assert(image_height % (int)h == 0);

			puzzle_descs[i].solved = 
				(uint*)MEM_ALLOC(sizeof(uint) * width * height);
			for(; j < width * height; ++j)
				puzzle_descs[i].solved[j] = j;
		}

		if(width < 3 || height < 3)
			LOG_WARNING("Puzzle %s size is really weird..", puzzle_descs[i].name);

		puzzle_descs[i].width = width;
		puzzle_descs[i].height = height;
		
		puzzle_desc = mml_get_next(&desc, puzzle_desc);
	}
}

void puzzles_close(void) {
	for(uint i = 0; i < puzzle_count; ++i) {
		MEM_FREE(puzzle_descs[i].name);
		MEM_FREE(puzzle_descs[i].solved);
	}
	MEM_FREE(puzzle_descs);
}

