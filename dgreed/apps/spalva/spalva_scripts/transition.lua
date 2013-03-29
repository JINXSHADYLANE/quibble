local transition = {}

function transition.update()
	states.pop()
	if transition.cb then
		transition.cb()
		transition.cb = nil
	end
	return true
end

function transition.render(t)
	local alpha = 1 - math.abs(t)
	sprsheet.draw('background', 15, scr_rect, rgba(0, 0, 0, alpha))
	return true
end

return transition
