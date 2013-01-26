local character = {}
local character_mt = {}
character_mt.__index = character

local function controls1(self, switch_hearts)
	if key.pressed(key._left) then
		self.vel.x = self.vel.x - self.move_acc
		self.dir = true
	end
	if key.pressed(key._right) then
		self.vel.x = self.vel.x + self.move_acc
		self.dir = false
	end
	if key.down(key._up) and self.ground then
		self.ground = false
		self.vel.y = -self.jump_acc
		--mfx.trigger('jump')
	end
	
	if self.heart and key.down(key._down) then
		switch_hearts()
	end
end

local function controls2(self, switch_hearts)
	if char.pressed('a') then
		self.vel.x = self.vel.x - self.move_acc
		self.dir = true
	end
	if char.pressed('d') then
		self.vel.x = self.vel.x + self.move_acc
		self.dir = false
	end
	if char.down('w') and self.ground then
		self.ground = false
		self.vel.y = -self.jump_acc
		--mfx.trigger('jump')
	end

	if self.heart and char.down('s') then
		switch_hearts()
	end
end

local controls = {controls1, controls2}
local sprites = {'char1', 'char2'}

function character:new(obj, i)
	local o = {
		width = 32,
		height = 64,
		move_acc = 0.5,
		move_damp = 0.8,
		jump_acc = 7.6,

		pos = obj.pos + vec2(32/2, 64/2),
		vel = vec2(0, 0),
		dir = false,
		ground = true,
		frame = 0,
		sprite = sprites[i],
		controls = controls[i]
	}
	if i == 1 then
		o.heart = true
	end
	setmetatable(o, character_mt)
	return o
end

function character:update(sweep_rect, switch_hearts)
	self:controls(switch_hearts)

	self.vel.y = self.vel.y + gravity
	self.vel.x = self.vel.x * self.move_damp

	local bbox = rect(
		self.pos.x - self.width / 2,
		self.pos.y - self.height / 2
	)
	bbox.r = bbox.l + self.width
	bbox.b = bbox.t + self.height

	local dx = sweep_rect(bbox, vec2(self.vel.x, 0), not self.ground)
	self.pos = self.pos + dx
	bbox.l = bbox.l + dx.x
	bbox.r = bbox.r + dx.x
	self.vel.x = dx.x
	
	local dy = sweep_rect(bbox, vec2(0, self.vel.y), true)
	local was_on_ground = self.ground
	if self.vel.y > 0 then
		self.ground = dy.y == 0
		if self.ground then
			--self.frame = 0
		end
	end

	--[[
	if (not was_on_ground) and self.ground then
		mfx.trigger('land')
	end
	]]

	self.pos = self.pos + dy
	self.vel.y = dy.y
	bbox.t = bbox.t + dy.y
	bbox.b = bbox.b + dy.y
	self.bbox = bbox

	if self.ground then
		if math.abs(self.vel.x) > 0.01 then
			local t = time.s()
			--self.frame = math.fmod(math.floor(t * 10), 3)
		else
			--self.frame = 0
		end
	end
end

function character:render(level)
	if self.bbox then
		local pos = tilemap.world2screen(level, scr_rect, self.bbox)
		if self.dir then
			pos.l, pos.r = pos.r, pos.l
		end

		local spr = self.sprite
		if self.heart then
			spr = spr .. 'h'
		end
		sprsheet.draw(spr, 3, pos)
	end
end

return character
