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
	hud.set_buttons({hud.back, nil, hud.play, nil, hud.replay})
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

	local puzzle = scores_grid.puzzle
	if not puzzle.solved_original then
		puzzle.solved_original = puzzle.solved
		puzzle.solved = {}
	end

	local o = puzzle.solved_original
	local p = puzzle.solved

	-- restructure the puzzle - move sign to the middle, add score
	for i=26,6,-1 do
		p[i] = o[i-5]
	end

	p[12+5] = score_d2
	p[13+5] = score_d1
	p[14+5] = score_d0

	for i=1,5 do
		p[i] = 27
	end
	for i=26,30 do
		p[i] = 27 
	end

	scores_grid:scramble()
	scores_grid:unscramble()
end

function scores.leave()
	scores_grid.can_shuffle = false
end

function scores.play()
	scores.old_delegate.play()
	states.replace('game')
end

function scores.replay()
	scores.old_delegate.replay()
	states.replace('game')
end

function scores.back()
	scores.render_hud = false
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

	if scores.render_hud then
		hud.render()
	end

	return true
end

return scores
