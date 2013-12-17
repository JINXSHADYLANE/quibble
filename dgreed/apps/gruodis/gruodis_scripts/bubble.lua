local bubble = {}
local bubble_mt = {}
bubble_mt.__index = bubble

local n_points = 16 
local radius = 2

function bubble:new(pos, text_pos, text)
	local o = {
		pos = pos,
		sprite = sprsheet.get_handle('bubble'),
		layer = 2,

		text = text,
		text_pos = text_pos,
		text_size = vfont.size(text),
		text_layer = 4,
		text_visibility = 0
	}

	if o.text_pos.x + o.text_size.x > scr_size.x - 10 then
		o.text_pos.x = scr_size.x - o.text_size.x - 10
	end

	if o.text_pos.y + o.text_size.y > scr_size.y - 20 then
		o.text_pos.y = scr_size.y - o.text_size.y - 20
	end

	local rf = rand.float
	local pi = math.pi
	
	-- sin phases, cos phases, sin scale, cos scale
	o.ps, o.pc, o.ss, o.sc = {}, {}, {}, {}
	for i=1,n_points do
		o.ps[i] = rf(0, pi)
		o.pc[i] = rf(0, pi)
		o.ss[i] = rf(1, 4)
		o.sc[i] = rf(1, 4)
	end

	setmetatable(o, bubble_mt)
	return o
end

function bubble:update(sector)
	self.text_visibility = math.max(0, self.text_visibility - 0.01)
end

function bubble:collide(sector, other)
	if other.sprite == 'player_ship' then
		self.text_visibility = math.min(1, self.text_visibility + 0.04)
	end
end

function bubble:render(sector)
	local t = time.s()
	local sin = math.sin
	local cos = math.cos
	local draw = sprsheet.draw_centered
	local p = tilemap.world2screen(sector, scr_rect, snap_pos(self.pos))
	local d = vec2()
	for i=1,n_points do
		d.x = p.x + sin(t*self.ss[i]+self.ps[i]) * radius
		d.y = p.y + cos(t*self.sc[i]+self.pc[i]) * radius
		draw(self.sprite, self.layer, d)
	end

	local tv = self.text_visibility
	if tv > 0 then
		p = self.text_pos

		-- text
		local col = rgba(0.9, 0.9, 0.9, tv*tv)
		vfont.draw(self.text, self.text_layer, p, col)

		-- background
		local back_dest = rect(
			p.x - 2, p.y - 1,
			p.x + self.text_size.x + 2, p.y + self.text_size.y + 1
		)
		local back_col = rgba(0.1, 0.1, 0.1, tv*tv*0.85)
		sprsheet.draw('empty', self.text_layer-1, back_dest, back_col)
	end

	if self.last_tv and self.last_tv > 0 and tv == 0 then
		vfont.invalidate(self.text)
	end

	self.last_tv = tv
end

return bubble

