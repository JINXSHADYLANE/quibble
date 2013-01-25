local room = {}
local room_mt = {}
room_mt.__index = room

local object = require('object')

room.descs = {}

room.descs[1] = {
	'######################',
	'#                    #',
	'#  #  #  #  #  #  #  #',
	'                 @   #',
	'#  #  #  #  #  #  #  #',
	'#                    #',
	'######################' 
}

local function parse_tiles(room, desc)
	room.width, room.height = desc[1]:len(), #desc
	local dx = math.floor((40 - room.width)/2)
	local dy = math.floor((20 - room.height)/2)

	for y, line in ipairs(desc) do
		local x = 0
		for c in line:gmatch('.') do
			local idx = (y-1) * room.width + x
			if c == ' ' or c == '#' then
				room.tiles[idx] = c	
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
		tiles = {}
	}
	setmetatable(o, room_mt)
	parse_tiles(o, desc)
	return o
end

function room:update()
	for i,obj in ipairs(self.objs) do
		if obj.update then
			obj:update()
		end
	end
end

function room:render(textmode)
	textmode:clear()
	local dx = math.floor((textmode.size.x - self.width)/2)
	local dy = math.floor((textmode.size.y - self.height)/2)

	-- render tiles
	for i,c in pairs(self.tiles) do
		local x = i % self.width
		local y = (i - x) / self.width
		textmode:put(x+dx, y+dy, c)
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
end

return room
