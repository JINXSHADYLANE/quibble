local intro = {}

local textmode = require('textmode')
local game = {}

local tm = nil

function intro.init()
	tm = textmode:new()
	tm:write_middle(6, 'Urvas')
	tm.selected_fg = rgba(0.4, 0.4, 0.4)
	tm:write_middle(15, 'press space to start')
	tm.selected_fg = rgba(0.1, 0.1, 0.1)
	tm:rect(0, 0, 39, 19, '#')
end

function intro.update()
	if char.down('q') or key.down(key.quit) then
		return false
	end

	if char.down(' ') then
		game.level = 1
		states.replace('game')
	end

	return true
end

function intro.render(t)
	tm:present(3, t)
	return true
end

return intro
