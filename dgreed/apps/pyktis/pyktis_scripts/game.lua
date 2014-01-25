local game = {}
local room = require('room')
local levels = require('levels')

local current_level = 1

function game.init()
end

function game.close()
end

function game.update()
	if room.win == true then
		room.win = false
		current_level = current_level + 1
	end

	room.level = levels[current_level]
	room.texts = levels.texts[current_level]
	states.push('room')

	return not key.pressed(key.quit) 
end

function game.render(t)
	return true
end

return game
