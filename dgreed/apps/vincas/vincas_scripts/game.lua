local game = {}

local level = require('level')
local objects = require('objects')
local player = require('player')

local tiles_texture = nil
local tiles = nil

function game.init()
	objects.reset()
	game.camera = rect(0, 0, scr_size.x, scr_size.y)

	objects.add(player.new(vec2(scr_size.x / 2, 480)))

	tiles_texture = tex.load(asset_dir..'tiles.png')
	tiles = level.generate(40, 7100, tiles_texture)
end

function game.close()
	objects.close()
	tilemap.free(tiles)
	--tex.free(tiles_texture)
end

function game.update()
	objects.update()

	-- camera follows player
	game.camera.b = objects.player.pos.y + 80
	game.camera.t = game.camera.b - scr_size.y

	return true
end

function game.render(t)
	objects.render(game.camera, 3)

	local camera = vec2(
		(game.camera.l + game.camera.r) / 2,
		(game.camera.t + game.camera.b) / 2 + (7100-71)*8
	)
	tilemap.set_camera(tiles, camera, 1, 0)
	tilemap.render(tiles, scr_rect)

	return true
end

return game
