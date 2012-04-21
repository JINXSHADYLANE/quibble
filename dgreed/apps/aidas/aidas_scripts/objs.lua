local objs = {}

-- global tweaks
local gravity = 1

-- object containers
objs.player = nil
objs.world = nil
objs.boxes = {}
objs.buttons = {}
objs.doors = {}

local function sweep_rect(rect, offset, no_push)
	local off, d

	-- collide against level
	if objs.level then
		off = tilemap.collide_swept(objs.level, rect, offset)
		d = length_sq(off)
	end

	-- collide against boxes
	local box = nil
	if d > 0 then
		for i,b in ipairs(objs.boxes) do
			if not b.skip then
				local box_off = rect_rect_sweep(b.rect, rect, offset)
				local box_d = length_sq(box_off)
				if d - box_d > 0.001 then
					box = b
					d = box_d
					off = box_off
				end
			end
		end
	end

	if not no_push and box and offset.x ~= 0 then
		box:move(offset.x < 0)
	end

	if box then
		return off, box
	else
		return off
	end
end

-- box

local box = {}
box.__index = box

function box:new(obj)
	local o = {
		width = 32,
		height = 32,

		falling = true,
		pos = obj.pos + vec2(16, 16),
		rect = rect(
			obj.pos.x, obj.pos.y, 
			obj.pos.x + 32, obj.pos.y + 32
		),
		top = nil
	}
	setmetatable(o, self)
	return o
end

function box:move(dir, test_only)
	local offset = nil
	if dir then
		offset = vec2(-32, 0)
	else
		offset = vec2(32, 0)
	end

	self.skip = true
	local move_off = sweep_rect(self.rect, offset, true)
	local can_move = length_sq(move_off) >= 31*31
	self.skip = nil

	if can_move and self.top then
		can_move = self.top:move(dir, true)
	end

	if not test_only and can_move then
		local b = self
		while b ~= nil do	
			b.pos = b.pos + offset
			b.rect = rect(
				b.pos.x - b.width/2, b.pos.y - b.height/2, 
				b.pos.x + b.width/2, b.pos.y + b.height/2
			)

			b.move_anim = true
			b.anim_off = vec2(0, 0)
			b.move_delta = offset/16

			b = b.top
		end
	end

	return can_move
end

function box:update()
	if self.move_anim then
		self.anim_off = self.anim_off + self.move_delta
		if math.abs(self.anim_off.x) < 32 then
			return
		else
			self.move_anim = nil
			self.anim_off = nil
		end
	end

	if self.fall_anim then
		self.anim_off = self.anim_off + vec2(0, gravity*4)
		if self.anim_off.y < 32 then
			return
		else
			self.anim_off.y = self.anim_off.y - 32
		end
	end

	self.skip = true
	local fall_off, b = sweep_rect(self.rect, vec2(0, 32), true)
	local can_fall = length_sq(fall_off) >= 31*31
	self.skip = nil

	if self.falling and not can_fall then
		-- fell on something
		self.falling = false
		self.fall_anim = false
		self.anim_off = nil
		if b then
			b.top = self
		end
	end

	if can_fall then
		self.falling = true
		self.fall_anim = true
		if not self.anim_off then
			self.anim_off = vec2(0, 0)
		end
		self.pos.y = self.pos.y + 32
		self.rect.t = self.rect.t + 32
		self.rect.b = self.rect.b + 32
	end
end

function box:render()
	local p = self.pos
	if self.anim_off and self.fall_anim then
		p = p - vec2(0, 32) + self.anim_off
	end

	if self.anim_off and self.move_anim then
		if self.move_delta.x > 0 then
			p = p - vec2(32, 0)
		else
			p = p + vec2(32, 0)
		end
		p = p + self.anim_off
	end

	local pos = tilemap.world2screen(objs.level, screen_rect, p)
	sprsheet.draw_centered('box', 2, pos)
end

-- player

local player = {}
player.__index = player

function player:new(obj)
	local o = {
		width = 16,
		height = 30,
		move_acc = 0.5,
		move_damp = 0.8,
		jump_acc = 12,

		pos = obj.pos + vec2(16, 16),
		vel = vec2(0, 0),
		dir = false,
		ground = true
	}
	setmetatable(o, self)
	return o
end

function player:update()
	if key.pressed(key._left) then
		self.vel.x = self.vel.x - self.move_acc
		dir = true
	end
	if key.pressed(key._right) then
		self.vel.x = self.vel.x + self.move_acc
		dir = false
	end
	if (key.down(key._up) or key.down(key.a)) and self.ground then
		self.ground = false
		self.vel.y = -self.jump_acc
	end
	self.vel.y = self.vel.y + gravity
	self.vel.x = self.vel.x * self.move_damp

	local bbox = rect(
		self.pos.x - self.width / 2,
		self.pos.y - self.height / 2
	)
	bbox.r = bbox.l + self.width
	bbox.b = bbox.t + self.height

	local dx = sweep_rect(bbox, vec2(self.vel.x, 0))
	self.pos = self.pos + dx
	bbox.l = bbox.l + dx.x
	bbox.r = bbox.r + dx.x
	self.vel.x = dx.x
	
	local dy = sweep_rect(bbox, vec2(0, self.vel.y))
	if self.vel.y > 0 then
		self.ground = dy.y == 0
	end
	self.pos = self.pos + dy
	self.vel.y = dy.y
	bbox.t = bbox.t + dy.y
	bbox.b = bbox.b + dy.y
	self.bbox = bbox

	-- move camera
	local cam_pos, z, rot = tilemap.camera(objs.level)
	cam_pos.x = lerp(cam_pos.x, self.pos.x, 0.1)
	cam_pos.y = lerp(cam_pos.y, self.pos.y, 0.02)
	tilemap.set_camera(objs.level, cam_pos)
end

function player:render()
	if self.bbox then
		local pos = tilemap.world2screen(objs.level, screen_rect, self.bbox)
		sprsheet.draw('player', 2, pos)
	end
end

local object_type = {
	[0] = 'start',
	[1] = 'world',
	[2] = 'box',
	[3] = 'button_a',
	[4] = 'door_a',
	[5] = 'button_b',
	[6] = 'door_b',
	[7] = 'button_c',
	[8] = 'door_c'
}

function objs.reset(level)
	objs.level = level
	local objects = tilemap.objects(level)
	for i,obj in ipairs(objects) do
		local t = object_type[obj.id]
		if t == 'start' then
			objs.player = player:new(obj)
		end
		if t == 'box' then
			table.insert(objs.boxes, box:new(obj))
		end
	end
end

function objs.update()
	objs.player:update()

	for i,b in ipairs(objs.boxes) do
		b:update()
	end
end

function objs.render()
	objs.player:render()

	for i,b in ipairs(objs.boxes) do
		b:render()
	end
end

return objs
