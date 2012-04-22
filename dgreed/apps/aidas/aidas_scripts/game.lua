local objs = require('objs')

local game = {}

local levels = {
	'first.btm',
	'goal.btm',
	'obstacles.btm',
	'path.btm',
	'fall.btm',
	'control.btm',
	'push.btm',
	'chasm.btm'
}

local level_index = 1

local function load_level(name)
	if level then
		tilemap.free(level)
	end

	level = tilemap.load(pre..name)
	objs.reset(level)

	tilemap.set_camera(level, objs.player.pos)
end

-- game state --

function game.init()
	load_level(levels[level_index])
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

	if objs.update() then
		level_index = level_index + 1
		load_level(levels[level_index])
	end
	
	return true
end

function game.render(t)
	
	objs.render()

	if level then
		tilemap.render(level, screen_rect)
	end

	return true
end

return game

