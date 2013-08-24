local object = {}
local object_mt = {}
object_mt.__index = object

local timeline = require('timeline')

function object:new(pos, overrides)
	local o = {
		update = overrides.update,
		player_collide = overrides.player_collide,
		tick = overrides.tick,
		pos = pos or vec2(0, 0),
		char = overrides.char or '$',
		color = overrides.color or rgba(1, 1, 1),
		enemy = overrides.enemy
	}
	setmetatable(o, object_mt)
	return o
end

function object:draw(textmode)
	local p = self.pos
	local old_color = textmode.selected_fg
	textmode.selected_fg = self.color
	textmode:put(p.x, p.y, self.char)
	textmode.selected_fg = old_color
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
	local moved = 0
	if char.pressed('h') or key.pressed(key._left) then
		self.pos.x = self.pos.x - 1
		moved = moved + 1
	end
	if (char.pressed('j') or key.pressed(key._down)) then
		self.pos.y = self.pos.y + 1
		moved = moved + 1
	end
	if char.pressed('k') or key.pressed(key._up) then
		self.pos.y = self.pos.y - 1
		moved = moved + 1
	end
	if char.pressed('l') or key.pressed(key._right) then
		self.pos.x = self.pos.x + 1
		moved = moved + 1
	end

	if char.pressed('w') then
		moved = moved + 1
	end

	if moved == 1 then
		last_keypress = time.s()
	end

	if moved > 1 or (moved == 1 and not room:player_collide(self)) then
		self.pos = old_pos
	else
		if moved == 1 then
			room:player_moved(self)
		end
	end
end

--- exit ---

local exit = {}

exit.char = '>'
exit.color = rgba(0.9, 0.7, 0.05)

function exit:player_collide(room, player)
	return false 
end

--- golem ---

local golem = {}

golem.char = 'G'
golem.color = rgba(0.7, 0.5, 0.5)
golem.enemy = true

function golem:player_collide(room, player)
	return true
end

function golem:tick(room, player)
	local sx, sy = self.pos.x, self.pos.y
	local px, py = player.pos.x, player.pos.y
	local sees_player = not room:ray(
		sx, sy, px, py, true, true
	)

	if sees_player then
		local dx = px - sx
		local dy = py - sy

		if math.abs(dx) + math.abs(dy) == 1 then
			print('damage!')
		end

		local x_first = (dx ~= 0 and rand.int(0, 2) == 0) or dy == 0

		if x_first then
			if dx > 0 then
				self.pos.x = sx + 1
			else
				self.pos.x = sx - 1
			end
		else
			if dy > 0 then
				self.pos.y = sy + 1
			else
				self.pos.y = sy - 1
			end
		end

		if room:collide(self.pos.x, self.pos.y, true, true, true, self) then
			self.pos.x = sx
			self.pos.y = sy
		end
	end
end

---

local obj_types = {
	['@'] = player,
	['>'] = exit,
	['G'] = golem
}

function object.make_obj(pos, char)
	local t = obj_types[char]
	assert(t)
	return object:new(pos, t)
end


return object

