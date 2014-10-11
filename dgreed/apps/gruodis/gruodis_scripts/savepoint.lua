local savepoint = {}
local savepoint_mt = {}
savepoint_mt.__index = savepoint

local color = rgba(1, 1, 1, 0.75)

function savepoint:new(pos, sector)
	local o = {
		size = 8,
		pos = vec2(pos),
		sprite = 'empty',
		layer = 1,
		start_t = nil,
		sector = vec2(sector)
	}
	o.bbox = calc_bbox(o)
	setmetatable(o, savepoint_mt)
	return o
end

function savepoint:render(sector)
	local dest = tilemap.world2screen(sector, scr_rect, self.bbox)
	sprsheet.draw(self.sprite, self.layer, dest, color)

	local t = 0
	if self.start_t then
		t = (states.time() - self.start_t) * 2
	end
	if t > 0 and t < 1 then
		local e = t * t * 5
		local dest2 = rect(dest.l - e, dest.t - e, dest.r + e, dest.b + e)
		local col2 = rgba(1, 1, 1, 0.5 * math.sin(t * math.pi))
		sprsheet.draw(self.sprite, self.layer, dest2, col2)
	end
end

function savepoint:collide(sector, other)
	if other.is_player and not veql(self.sector, other.save_sector) then
		self.start_t = states.time()
		other.save_pos = self.pos
		other.save_sector = self.sector
	end
end

return savepoint
