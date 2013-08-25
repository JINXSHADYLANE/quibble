local object = {}
local object_mt = {}
object_mt.__index = object

local timeline = require('timeline')

local function random_move()
	local dir = rand.int(0, 4)
	if dir == 0 then
		return vec2(0, -1)
	elseif dir == 1 then
		return vec2(1, 0)
	elseif dir == 2 then
		return vec2(0, 1)
	else
		return vec2(-1, 0)
	end
end

local function move_towards(obj, dest)
	local d = normalize(dest.pos - obj.pos)
	d.x = round(d.x)
	d.y = round(d.y)
	return d
end

function object:new(pos, overrides)
	local o = {
		update = overrides.update,
		player_collide = overrides.player_collide,
		tick = overrides.tick,
		die = overrides.die,
		pos = pos or vec2(0, 0),
		char = overrides.char or '$',
		color = overrides.color or rgba(1, 1, 1),
		enemy = overrides.enemy,
		movable = overrides.movable
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
	self.color = lerp(self.color, rgba(1, 1, 1), 0.1)

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

--- skeleton ---

local skeleton = {}
skeleton.char = 'S'
skeleton.color = rgba(0.6, 0.6, 0.6)
skeleton.enemy = false
skeleton.movable = true

function skeleton:die(room, player)
	self.remove = true
	timeline.text = 'Skeleton disappears.'
	timeline.text_color = rgba(1, 0, 0)
end

function skeleton:player_collide(room, player)
	return false
end

function skeleton:tick(room, player)
	local sx, sy = self.pos.x, self.pos.y

	if rand.int(0, 100) < 75 then
		-- find closest enemy
		local target = nil
		local best = 100*100 
		for i,obj in ipairs(room.objs) do
			if obj.enemy and not obj.remove then
				local d = obj_sqr_distance(self, obj)
				if d < best then
					target = obj
					best = d
				end
			end
		end

		if target then
			local tx, ty = target.pos.x, target.pos.y
			local dd = math.abs(sx-tx) + math.abs(sy-ty)
			assert(dd > 0)
			if dd > 1 then
				-- go towards
				local can_see = not room:ray(
					self.pos.x, self.pos.y, target.pos.x, target.pos.y,
					true, true
				)

				if can_see then
					self.pos = self.pos + move_towards(self, target)
				end
			else
				-- attack
				assert(dd == 1)
				if target.die then
					target:die(room, player)
				end
			end
		end
	else
		if rand.int(0, 100) < 60 then
			-- move randomly
			self.pos = self.pos + random_move()
		else
			-- move towards player
			self.pos = self.pos + move_towards(self, player)
		end
	end

	if room:collide(self.pos.x, self.pos.y, true, true, true, self) then
		self.pos.x = sx
		self.pos.y = sy
	end
end

--- golem ---

local golem = {}

golem.char = 'G'
golem.color = rgba(0.7, 0.5, 0.5)
golem.enemy = true
golem.movable = true

function golem:die(room, player)
	self.remove = true
	timeline.pass(-3)
	timeline.text = 'Golem dies.'
	timeline.text_color = rgba(0, 1, 0)
end

function golem:player_collide(room, player)
	self:die(room, player)
	room:player_moved(player)
	return false
end

function golem:tick(room, player)
	local sx, sy = self.pos.x, self.pos.y
	local px, py = player.pos.x, player.pos.y
	local sees_player = not room:ray(
		sx, sy, px, py, true, true
	)

	if sees_player and rand.int(0, 100) > 20 then
		local dx = px - sx
		local dy = py - sy

		if math.abs(dx) + math.abs(dy) == 1 then
			timeline.pass(1)
			timeline.text = 'Golem hits, you stumble for 1s.'
			timeline.text_color = rgba(1, 0, 0)
			player.color = rgba(1, 0, 0)
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

	else
		-- move randomly
		if rand.int(0, 100) < 30 then
			self.pos = self.pos + random_move()
		end
	end

	if room:collide(self.pos.x, self.pos.y, true, true, true, self) then
		self.pos.x = sx
		self.pos.y = sy
	end
end

---

local obj_types = {
	['@'] = player,
	['>'] = exit,
	['S'] = skeleton,
	-- enemies
	['G'] = golem,
	['O'] = ogre,
	['V'] = vulture
}

function object.make_obj(pos, char)
	local t = obj_types[char]
	assert(t)
	return object:new(pos, t)
end


return object

