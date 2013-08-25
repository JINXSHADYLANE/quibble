local game = {}

local textmode = require('textmode')
local room = require('room')
local cavegen = require('cavegen')
local spells = require('spells')
local timeline = require('timeline')

local tm = nil
local r = nil

local show_spells = false
local show_quit = false
current_level = 1

function game.init()
	tm = textmode:new()

	current_level = 1
	r = room:new(cavegen.make(40, 17), 1)
end

function game.reset()
	r = room:new(cavegen.make(40, 17), current_level)
	r.fadein_t = time.s()
end

function game.update()
	if show_quit then
		if char.down('y') then
			return false
		end
		if char.down('n') or key.down(key.quit) then
			show_quit = false
		end
	elseif key.down(key.quit) then
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
	else
		timeline.text2 = 'hjkl/arrows - move, w - wait, s - spells'
	end
	tm:present(3, t)
	return true
end

return game
