local reset = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local levels = require('levels')

local reset_grid

function reset.init()
	reset_grid = grid:new(puzzles['reset'])
end

function reset.close()
end

function reset.enter()
	hud.set_title('reset game?')
	hud.set_buttons({hud.back, nil, nil, nil, nil})
end

function reset.leave()
end

function reset.preenter()
	reset_grid:reset_state()
end

function reset.update()
	if touch.count() > 0 then
		reset_grid:touch(touch.get(0), grid_pos)
	else
		reset_grid:touch(nil, grid_pos, function(t)
			local btn = reset_grid.puzzle.map[t]
			if btn == 'yes' then
				mfx.trigger('back')
				keyval.wipe()
				levels.init()
				states.pop()
			elseif btn == 'no' then
				mfx.trigger('back')
				states.pop()
			elseif btn == 'cheat' then
				levels.unlock_all()
			end
		end)
	end

	hud.update()
	
	return true
end

function reset.render(t)
	reset_grid:draw(grid_pos, 1, t)
	hud.render()
	return true
end

return reset
