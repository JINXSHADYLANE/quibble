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

local last_keypress = 0
function player:update(room)
	if time.s() - last_keypress < 0.20 then
		return
	end

	local old_pos = vec2(self.pos)
	local moved = false
	if char.pressed('h') or key.pressed(key._left) then
		self.pos.x = self.pos.x - 1
		moved = true
	end
	if char.pressed('j') or key.pressed(key._down) then
		self.pos.y = self.pos.y + 1
		moved = true
	end
	if char.pressed('k') or key.pressed(key._up) then
		self.pos.y = self.pos.y - 1
		moved = true
	end
	if char.pressed('l') or key.pressed(key._right) then
		self.pos.x = self.pos.x + 1
		moved = true
	end

	if moved then
		last_keypress = time.s()
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
	room.text_start_t = time.s()
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

