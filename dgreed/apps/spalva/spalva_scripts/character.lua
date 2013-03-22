local character = {}
local character_mt = {}
character_mt.__index = character

local gravity = 0.3

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

		if math.abs(self.vel.x) > 0.05 then
			anim.play(self.anim, 'jump')
		else
			anim.play(self.anim, 'stand_jump')
		end
	end
end

function character:new(obj)
	local o = {
		completed = false,
		width = 22,
		height = 30,
		move_acc = 0.5,
		move_damp = 0.8,
		jump_acc = 6.0,

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

function character:update(level, world_bottom)
	if not self.completed then
		self:controls()
	end

	self.vel.y = self.vel.y + gravity
	self.vel.x = self.vel.x * self.move_damp

	local bbox = rect(
		self.pos.x - self.width / 2,
		self.pos.y - self.height / 2
	)
	bbox.r = bbox.l + self.width
	bbox.b = bbox.t + self.height

	if self.completed then
		self.bbox = bbox
		return
	end

	local dx = tilemap.collide_swept(level, bbox, vec2(self.vel.x, 0))
	self.pos = self.pos + dx
	bbox.l = bbox.l + dx.x
	bbox.r = bbox.r + dx.x
	self.vel.x = dx.x
	
	local dy = tilemap.collide_swept(level, bbox, vec2(0, self.vel.y))
	local was_on_ground = self.ground
	if self.vel.y > 0 then
		self.ground = dy.y == 0
	end

	if (not was_on_ground) and self.ground then
		anim.play(self.anim, 'land')
		self.walking = false
	end

	self.pos = self.pos + dy
	self.vel.y = dy.y
	bbox.t = bbox.t + dy.y
	bbox.b = bbox.b + dy.y
	self.bbox = bbox

	if self.ground and bbox.b + 2 < world_bottom then
		if math.abs(self.vel.x) > 0.05 then
			if not self.walking then
				anim.play(self.anim, 'run')
				self.walking = true
			end
		else
			anim.play(self.anim, 'stand')
			self.walking = false
		end

		-- return color of tile we're standing on
		local p = vec2((bbox.l + bbox.r) / 2, bbox.b + 2)
		p.x = math.floor(p.x / 32)
		p.y = math.floor(p.y / 32)
		local tileset, tile = tilemap.tile(level, p.x, p.y, 0)
		if tile > 0 and tile < 10 then
			return tile
		end
	end
end

function character:render(level, world_bottom)
	local b = self.bbox
	if b then
		local dest = rect(
			b.l - 5, b.t - 2, b.r + 5, b.b	
		)
		local pos = tilemap.world2screen(level, scr_rect, dest)
		if self.dir then
			pos.l, pos.r = pos.r, pos.l
		end

		local spr = self.sprite

		local f = anim.frame(self.anim)
		if f == 10 then
			-- show curse in 1 seconds
			self.curse = time.s() + 1
		end
		sprsheet.draw_anim(spr, f, character_layer, pos)

		if self.curse and time.s() > self.curse and time.s() < self.curse + 2 then
			local p = vec2(pos.l, pos.t) + vec2(24, -16)
			sprsheet.draw('curse', character_layer, p)
		end

		if self.curse and time.s() > self.curse + 3 then
			self.completed = false
		end
	end
end

return character
