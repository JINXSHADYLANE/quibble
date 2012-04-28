local hexgrid = require('hexgrid')

local game = {}

function game.init()
	game.map = hexgrid:new()

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

	if key.pressed(key._up) then
		game.camera.y = game.camera.y - 2
	end

	if key.pressed(key._down) then
		game.camera.y = game.camera.y + 2
	end
	
	if key.pressed(key._left) then
		game.camera.x = game.camera.x - 2
	end

	if key.pressed(key._right) then
		game.camera.x = game.camera.x + 2
	end

	return true
end

function game.render(t)
	game.map:draw(game.camera, rect(0, 0, scr_size.x, scr_size.y))

	return true
end

return game
