local player = {}
player.__index = player

local move_speed = 2

function player:new()
	local o = {
		p = vec2(0, 0),
		r = 20 
	}
	setmetatable(o, self)
	return o
end

function player:update(hexgrid)
	local dir = vec2(0, 0)

	if key.pressed(key._up) then
		dir = dir + vec2(0, -1)
	end
	if key.pressed(key._down) then
		dir = dir + vec2(0, 1)
	end
	if key.pressed(key._left) then
		dir = dir + vec2(-1, 0)
	end
	if key.pressed(key._right) then
		dir = dir + vec2(1, 0)
	end

	if length_sq(dir) > 0 then
		dir = normalize(dir)
		local offset = dir * move_speed
		offset = hexgrid:collide_circle(self.p, self.r, offset)
		self.p = self.p + offset
	end
end

function player:draw(camera)
	local w, h = width(screen_rect), height(screen_rect)
	local topleft = camera - vec2(w/2, h/2)

	-- draw a circle
	for i=1,10 do
		local ox1 = math.cos((i-1)/10 * math.pi * 2)
		local oy1 = math.sin((i-1)/10 * math.pi * 2)
		local ox2 = math.cos(i/10 * math.pi * 2)
		local oy2 = math.sin(i/10 * math.pi * 2)

		video.draw_seg(1, 
			self.p + vec2(ox1, oy1) * self.r - topleft, 
			self.p + vec2(ox2, oy2) * self.r - topleft,
			rgba(1, 1, 1, 1)
		)
	end
end

return player
