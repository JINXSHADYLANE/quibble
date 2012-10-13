local levels = {}

local ghost = require('ghost')

local exit_color = rgba(0.0, 0.4, 0.3)

levels[1] = {
	' S                  E '
}

levels[2] = {
	'#################',
	'###############E#',
	'#             # #',
	'#             # #',
	'#             # #',
	'#     S       # #',
	'#             # #',
	'#         #   # #',
	'#         #     #',
	'#################',
	'#################'
}

levels[3] = {
	'   ###############             ',
	' ####  #     #   #             ',
	'## E#    ### # # #             ',
	'#  #######   # # #             ',
	'#   #   #  ### # #             ',
	'##  # #     #  #               ',
	'#   # ####### ####             ',
	'# ###    #      #              ',
	'# #   #  ##    #               ',
	'# # #       # #                ',
	'#   ######## #              S  ',
	'#####                          '
}

levels[4] = {
	'###########################',
	'#                         #',
	'#               0  00     #',
	'#                00  0    #',
	'#               000   0   #',
	'#             0 0  0  0  1#',
	'#             0   0E  0   #',
	'#             0 0  0000   #',
	'#             0   0       #',
	'#   S         0000        #',
	'#                         #',
	'###########################'
}

local key_colors = {
	[1] = rgba(0.3, 0.1, 0.1),
	[3] = rgba(0.1, 0.3, 0.1),
	[5] = rgba(0.1, 0.1, 0.3)
}

local tiles = nil
local collision = nil
local ghosts = nil
local end_pos = nil

function levels.reset(n)
	tiles = {}
	collision = {}
	ghosts = {}
	local desc = levels[n]

	local w, h = desc[1]:len(), #desc

	local half_w = math.floor(w / 2)
	local half_h = math.floor(h / 2)

	local player_pos = nil

	-- iterate over lines
	for y, line in ipairs(desc) do
		collision[y-1 - half_h] = {}
		-- itarate over chars
		local x = 0
		for c in line:gmatch('.') do
			local p = vec2(x - half_w, y-1 - half_h)
			local n = tonumber(c)

			if c == '#' or (n ~= nil and math.fmod(n, 2) == 0) then
				local wall = {
					pos = p,
					away_pos = p + rand_vec2(4),
					away_rot = rand.float(-3 * math.pi, 3 * math.pi), 
					away_factor = 1.0,
					rand = rand.float(0, 1),
					number = n,
					visible = (n == 0 or n == nil)
				}
				table.insert(tiles, wall)
				if wall.visible then
					collision[p.y][p.x] = true
				end
			elseif (n ~= nil and math.fmod(n, 2) == 1) then
				collision[p.y][p.x] = n
				table.insert(ghosts, ghost.make(p, key_colors[n]))
			elseif c == 'S' then
				player_pos = p 
			elseif c == 'E' then
				end_pos = p
			end
			x = x + 1
		end
	end

	return player_pos
end

function levels.take_switch(n)
	for i, t in ipairs(tiles) do
		if t.number == n-1 then
			-- turn tile off
			t.visible = false
			collision[t.pos.y][t.pos.x] = nil
		end

		if t.number == n+1 then
			-- turn tile on
			t.visible = true
			collision[t.pos.y][t.pos.x] = true
		end
	end
end

function levels.is_solid(pos)
	if math.abs(pos.x) > 15 or math.abs(pos.y) > 10 then
		return true 
	end
	if collision[pos.y] then
		local c = collision[pos.y][pos.x]
		if type(c) == 'number' then
			levels.take_switch(c)
			collision[pos.y][pos.x] = nil
			return false
		end

		return collision[pos.y][pos.x]
	else
		return false
	end
end

function levels.update(player_pos)
	for i, t in ipairs(tiles) do
		local d = t.pos - player_pos
		local len_sq = length_sq(d)
		if len_sq < 4*4 then
			local len = length(d)
			local f = clamp(0, 1, len/2 - 1)/20
			t.away_factor = lerp(t.away_factor, f, 0.08)
		else
			t.away_factor = lerp(t.away_factor, 1, 0.08)
		end
	end

	if player_pos.x == end_pos.x and player_pos.y == end_pos.y then
		return true
	end

	return false
end

function levels.draw()
	local ts = time.s()

	-- ghost keys
	for i, g in ipairs(ghosts) do
		ghost.draw(g, ts)
	end

	-- walls
	for i, t in ipairs(tiles) do
		if t.visible and t.away_factor < 0.99 then
			local p = lerp(t.pos, t.away_pos, t.away_factor)
			local a = lerp(0, t.away_rot, t.away_factor)

			p = world2screen(p)

			local col = rgba(1, 1, 1, 1 - t.away_factor)

			sprsheet.draw_centered('wall_glow', glow_layer, p, a, 1, col)
			sprsheet.draw_centered('wall', tile_layer, p, a, 1, col)
		end
	end
	
	-- end
	local p = world2screen(end_pos)
	local s = 0.75 + (math.max(math.sin(ts*6), 0)^4)/4
	sprsheet.draw_centered('blob_small', ghost_layer, p, 0, s)
end

return levels
