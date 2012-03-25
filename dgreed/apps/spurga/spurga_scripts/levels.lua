local levels = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local colours = require('colours')
local missions = require('missions')

local levels_grid
local first_level

function levels.init()
	levels_grid = grid:new(puzzles['levels'])
	first_level = levels_grid.puzzle.map[0]
	keyval.set('pulock:'..first_level, true)

	levels.unlock = {}
end

function levels.close()
end

function levels.prep_colors()
	local c_white = rgba(1, 1, 1, 1)
	local c_locked = rgba(1, 1, 1, 0.1)
	local c_unlocked = rgba(255/255, 255/255, 255/255, 1)
	local c_solved = rgba(235/255, 206/255, 12/255, 1)
	local c_pro = rgba(204/255, 90/255, 44/255)
	local c_wizard = rgba(108/255, 187/255, 42/255)

	local m = levels_grid.color_map

	for i=1,30 do
		m[i] = c_locked
	end

	for i,puzzle in pairs(levels_grid.puzzle.map) do
		if not levels.relax then
			local score = keyval.get('pscore:'..puzzle, -1)
			local pro_score = puzzles[puzzle].par / 2
			local wizard_score = pro_score * 3 / 2

			if score > -1 then
				if score >= wizard_score then
					m[i] = c_wizard
				elseif score >= pro_score then
					m[i] = c_pro
				else
					m[i] = c_solved
				end
			else
				if not levels.is_locked(puzzle) then
					m[i] = c_unlocked
				end
			end
		else
			if not levels.is_locked(puzzle) then
				m[i] = c_white
			end
		end
	end
end

function levels.is_locked(lvl)
	return not levels.unlock[lvl] and not keyval.get('pulock:'..lvl, false)
end

function levels.first_unsolved()
	for i,puzzle in pairs(levels_grid.puzzle.map) do
		local score = keyval.get('pscore:'..puzzle, -1)
		if not levels.is_locked(puzzle) and score == -1 then
			return puzzle
		end
	end
	return levels_grid.puzzle.map[#levels_grid.puzzle.map]
end

function levels.unlock_all()
	-- a cheat, unlocks all levels
	for i,puzzle in ipairs(levels_grid.puzzle.map) do
		levels.unlock[puzzle] = true
	end
end

function levels.help()
	mfx.trigger('transition')
	colours.preenter()
	states.push('colours')
end

function levels.enter()
	hud.set_title('choose a puzzle')
	local buttons
	if not levels.relax then
		buttons = {hud.back, hud.music, hud.sound, hud.help, hud.play}
	else
		buttons = {hud.back, hud.music, hud.sound, nil, hud.play}
	end
	hud.set_buttons(buttons)
	hud.delegate = levels

	acceleration.shake_callback(function ()
		levels_grid:reset_state()
		missions.shake()
	end)
end

function levels.leave()
	levels_grid:save_state()
	levels.unlock = {}
	levels.prep_colors()

	acceleration.shake_callback(nil)
end

function levels.play()
	mfx.trigger('transition')
	levels.current_grid = grid:new(puzzles[levels.last or levels.first_unsolved()])
	puzzles.preload(levels.current_grid.puzzle)
	states.push('game')
end

function levels.update()
	if touch.count() > 0 then
		levels_grid:touch(touch.get(0), grid_pos)
	else
		levels_grid:touch(nil, grid_pos, function (t)
			local lvl = levels_grid.puzzle.map[t]
			if lvl then
				if not levels.is_locked(lvl) then
					mfx.trigger('transition')
					levels.last = lvl
					local puzzle = puzzles[lvl]
					levels.current_grid = grid:new(puzzle, levels.relax)
					puzzles.preload(levels.current_grid.puzzle)
					states.push('game')
				end
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

