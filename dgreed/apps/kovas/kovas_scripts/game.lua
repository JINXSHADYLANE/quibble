local hexgrid = require('hexgrid')
local player = require('player')

local game = {}

function game.init()
	game.map = hexgrid:new()
	game.player = player:new()

	game.camera = vec2(0, 0)

	-- plant 100000 trees
	for i=1,100000 do
		local x = rand.int(-500, 500)
		local y = rand.int(-500, 500)
		game.map:set(x, y, 'tree')
	end

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

	game.player:update(game.map)
	game.camera = lerp(game.camera, game.player.p, 0.1)

	return true
end

function game.render(t)
	local screen = rect(0, 0, scr_size.x, scr_size.y)
	game.map:draw(game.camera, screen)
	game.player:draw(game.camera, screen)

	return true
end

return game
