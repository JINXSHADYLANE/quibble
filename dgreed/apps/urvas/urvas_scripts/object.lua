local object = {}
local object_mt = {}
object_mt.__index = object

function object:new(pos, overrides)
	local o = {
		update = overrides.update,
		player_collide = overrides.player_collide,
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

function player:update(room)
	local old_pos = vec2(self.pos)
	local moved = false
	if char.down('h') or key.down(key._left) then
		self.pos.x = self.pos.x - 1
		moved = true
	end
	if char.down('j') or key.down(key._down) then
		self.pos.y = self.pos.y + 1
		moved = true
	end
	if char.down('k') or key.down(key._up) then
		self.pos.y = self.pos.y - 1
		moved = true
	end
	if char.down('l') or key.down(key._right) then
		self.pos.x = self.pos.x + 1
		moved = true
	end

	if moved and not room:player_collide(self) then
		self.pos = old_pos
	end
end

--- exit ---

local exit = {}

exit.char = 'E'

function exit:player_collide(room, player)
	room.text = 'Rytas entered into the dark cave.'
	return false 
end

---

local obj_types = {
	['@'] = player,
	['E'] = exit
}

function object.make_obj(pos, char)
	local t = obj_types[char]
	assert(t)
	return object:new(pos, t)
end

return object

