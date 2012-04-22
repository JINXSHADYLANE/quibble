local objs = {}

-- global tweaks
local gravity = 1 

-- object containers
objs.player = nil
objs.world = nil
objs.goal = nil
objs.boxes = {}
objs.buttons = {}
objs.doors = {}

local function sweep_rect(r, offset, no_push)
	local off, d

	-- collide against level
	if objs.level then
		off = tilemap.collide_swept(objs.level, r, offset)
		d = length_sq(off)
	end

	-- collide against doors
	if d > 0 then
		for i,dr in ipairs(objs.doors) do
			if not dr.open then
				local door_off = rect_rect_sweep(dr.rect, r, offset)
				local door_d = length_sq(door_off)
				if d - door_d > 0.001 then
					d = door_d
					off = door_off
				end
			end
		end
	end

	-- collide against boxes
	local box = nil
	if d > 0 then
		for i,b in ipairs(objs.boxes) do
			if not b.skip then
				local box_off = rect_rect_sweep(b.rect, r, offset)
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
		objs.player.push = 7
		box:move(offset.x < 0)
 	end

	if box then
		return off, box
	else
		return off
	end
end

-- door

local door = {}
door.__index = door

function door:new(obj, t)
	local o = {
		width = 32,
		height = 32,

		pos = obj.pos + vec2(16, 16),
		rect = rect(
			obj.pos.x, obj.pos.y,
			obj.pos.x + 32, obj.pos.y + 32
		),

		type = t,
		open = false
	}
	setmetatable(o, self)
	return o
end

function door:render()
	local pos = tilemap.world2screen(objs.level, screen_rect, self.pos)
	local col = rgba(1, 1, 1, 1)
	if self.open then
		col.a = 0.2
	end
	sprsheet.draw_anim_centered('doors', self.type, 2, pos, col)
end

-- switch

local switch = {}
switch.__index = switch

function switch:new(obj, t)
	local o = {
		width = 32,
		height = 32,

		pos = obj.pos + vec2(16, 16),
		rect = rect(
			obj.pos.x, obj.pos.y,
			obj.pos.x + 32, obj.pos.y + 32
		),

		type = t,
		pressed = false
	}
	setmetatable(o, self)
	return o
end

function switch:update()
	local pressed = false
	local r = rect(self.rect)
	r.l = r.l + 2
	r.r = r.r - 2
	r.t = r.t + 28
	if rect_rect_collision(r, objs.player.bbox) then
		pressed = true
	end
	for i,b in ipairs(objs.boxes) do
		if rect_rect_collision(r, b.rect) then
			pressed = true
			break
		end
	end

	if pressed ~= self.pressed then
		-- change state
		self.pressed = pressed
		for i,d in ipairs(objs.doors) do
			if d.type == self.type then
				d.open = pressed
			end
		end

		-- make boxes fall
		for i,b in ipairs(objs.boxes) do
			b.falling = true
		end
	end
end

function switch:render()
	local p = vec2(self.pos)
	if self.pressed then
		p.y = p.y + 2
	end
	local pos = tilemap.world2screen(objs.level, screen_rect, p)
	sprsheet.draw_anim_centered('buttons', self.type, 0, pos)
end

-- box

local box = {}
box.__index = box

