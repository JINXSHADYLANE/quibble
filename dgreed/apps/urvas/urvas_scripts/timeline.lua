local timeline = {}

timeline.current = 10
timeline.display = 10

function timeline.pass(n)
	timeline.current = timeline.current - n
end

function timeline.update()
	timeline.display = lerp(timeline.display, timeline.current, 0.1)
end

local good_color = rgba(0.2, 0.2, 0.8)
local bad_color = rgba(0.8, 0.2, 0.2)

function timeline.render(tm)
	local t = timeline.display / 10
	local color = lerp(bad_color, good_color, t)
	local int_tiles, frac_tiles = math.modf(t * 40)
	tm:push()
	tm.selected_fg = rgba(0.9, 0.9, 0.9)
	tm:write(0, 17, string.rep(' ', int_tiles-1))
	tm:write_middle(17, string.format('%d seconds left', timeline.current))
	tm.selected_bg = color
	tm:recolour(0, 17, int_tiles)
	tm.selected_bg = lerp(rgba(0, 0, 0), color, frac_tiles)
	tm:recolour(int_tiles, 17,  1)
	tm:pop()
end

return timeline
