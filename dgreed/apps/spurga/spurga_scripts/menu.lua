local menu = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local levels = require('levels')

local menu_grid

function menu.init()
	menu_grid = grid:new(puzzles['menu'])
end

function menu.close()
end

function menu.enter()
	hud.set_title('spurga')
end

function menu.leave()
	menu_grid:save_state()
end

function menu.update()
	if touch.count() > 0 then
		menu_grid:touch(touch.get(0), grid_pos)
	else
		menu_grid:touch(nil, grid_pos, function (t)
			if menu_grid.puzzle.map[t] == 'play' then
				states.push('levels')
			end
		end)
	end

	return true
end

function menu.render(t)
	menu_grid:draw(grid_pos, 1, t)

	if t == 0 then
		hud.render()
	end

	return true
end

return menu
