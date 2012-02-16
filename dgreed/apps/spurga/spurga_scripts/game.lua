local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')

local current_grid

function game.init()
end

function game.close()
end

function game.enter()
	assert(game.current_grid)
	current_grid = game.current_grid	
	hud.set_title(current_grid.puzzle.name)
end

function game.leave()
	current_grid:save_state()
end

function game.update()
	if touch.count() > 0 then
		current_grid:touch(touch.get(0), grid_pos)
	else
		current_grid:touch(nil, grid_pos)
	end

	return true
end

function game.render(t)
	current_grid:draw(grid_pos, 1)

	hud.render()

	return true
end

return game
