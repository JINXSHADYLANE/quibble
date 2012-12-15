local game = {}

local textmode = require('textmode')

function game.init()
	textmode.init()

	textmode.write_middle(6, 'Urvas')
	textmode.selected_fg = rgba(0.4, 0.4, 0.4)
	textmode.write_middle(15, 'press space to start')
	textmode.selected_fg = rgba(0.1, 0.1, 0.1)
	textmode.rect(0, 0, 39, 19, '#')
end

function game.close()
	textmode.close()
end

function game.update()
	if char.down('q') or key.down(key.quit) then
		return false
	end
	return true
end

function game.render(t)
	textmode.present(3)
	return true
end

return game
