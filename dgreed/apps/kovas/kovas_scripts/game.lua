local hexgrid = require('hexgrid')

local game = {}

function game.init()
	game.map = hexgrid:new()

	game.map:set(0, 0, 1)
	game.map:set(-1, 0, 1)
	game.map:set(0, 1, 1)

end

function game.close()
end

function game.enter()
end

function game.leave()
end

function game.update()
	if key.down(key.quit) then
		states.pop()
	end
	return true
end

function game.render(t)
	game.map:draw(vec2(0, 0), rect(0, 0, scr_size.x, scr_size.y))

	return true
end

return game
