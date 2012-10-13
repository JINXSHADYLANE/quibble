local game = {}

local levels = require('levels')
local ghost = require('ghost')

local player = nil
local player_pos = nil

function game.init()
	video.set_blendmode(glow_layer, 'add')
	video.set_blendmode(ghost_layer, 'add')
	player_pos = levels.reset(1)

	player = ghost.make(player_pos, rgba(0.5, 0.5, 0.5))
end

function game.close()
end

function game.enter()
end

function game.close()
end

function game.update()

	local delta = nil
	if key.down(key._up) or char.down('w') then
		delta = vec2(0, -1)
	elseif key.down(key._right) or char.down('d') then
		delta = vec2(1, 0)
	elseif key.down(key._down) or char.down('s') then
		delta = vec2(0, 1)
	elseif key.down(key._left) or char.down('a') then
		delta = vec2(-1, 0)
	end

	if delta then
		ghost.move(player, delta)
		player_pos = player_pos + delta
	end

	levels.update(player_pos)

	return not key.down(key.quit)
end

function game.render(t)
	local ts = time.s()

	levels.draw()
	ghost.draw(player, ts)
	return true
end

return game

