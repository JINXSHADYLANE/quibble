local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')

local current_grid

local grid_pos = vec2(scr_size.x / 2, 32 + 384/2)

function game.init()
	hud.init()
	hud.set_title('spurga')
	current_grid = grid:new(puzzles[1])	
end

function game.close()
	hud.close()
end

function game.enter()
end

function game.leave()
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
