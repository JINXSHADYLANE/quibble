local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')

local current_grid

function game.init()
	hud.init()
	hud.set_title('spurga')
	current_grid = grid:new(puzzles[2])	
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
		current_grid:touch(touch.get(0))
	else
		current_grid:touch(nil)
	end

	return true
end

function game.render(t)
	local pos = vec2(scr_size.x / 2, scr_size.y / 2)
	current_grid:draw(pos, 1)

	hud.render()

	return true
end

return game
