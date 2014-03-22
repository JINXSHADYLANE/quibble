local player = {}
local player_mt = {}
player_mt.__index = player

function player.new(pos)
	local o = {
		id = 'player',
		pos = pos,
		sprite = 'spaceship',
		collision = nil,
		removed = false,

		speed = -20,
		vel = vec2(0, 0),
	}
	setmetatable(o, player_mt)
	return o
end

function player.create(self, cd)
	local r = rect(
		self.pos.x - 20, self.pos.y - 20,
		self.pos.x + 20, self.pos.y + 20
	)
	self.collision = coldet.new_aabb(cd, r)
end

function player.destroy(self, cd)
	cdobj.remove(cd, self.collision)
end

function player.update(self, time)
	self.vel.x = self.vel.x * 0.95	
	self.vel.y = lerp(self.vel.y, self.speed, 0.05)

	if key.pressed(key._left) then
		self.vel.x = self.vel.x - 0.1 
	end

	if key.pressed(key._right) then
		self.vel.x = self.vel.x + 0.1
	end

	self.pos = self.pos + self.vel * (1/60)

	self.pos.x = clamp(40, scr_size.x - 40, self.pos.x)
end

return player

