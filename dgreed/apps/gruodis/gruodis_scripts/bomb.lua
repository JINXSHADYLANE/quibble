local bomb = {}
local bomb_mt = {}
bomb_mt.__index = bomb

function bomb:new(pos)
	local o = {
		pos = snap_pos(pos),
		sprite = 'bomb',
		layer = 1,
		place_time = time.s(),
		fuse_length = 3
	}
	setmetatable(o, bomb_mt)
	return o
end

function bomb:update(sector)
	if self.exploding then
		self.dead = true
	end

	if time.s() > self.place_time + self.fuse_length then
		-- explode
		self.exploding = true
		self.size = 30
		self.bbox = calc_bbox(self)
		self.collide = self.explode_collide
		local pos = tilemap.world2screen(sector, scr_rect, self.pos)
		mfx.trigger('bomb', pos)
	end
end

function bomb:render(sector)
	local t = time.s()
	local tt = (t - self.place_time) / self.fuse_length
	local blink_rate = 0
	if tt > 0.5 then
		blink_rate = 3
	end
	if tt > 0.75 then
		blink_rate = 12
	end
	if tt > 0.9 then
		blink_rate = 24
	end

	local frame = (math.floor(t * blink_rate) % 2) == 1
	local col = rgba(0.1, 0.1, 0.1)
	if frame then
		col = rgba(0.5, 0.1, 0.1)
	end

	local size = 1 + math.max(0, tt - 0.9) * 4

	local dest = tilemap.world2screen(sector, scr_rect, self.pos)		
	sprsheet.draw_centered(self.sprite, self.layer, dest, 0, size, col)
end

function bomb:explode_collide(sector, other)
	if not other.dead then
		other.dead = true
		local screen_pos = tilemap.world2screen(sector, scr_rect, other.pos)
		mfx.trigger('explode', screen_pos)
	end
end

return bomb

