local room = {}
local room_mt = {}
room_mt.__index = room

local object = require('object')
local timeline = require('timeline')
local spells = require('spells')

local function parse_tiles(room, desc)
	room.width, room.height = desc[1]:len(), #desc
	local dx = math.floor((40 - room.width)/2)
	local dy = math.floor((17 - room.height)/2)
	room.dx, room.dy = dx, dy

	local stone_col1 = rgba(0.7, 0.7, 0.7)
	local stone_col2 = rgba(0.3, 0.3, 0.35)

	for y, line in ipairs(desc) do
		local x = 0
		for c in line:gmatch('.') do
			local idx = (y-1) * room.width + x
			if c == ' ' then
				--
			elseif c == '#' then
				room.tiles[idx] = c
				room.colour[idx] = lerp(
					stone_col1, stone_col2, rand.float(0, 1)
				)
			else
				local new_obj = object.make_obj(vec2(dx+x, dy+y-1), c)
				table.insert(room.objs, new_obj)
			end
			x = x + 1
		end
	end
end

function room:new(desc)
	local o = {
		objs = {},
		tiles = {},
		colour = {},
		text = nil,
		spell = nil,
		spell_t = 0
	}
	setmetatable(o, room_mt)
	parse_tiles(o, desc)

	-- place golems
	for i=1,40 do
		local x, y
		repeat
			x, y = rand.int(0, 40), rand.int(0, 17)
		until not o:collide(x, y, true, true, true)	

		table.insert(o.objs, 
			object.make_obj(vec2(x, y), 'G')
		)
	end

	return o
end

function room:learn_spell()
	for i,spell in ipairs(spells) do
		if not spell.have then
			spell.have = true
			timeline.text = string.format('You learned %s spell!', spell.name)
			timeline.text_color = rgba(0, 1, 0)
			current_leve = current_level + 1
			return
		end
	end

	timeline.text = 'You already know this spell.'
	timeline.text_color = rgba(0, 0, 1)
end

function room:ray(start_x, start_y, end_x, end_y, nopath, skipobj)
	local tiles = nil
	if not nopath then
		tiles = {}
	end

	-- simply do bresenheim's
	local dx = end_x - start_x
	local dy = end_y - start_y
	local e = 0
	local de = math.abs(dy / dx)
	local y = start_y

	local x_dir = 1
	if dx < 0 then
		x_dir = -1
	end

	local y_dir = 1
	if dy < 0 then
		y_dir = -1
	end

	local obj, res
	local last_x
	for x=start_x,end_x,x_dir do
		last_x = x
		if not nopath then
			table.insert(tiles, {x, y})
		end
		res, obj = self:collide(x, y, true, not skipobj, false)
		if res then
			break
		end
		e = e + de

		while e > 0.5 do
			e = e - 1
			y = y + y_dir

			if e > 0.5 then
				if not nopath then
					table.insert(tiles, {x, y})
				end
				res, obj = self:collide(x, y, true, not skipobj, false)
				if res then
					break
				end
			end

			if y == end_y and x == end_x then
				if e <= 0.5 and not nopath then
					table.insert(tiles, {x, y})
				end
				break
			end
		end
	end

	local reached = not (last_x ~= end_x or y ~= end_y)
	
	if nopath then
		return not reached, obj
	else
		return tiles, obj, reached
	end
end

function room:update()
	if self.spell or self.reset then
		return
	end

	local to_remove = 0

	for i,obj in ipairs(self.objs) do
		if obj.update then
			obj:update(self)
		end

		if obj.remove then
			to_remove = to_remove + 1
		end
	end

	if to_remove > 0 then
		local new_objs = {}
		for i,obj in ipairs(self.objs) do
			if not obj.remove then
				table.insert(new_objs, obj)
			end
		end
		self.objs = new_objs
	end
end

function room:collide(x, y, with_tiles, with_objs, with_player, o)
	-- objs
	if with_objs or with_player then
		for i,obj in ipairs(self.objs) do
			if obj ~= o then
				local is_player = obj.char == '@'
				if obj.pos.x == x and obj.pos.y == y then
					if is_player and with_player then
						return true, obj
					end
					if not is_player and with_objs then
						return true, obj
					end
				end
			end
		end
	end

	-- tiles
	local idx = (y - self.dy) * self.width + (x - self.dx)
	if self.tiles[idx] == '#' then
		return true
	end
	return false
