local levels = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local game = require('game')

local levels_grid

function levels.init()
	levels_grid = grid:new(puzzles['levels'])
end

function levels.close()
end

function levels.enter()
	hud.set_title('choose a puzzle')
	hud.set_buttons({hud.play, hud.music, hud.sound, hud.help, hud.back})
	hud.delegate = levels
end

function levels.leave()
	levels_grid:save_state()
end

function levels.play()
	game.current_grid = grid:new(puzzles[levels.last or 'square'])
	states.push('game')
end

function levels.update()
	if touch.count() > 0 then
		levels_grid:touch(touch.get(0), grid_pos)
	else
		levels_grid:touch(nil, grid_pos, function (t)
			local lvl = levels_grid.puzzle.map[t]
			if lvl then
				levels.last = lvl
				local puzzle = puzzles[lvl]
				game.current_grid = grid:new(puzzle)
				new_transition_mask(puzzle.w * puzzle.h)
				states.push('game')
			end
		end)
	end

	hud.update()

	return true
end

function levels.render(t)
	levels_grid:draw(grid_pos, 1, t)

	hud.render()

	return true
end

return levels

