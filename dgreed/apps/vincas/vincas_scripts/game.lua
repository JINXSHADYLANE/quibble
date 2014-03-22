local game = {}

local objects = require('objects')
local player = require('player')

function game.init()
	objects.reset()
	game.camera = rect(0, 0, scr_size.x, scr_size.y)

	objects.add(player.new(vec2(scr_size.x / 2, 480)))
end

function game.close()
	objects.close()
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

	return true
end

return game
