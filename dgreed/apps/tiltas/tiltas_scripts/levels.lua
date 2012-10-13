local levels = {}

levels[1] = {
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

function levels.reset(n)
	tiles = {}
	collision = {}
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
			end
			if c == 'S' then
				player_pos = p 
			end
			x = x + 1
		end
	end

	return player_pos
end

function levels.is_solid(pos)
	return collision[pos.y][pos.x]
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
end

function levels.draw()
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
end

return levels
