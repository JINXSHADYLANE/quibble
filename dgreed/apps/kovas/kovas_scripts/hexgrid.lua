local hexgrid = {}
hexgrid.__index = hexgrid

local side_len = 40 
local cell_height = math.sqrt(3) * side_len
local max_width = 43640 -- (2**31) / 43640 = 43641 

local ghosts = require('ghosts')

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

function hexgrid:collide_circle(p, r, offset)
	-- returns collision normal or nil
	local function circle_to_hex(hpos, cpos, cr, offset)
		local a = hpos + vec2(-side_len / 2, -cell_height / 2)
		local b = hpos + vec2(side_len / 2, -cell_height / 2)
		local c = hpos + vec2(side_len, 0)
		local d = hpos + vec2(side_len / 2, cell_height / 2)
		local e = hpos + vec2(-side_len / 2, cell_height / 2)
		local f = hpos + vec2(-side_len, 0)
		local pts = {a, b, c, d, e, f}

		local maxp, maxi = 0, 0
		local p = cpos + offset
		local pd = {}
		for i=1,6 do
			local j = i+1
			if j > 6 then j = j - 6 end
			pd[i] = segment_to_point(pts[i], pts[j], p)
			pd[i] = math.abs(pd[i]) - r
			if pd[i] < 0 then
				if pd[i] < maxp then
					maxp = pd[i]
					maxi = i
				end
			end
		end

		if maxi > 0 then
			return rotate(vec2(0, maxp), (math.pi*2 / 6) * (maxi-1)) 
		else
			return nil
		end
	end

	local noff = vec2(offset)

	local min_x, min_y = self:world2grid(p - vec2(r*3.5, r*3.5))
	local max_x, max_y = self:world2grid(p + vec2(r*3.5, r*3.5))

	local check_n = 0
	local deflect_n = 0
	local deflect_acc = vec2(0, 0)
	for y = min_y, max_y do
		for x = min_x, max_x do
			local t = self:get(x, y)
			local c = self:center(x, y) 
			if t ~= nil then
				local n = circle_to_hex(c, p, r, offset)
				check_n = check_n + 1
				if n ~= nil then
					deflect_acc = deflect_acc + n
					deflect_n = deflect_n + 1
				end
			end
		end
	end

	if deflect_n > 1 then
		deflect_acc = deflect_acc / deflect_n
	end

	return noff + deflect_acc
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

function hexgrid:draw(camera_pos, screen_rect, player_pos, loff)
	local w, h = width(screen_rect), height(screen_rect)
	local topleft = camera_pos - vec2(w/2, h/2)

	w = w + side_len * 4 + 800
	h = h + side_len * 4 + 800

	local min_x, min_y = self:world2grid(camera_pos - vec2(w/2, h/2))
	local max_x, max_y = self:world2grid(camera_pos + vec2(w/2, h/2))

	local col = rgba(1, 1, 1, 1)
	local fire_col = rgba(0.8, 0.2, 0.2, 1)

	local light_offset = vec2(0, 0)
	if loff then
		light_offset = loff
	end

	local lights = {{
		pos = player_pos - topleft + light_offset * 1.2,
		radius = 250 + length(light_offset),
		alpha = 1
	}}

	for y = min_y, max_y do
		for x = min_x, max_x do
			local t = self:get(x, y)
			local cnt = self:center(x, y)
			local c = cnt - topleft 
			if t ~= nil then
				if t == 1 then
					-- bush
					local layer = 1 -- pre player
					if cnt.y > player_pos.y then
						layer = 3
					end
					local p = c - vec2(0, 5)
					sprsheet.draw_centered('bush', layer, p)	
				elseif t == 2 or t == 3 then
					-- altar
					local r = 400
					if t == 2 then r = 100 end
					table.insert(lights, {
						pos = c,
						radius = r,
						alpha = 1
					})

					local layer = 1 -- pre player
					if cnt.y > player_pos.y then
						layer = 3
					end
					local p = c - vec2(0, 5)

					if t == 2 then
						sprsheet.draw_centered('altar', layer, p)
						if length(cnt - player_pos) < 80 then
							self:set(x, y, 3)
							if not self.active_altars then
								self.active_altars = 0
							end
							self.active_altars = self.active_altars + 1
						end
					else
						local f = x * 10 + y + time.s() * 16 
						f = math.fmod(math.abs(f), 8)
						f = math.floor(f)
						sprsheet.draw_anim_centered('altar_burn', f, layer, p)
					end
				else
					-- tree
					cnt = cnt + t
					c = c + t
					local layer = 1 -- pre player
					if cnt.y > player_pos.y then
						layer = 3
					end
					local p = c - vec2(0, 216)
					sprsheet.draw_centered('tree', layer, p)	
				end
			end
		end
	end

	ghosts.update(camera_pos, lights, player_pos)

	-- mouse mark
	local p = topleft + mouse.pos()
	local x, y = self:world2grid(p)
	local c = self:center(x, y) - topleft
	self:draw_hex(c, rgba(0.1, 0.1, 0.1, 1))

	clighting.render(14, lights)
end

function hexgrid:draw_background(camera_pos, screen_rect)
	local w, h = width(screen_rect), height(screen_rect)

	-- figure out screen space pos of central tile
	local x = math.fmod(camera_pos.x-256, 512)
	if x < 0 then x = x + 512 end
	local y = math.fmod(camera_pos.y-256, 512)
	if y < 0 then y = y + 512 end
	x, y = -x + w/2, -y + h/2

	while x > -512 do x = x - 512 end
	while y > -512 do y = y - 512 end

	while y < h + 512 do
		local nx = x
		while nx < w + 512 do
			sprsheet.draw('background', 0, vec2(nx, y))
			nx = nx + 512
		end
		y = y + 512
	end
end

function hexgrid:world2screen(pt, camera_pos, screen_rect)
end

return hexgrid

