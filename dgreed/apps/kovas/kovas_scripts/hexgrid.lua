local hexgrid = {}
hexgrid.__index = hexgrid

local side_len = 32
local cell_height = math.sqrt(3) * side_len
local max_width = 43640 -- (2**31) / 43640 = 43641 

function hexgrid:new()
	local o = {
		map = {}
	}
	setmetatable(o, self)
	return o
end

function hexgrid:set(x, y, o)
	assert(math.floor(x) == x)
	assert(math.floor(y) == y)
	self.map[y * max_width + x] = o
end

function hexgrid:get(x, y)
	assert(math.floor(x) == x)
	assert(math.floor(y) == y)
	return self.map[y * max_width + x]
end

function hexgrid:center(x, y)
	local wx = x * side_len * 1.5
	local wy = y * cell_height
	if math.abs(math.fmod(x, 2)) == 1 then
		wy = wy - cell_height/2
	end
	return vec2(wx, wy)
end

function hexgrid:world2grid(pt)
	local x = (pt.x + side_len) / (side_len * 1.5)
	local xi, xf = math.modf(x)
	if xf < 0 then
		xi = xi-1
		xf = 1 + xf
	end

	local y_shift = 0
	if math.abs(math.fmod(xi, 2)) == 1 then
		y_shift = cell_height / 2
	end

	local y = (pt.y + cell_height - y_shift) / cell_height
	local yi, yf = math.modf(y)
	if yf < 0 then
		yi = yi-1
		yf = 1 + yf
	end

	local ox1, oy1, ox2, oy2, ox3, oy3

	ox1, oy1 = xi, yi
	ox2, oy2 = xi-1, yi
	if y_shift == 0 then
		ox3, oy3 = xi, yi-1
	else
		ox3, oy3 = xi, yi+1
	end	

	local c1 = self:center(ox1, oy1)
	local c2 = self:center(ox2, oy2)
	local c3 = self:center(ox3, oy3)

	local d1 = length_sq(c1 - pt)
	local d2 = length_sq(c2 - pt)
	local d3 = length_sq(c3 - pt)

	if d1 <= d2 and d1 <= d3 then
		return ox1, oy1
	elseif d2 < d1 and d2 < d3 then
		return ox2, oy2
	else
		return ox3, oy3
	end
end

function hexgrid:draw_hex(pos, col)
	local a = pos + vec2(-side_len / 2, -cell_height / 2)
	local b = pos + vec2(side_len / 2, -cell_height / 2)
	local c = pos + vec2(side_len, 0)
	local d = pos + vec2(side_len / 2, cell_height / 2)
	local e = pos + vec2(-side_len / 2, cell_height / 2)
	local f = pos + vec2(-side_len, 0)

	video.draw_seg(1, a, b, col)
	video.draw_seg(1, b, c, col)
	video.draw_seg(1, c, d, col)
	video.draw_seg(1, d, e, col)
	video.draw_seg(1, e, f, col)
	video.draw_seg(1, f, a, col)
end

function hexgrid:draw(camera_pos, screen_rect)
	local w, h = width(screen_rect), height(screen_rect)
	local topleft = camera_pos - vec2(w/2, h/2)

	local min_x, min_y = self:world2grid(camera_pos - vec2(w/2, h/2))
	local max_x, max_y = self:world2grid(camera_pos + vec2(w/2, h/2))

	local col = rgba(1, 1, 1, 1)

	for y = min_y, max_y do
		for x = min_x, max_x do
			local t = self:get(x, y)
			local c = self:center(x, y) - topleft 
			if t == nil then
				self:draw_hex(c, col)
			else
				video.draw_seg(1, c - vec2(10, 10), c + vec2(10, 10), rgba(0, 0.8, 0, 1))
				video.draw_seg(1, c - vec2(-10, 10), c + vec2(-10, 10), rgba(0, 0.8, 0, 1))
			end
		end
	end

	local p = topleft + mouse.pos()
	local x, y = self:world2grid(p)
	local c = self:center(x, y) - topleft
	self:draw_hex(c, rgba(0.1, 0.1, 0.1, 1))
end

function hexgrid:world2screen(pt, camera_pos, screen_rect)
end

return hexgrid

