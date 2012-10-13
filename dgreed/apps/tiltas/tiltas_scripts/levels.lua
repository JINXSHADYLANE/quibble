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
			if c == '#' then
				local wall = {
					pos = p,
					away_pos = p + rand_vec2(4),
					away_rot = rand.float(-4 * math.pi, 4 * math.pi), 
					away_factor = 1.0,
					rand = rand.float(0, 1)
				}
				table.insert(tiles, wall)
				collision[p.y][p.x] = true
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

function levels.is_solid(pos)
	if math.abs(pos.x) > 15 or math.abs(pos.y) > 10 then
		return true 
	end
	if collision[pos.y] then
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
		if t.away_factor < 0.99 then
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
