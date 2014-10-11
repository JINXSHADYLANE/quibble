local epilogue = {}

function epilogue.update()
	if char.pressed(' ') then
		return false
	end
	return true
end

function epilogue.render(t)
	local alpha = 1 - math.abs(t)
	local white = rgba(1, 1, 1, alpha)
	sprsheet.draw('outro', 1, scr_rect, white)

	return true
end

return epilogue 
