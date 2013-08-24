local room = {}
local room_mt = {}
room_mt.__index = room

local object = require('object')

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
		text = nil
	}
	setmetatable(o, room_mt)
	parse_tiles(o, desc)
	return o
end

function room:update()
	for i,obj in ipairs(self.objs) do
		if obj.update then
			obj:update(self)
		end
	end
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
	for i,o in ipairs(self.objs) do
		o:draw(textmode)
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
end

return room
