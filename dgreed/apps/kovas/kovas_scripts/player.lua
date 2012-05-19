local player = {}
player.__index = player

local move_speed = 2
local hit_anim_len = 0.5 

function player:new()
	local o = {
		p = vec2(0, 0),
		r = 20,
		dir = 0,
		frame = 0,
		hit_t = nil
	}
	setmetatable(o, self)
	return o
end

function player:update(hexgrid)
	local dir = vec2(0, 0)

	if not self.hit_t then 
		if char.down('z') then
			self.hit_t = time.s()
		end

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
			self.frame = self.frame + length(offset)/6
			if self.frame >= 15 then
				self.frame = self.frame - 15
			end
			offset = hexgrid:collide_circle(self.p, self.r, offset)
			self.p = self.p + offset

			-- player sprite direction
			local angle = (math.atan2(dir.y, dir.x) + math.pi) / (math.pi * 2)
			angle = math.fmod(angle + 0.75, 1)
			self.dir = math.floor(angle * 8 + 0.5)
			if self.dir == 8 then self.dir = 0 end
		end
	else
		local t = time.s()
		if t - self.hit_t >= hit_anim_len then
			self.hit_t = nil
		end
	end
end

function player:draw(camera)
	local w, h = width(screen_rect), height(screen_rect)
	local topleft = camera - vec2(w/2, h/2)

	-- draw a circle
	--[[
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
	]]

	-- draw player
	local f = math.floor(self.frame)
	local p = self.p - topleft
	if not self.hit_t then
		p = p - vec2(0, 66)
		sprsheet.draw_anim_centered('player_walk', self.dir * 15 + f, 1, p)
	else
		p = p - vec2(0, 70)
		local t = (time.s() - self.hit_t) / hit_anim_len
		t = clamp(0, 0.999, t)
		f = math.floor(t * 10)
		sprsheet.draw_anim_centered('player_hit', self.dir * 10 + f, 1, p)
	end
end

return player
