local game = {}

local textmode = require('textmode')
local room = require('room')
local cavegen = require('cavegen')
local timeline = require('timeline')

local tm = nil
local r = nil

function game.init()
	tm = textmode:new()

	r = room:new(cavegen.make(40, 17))
end

function game.update()
	if char.down('q') or key.down(key.quit) then
		return false
	end

	r:update()
	timeline.update()

	return true
end

function game.render(t)
	r:render(tm)
	timeline.render(tm)
	tm:present(3, t)
	return true
end

return game
