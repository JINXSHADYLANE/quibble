local game = {}

local levels = require('levels')
local character = require('character')

local level = nil
local level_desc = nil
local char = nil
local tileset = nil

local background = rgba(0, 0, 0)
local world_color = 8
local camera_pos = nil

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
	tileset = tex.load(asset_dir..'tileset.png')
	game.reset(levels[6])
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
	level_desc = desc
	level = tilemap.new(tile_size, tile_size, w, h+1, 1)
	tilemap.set_tileset(level, 0, tileset)

	local ord_0 = string.byte('0')
	for y, line in ipairs(desc) do
		local x = 0
		for c in line:gmatch('.') do
			if c == ' ' then
				-- nothing
			elseif c == 's' then
				local p = vec2(x, y) * tile_size
				camera_pos = p
				tilemap.set_camera(level, p, 1, 0)
				char = character:new({pos = p})
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
			if c ~= ' ' and c ~= 's' then
				local tile = c:byte() - ord_0
				tilemap.set_collision(level, x, y, tile ~= col)
			end
			x = x + 1
		end
	end
end

function game.update()
	local c = char:update(level)
	if c and c ~= world_color and c ~= transitions[world_color] then
		background = colors[transitions[c]]
		game.switch_off(transitions[c])
		world_color = c
	end

	local p = vec2()
	p.x = lerp(camera_pos.x, char.pos.x, 0.1)
	p.y = lerp(camera_pos.y, char.pos.y, 0.03)
	tilemap.set_camera(level, p, 1, 0)
	camera_pos = p

	if key.up(key.quit) then
		states.pop()
	end
	return true
end

function game.render(t)
	sprsheet.draw('background', 0, scr_rect, background)

	if level then
		tilemap.render(level, scr_rect)
		char:render(level)
	end

	return true
end

return game

