local objs = {}

-- global tweaks
local gravity = 1

-- object containers
objs.player = nil
objs.world = nil
objs.boxes = {}
objs.buttons = {}
objs.doors = {}

local function sweep_rect(rect, offset)
	if objs.level then
		return tilemap.collide_swept(objs.level, rect, offset)
	end

	return offset
end

-- player

local player = {}
player.__index = player

function player:new(obj)
	local o = {
		width = 32,
		height = 32,
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
	local pos = tilemap.world2screen(objs.level, screen_rect, self.pos)
	sprsheet.draw_centered('player', 2, pos)
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
	end
end

function objs.update()
	objs.player:update()
end

function objs.render()
	objs.player:render()
end

return objs
