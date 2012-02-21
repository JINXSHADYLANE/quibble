local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local scores = require('scores')
local levels = require('levels')

local hint_show_speed = 1/20

function game.init()
	game.hint_alpha = 0
end

function game.close()
end

function game.enter()
	hud.set_title(levels.current_grid.puzzle.name)
	hud.set_buttons({hud.replay, nil, hud.hint, nil, hud.back})
	hud.delegate = game

	assert(levels.current_grid)
	levels.current_grid.can_shuffle = true
end

function game.leave()
	levels.current_grid.can_shuffle = false
	levels.current_grid:save_state()
	levels.prep_colors()
end

function game.replay()
	--states.replace('game')
	levels.current_grid:reset_state(true)
end

function game.hint()
	if not levels.current_grid.shuffling then
		if game.hint_alpha == 0 then
			local p = levels.current_grid.puzzle
			new_transition_mask(p.w * p.h)
		end
		game.hint_alpha = math.min(1, game.hint_alpha + hint_show_speed*2)
	end
end

function game.update()
	if touch.count() > 0 then
		levels.current_grid:touch(touch.get(0), grid_pos)
	else
		levels.current_grid:touch(nil, grid_pos)
	end

	local score = levels.current_grid:score()
		if levels.current_grid.moves > 0 then
		local score_text = 'score: '..tostring(score)
		if hud.title ~= score_text then
			hud.set_title(score_text)	
		end
	end

	hud.update()

	if levels.current_grid:is_solved() then
		scores.bake(score)
		scores.render_hud = true
		states.replace('scores')
	end

	return true
end

function game.render(t)
	if t == 0 and game.hint_alpha ~= 0 then
		levels.current_grid:draw(grid_pos, 1, -game.hint_alpha, true)
		game.hint_alpha = math.max(0, game.hint_alpha - hint_show_speed)
	else
		levels.current_grid:draw(grid_pos, 1, t)
	end

	if t == 0 then
		hud.render()
	end

	return true
end

return game
