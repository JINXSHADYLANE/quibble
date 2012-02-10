local puzzles = {}

local function parse_puzzle(node)
	local new = {}
	assert(node.name == 'puzzle')
	new.name = node.value

	for i,child in ipairs(node.childs) do
		if child.name == 'img' then
			new.spr = child.value
		elseif child.name == 'def' then
			-- count newlines to determine height
			local line_count = 0
			for newline in child.value:gmatch('\n') do
				line_count = line_count + 1
			end

			-- parse individual tiles, put them in a table
			new.solved = {}
			for token in child.value:gmatch('([%d@#]+)%s+') do
				local parsed_token = tonumber(token)
				if not parsed_token then
					assert(token == '#' or token == '@')
					parsed_token = token
				end
				table.insert(new.solved, parsed_token)
			end

			-- calculate size
			local tile_count = #new.solved
			new.h = line_count - 1
			new.w = tile_count / new.h

			assert(tile_count >= 4)
			assert(math.floor(new.h) == new.h)
			assert(math.floor(new.w) == new.w)
			assert(tile_count == new.w * new.h)
		end
	end

	return new
end

function puzzles.load(filename)
	puzzles_mml = mml.read(filename)

	assert(puzzles_mml.name == 'puzzles')

	for i,puzzle in ipairs(puzzles_mml.childs) do
		table.insert(puzzles, parse_puzzle(puzzle))
	end
end

return puzzles
