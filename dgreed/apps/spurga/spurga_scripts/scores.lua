local scores = {}

local grid = require('grid')
local puzzles = require('puzzles')
local game = require('game')
local hud = require('hud')

local scores_grid

function scores.init()
	scores_grid = grid:new(puzzles['score'])
end

function scores.close()
end

function scores.enter()
	hud.set_title('score')
	hud.set_buttons({hud.replay, nil, nil, nil, hud.back})
	hud.delegate = scores
end

function scores.leave()
end

function scores.replay()
	states.replace('game')
	game.replay()
end

function scores.update()
	if touch.count() > 0 then
		scores_grid:touch(touch.get(0), grid_pos)
	else
		scores_grid:touch(nil, grid_pos)
	end

	hud.update()

	return true
end

function scores.render(t)
	scores_grid:draw(grid_pos, 1, t)

	hud.render()

	return true
end

return scores
