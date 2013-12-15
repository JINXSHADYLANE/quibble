local bullet = {}
local bullet_mt = {}
bullet_mt.__index = bullet

local offset_right = vec2(6, 0)

function bullet:new(shooter)
	local dir = vec2(6, rand.float(-0.2, 0.2))
	local offset = rotate(dir, shooter.dir)
	local o = {
		pos = shooter.pos + offset,
		vel = normalize(offset) * 2,
		sprite = 'bullet',
		layer = 1
	}
	setmetatable(o, bullet_mt)
	return o
end

function bullet:update(sector)
	self.pos = self.pos + self.vel

	-- collide against sector
	if tilemap.collide(sector, self.pos) then
		self.dead = true
	end

	-- collide against screen
	if not rect_point_collision(scr_rect, self.pos) then
		self.dead = true
	end

	if self.dead then
		local screen_pos = tilemap.world2screen(sector, scr_rect, self.pos)
		mfx.trigger('bullet_hit', screen_pos)
	end
end

function bullet:render(sector)
	local pos = tilemap.world2screen(sector, scr_rect, snap_pos(self.pos))
	sprsheet.draw_centered(self.sprite, self.layer, pos, 0, 1)
end

function bullet:collide(sector, other)
	if not other.dead and not other.invincible then
		other.dead = true
		self.dead = true
		local screen_pos = tilemap.world2screen(sector, scr_rect, other.pos)
		mfx.trigger('explode', screen_pos)
	end
end

return bullet

