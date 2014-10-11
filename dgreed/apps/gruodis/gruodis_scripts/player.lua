local player = {}
local player_mt = {}
player_mt.__index = player

local bullet = require('bullet')
local bomb = require('bomb')

function player:new(pos)
	local o = {
		is_player = true,
		size = 8,
		pos = vec2(pos),
		vel = vec2(0, 0),
		move_acc = 0.2, 
		dir = 0,
		sprite = 'player_ship',
		layer = 3,
		last_shot = 0,
		shoot_interval = 0.2,
		last_bomb = 0,
		bomb_interval = 3,
		shield = false,
		shield_energy = 1,

		save_sector = nil,
		save_pos = nil
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
			return {bullet:new(self)}
		end
	end

	-- shield
	if char.pressed('x') then
		self.shield_energy = math.max(0, self.shield_energy-0.004)
		self.shield = true
	else
		self.shield_energy = math.min(1, self.shield_energy+0.004)
		self.shield = false
	end

	-- bomb
	if self.last_bomb + self.bomb_interval < time.s() then
		if char.pressed('c') then
			self.last_bomb = time.s()
			return {bomb:new(self.pos)}
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

	-- ask game module to switch sector if we're on the screen edge
	if bbox.l <= 1 then
		self.change_sector = vec2(-1, 0)
	elseif bbox.r >= scr_size.x-1 then
		self.change_sector = vec2(1, 0)
	elseif bbox.t <= 1 then
		self.change_sector = vec2(0, -1)
	elseif bbox.b >= scr_size.y-1 then
		self.change_sector = vec2(0, 1)
	end

	return new
end

function player:render(sector)
	local dest = tilemap.world2screen(sector, scr_rect, snap_pos(self.pos))
	sprsheet.draw_centered(
		self.sprite, self.layer,
		dest, self.dir, 1
	)

	if self.shield and self.shield_energy > 0 then
		local col = rgba(1, 1, 1, self.shield_energy)
		sprsheet.draw_centered(
			'shield', self.layer,
			dest, 0, 1.3, col
		)
	end
	
	-- draw debug bbox
	local box = tilemap.world2screen(sector, scr_rect, self.bbox)
	sprsheet.draw('empty', self.layer+1, box, rgba(1,1,1,0.5))
end

return player
