local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local scores = require('scores')
local levels = require('levels')
local tutorials = require('tutorials')
local missions = require('missions')

local hint_show_speed = 1/20

local buttons_normal = {hud.back, nil, hud.hint, nil, hud.replay}
local buttons_solved = {hud.back, nil, hud.play_next, nil, hud.replay}
local buttons_locked = {hud.back, nil, nil, nil, hud.replay}

function game.init()
	game.hint_alpha = 0
end

function game.close()
end

function game.enter()
	hud.set_buttons(buttons_normal)
	game.solved_buttons = false
	hud.delegate = game

	local grid = levels.current_grid
	assert(grid)
	grid.can_shuffle = true

	game.solved_mode = grid:is_solved()
	if not levels.relax and game.solved_mode then
		score = grid:score()
		local score_text = 'score: '..tostring(score)
		hud.set_title(score_text)	
	else
		hud.set_title(grid.puzzle.name)
	end

	grid.finish_shuffle_cb = function()
		tutorials.show(grid.puzzle.name)
		last_touch = false
	end

	solve_t = nil

	runstate.background_callback(function ()
		levels.current_grid:save_state()
	end)
end

function game.leave()
	hud.set_score(nil)
	local grid = levels.current_grid
	grid.can_shuffle = false
	if not game.solved_mode then 
		levels.current_grid:save_state()
	end
	levels.prep_colors()
	tutorials.hide(true)

	runstate.background_callback(nil)
end

-- special transition which can't be handled by
-- states since game state remains the same!
function game.next_level_transition()
	local t = time.s()
	local tt = (t - game.level_transition_t) / states.transition_len

	if tt >= 1 then
		-- end transition
		game.level_transition_t = nil
		local grid = game.next_grid
		levels.current_grid = grid
		grid:draw(grid_pos, 1, 0)
		game.enter()
	else
		levels.current_grid:draw(grid_pos, 1, tt)
		game.next_grid:draw(grid_pos, 1, -1 + tt)
	end
end

function game.play()
	local p = levels.current_grid.puzzle
	levels.current_grid:save_state()
	if not levels.relax and not game.solved_mode then
		-- play next level
		levels.current_grid = grid:new(puzzles.get_next(p), false)
		puzzles.preload(levels.current_grid.puzzle)
	else
		-- relax mode, do manual transition to next level
		game.next_grid = grid:new(puzzles.get_next(p), true)
		game.level_transition_t = time.s()
	end
	mfx.trigger('transition')
end

function game.replay()
	--states.replace('game')
	mfx.trigger('click')
	levels.current_grid:reset_state(true)
	game.solved_mode = false
end

function game.hint()
	if not levels.current_grid.shuffling then
		if game.hint_alpha == 0 then
			local p = levels.current_grid.puzzle
			mfx.trigger('hint')
			missions.remind(p.name)
		end
		game.hint_alpha = math.min(1, game.hint_alpha + hint_show_speed*2)
	end
end

function game.update()
	local grid = levels.current_grid
	local score

	if touch.count() > 0 then
		grid:touch(touch.get(0), grid_pos)
		last_touch = true
	else
		if last_touch then
			tutorials.hide()
		end
		grid:touch(nil, grid_pos)
		last_touch = false
	end

	if not levels.relax and not game.solved_mode then
		score = grid:score()
		if grid.moves > 0 then
			local score_text = 'score: '..tostring(score)
			hud.set_title(score_text)	
		end
	end

	hud.update()

	if grid:is_solved() then
		if levels.relax or game.solved_mode then
			if not game.solved_buttons then
				local next_level = puzzles.get_next(grid.puzzle)
				if levels.is_locked(next_level.name) then
					hud.set_buttons(buttons_locked)
				else
					mfx.trigger('solve')
					hud.set_buttons(buttons_solved)
					missions.relax_solved(grid.puzzle.name)
				end
				game.solved_buttons = true
			end
		else
			if solve_t == nil then
				solve_t = time.s() + 1
				mfx.trigger('solve')
			end
		end
	else
		if game.solved_buttons then
			hud.set_buttons(buttons_normal)
			game.solved_buttons = false
		end
	end

	if solve_t and solve_t < time.s() then
		local total_score = puzzles.total_score()
		if gamecenter then
			if gamecenter.is_active() then
				gamecenter.report_score('default', 42, total_score)
			end
		end

		scores.bake(score)
		scores.render_hud = true
		states.replace('scores')
		solve_t = nil
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

	if not levels.relax then
		tutorials.render(t)
	end

	return true
end

return game
