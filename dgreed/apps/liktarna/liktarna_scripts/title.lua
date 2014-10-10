local title = {}

function title.update()
	if char.pressed(' ') then
		states.replace('game')
	end
	return true
end

function title.render(t)
	local alpha = 1 - math.abs(t)
	local white = rgba(1, 1, 1, alpha)
	local black = rgba(0, 0, 0, alpha)
	local grey = rgba(0.5, 0.5, 0.5, alpha)
	sprsheet.draw('empty', 0, scr_rect, white)

	vfont.select(fnt, 64)
	vfont.draw('Liktarna', 1, vec2(48, 48), black)
	vfont.select(fnt, 32)
	vfont.draw('light my way', 1, vec2(48, 112), grey)
	vfont.select(fnt, 24)
	local txt = 'press space to play'
	local size = vfont.size(txt)
	vfont.draw(txt, 1, vec2(scr_size.x/2, 410) - size*0.5, grey)

	vfont.draw('Controls:', 1, vec2(410, 148), grey)
	vfont.draw('arrows - move', 1, vec2(410, 175), grey)
	vfont.draw('space - undo', 1, vec2(410, 200), grey)
	vfont.draw('esc - reset puzzle', 1, vec2(410, 225), grey)

	vfont.select(fnt, 40)

	return true
end

return title
