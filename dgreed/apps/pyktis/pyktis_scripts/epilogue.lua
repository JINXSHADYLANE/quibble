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
	local black = rgba(0, 0, 0, alpha)
	local grey = rgba(0.5, 0.5, 0.5, alpha)
	sprsheet.draw('empty', 0, scr_rect, white)

	vfont.select(fnt, 64)
	local txt = 'The end.'
	local size = vfont.size(txt)
	vfont.draw(txt, 15, (scr_size - size) * 0.5)
	vfont.select(fnt, 40)

	return true
end

return epilogue 
