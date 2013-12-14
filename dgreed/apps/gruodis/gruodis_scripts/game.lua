local game = {}

local tile_size = 10
local tiles_x = 20
local tiles_y = 15
local sector_pos = vec2(0, 0)

local sector 	-- tilemap handle
local tileset	-- texture handle
local world	-- image handle

function game.init()
	world = img.load(asset_dir..'world.png')
	video.clear_color(rgba(0.5, 0.5, 0.5))

	game.load_sector(sector_pos)
end

function game.close()
	if world then
		img.free(world)
	end

	if sector then
		tilemap.free(sector)
	end
end

function game.load_sector(sector_pos)
	if sector then
		tilemap.free(sector)
	end

	-- create new tilemap
	tileset = tex.load(asset_dir..'tiles.png')
	sector = tilemap.new(tile_size, tile_size, tiles_x, tiles_y, 1)
	tilemap.set_tileset(sector, 0, tileset)

	-- iterate over world image pixels and construct sector tilemap
	local pix_x, pix_y = sector_pos.x * tiles_x, sector_pos.y * tiles_y
	for x = 0,tiles_x-1 do
		for y = 0,tiles_y-1 do
			local pix = img.pixel(world, pix_x+x, pix_y+y)
			if pix.a > 0.5 then
				tilemap.set_tile(sector, x, y, 0, 0, 1)
				tilemap.set_collision(sector, x, y, true)
			end
		end
	end

	tilemap.set_camera(sector, scr_size * 0.5, 1)
end

function game.update()
	return true
end

function game.render(t)
	tilemap.render(sector, scr_rect)
	return true
end

return game
