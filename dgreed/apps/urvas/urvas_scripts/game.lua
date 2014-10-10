local game = {}

local textmode = require('textmode')
local room = require('room')
local cavegen = require('cavegen')
local spells = require('spells')
local timeline = require('timeline')
local object = require('object')

local tm = nil
local r = nil

local show_spells = false
local show_quit = false
local show_gameover = false
current_level = 1

function game.init()
	tm = textmode:new()

	current_level = 1
	r = room:new(cavegen.make(40, 17), current_level)
end

function game.reset()
	r = room:new(cavegen.make(40, 17), current_level)
	r.fadein_t = time.s()
	timeline.text = ''
	timeline.text2 = ''
end

function game.update()
	if show_quit then
		if char.down('y') then
			return false
		end
		if char.down('n') or key.down(key.quit) then
			show_quit = false
		end
	elseif not r.spell and key.down(key.quit) then
		if show_spells then
			show_spells = false
		else
			show_quit = true
		end
	end

	if not show_spells and char.down('s') then
		show_spells = true
	end

	if show_spells then
		show_spells = spells.update(r)
	elseif show_gameover then
		timeline.update()
		if not timeline.ascended then
			timeline.text2 = 'Play again? (y/n)'
		end
		if char.down('n') then
			return false
		end
		
		if char.down('y') then
			current_level = 1
			game.reset()
			for i=3,9 do
				spells[i].have = false
			end
			show_gameover = false
			timeline.ascended = false
			timeline.current = 10
			timeline.display = 10
			object.kill_count = 0
			cheated = false
		end
	elseif not show_quit then
		if r.spell == nil then
			for i=1,9 do
				if char.down(tostring(i)) then
					spells.cast(i, r)
				end
			end
		end
		r:update()
		timeline.update()
		if spells.delegate then
			spells.delegate()
			spells.delegate = nil
		end
	end

	-- cheat to unlock all spells
	if char.pressed('c') and char.pressed('a') and char.pressed('t') then
		if not cheated then
			timeline.text = 'You learn all spells.'
			for i,spell in ipairs(spells) do
				spell.have = true
			end
		end
		cheated = true
	end

	if timeline.current == 0 or timeline.ascended then
		show_gameover = true
	end

	return true
end

function game.render(t)
	if r:render(tm) then
		game.reset()
		return true
	end
	timeline.render(tm, show_spells or show_quit)
	if show_spells then
		spells.render(tm)
	elseif show_quit then
		tm:write(0,19, 'Quit? Progress will be lost! (y/n)    ')
	elseif not show_gameover and not timeline.ascended and not r.spell then
		timeline.text2 = 'hjkl/arrows - move, w - wait, s - spells'
	end
	tm:present(3, t)
	return true
end

return game
