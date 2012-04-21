local objs = require('objs')

local game = {}

local function load_level(name)
	level = tilemap.load(pre..name)
	objs.reset(level)

	tilemap.set_camera(level, objs.player.pos)
end

-- game state --

function game.init()
	load_level('prelude.btm')
end

function game.close()
	if level then
		tilemap.free(level)
	end
end

function game.enter()
end

function game.leave()
end

function game.update()
	if key.up(key.quit) then
		states.pop()
	end

	sound.update()

	objs.update()
	
	return true
end

function game.render(t)
	
	if level then
		tilemap.render(level, screen_rect)
	end

	objs.render()

	return true
end

return game

