local game = {}

local grid = require('grid')
local puzzles = require('puzzles')

local current_grid

function game.init()
	current_grid = grid:new(puzzles[1])	
end

function game.close()
end

function game.enter()
end

function game.leave()
end

function game.update()
	return true
end

function game.render(t)
	local pos = vec2(scr_size.x / 2, scr_size.y / 2)
	current_grid:draw(pos, 1)

	return true
end

return game
