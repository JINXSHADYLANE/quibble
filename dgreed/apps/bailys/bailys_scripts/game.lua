local game = {}

local levels = require('levels')

local bg_color = rgba(0.92, 0.9, 0.85, 1)

-- game state:

-- logical player position
local player_pos = nil

-- visual player position, with nice 
-- interpolations and everything
local player_draw_pos = nil

-- list of grid space vec2s
local eggs = nil

-- flattened 2d list of level tiles
local lvl = nil

-- game logic:

function game.init()
	game.load_level(levels[1])
end

function game.close()
end

function game.update()
	-- exit game if esc or q was pressed
	if key.down(key.quit) or char.down('q') then
		return false
	end

	return true
end

function game.render(t)
	-- slightly off-white background
	sprsheet.draw('empty', 0, scr_rect, bg_color)

	game.draw_level(1)
	
	return true
end

-- parses levels and preps everything up for rendering
-- lvldesc is one of the levels defined in levels.lua
function game.load_level(lvldesc)
	-- reset game state
	player_pos = nil
	player_draw_pos = nil
	eggs = {}
	lvl = {}

	local w, h = lvldesc[1]:len(), #lvldesc
	level_size = vec2(w, h)

	for y, line in ipairs(lvldesc) do
		local x = 0
		for c in line:gmatch('.') do
			-- grid space pos of this tile
			local pos = vec2(x, y-1)

			if c == 'S' then
				-- player start
				player_pos = pos
				player_draw_pos = pos
			elseif c == 'e' then
				-- egg
				table.insert(eggs, pos)
			elseif c == '#' or c == 'l' or c == 'r' then
				-- wall
				lvl[grid_idx(pos.x, pos.y)] = c 
			end

			x = x + 1
		end
	end
end

-- draws level geometry - walls, mirrors 
-- and outer edges/corners
function game.draw_level(layer)
	local nx, ny = level_size.x, level_size.y

	-- draw inner tiles
	for y=0,ny-1 do
		for x=0,nx-1 do
			local screen_pos = grid2screen(vec2(x, y))
			local tile = lvl[grid_idx(x, y)]
			if tile == nil then
				sprsheet.draw_centered('tile', layer, screen_pos)
			elseif tile == '#' then
				sprsheet.draw_centered('wall', layer, screen_pos)
			elseif tile == 'l' then
				sprsheet.draw_centered('mirror', layer, screen_pos)
			elseif tile == 'r' then
				sprsheet.draw_centered('mirror', layer, screen_pos, math.pi/2)
			end
		end
	end

	-- draw outer stuff
	
	-- top edge
	for x=0,nx-1 do
		local screen_pos = grid2screen(vec2(x, -1))
		sprsheet.draw_centered('edge', layer, screen_pos)
	end
	-- bottom edge
	for x=0,nx-1 do
		local screen_pos = grid2screen(vec2(x, ny))
		sprsheet.draw_centered('edge', layer, screen_pos, math.pi)
	end
	-- left edge
	for y=0,ny-1 do
		local screen_pos = grid2screen(vec2(-1, y))
		sprsheet.draw_centered('edge', layer, screen_pos, math.pi*3/2)
	end
	-- right edge
	for y=0,ny-1 do
		local screen_pos = grid2screen(vec2(nx, y))
		sprsheet.draw_centered('edge', layer, screen_pos, math.pi/2)
	end

	-- four corners
	sprsheet.draw_centered('corner', layer, grid2screen(vec2(-1, -1)))
	sprsheet.draw_centered('corner', layer, grid2screen(vec2(nx, -1)), math.pi/2)
	sprsheet.draw_centered('corner', layer, grid2screen(vec2(-1, ny)), math.pi*3/2)
	sprsheet.draw_centered('corner', layer, grid2screen(vec2(nx, ny)), math.pi)
end

return game

