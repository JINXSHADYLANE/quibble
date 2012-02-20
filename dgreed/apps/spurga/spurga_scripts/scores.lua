local scores = {}

local grid = require('grid')
local puzzles = require('puzzles')
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
	scores.old_delegate = hud.delegate
	hud.delegate = scores
	scores_grid.can_shuffle = true
end

function scores.bake(score)
	scores_grid:reset_state()

	-- extract digits
	local score_d0 = math.fmod(score, 10)
	local score_d1 = math.fmod(score - score_d0, 100) / 10
	local score_d2 = (score - score_d1*10 - score_d0) / 100

	local p = scores_grid.puzzle.solved
	p[12] = score_d2
	p[13] = score_d1
	p[14] = score_d0

	for i=21,30 do
		p[i] = rand.int(0, 10)
	end

	scores_grid:scramble()
	scores_grid:unscramble()
end

function scores.leave()
	scores_grid.can_shuffle = false
end

function scores.replay()
	states.replace('game')
	scores.old_delegate.replay()
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
