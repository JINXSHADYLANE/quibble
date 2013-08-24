local timeline = {}

timeline.current = 10
timeline.display = 10

function timeline.pass(n)
	timeline.current = math.max(0, timeline.current - n)
end

function timeline.update()
	timeline.display = lerp(timeline.display, timeline.current, 0.1)
end

local good_color = rgba(0.2, 0.2, 0.8)
local bad_color = rgba(0.8, 0.2, 0.2)

function timeline.render(tm, hide_text)
	local t = timeline.display / 10
	local color = lerp(bad_color, good_color, t)
	local int_tiles, frac_tiles = math.modf(t * 40)
	tm:push()
	tm.selected_fg = rgba(0.9, 0.9, 0.9)
	tm:write(0, 17, string.rep(' ', int_tiles-1))
	local str 
	if timeline.current > 1 then
		str = string.format('%d seconds left', timeline.current)
	elseif timeline.current == 1 then
		str = string.format('1 second left')
	else
		str = 'Game over'	
	end
	tm:write_middle(17, str)
	tm.selected_bg = color
	tm:recolour(0, 17, int_tiles)
	tm.selected_bg = lerp(rgba(0, 0, 0), color, frac_tiles)
	tm:recolour(int_tiles, 17,  1)
	tm:pop()

	if not hide_text then
		if timeline.text then
			tm:write(0, 18, timeline.text)
		end

		if timeline.text2 then
			tm:write(0, 19, timeline.text2)
		end
	end
end

return timeline
