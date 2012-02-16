local menu = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local levels = require('levels')

local play_btn = 1
local relax_btn = 2
local scores_btn = 3
local missions_btn = 4

local menu_grid

function menu.init()
	menu_grid = grid:new(puzzles[1])
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
			if t == play_btn then
				states.push('levels')
			end
		end)
	end

	return true
end

function menu.render()
	menu_grid:draw(grid_pos, 1)
	hud.render()
	return true
end

return menu
