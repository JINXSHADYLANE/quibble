local game = {}

local player = require('player')

local tile_size = 10
local tiles_x = 20
local tiles_y = 15
local sector_pos = vec2(0, 0)

local sector 	-- tilemap handle
local tileset	-- texture handle
local world		-- image handle
local objs		-- list

-- world color legend:
local map_player = rgba(1, 0, 0, 1)

function color_equal(a, b)
	return a.r == b.r and a.g == b.g and a.b == b.b and a.a == b.a
end

function game.init()
	world = img.load(asset_dir..'world.png')
	video.clear_color(rgba(0.5, 0.5, 0.5))

	objs = game.load_sector(sector_pos)
end

function game.close()
	if world then
		img.free(world)
	end

	if sector then
		tilemap.free(sector)
	end
end

function game.load_sector(sector_pos, player_pos)
	if sector then
		tilemap.free(sector)
	end

	player_pos = vec2(0, 0)

	-- create new tilemap
	tileset = tex.load(asset_dir..'tiles.png')
	sector = tilemap.new(tile_size, tile_size, tiles_x, tiles_y, 1)
	tilemap.set_tileset(sector, 0, tileset)

	-- iterate over world image pixels and construct sector tilemap
	local pix_x, pix_y = sector_pos.x * tiles_x, sector_pos.y * tiles_y
	for x = 0,tiles_x-1 do
		for y = 0,tiles_y-1 do
			local pix = img.pixel(world, pix_x+x, pix_y+y)
			if color_equal(pix, map_player) then
				player_pos = vec2(x, y) * (tile_size + 0.5)
			elseif pix.a > 0.5 then
				tilemap.set_tile(sector, x, y, 0, 0, 1)
				tilemap.set_collision(sector, x, y, true)
			end
		end
	end

	tilemap.set_camera(sector, scr_size * 0.5, 1)

	-- create objects
	local objs = {}
	table.insert(objs,
		player:new({pos = player_pos})
	)

	return objs
end

function game.update()
	local new_objs = {}
	for i,obj in ipairs(objs) do
		local new_obj = obj:update(sector)

		if new_obj then
			table.insert(new_objs, new_obj)
		end
		
		if obj.collide then
			-- todo, detect collisions
		end
	end

	-- add newly spawned objects
	for i,obj in ipairs(new_objs) do
		table.insert(objs, obj)
	end

	-- collect garbage
	new_objs = {}
	for i,obj in ipairs(objs) do
		if not obj.dead then
			table.insert(new_objs, obj)
		end
	end
	objs = new_objs

	return true
end

function game.render(t)
	sprsheet.draw('background', 0, vec2(0, 0))

	tilemap.render(sector, scr_rect)

	for i,obj in ipairs(objs) do
		obj:render(sector)
	end

	return true
end

return game
