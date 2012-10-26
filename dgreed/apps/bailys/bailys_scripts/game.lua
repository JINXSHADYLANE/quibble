local game = {}

function game.init()
end

function game.close()
end

function game.update()
	-- exit game if esq or q was pressed
	if key.down(key.quit) or char.down('q') then
		return false
	end

	return true
end

function game.render(t)
	return true
end

return game

