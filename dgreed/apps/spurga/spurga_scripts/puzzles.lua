local puzzles = {}

local function parse_puzzle(node)
	local new = {}
	assert(node.name == 'puzzle')
	new.name = node.value

	for i,child in ipairs(node.childs) do
		if child.name == 'img' then
			new.spr = child.value
		elseif child.name == 'par' then
			new.par = tonumber(child.value)
		elseif child.name == 'def' then
			-- count newlines to determine height
			local line_count = 0
			for newline in child.value:gmatch('\n') do
				line_count = line_count + 1
			end

			-- parse individual tiles, put them in a table
			new.solved = {}
			for token in child.value:gmatch('([%d%a]+)%s+') do
				local parsed_token = tonumber(token)
				if not parsed_token then
					assert(is_wall(token) or is_portal(token))
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
		elseif child.name == 'map' then
			local map = {}
			for key,val in child.value:gmatch('(%d+) = (%a+)') do
				map[tonumber(key)] = val
			end
			new.map = map
		end
	end

	return new
end

function puzzles.load(descs_filename, slices_filename)
	local puzzles_mml = mml.read(descs_filename)
	assert(puzzles_mml.name == 'puzzles')
	for i,puzzle in ipairs(puzzles_mml.childs) do
		local p = parse_puzzle(puzzle)
		puzzles[p.name] = p
	end

	-- also parse puzzle slices
	local slices = {}
	local slices_mml = mml.read(slices_filename)
	assert(slices_mml.name == 'puzzle_tiles')
	for i,puzzle in ipairs(slices_mml.childs) do
		local sliced_puzzle = {}	
		for j,slice in ipairs(puzzle.childs) do
			local t = slice.name
			if not is_wall(t) and not is_portal(t) then
				t = tonumber(t)
			end

			local _l, _t, _r, _b = slice.value:match('(%d+),(%d+),(%d+),(%d+)')

			sliced_puzzle[t] = rect(
				tonumber(_l), tonumber(_t), 
				tonumber(_r), tonumber(_b)
			)
	
			if not sliced_puzzle.tile_w then
				sliced_puzzle.tile_w = width(sliced_puzzle[t])
				sliced_puzzle.tile_h = height(sliced_puzzle[t])
			end
		end

		sliced_puzzle.texture = puzzle.value
		slices[puzzle.name] = sliced_puzzle
	end

	puzzles.slices = slices
end

function puzzles.free()
	for name,puzzle in pairs(puzzles) do
		if type(puzzle) == 'table' and puzzle.tex then
			tex.free(puzzle.tex)
		end
	end
end

function puzzles.preload(puzzle)
	-- figures out puzzle.tex and puzzle.tile_src
	local slices = puzzles.slices[puzzle.name]
	assert(slices)

	puzzle.tile_src = slices

	if retina then
		puzzle.tex = tex.load(pre..(slices.texture:gsub('.png', 'hd.png')))
		tex.scale(puzzle.tex, 0.5)
	else
		puzzle.tex = tex.load(pre..slices.texture)
	
	end

	puzzle.tile_w = slices.tile_w
	puzzle.tile_h = slices.tile_h
end

function puzzles.get_next(puzzle)
	local levels_puzzle = puzzles['levels']
	local m = levels_puzzle.map
	for i,p in pairs(m) do
		if p == puzzle.name then
			if m[i+1] then 
				return puzzles[m[i+1]]
			else
				-- finished last puzzle !
				return puzzles[m[0]]
			end
		end
	end
end

function puzzles.unlock_next(puzzle)
	local levels_puzzle = puzzles['levels']

	local m = levels_puzzle.map
	for i,p in pairs(m) do
		if p == puzzle.name then
			if m[i+1] then 
				keyval.set('pulock:'..m[i+1], true)
			end

			if m[i+2] then 
				keyval.set('pulock:'..m[i+2], true)
			end

			break
		end
	end
end

return puzzles
