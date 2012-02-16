local levels = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local game = require('game')

local levels_grid

local levels_map = {
	[0] = 3,
	[1] = 4,
	[2] = 5,
	[3] = 6,
	[4] = 7
}

function levels.init()
	levels_grid = grid:new(puzzles[2])
end

function levels.close()
end

function levels.enter()
	hud.set_title('choose a puzzle')
end

function levels.leave()
	levels_grid:save_state()
end

function levels.update()
	if touch.count() > 0 then
		levels_grid:touch(touch.get(0), grid_pos)
	else
		levels_grid:touch(nil, grid_pos, function (t)
			local lvl = levels_map[t]
			if lvl then
				game.current_grid = grid:new(puzzles[lvl])
				states.push('game')
			end
		end)
	end

	return true
end

function levels.render()
	levels_grid:draw(grid_pos, 1)
	hud.render()
	return true
end

return levels

