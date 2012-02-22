local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local scores = require('scores')
local levels = require('levels')

local hint_show_speed = 1/20

local buttons_normal = {hud.back, nil, hud.hint, nil, hud.replay}
local buttons_solved = {hud.back, nil, hud.play, nil, hud.replay}
local buttons_locked = {hud.back, nil, nil, nil, hud.replay}

function game.init()
	game.hint_alpha = 0
end

function game.close()
end

function game.enter()
	hud.set_title(levels.current_grid.puzzle.name)
	hud.set_buttons(buttons_normal)
	game.solved_buttons = false
	hud.delegate = game

	assert(levels.current_grid)
	levels.current_grid.can_shuffle = true
end

function game.leave()
	levels.current_grid.can_shuffle = false
	levels.current_grid:save_state()
	levels.prep_colors()
end

-- special transition which can't be handled by
-- states since game state remains the same!
function game.next_level_transition()
	local t = time.s()
	local tt = (t - game.level_transition_t) / states.transition_len

	if tt >= 1 then
		-- end transition
		game.level_transition_t = nil
		levels.current_grid = game.next_grid
		levels.current_grid:draw(grid_pos, 1, 0)
		game.enter()
	else
		levels.current_grid:draw(grid_pos, 1, tt)
		game.next_grid:draw(grid_pos, 1, -1 + tt)
	end
end

function game.play()
	local p = levels.current_grid.puzzle
	if not levels.relax then
		-- play next level
		levels.current_grid:save_state()
		levels.current_grid = grid:new(puzzles.get_next(p), false)
	else
		-- relax mode, do manual transition to next level
		game.next_grid = grid:new(puzzles.get_next(p), true)
		game.level_transition_t = time.s()
	end
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
	local grid = levels.current_grid
	local score

	if touch.count() > 0 then
		grid:touch(touch.get(0), grid_pos)
	else
		grid:touch(nil, grid_pos)
	end

	if not levels.relax then
		score = grid:score()
		if grid.moves > 0 then
			local score_text = 'score: '..tostring(score)
			if hud.title ~= score_text then
				hud.set_title(score_text)	
			end
		end
	end

	hud.update()

	if grid:is_solved() then
		if levels.relax then
			if not game.solved_buttons then
				local next_level = puzzles.get_next(grid.puzzle)
				if levels.is_locked(next_level.name) then
					hud.set_buttons(buttons_locked)
				else
					hud.set_buttons(buttons_solved)
				end
				game.solved_buttons = true
			end
		else
			scores.bake(score)
			scores.render_hud = true
			states.replace('scores')
		end
	else
		if game.solved_buttons then
			hud.set_buttons(buttons_normal)
			game.solved_buttons = false
		end
	end

	return true
end

function game.render(t)
	if game.level_transition_t then
		game.next_level_transition()
	else
		if t == 0 and game.hint_alpha ~= 0 then
			levels.current_grid:draw(grid_pos, 1, -game.hint_alpha, true)
			game.hint_alpha = math.max(0, game.hint_alpha - hint_show_speed)
		else
			levels.current_grid:draw(grid_pos, 1, t)
		end
	end

	if t == 0 then
		hud.render()
	end

	return true
end

return game
