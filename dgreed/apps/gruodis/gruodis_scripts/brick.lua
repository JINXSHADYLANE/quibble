local brick = {}
local brick_mt = {}
brick_mt.__index = brick

function brick:new(pos, tile_x, tile_y)
	local o = {
		size = 10,
		pos = pos,
		invincible = true,
		x = tile_x,
		y = tile_y
	}
	self.bbox = calc_bbox(o)
	setmetatable(o, brick_mt)
	return o
end

function brick:explode(sector)
	tilemap.set_tile(sector, self.x, self.y, 0, 0, 0)
	tilemap.set_collision(sector, self.x, self.y, false)
	local pos = tilemap.world2screen(sector, scr_rect, self.pos)
	mfx.trigger('wall_explode', pos)
end

return brick
