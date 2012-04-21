local game = {}

function game.init()
end

function game.close()
end

function game.enter()
end

function game.close()
end

function game.update()
	if key.up(key.quit) then
		states.pop()
	end
	
	return true
end

function game.render(t)
	video.present()
	return true
end

return game