function box:new(obj)
	local o = {}
	if obj then
		o = {
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
	end
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
			b.falling = true

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

-- world

local world = box:new() 
world.__index = world 

function world:new(obj)
	local o = box:new(obj)
	o.angle = 0
	o.old_update = o.update
	setmetatable(o, self)
	return o
end

local function px_to_angle(px)
	return px * 0.05
end

function world:update()
	-- if player is standing on top...
	if not self.move_anim and not self.falling then
		local wb = self.rect
		local pb = objs.player.bbox
		if  math.abs(wb.t - pb.b) < 0.1 then
			if wb.l <= pb.r and wb.r >= pb.l then
				local d = self.pos.x - objs.player.pos.x
				local ad = math.abs(d)
				if ad > 10 then
					if d > 0 then
						if self:move(true) then
							objs.player.vel.x = objs.player.vel.x - 4 
							--objs.player.vel.y = objs.player.vel.y - 4 
						end
					else
						if self:move(false) then
							objs.player.vel.x = objs.player.vel.x + 4 
							--objs.player.vel.y = objs.player.vel.y - 4 
						end
					end
				end
			end
		end
	end
	if self.move_anim then
		local new_off = self.anim_off.x + self.move_delta.x
		if math.abs(new_off) < 32 then
		else
			if self.move_delta.x > 0 then
				self.angle = self.angle + px_to_angle(32)
			else
				self.angle = self.angle - px_to_angle(32)
			end
		end
	end
	self:old_update()
end

function world:render()
	local p = self.pos
	local a = self.angle
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
		a = a + px_to_angle(self.anim_off.x)
	end

	local pos = tilemap.world2screen(objs.level, screen_rect, p)
	sprsheet.draw_centered('world', 2, pos, a, 1)
end



-- player

local player = {}
player.__index = player

function player:new(obj)
	local o = {
		width = 16,
		height = 26,
		move_acc = 0.5,
		move_damp = 0.8,
		jump_acc = 12,

		pos = obj.pos + vec2(16, 16),
		vel = vec2(0, 0),
		dir = false,
		ground = true,
		frame = 0
	}
	setmetatable(o, self)
	return o
end

function player:update()
	if key.pressed(key._left) then
		self.vel.x = self.vel.x - self.move_acc
		self.dir = true
	end
	if key.pressed(key._right) then
		self.vel.x = self.vel.x + self.move_acc
		self.dir = false
	end
	if (key.down(key._up) or key.down(key.a)) and self.ground then
		self.ground = false
		self.vel.y = -self.jump_acc
		self.frame = 4
	end
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
	if self.vel.y > 0 then
		self.ground = dy.y == 0
		if self.ground then
			self.frame = 0
		end
	end
	self.pos = self.pos + dy
	self.vel.y = dy.y
	bbox.t = bbox.t + dy.y
	bbox.b = bbox.b + dy.y
	self.bbox = bbox

	if self.ground and self.frame ~= 3 then
		if math.abs(self.vel.x) > 0.01 then
			local t = time.s()
			self.frame = math.fmod(math.floor(t * 10), 3)
		else
			self.frame = 0
		end
	end

	if self.push then
		self.push = self.push - 1
		if self.push == 0 then
			self.push = nil
		else
			self.frame = 3
		end
	end

	-- move camera
	local cam_pos, z, rot = tilemap.camera(objs.level)
	cam_pos.x = lerp(cam_pos.x, self.pos.x, 0.1)
	cam_pos.y = lerp(cam_pos.y, self.pos.y, 0.02)
	tilemap.set_camera(objs.level, cam_pos)
end

function player:render()
	if self.bbox then
		local pos = tilemap.world2screen(objs.level, screen_rect, self.bbox)
		if self.dir then
			pos.l, pos.r = pos.r, pos.l
		end

		sprsheet.draw_anim('guy', self.frame, 2, pos)
	end
end

-- goal

local goal = {}
goal.__index = goal

function goal:new(obj)
	local o = {
		width = 32,
		height = 32,

		pos = obj.pos + vec2(16, 16),
		rect = rect(
			obj.pos.x, obj.pos.y,
			obj.pos.x + 32, obj.pos.y + 32
		)
	}
	setmetatable(o, self)
	return o
end

function goal:update()
	-- returns true if goal is reached
	if not objs.world.move_anim and not objs.world.fall_anim then
		if rect_rect_collision(self.rect, objs.world.rect) then
			return length_sq(self.pos - objs.world.pos) < 1
		end
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
	[8] = 'door_c',
	[9] = 'end'
}

function objs.reset(level)
	objs.player = nil
	objs.world = nil
	objs.goal = nil
	objs.boxes = {}
	objs.buttons = {}
	objs.doors = {}

	objs.level = level
	local objects = tilemap.objects(level)
	for i,obj in ipairs(objects) do
		local t = object_type[obj.id]
		if t == 'start' then
			objs.player = player:new(obj)
		end
		if t == 'end' then
			objs.goal = goal:new(obj)
		end
		if t == 'box' then
			table.insert(objs.boxes, box:new(obj))
		end
		if t == 'world' then
			local w = world:new(obj)
			objs.world = w
			table.insert(objs.boxes, w)
		end
		if t == 'button_a' then
			table.insert(objs.buttons, switch:new(obj, 0))
		end
		if t == 'button_b' then
			table.insert(objs.buttons, switch:new(obj, 1))
		end
		if t == 'button_c' then
			table.insert(objs.buttons, switch:new(obj, 2))
		end
		if t == 'door_a' then
			table.insert(objs.doors, door:new(obj, 0))
		end
		if t == 'door_b' then
			table.insert(objs.doors, door:new(obj, 1))
		end
		if t == 'door_c' then
			table.insert(objs.doors, door:new(obj, 2))
		end
	end
end

function objs.update()
	objs.player:update()

	for i,b in ipairs(objs.boxes) do
		b:update()
	end

	for i,b in ipairs(objs.buttons) do
		b:update()
	end

	return objs.goal:update()
end

function objs.render()
	objs.player:render()

	for i,b in ipairs(objs.boxes) do
		b:render()
	end

	for i,b in ipairs(objs.buttons) do
		b:render()
	end

	for i,d in ipairs(objs.doors) do
		d:render()
	end
end

return objs