end

function room:player_collide(player)
	-- collide with objs
	for i,obj in ipairs(self.objs) do
		if obj.player_collide and 
			obj.pos.x == player.pos.x and
			obj.pos.y == player.pos.y then
			return obj:player_collide(self, player)
		end
	end

	-- collide with tiles
	local idx = (player.pos.y - self.dy) * self.width + (player.pos.x - self.dx)
	if self.tiles[idx] == '#' then
		return false
	end
	return true
end

function room:player_moved(player, time)
	if time then
		timeline.pass(1)
	end

	for i,obj in ipairs(self.objs) do
		if obj.tick then
			obj:tick(self, player)
		end
	end
end

function room:render_circle(textmode, x, y, r, col)
	local transp = rgba(0, 0, 0, 1)

	local rr = math.ceil(r)

	local w, h = textmode.size.x, textmode.size.y

	textmode:push()
	for ix=x-rr,x+rr do
		for iy=y-rr,y+rr do
			if ix >= 0 and ix < w and iy >= 0 and iy < h then
				local dx = x-ix
				-- acount for non-square tiles
				local dy = (y-iy) -- * (24 / 28)
				local d_sqr = math.abs(dx*dx + dy*dy - r*r) / 6
				if d_sqr < 1 then
					local c = lerp(col, transp, math.sqrt(d_sqr))
					textmode.selected_bg = c
					textmode:recolour(ix, iy, 1)
				end
			end
		end
	end
	textmode:pop()
end

function room:render(textmode)
	textmode:clear()

	local dx, dy = self.dx, self.dy

	-- render tiles
	for i,c in pairs(self.tiles) do
		local x = i % self.width
		local y = (i - x) / self.width
		if self.colour[i] ~= nil then
			textmode:push()
			textmode.selected_fg = self.colour[i]
		end
		textmode:put(x+dx, y+dy, c)
		if self.colour[i] ~= nil then
			textmode:pop()
		end
		x = x + 1
		if x == self.width then
			x = 0
			y = y + 1
		end
	end

	-- render objects
	local player = nil
	for i,o in ipairs(self.objs) do
		o:draw(textmode)
		if o.char == '@' then
			player = o
		end
	end

	-- render spell
	local reset_room = false
	local st = time.s() - self.spell_t
	if self.spell then
		if st < self.spell.effect_len or self.spell.effect_len == -1 then
			local t = st / self.spell.effect_len
			self.spell = self.spell.effect(player, self, textmode, t)
		else
			timeline.pass(self.spell.cost)
			if self.spell.post then
				-- planeshift spell tells us to reset room here
				if self.spell.post(player, self) then
					reset_room = true
				end
			end
			self.spell = nil
		end
	end

	-- render text
	if self.text then
		local t = time.s() - self.text_start_t
		local syms = math.ceil(t / 0.05)
		local text = self.text:sub(1, syms)
		-- greedily split text into lines
		local lines = {}
		for word in text:gmatch('%S+') do
			local line = lines[#lines]
			if not line or line:len() + 1 + word:len() >= 40 then
				table.insert(lines, '')
				line = lines[#lines]
			end
			lines[#lines] = line .. ' ' .. word
		end

		-- render
		local i = 1
		for y=textmode.size.y-#lines,textmode.size.y-1 do
			textmode:write(0, y, lines[i])
			i = i + 1
		end
	end

	local t = time.s()
	if self.reset then
		if not self.reset_t then
			self.reset_t = time.s()
			timeline.text = 'You descend deeper into the cave.'
		end

		if t - self.reset_t < 1 then
			local tt = t - self.reset_t
			textmode:draw_overlay(5, tt)
		else
			return true
		end
	end

	if self.fadein_t then
		if t - self.fadein_t < 1 then
			local tt = t - self.fadein_t
			textmode:draw_overlay(5, 1 - tt)
		end
	end

	return reset_room
end

return room
