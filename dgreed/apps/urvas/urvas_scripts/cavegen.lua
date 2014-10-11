local cavegen = {}

local empty_char = ' '
local solid_char = '#'
local empty_code = string.byte(empty_char)
local solid_code = string.byte(solid_char)

local function sample_map(map, w, h, x, y)
	if x > 0 and x <= w and y > 0 and y <= h then
		return string.byte(map[y], x) == solid_code 
	else
		return true
	end
end

local offsets_x = {-1, 0, 1, -1, 0, 1, -1, 0, 1}
local offsets_y = {-1, -1, -1, 0, 0, 0, 1, 1, 1}
local function sample_map3x3(map, w, h, x, y)
	local c = 0
	for i=1,9 do
		if sample_map(map, w, h, x + offsets_x[i], y + offsets_y[i]) then
			c = c + 1
		end
	end
	return c
end

local function smoothen_map(map, w, h)
	local new_map = {}
	local line = {}
	for i=1,h do
		for j=1,w do
			local c = empty_char
			if sample_map3x3(map, w, h, j, i) >= 5 then
				c = solid_char
			end
			line[j] = c
		end
		new_map[i] = table.concat(line)
	end
	return new_map
end

-- Cellular-automata based generator as described in roguebasin
-- returns array-table of 'height' strings, each 'width' chars long
function cavegen.make(width, height)
	local map = {}
	local line = {}
	for i=1,height do
		for j=1,width do
			local c = empty_char 
			if rand.int(0, 100) <= 45 then
				c = solid_char
			end
			line[j] = c
		end
		map[i] = table.concat(line)
	end

	-- do three rounds of cellular-automata smoothing
	for i=1,3 do
		map = smoothen_map(map, width, height)
	end

	-- copy to 2d array-table for easy modification
	local tmap = {}
	for y=1,height do
		local line = {}
		for x=1,width do
			line[x] = sample_map(map, width, height, x, y)
		end
		tmap[y] = line
	end

	-- mark islands
	local island_size = {}
	local island_randpt = {}
	local island_count = 0

	local function dfs(map, y, x, val)
		map[y][x] = val

		-- only choose non-border tiles for island connection pts
		if x > 1 and x < width and y > 1 and y < height then
			if island_size[val] == nil then
				island_size[val] = 1
			else
				island_size[val] = island_size[val] + 1
			end

			-- randomly choose one tile in an island with reservoir sampling
			if rand.int(0, island_size[val]) == 0 then
				island_randpt[val] = {x, y}
			end
		end

		if map[y+1] ~= nil and map[y+1][x] == false then
			dfs(map, y+1, x, val)
		end
		if map[y][x+1] == false then
			dfs(map, y, x+1, val)
		end
		if map[y-1] ~= nil and map[y-1][x] == false then
			dfs(map, y-1, x, val)
		end
		if map[y][x-1] == false then
			dfs(map, y, x-1, val)
		end
	end

	for y=1,height do
		for x=1,width do
			if tmap[y][x] == false then
				dfs(tmap, y, x, island_count)
				island_count = island_count + 1
			end
		end
	end

	-- connect disconnected islands
	local function dig_tunnel(map, start_x, start_y, end_x, end_y)
		local x_first = rand.int(0, 2) == 0
		if x_first then
			local x_dir = 1
			if start_x > end_x then
				x_dir = -1
			end
			for x=start_x,end_x,x_dir do
				map[start_y][x] = 0
			end
			local y_dir = 1
			if start_y > end_y then
				y_dir = -1
			end
			for y=start_y,end_y,y_dir do
				map[y][end_x] = 0
			end
		else
			local y_dir = 1
			if start_y > end_y then
				y_dir = -1
			end
			for y=start_y,end_y,y_dir do
				map[y][start_x] = 0
			end
			local x_dir = 1
			if start_x > end_x then
				x_dir = -1
			end
			for x=start_x,end_x,x_dir do
				map[end_y][x] = 0
			end
		end
	end

	for i=1,island_count-1 do
		local s = island_randpt[i-1]
		local e = island_randpt[i]
		dig_tunnel(tmap, s[1], s[2], e[1], e[2])
	end

	-- make sure borders are solid wall
	for y=1,height do
		tmap[y][1] = true
		tmap[y][width] = true
	end
	for x=1,width do
		tmap[1][x] = true
		tmap[height][x] = true
	end

	-- place player, stairs and spellbook
	local n = 1
	local player_pt = nil
	local stairs_pt = nil
	local book_pt = nil
	for y=1,height do
		for x=1,width do
			if tmap[y][x] ~= true then
				if rand.int(0, n) == 0 then
					player_pt = {x, y}
				end
				if rand.int(0, n) == 0 then
					stairs_pt = {x, y}
				end
				if rand.int(0, n) == 0 then
					book_pt = {x, y}
				end
				n = n + 1
			end
		end
	end
	assert(player_pt)
	assert(stairs_pt)

	-- write map back to strs
	for y=1,height do
		for x=1,width do
			local c = empty_char
			if tmap[y][x] == true then
				c = solid_char
			else
				if x == player_pt[1] and y == player_pt[2] then
					c = '@'
				end
				if x == stairs_pt[1] and y == stairs_pt[2] then
					c = '>'
				end
				if x == book_pt[1] and y == book_pt[2] then
					c = '?'
				end
			end
			line[x] = c
		end
		map[y] = table.concat(line)
	end

	return map
end

return cavegen
