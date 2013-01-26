local game = {}

function game.init()
	game.reset()
end

function game.reset()
end

function game.close()
end

function game.enter()
end

function game.close()
end

function game.update()
	local ts = time.s()

	sound.update()

	return not key.down(key.quit)
end

function game.render(t)
	local ts = time.s()

	return true
end

return game

