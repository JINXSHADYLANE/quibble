local game = {}

local levels = require('levels')

local level = nil
local tileset = nil

function game.init()
	tileset = tex.load(asset_dir..'tileset.png')
	game.reset(levels[1])
end

function game.close()
	if level then
		tilemap.free(level)
	end
end

function game.reset(desc)
	if level then
		tilemap.free(level)
	end

	local w, h = desc[1]:len(), #desc
	level = tilemap.new(tile_size, tile_size, w, h, 1)
	tilemap.set_tileset(level, 0, tileset)

	local ord_0 = string.byte('0')
	for y, line in ipairs(desc) do
		local x = 0
		for c in line:gmatch('.') do
			if c == ' ' then
				-- nothing
			elseif c == 's' then
				-- player start, center camera
				tilemap.set_camera(level, vec2(x * tile_size, y * tile_size), 1, 0)
			else
				-- tile
				local tile = c:byte() - ord_0
				tilemap.set_collision(level, x, y, true)
				tilemap.set_tile(level, x, y, 0, 0, tile)
			end

			x = x + 1
		end
	end
end

function game.update()
	if key.up(key.quit) or char.up('q') then
		states.pop()
	end
	return true
end

function game.render(t)
	if level then
		tilemap.render(level, scr_rect)
	end
	return true
end

return game

