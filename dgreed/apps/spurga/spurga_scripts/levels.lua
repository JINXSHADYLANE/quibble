local levels = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')

local levels_grid

local first_level = 'square'

function levels.init()
	levels_grid = grid:new(puzzles['levels'])
	keyval.set('pulock:'..first_level, true)
	levels.unlock = {}
	levels.prep_colors()
end

function levels.close()
end

function levels.prep_colors()
	local c_locked = rgba(1, 1, 1, 0.1)
	local c_unlocked = rgba(122/255, 160/255, 211/255, 1)
	local c_solved = rgba(236/255, 207/255, 14/255, 1)
	local c_pro = rgba(201/255, 50/255, 49/255)
	local c_wizard = rgba(201/255, 49/255, 143/255)

	local m = levels_grid.color_map

	for i=1,30 do
		m[i] = c_locked
	end

	for i,puzzle in pairs(levels_grid.puzzle.map) do
		local score = keyval.get('pscore:'..puzzle, -1)
		local pro_score = puzzles[puzzle].par / 2
		local wizard_score = pro_score * 3 / 2

		if score > 0 then
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
	end
end

function levels.is_locked(lvl)
	return not levels.unlock[lvl] and not keyval.get('pulock:'..lvl, false)
end

function levels.help()
	-- a cheat, unlocks all levels
	for i,puzzle in ipairs(levels_grid.puzzle.map) do
		levels.unlock[puzzle] = true
	end
	levels.prep_colors()
end

function levels.enter()
	hud.set_title('choose a puzzle')
	hud.set_buttons({hud.play, hud.music, hud.sound, hud.help, hud.back})
	hud.delegate = levels
end

function levels.leave()
	levels_grid:save_state()
	levels.unlock = {}
	levels.prep_colors()
end

function levels.play()
	game.current_grid = grid:new(puzzles[levels.last or first_level])
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
					levels.last = lvl
					local puzzle = puzzles[lvl]
					levels.current_grid = grid:new(puzzle)
					new_transition_mask(puzzle.w * puzzle.h)
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
