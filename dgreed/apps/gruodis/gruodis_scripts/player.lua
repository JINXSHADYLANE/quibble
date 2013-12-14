local player = {}
local player_mt = {}
player_mt.__index = player

local bullet = require('bullet')

function player:new(obj)
	local o = {
		size = 8,
		pos = obj.pos + vec2(5, 5),
		vel = vec2(0, 0),
		move_acc = 0.2, 
		dir = 0,
		sprite = 'player_ship',
		layer = 1,
		last_shot = 0,
		shoot_interval = 0.2
	}
	setmetatable(o, player_mt)
	return o
end

function player:control()
	-- move
	if key.pressed(key._left) then
		self.vel.x = self.vel.x - self.move_acc
		self.dir = math.pi 
	end
	if key.pressed(key._right) then
		self.vel.x = self.vel.x + self.move_acc
		self.dir = 0
	end
	if key.pressed(key._up) then
		self.vel.y = self.vel.y - self.move_acc
		self.dir = math.pi / 2 * 3
	end
	if key.pressed(key._down) then
		self.vel.y = self.vel.y + self.move_acc
		self.dir = math.pi / 2
	end

	-- move diagonally at the same speed
	if length_sq(self.vel) > 1 then
		self.vel = self.vel / math.sqrt(2)
	end

	-- shoot
	if self.last_shot + self.shoot_interval < time.s() then
		if char.pressed('z') then
			self.last_shot = time.s()
			return bullet:new(self)
		end
	end
end

function player:update(sector)
	local new = self:control()

	-- heavily damp movement
	self.vel = self.vel * 0.9

	-- calculate bounding box
	local s = self.size
	local bbox = rect(
		self.pos.x - s * 0.5,
		self.pos.y - s * 0.5
	)
	bbox.r = bbox.l + s
	bbox.b = bbox.t + s

	-- move it against sector collision
	local dp = tilemap.collide_swept(sector, bbox, self.vel)
	self.pos = self.pos + dp
	bbox.l = bbox.l + dp.x
	bbox.r = bbox.r + dp.x
	bbox.t = bbox.t + dp.y
	bbox.b = bbox.b + dp.y
	self.vel = dp
	self.bbox = bbox
	
	return new
end

function player:render(sector)
	local dest = vec2(
		math.floor(self.pos.x),
		math.floor(self.pos.y)
	)

	tilemap.world2screen(sector, scr_rect, dest)

	sprsheet.draw_centered(
		self.sprite, self.layer,
		dest, self.dir, 1
	)
end




return player
