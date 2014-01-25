local game = {}
local room = require('room')
local levels = require('levels')

function game.init()
end

function game.close()
end

function game.update()
	if char.down('1') then
		room.level = levels[1]
		states.push('room')
	end
	return not key.pressed(key.quit) 
end

function game.render(t)
	return true
end

return game
