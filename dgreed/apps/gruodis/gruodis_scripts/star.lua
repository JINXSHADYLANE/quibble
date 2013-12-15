local star = {}
local star_mt = {}
star_mt.__index = star

local bullet = require('bullet')

function star:new(pos)
	local o = {
		size = 8,
		pos = pos,
		rot = 0,
		target_rot = 0,
		sprite = 'star',
		layer = 1,
		last_shot = 0,
		shoot_interval = 0.8
	}
	local hs = o.size * 0.5
	o.bbox = rect(
		o.pos.x - hs, o.pos.y - hs,
		o.pos.x + hs, o.pos.y + hs
	)
	setmetatable(o, star_mt)
	return o
end

function star:update(sector)
	local new = nil
	local half_pi = math.pi*0.5
	local t = time.s()
	local ls = self.last_shot
	local dt = self.shoot_interval

	if ls + dt * 0.5 > t then
		local tt = (t - ls) / (dt * 0.5)
		self.rot = smoothstep(self.target_rot - half_pi, self.target_rot, tt)
	end

	-- shoot in every four directions every 'shoot_interval' seconds
	if ls + dt < t then
		self.last_shot = t
		self.target_rot = self.target_rot + half_pi
		new = {}
		for dir = 0,3*half_pi,half_pi do
			self.dir = dir
			table.insert(new, bullet:new(self))
		end
		self.dir = nil
	end

	return new
end

function star:render(sector)
	local pos = tilemap.world2screen(sector, scr_rect, snap_pos(self.pos))
	sprsheet.draw_centered(self.sprite, self.layer, pos, self.rot, 1)
end

return star
