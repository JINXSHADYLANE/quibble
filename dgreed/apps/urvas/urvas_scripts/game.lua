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

function game.init()
	tm = textmode:new()

	r = room:new(cavegen.make(40, 17))
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
		spells.update()
	elseif not show_quit then
		r:update()
		timeline.update()
	end

	return true
end

function game.render(t)
	r:render(tm)
	timeline.render(tm)
	if show_spells then
		spells.render(tm)
	elseif show_quit then
		tm:write(0,19, 'Quit? Progress will be lost! (y/n)')
	else
		tm:write(0,19,'hjkl/arrows - move, w - wait, s - spells')
	end
	tm:present(3, t)
	return true
end

return game
