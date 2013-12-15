local crusher = {}
local crusher_mt = {}
crusher_mt.__index = crusher

function crusher:new(pos)
	local o = {
		size = 10,
		pos = pos,
		vel = vec2(0, 0.2),
		sprite = 'crusher',
		layer = 1,
		invincible = true
	}
	self.bbox = calc_bbox(o)
	setmetatable(o, crusher_mt)
	return o
end

function crusher:update(sector)
	local dp = tilemap.collide_swept(sector, self.bbox, self.vel)
	self.pos = self.pos + dp
	self.bbox = calc_bbox(self)

	-- switch directions
	if not feql(dp.y, self.vel.y) then
		self.vel.y = self.vel.y * -1
	end
	if not feql(dp.x, self.vel.x) then
		self.vel.x = self.vel.x * -1
	end
end

function crusher:render(sector)
	local pos = tilemap.world2screen(sector, scr_rect, snap_pos(self.pos))
	local rot = self.rot or 0
	sprsheet.draw_centered(self.sprite, self.layer, pos, rot, 1)
end

function crusher:collide(sector, other)
	if not other.dead then
		other.dead = true
		local screen_pos = tilemap.world2screen(sector, scr_rect, other.pos)
		mfx.trigger('explode', screen_pos)
	end
end

return crusher
