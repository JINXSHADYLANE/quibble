local colours = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')

local colours_grid

function colours.init()
	colours_grid = grid:new(puzzles['colors'])
end

function colours.close()
end

function colours.enter()
	hud.set_title('colours')
	hud.set_buttons({hud.back, nil, nil, nil, nil})
end

function colours.leave()
end

function colours.preenter()
	colours_grid:reset_state()
end

function colours.update()
	if touch.count() > 0 then
		colours_grid:touch(touch.get(0), grid_pos)
	else
		colours_grid:touch(nil, grid_pos)
	end

	hud.update()

	return true
end

function colours.render(t)
	colours_grid:draw(grid_pos, 1, t)

	if t == 0 then
		hud.render()
	end

	return true
end

return colours
