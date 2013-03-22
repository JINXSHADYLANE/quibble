local game = {}

local levels = require('levels')
local character = require('character')
local transition = require('transition')

local level = nil
local level_desc = nil
local level_number = 5
local char = nil
local exit = nil
local tileset = nil

local max = math.max
local min = math.min

local background = rgba(0, 0, 0)
local world_color = 8
local camera_pos = nil
local world_bottom = nil

local colors = {
	rgba(1, 0, 0),
	rgba(1, 0.5, 0),
	rgba(1, 1, 0),
	rgba(0, 1, 0),
	rgba(0, 1, 1),
	rgba(0, 0, 1),
	rgba(0.5, 0, 0.5),
	rgba(1, 1, 1),
	rgba(0, 0, 0)
}

local transitions = { 2, 3, 4, 5, 6, 7, 1, 9, 8 }

function game.init()
	game.reset(levels[level_number])
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

	tileset = tex.load(asset_dir..'tileset.png')

	local w, h = desc[1]:len(), #desc
	world_bottom = (h+1) * tile_size
	level_desc = desc
	level = tilemap.new(tile_size, tile_size, w, h+1, 1)
	exit = nil
	tilemap.set_tileset(level, 0, tileset)

	local ord_0 = string.byte('0')
	for y, line in ipairs(desc) do
		local x = 0
		for c in line:gmatch('.') do
			local p = vec2(x, y) * tile_size
			if c == ' ' then
				-- nothing
			elseif c == 's' then
				camera_pos = p
				tilemap.set_camera(level, p, 1, 0)
				char = character:new({pos = p})
			elseif c == 'e' then
				exit = rect(
					p.x, p.y - tile_size, p.x + tile_size, p.y + tile_size
				)
			else
				-- tile
				local tile = c:byte() - ord_0
				tilemap.set_collision(level, x, y, true)
				tilemap.set_tile(level, x, y, 0, 0, tile)
			end

			x = x + 1
		end
	end

	game.switch_off(9)
end

function game.switch_off(col)
	local desc = level_desc
	local ord_0 = string.byte('0')
	for y, line in ipairs(desc) do
		local x = 0
		for c in line:gmatch('.') do
			if c ~= ' ' and c ~= 's' and c ~= 'e' then
				local tile = c:byte() - ord_0
				tilemap.set_collision(level, x, y, tile ~= col)
			end
			x = x + 1
		end
	end
end
local function rect_inside_rect(a, b)
	if not rect_rect_collision(a, b) then
		return false
	end

	-- find logical intersection of both rects
	local l = max(a.l, b.l)
	local r = min(a.r, b.r)
	local t = max(a.t, b.t)
	local b = min(a.b, b.b)

	-- find areas of intersection and a rect
	local area = (r - l) * (b - t)
	local a_area = (a.r - a.l) * (a.b - a.t)

	-- define 'inside' as 90% inside
	return area >= a_area * 0.9
end

function game.update()
	local c = char:update(level, world_bottom)

	-- check if player fell too far
	if char.bbox.b+1 >= world_bottom then
		states.push('transition')
		transition.cb = function ()
			game.reset(levels[level_number])
		end
	end

	-- switch world color
	if c and c ~= world_color and c ~= transitions[world_color] then
		background = colors[transitions[c]]
		game.switch_off(transitions[c])
		world_color = c
	end

	-- update camera
	local p = vec2()
	p.x = lerp(camera_pos.x, char.pos.x, 0.1)
	p.y = lerp(camera_pos.y, char.pos.y, 0.03)
	tilemap.set_camera(level, p, 1, 0)
	camera_pos = p

	-- check if player collides with exit
	if exit and rect_inside_rect(char.bbox, exit) then
		anim.play(char.anim, 'stand')
		states.push('transition')
		transition.cb = function ()
			level_number = level_number + 1
			game.reset(levels[level_number])
		end
	end

	if key.up(key.quit) then
		states.pop()
	end

	return true
end

function game.render(t)
	sprsheet.draw('background', 0, scr_rect, background)

	if level then
		-- map
		tilemap.render(level, scr_rect)
		-- character
		char:render(level, world_bottom)
		-- exit
		if exit then
			local dest = tilemap.world2screen(level, scr_rect, exit)
			sprsheet.draw('exit', tile_layer, dest)
		end
	end

	return true
end

return game

