local object = {}
local object_mt = {}
object_mt.__index = object

function object:new(pos, overrides)
	local o = {
		update = overrides.update,
		pos = pos or vec2(0, 0),
		char = overrides.char or '$',
		color = overrides.color or rgba(1, 1, 1)
	}
	setmetatable(o, object_mt)
	return o
end

function object:draw(textmode)
	local p = self.pos
	textmode:put(p.x, p.y, self.char)
end

--- player ---

local player = {}

player.char = '@'

function player:update()
	if char.down('h') or key.down(key._left) then
		self.pos.x = self.pos.x - 1
	end
	if char.down('j') or key.down(key._down) then
		self.pos.y = self.pos.y + 1
	end
	if char.down('k') or key.down(key._up) then
		self.pos.y = self.pos.y - 1
	end
	if char.down('l') or key.down(key._right) then
		self.pos.x = self.pos.x + 1
	end
end

---

local obj_types = {
	['@'] = player
}

function object.make_obj(pos, char)
	local t = obj_types[char]
	assert(t)
	return object:new(pos, t)
end

return object

