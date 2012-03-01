local tutorials = {}

local tutorial_map = {
	['two'] =	{
		['tut_arrow'] = vec2(150, 380),
		['tut_finger'] = vec2(54, 160),
		['tut1'] = vec2(170, 105),
		['tut2'] = vec2(80, 336)
	},
	['button'] = {
		['tut3'] = vec2(104, 370)
	},
	['stairs'] = {
		['tut4'] = vec2(140, 294)
	}
}

local layer = 6
local fadein_len = 1
local fadeout_len = 1

local show_t = 0 
local hide_t = nil 
local current = nil

local ipad_offset = vec2(grid_pos.x - 128 * 2.5, grid_pos.y - 128 * 3 - 64)

function tutorials.init()
end

function tutorials.close()
end

function tutorials.show(levelname)
	hide_t = nil
	show_t = time.s()
	current = tutorial_map[levelname]
end

function tutorials.hide(instant)
	if instant then
		hide_t = time.s() + states.transition_len
	else
		if hide_t == nil then
			hide_t = time.s() + 2
		end
	end
end

function tutorials.render(t)
	if current then
		local s = time.s()
		local ts = clamp(0, 1, (s - show_t) / fadein_len)
		if hide_t then
			local th = clamp(0, 1, (s - hide_t) / fadeout_len)
			ts = math.min(ts, 1 - th)

			if th == 1 then
				current = nil
				hide_t = nil
				return
			end
		end

		local col = rgba(1, 1, 1, (1 - math.abs(t)) * ts)

		for spr,pos in pairs(current) do
			if ipad then
				sprsheet.draw(spr, layer, pos*2 + ipad_offset, col) 
			else
				sprsheet.draw(spr, layer, pos, col)
			end
		end
	end
end

return tutorials

