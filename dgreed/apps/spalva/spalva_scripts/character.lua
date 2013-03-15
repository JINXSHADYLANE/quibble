local character = {}
local character_mt = {}
character_mt.__index = character

local gravity = 0.8

local function controls1(self)
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
end

function character:new(obj)
	local o = {
		width = 32,
		height = 32,
		move_acc = 0.5,
		move_damp = 0.8,
		jump_acc = 9.0,

		pos = obj.pos + vec2(32/2, 32/2),
		vel = vec2(0, 0),
		dir = false,
		ground = true,
		frame = 0,
		sprite = 'char',
		anim = anim.new('char'),
		controls = controls1,
		color = i
	}
	setmetatable(o, character_mt)
	return o
end

function character:update(level)
	self:controls()

	self.vel.y = self.vel.y + gravity
	self.vel.x = self.vel.x * self.move_damp

	local bbox = rect(
		self.pos.x - self.width / 2,
		self.pos.y - self.height / 2
	)
	bbox.r = bbox.l + self.width
	bbox.b = bbox.t + self.height

	local dx = tilemap.collide_swept(level, bbox, vec2(self.vel.x, 0))
	self.pos = self.pos + dx
	bbox.l = bbox.l + dx.x
	bbox.r = bbox.r + dx.x
	self.vel.x = dx.x
	
	local dy = tilemap.collide_swept(level, bbox, vec2(0, self.vel.y))
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
		if math.abs(self.vel.x) > 0.05 then
			if not self.walking then
				anim.play(self.anim, 'run')
				self.walking = true
			end
		else
			anim.play(self.anim, 'stand')
			self.walking = false
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

		local f = anim.frame(self.anim)
		sprsheet.draw_anim(spr, f, character_layer, pos)
	end
end

return character
