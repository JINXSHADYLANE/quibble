local menu = {}

local grid = require('grid')
local puzzles = require('puzzles')
local hud = require('hud')
local levels = require('levels')
local reset = require('reset')
local missions = require('missions')

local menu_grid

function menu.init()
	menu_grid = grid:new(puzzles['menu'])
end

function menu.close()
end

function menu.enter()
	hud.set_title('spurga')
	hud.set_buttons({nil, hud.music, hud.sound, hud.help, nil})
	hud.delegate = menu

	acceleration.shake_callback(function ()
		menu_grid:reset_state()
		missions.shake()
	end)
end

function menu.leave()
	mfx.trigger('transition')
	menu_grid:save_state()

	acceleration.shake_callback(nil)
end

function menu.help()
	web.open('http://www.qbcode.com')
end

function menu.update()
	if touch.count() > 0 then
		menu_grid:touch(touch.get(0), grid_pos)
	else
		menu_grid:touch(nil, grid_pos, function (t)
			local btn = menu_grid.puzzle.map[t]
			if btn == 'play' then
				levels.relax = false
				levels.prep_colors()
				states.push('levels')
			elseif btn == 'relax' then
				levels.relax = true
				levels.prep_colors()
				states.push('levels')
			elseif btn == 'reset' then
				reset.preenter()
				states.push('reset')
			elseif btn == 'scores' then
				if gamecenter and gamecenter.is_active() then
					gamecenter.show_leaderboard('default', 'all')
				end
			end
		end)
	end

	hud.update()

	return true
end

function menu.render(t)
	menu_grid:draw(grid_pos, 1, t)

	if t == 0 then
		hud.render()
	end

	return true
end

return menu
