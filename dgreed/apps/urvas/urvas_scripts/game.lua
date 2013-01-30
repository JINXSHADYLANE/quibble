local game = {}

local textmode = require('textmode')
local room = require('room')

local tm = nil
local r = nil

function game.init()
	tm = textmode:new()

	r = room:new(room.descs[1])
end

function game.update()
	if char.down('q') or key.down(key.quit) then
		return false
	end

	r:update()

	return true
end

function game.render(t)
	r:render(tm)
	tm:present(3, t)
	return true
end

return game
