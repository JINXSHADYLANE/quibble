local game = {}

local levels = require('levels')
local laser = require('laser')

local bg_color = rgba(0.8, 0.8, 0.8, 1)

-- game state:

local current_level = 5

-- logical player position
local player_pos = nil

-- visual player position, with nice 
-- interpolations and everything
local player_draw_pos = nil

local player_has_egg = false

-- list of grid space vec2s
local eggs = nil
local exploded_eggs = nil

-- flattened 2d list of level tiles
local lvl = nil


-- time when laser will be turned on/off
local laser_on_t = nil
local laser_off_t = nil

-- how long laser is turned on/off
local laser_on_len = 4
local laser_off_len = 1

-- laser column
local laser_start_x = nil

-- path of laser beam
local laser_path = nil

-- time when fadeout starts
local fadeout_t = nil
-- did game reset at fadeout middle happen?
local did_reset = false

-- game logic:

function game.init()
	laser.init()
	game.load_level(levels[current_level])

	video.set_blendmode(15, 'multiply')
end

function game.close()
	laser.close()
end

function game.update_player()
	-- control player
	local player_off = vec2(0, 0)
	local move_dirs = {
		[key._up] = vec2(0, -1),
		[key._right] = vec2(1, 0),
		[key._down] = vec2(0, 1),
		[key._left] = vec2(-1, 0)
	}

	for dir, off in pairs(move_dirs) do
		if key.down(dir) then
			player_off = player_off + off
		end
	end
	
	local new_pos = player_pos + player_off
	if not game.is_solid(new_pos) then
		player_pos = new_pos
	end

	-- egg take logic
	if key.down(key.a) or char.down(' ') then
		if not player_has_egg then
			-- linear search for the egg player is standing on
			for i,egg in ipairs(eggs) do
				if egg.x == player_pos.x and egg.y == player_pos.y then
					-- found right egg, take it
					player_has_egg = true

					-- remove egg from eggs list
					table.remove(eggs, i)
				end
			end
		else
			-- put egg down
			if lvl[grid_idx(player_pos.x, player_pos.y)] == nil then
				-- make sure we're not putting egg on top of another egg
				local can_put = true
				for i,egg in ipairs(eggs) do
					if egg.x == player_pos.x and egg.y == player_pos.y then
						can_put = false
						break
					end
				end

				if can_put then
					player_has_egg = false
					table.insert(eggs, player_pos)
				end
			end
		end
	end

	-- smooth player movement by lerping visual pos to logical pos
	player_draw_pos = lerp(player_draw_pos, player_pos, 0.2)
end

function game.laser_path(x)
	local path = {}
	local dir = vec2(0, 1)
	local p = vec2(x, 0)

	local nx, ny = level_size.x, level_size.y

	-- advance laser beam while it's in the level grid
	while p.x >= 0 and p.x < nx and p.y >= 0 and p.y <= ny do

		local tile = lvl[grid_idx(p.x, p.y)]

		-- stop at walls
		if tile == '#' then
			break
		end
		
		table.insert(path, p)

		-- change direction at mirrors
		if tile == 'r' then
			dir = vec2(dir.y, dir.x) 
		elseif tile == 'l' then
			dir = vec2(-dir.y, -dir.x)
		end

		p = p + dir
	end

	laser_path = path

	return path
end

function game.update_laser()
	local t = time.s()

	if t >= laser_on_t then
		--print('laser on')
		local path = game.laser_path(laser_start_x)
		laser.on(path)

		laser_on_t = laser_off_t + laser_off_len
	end

	if t >= laser_off_t then
		--print('laser off')

		laser.off()
		laser_path = nil

		laser_off_t = laser_on_t + laser_on_len
		laser_start_x = laser_start_x + 1

		-- level end check
		if laser_start_x == level_size.x then
			fadeout_t = time.s()
			did_reset = false
			if #exploded_eggs == 0 then
				-- level solved
				current_level = current_level + 1
			else
				-- level failed, autoreset
			end
		end
	end

	if laser_path then
		for i,p in ipairs(laser_path) do
			-- if egg was hit - explode it
			for j,egg in ipairs(eggs) do
				if egg.x == p.x and egg.y == p.y then
					table.remove(eggs, j)
					table.insert(exploded_eggs, p)
					break
				end
			end

			-- explode egg being carried by player
			if player_has_egg then
				if player_pos.x == p.x and player_pos.y == p.y then
					player_has_egg = false
					table.insert(exploded_eggs, p)
				end
			end
		end
	end

	laser.update(time.dt())
end

function game.update()
	-- exit game if esc or q was pressed
	if key.down(key.quit) or char.down('q') then
		return false
	end

	if char.down('r') then
		fadeout_t = time.s()
		did_reset = false
	end
	
	if fadeout_t == nil then
		game.update_player()
		game.update_laser()
	end

	return true
end

function game.render(t)
	-- slightly off-white background
	sprsheet.draw('empty', 0, scr_rect, bg_color)

	-- vignette
	local c = 1
	if fadeout_t then
		local ts = time.s()
		c = (ts - fadeout_t) / 2
		if c > 0.5 and not did_reset then
			game.load_level(levels[current_level])
			did_reset = true
		end
		if c >= 1 then
			c = 1
			fadeout_t = nil
		end

		c = 1 - math.sin(c * math.pi)
	end
	sprsheet.draw('vignette', 15, scr_rect, rgba(c, c, c, 1))

	game.draw_level(1)
	game.draw_objs(2)
	laser.draw(3)
	
	return true
end

-- returns true if there is a wall or edge of 
-- the level at specific grid pos
function game.is_solid(grid_pos)
	if grid_pos.x < 0 or grid_pos.x >= level_size.x then
		return true
	elseif grid_pos.y < 0 or grid_pos.y >= level_size.y then
		return true
	end

	return lvl[grid_idx(grid_pos.x, grid_pos.y)] ~= nil
end

-- parses levels and preps everything up for rendering
-- lvldesc is one of the levels defined in levels.lua
function game.load_level(lvldesc)
	-- reset game state
	player_pos = nil
	player_draw_pos = nil
	eggs = {}
	exploded_eggs = {}
	lvl = {}

	laser_on_t = time.s() + 2
	laser_off_t = laser_on_t + laser_on_len
	laser_start_x = 0
	if laser_path then
		laser_path = nil
		laser.off()
	end

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

-- draws objects - player and eggs
function game.draw_objs(layer)
	-- eggs
	for i,egg in ipairs(eggs) do
		sprsheet.draw_centered('egg', layer, grid2screen(egg))
	end

	-- exploded eggs
	for i,egg in ipairs(exploded_eggs) do
		sprsheet.draw_centered('egg_explode', layer, grid2screen(egg))
	end

	-- player
	local player_screen_pos = grid2screen(player_draw_pos)
	sprsheet.draw_centered('player', layer, player_screen_pos)
	if player_has_egg then
		sprsheet.draw_centered('egg', layer+1, player_screen_pos) 
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
				sprsheet.draw_centered('tile', layer, screen_pos)
				sprsheet.draw_centered('mirror', layer, screen_pos)
			elseif tile == 'r' then
				sprsheet.draw_centered('tile', layer, screen_pos)
				sprsheet.draw_centered('mirror', layer+1, screen_pos, math.pi/2)
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

