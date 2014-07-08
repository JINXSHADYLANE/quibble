local game = {}

local player = require('player')
local star = require('star')
local crusher = require('crusher')
local bubble = require('bubble')
local spike = require('spike')
local brick = require('brick')
local savepoint = require('savepoint')

local texts = require('texts')

local tile_size = 10
local tiles_x = 20
local tiles_y = 15
local sector_pos = vec2(1, 3)

local sector 	-- tilemap handle
local tileset	-- texture handle
local world		-- image handle
local objs		-- list

local player_obj

-- fadeout starts when player dies
local fadeout_t = nil
local fadein_t = nil
local fade_len = 1.2

-- world color legend:
local map_player = rgba(1, 0, 0, 1)
local map_star = rgba(0, 1, 0, 1)
local map_crusher = rgba(0, 0, 1, 1)
local map_bubble = rgba(128/255, 128/255, 128/255, 1)
local map_spike = rgba(0, 1, 1, 1)
local map_wall = rgba(0, 0, 0, 1)
local map_brick = rgba(64/255, 64/255, 64/255, 1)
local map_savepoint = rgba(1, 128/255, 128/255, 1)

function color_equal(a, b)
	return a.r == b.r and a.g == b.g and a.b == b.b and a.a == b.a
end

function game.init()
	world = img.load(asset_dir..'world.png')

	objs = game.load_sector(sector_pos)
	assert(player_obj)
	player_obj.save_sector = sector_pos
	player_obj.save_pos = vec2(player_obj.pos)

	-- additive blend for bubbles
	video.set_blendmode(2, 'add')
end

function game.close()
	if world then
		img.free(world)
	end

	if sector then
		tilemap.free(sector)
	end
end

local cardinal_offsets = {
	vec2(0,-1), vec2(1,0), vec2(0,1), vec2(-1, 0)
}

function game.load_sector(sector_pos, player_pos, old_player)
	if sector then
		tilemap.free(sector)
	end

	local sector_id = tostring(sector_pos)
	--player_pos = vec2(0, 0)

	-- create new tilemap
	tileset = tex.load(asset_dir..'tiles.png')
	sector = tilemap.new(tile_size, tile_size, tiles_x, tiles_y, 1)
	tilemap.set_tileset(sector, 0, tileset)

	local text_idx = 1

	local objs = {}

	-- iterate over world image pixels and construct sector tilemap
	local pix_x, pix_y = sector_pos.x * tiles_x, sector_pos.y * tiles_y
	for y = 0,tiles_y-1 do
		for x = 0,tiles_x-1 do
			local pix = img.pixel(world, pix_x+x, pix_y+y)
			local screen_pos = vec2(x+0.5, y+0.5) * tile_size
			local text_pos = vec2(20.5-x, 15.5-y) * tile_size
			if color_equal(pix, map_player) then
				if not old_player and not player_pos then
					player_pos = screen_pos
				end
			elseif color_equal(pix, map_star) then
				table.insert(objs, star:new(screen_pos))			
			elseif color_equal(pix, map_crusher) then
				table.insert(objs, crusher:new(screen_pos))
			elseif color_equal(pix, map_bubble) then
				local text = nil
				if texts[sector_id] then
					text = texts[sector_id][text_idx]
				end
				text_idx = text_idx + 1
				if not text then text = 'TODO' end
				table.insert(objs, bubble:new(screen_pos, text_pos, text))
			elseif color_equal(pix, map_spike) then
				local rot = 0
				for i,off in ipairs(cardinal_offsets) do
					local w = img.pixel(world, pix_x+x+off.x, pix_y+y+off.y)
					if color_equal(w, map_wall) then
						rot = i-1
						break
					end
				end

				table.insert(objs, spike:new(screen_pos, rot))
			elseif color_equal(pix, map_savepoint) then
				table.insert(objs, savepoint:new(screen_pos, sector_pos))
			elseif color_equal(pix, map_wall) then
				tilemap.set_tile(sector, x, y, 0, 0, 1)
				tilemap.set_collision(sector, x, y, true)
			elseif color_equal(pix, map_brick) then
				tilemap.set_tile(sector, x, y, 0, 0, 2)
				tilemap.set_collision(sector, x, y, true)
				table.insert(objs, brick:new(screen_pos, x, y))
			end
		end
	end

	tilemap.set_camera(sector, scr_size * 0.5, 1)

	if not old_player then
		player_obj = player:new(player_pos)
		player_obj.save_sector = sector_pos
		player_obj.save_pos = player_pos
		table.insert(objs, player_obj)
	else
		table.insert(objs, old_player)
	end

	return objs
end

function game.update()
	local new_objs = {}
	for i,obj in ipairs(objs) do
		local new_obj = nil
		if obj.update then
			new_obj = obj:update(sector)
		end

		if new_obj then
			for j,nobj in ipairs(new_obj) do
				table.insert(new_objs, nobj)
			end
		end
		
		if obj.collide then
			local mt = getmetatable(obj)
			for j,other in ipairs(objs) do
				if other ~= obj and mt ~= getmetatable(other) then
					if other.bbox and not obj.bbox then
						if rect_point_collision(other.bbox, obj.pos) then
							obj:collide(sector, other)
						end
					elseif other.bbox and obj.bbox then
						if rect_rect_collision(other.bbox, obj.bbox) then
							obj:collide(sector, other)
						end
					end
				end
			end
		end
	end

	-- switch sectors if asked to
	-- (only player obj does this)
	if player_obj.change_sector then
		local obj = player_obj
		sector_pos = sector_pos + obj.change_sector
		local offset = vec2(
			obj.change_sector.x * (scr_size.x - tile_size * 1.5),
			obj.change_sector.y * (scr_size.y - tile_size * 1.5) 
		)
		obj.pos = obj.pos - offset
		objs = game.load_sector(sector_pos, obj.pos, obj)
		obj.change_sector = nil
		return true
	end


	-- add newly spawned objects
	for i,obj in ipairs(new_objs) do
		table.insert(objs, obj)
	end

	-- todo: handle death
	if player_obj.dead and fadeout_t == nil then
		fadeout_t = states.time()
	end

	-- collect garbage
	new_objs = {}
	for i,obj in ipairs(objs) do
		if obj.dead then
			local screen_pos = tilemap.world2screen(sector, scr_rect, obj.pos)
			mfx.trigger('explode', screen_pos)
		else
			table.insert(new_objs, obj)
		end
	end
	objs = new_objs

	return not key.pressed(key.quit) 
end

function game.render(t)
	sprsheet.draw('background', 0, vec2(0, 0))

	tilemap.render(sector, scr_rect)

	for i,obj in ipairs(objs) do
		if obj.render then
			obj:render(sector)
		end
	end

	-- fadein and fadeout
	assert(not (fadeout_t and fadein_t))
	if fadeout_t or fadein_t then
		local t
		if fadeout_t then
			t = (states.time() - fadeout_t) / fade_len
		else
			t = 1 - (states.time() - fadein_t) / fade_len
		end
		local col = rgba(0, 0, 0, clamp(0, 1, t))
		sprsheet.draw('empty', 15, scr_rect, col)
		if t >= 1 then
			fadein_t = states.time()
			fadeout_t = nil
			objs = game.load_sector(
				player_obj.save_sector,
				player_obj.save_pos
			)
			sector_pos = player_obj.save_sector
		end
		if t < 0 then
			fadein_t = nil
		end
	end

	return true
end

return game
