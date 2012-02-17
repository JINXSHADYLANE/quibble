local game = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')

local current_grid

local hint_show_speed = 1/20

function game.init()
	game.hint_alpha = 0
end

function game.close()
end

function game.enter()
	hud.set_title(game.current_grid.puzzle.name)
	hud.set_buttons({hud.replay, nil, hud.hint, nil, hud.back})
	hud.delegate = game

	assert(game.current_grid)
	game.current_grid.can_shuffle = true
end

function game.leave()
	game.current_grid.can_shuffle = false
	game.current_grid:save_state()
end

function game.replay()
	game.current_grid:reset_state()
	game.current_grid:shuffle()
end

function game.hint()
	if game.hint_alpha == 0 then
		local p = game.current_grid.puzzle
		new_transition_mask(p.w * p.h)
	end
	game.hint_alpha = math.min(1, game.hint_alpha + hint_show_speed*2)
end

function game.update()

	if touch.count() > 0 then
		game.current_grid:touch(touch.get(0), grid_pos)
	else
		game.current_grid:touch(nil, grid_pos)
	end

	hud.update()

	return true
end

function game.render(t)
	if t == 0 and game.hint_alpha ~= 0 then
		game.current_grid:draw(grid_pos, 1, -game.hint_alpha, true)
		game.hint_alpha = math.max(0, game.hint_alpha - hint_show_speed)
	else
		game.current_grid:draw(grid_pos, 1, t)
	end

	if t == 0 then
		hud.render()
	end

	return true
end

return game
