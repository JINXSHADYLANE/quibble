local spike = {}
local spike_mt = {}
spike_mt.__index = spike

local bbox_shrink = {
	[0] = vec2(0,-5),
	[1] = vec2(5,0),
	[2] = vec2(0,5),
	[3] = vec2(-5,0)
}

function spike:new(pos, rot)
	local o = {
		size = 10,
		pos = vec2(pos),
		rot = rot * math.pi * 0.5,
		sprite = 'spike',
		layer = 1,
		invincible = true
	}

	o.bbox = calc_bbox(o)
	local shrink = bbox_shrink[rot]
	if shrink.x > 0 then
		o.bbox.l = o.bbox.l + shrink.x
	elseif shrink.x < 0 then
		o.bbox.r = o.bbox.r + shrink.x
	end
	if shrink.y > 0 then
		o.bbox.t = o.bbox.t + shrink.y
	elseif shrink.y < 0 then
		o.bbox.b = o.bbox.b + shrink.y
	end

	setmetatable(o, spike_mt)
	return o
end

function spike:render(sector)
	local pos = tilemap.world2screen(sector, scr_rect, snap_pos(self.pos))
	sprsheet.draw_centered(self.sprite, self.layer, pos, self.rot, 1)

	-- draw debug bbox
	--local box = tilemap.world2screen(sector, scr_rect, self.bbox)
	--sprsheet.draw('empty', self.layer+1, box, rgba(1,1,1,0.5))
end

function spike:collide(sector, other)
	if not other.dead and not other.invincible then
		other.dead = true
	end
end

return spike
