local game = {}

local bg_color = rgba(0.92, 0.9, 0.85, 1)

function game.init()
end

function game.close()
end

function game.update()
	-- exit game if esc or q was pressed
	if key.down(key.quit) or char.down('q') then
		return false
	end

	return true
end

function game.render(t)
	-- slightly off-white background
	sprsheet.draw('empty', 0, scr_rect, bg_color)
	
	return true
end

return game

