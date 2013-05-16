local hexgrid = require('hexgrid')
local player = require('player')
local ghosts = require('ghosts')

local game = {}

function game.init()
	clighting.init(rect(0, 0, scr_size.x, scr_size.y))

	game.map = hexgrid:new()
	game.player = player:new()

	game.camera = vec2(0, 0)

	-- plant 100000 trees
	for i=1,100000 do
		local x = rand.int(-500, 500)
		local y = rand.int(-500, 500)
		local off = vec2(rand.float(-15, 15), rand.float(-5, 5))
		game.map:set(x, y, off)
	end

	-- plant 10000 bushes
	for i=1,10000 do
		local x = rand.int(-500, 500)
		local y = rand.int(-500, 500)
		game.map:set(x, y, 1)
	end

	-- light some fires
	for i=1,1000 do
		local x = rand.int(-400, 400)
		local y = rand.int(-400, 400)
		game.map:set(x, y, 2)
	end

	ghosts.init()
end

function game.close()
	clighting.close()
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
	game.map:draw_background(game.camera, screen)
	game.map:draw(game.camera, screen, game.player.p, game.player.light_offset)
	game.player:draw(game.camera, screen)
	ghosts.draw(game.camera)

	if game.map.active_altars then
		vfont.draw(tostring(game.map.active_altars), 15, vec2(20, 0), rgba(1, 1, 1, 1))
	end

	return true
end

return game
