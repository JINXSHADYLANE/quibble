module(..., package.seeall)

require 'sim'
require 'effects'

part_tex = nil
back_tex = nil

part_rects = {
	rect(0, 0, 32, 32),
	rect(0, 32, 32, 64),
	rect(32, 0, 85, 32),
	rect(32, 32, 85, 64),
	rect(0, 64, 52, 64 + 52),
	rect(52, 64, 52 + 52, 64 + 52)
}

part_layer = 3

affect_radius = 120 
affect_force = 0.02

inside_cols = {
	{rgba(242,255,0), rgba(255,44,0)},
	{rgba(0,129,255), rgba(255,44,0)},
	{rgba(0,129,255), rgba(242,255,0)},
	{rgba(242,255,0), rgba(147,255,0)},
	{rgba(255,0,236), rgba(242,255,0)},
	{rgba(255,255,255), rgba(242,255,0)},
	{rgba(242,255,0), rgba(0,129,255)},
	{rgba(255,44,0), rgba(242,255,0)},
	{rgba(255,255,255), rgba(242,255,0)}
}

inside_rects = {
	rect(85, 0, 85 + 32, 32),
	rect(85, 32, 85 + 32, 64)
}

function init()
	part_tex = tex.load(pre..'atlas.png')
	back_tex = tex.load(pre..'background.png')

	sim.init(scr_size.x, scr_size.y, 64, 64, 1)

	-- preprocess inside colours
	for i,pair in ipairs(inside_cols) do
		for j,col in ipairs(pair) do
			col.r = col.r / 255
			col.g = col.g / 255
			col.b = col.b / 255
		end
	end

	generate(40)
end

function close()
	tex.free(part_tex)
	tex.free(back_tex)
end

function render_particle_insides(pos, self, t)
	local grad = inside_cols[self.in_grad]
	local cola = lerp(grad[1], grad[2], self.in_cols[1])
	local colb = lerp(grad[1], grad[2], self.in_cols[2])
	cola.a = t
	colb.a = t
	local anga = time.s() * self.in_rot[1]
	local angb = time.s() * self.in_rot[2]

	video.draw_rect_centered(part_tex,
		part_layer+1, inside_rects[1], pos,
		anga, cola
	)

	video.draw_rect_centered(part_tex,
		part_layer+1, inside_rects[2], pos,
		angb, colb
	)
end

function render_particle(self)
	local c, r = self.center, self.radius
	local rect_i = self.color + (self.mass-1)*2
	local color = rgba(1, 1, 1, 1)
	if self.death_time ~= nil then
		local t = (time.s() - self.death_time)
		t = clamp(0, 1, t / sim.ghost_lifetime)
		color.a = 1 - t
	end
	local draw = function(pos)
		video.draw_rect_centered(
					part_tex, part_layer,
					part_rects[rect_i], pos,
					self.angle, color
		)

		local in_t = clamp(0, 1, self.in_t)	
		if in_t ~= 0 then
			if self.mass == 1 then
				render_particle_insides(pos, self, in_t)
			end
			if self.mass == 2 then
				local centers = self:centers()
				render_particle_insides(centers[1], self, in_t)
				render_particle_insides(centers[2], self, in_t)
			end
		end
	end

	draw(c)

	if c.x < r then
		draw(c + sim.path_offsets[1])
	end
	if c.x > sim.w - r then
		draw(c + sim.path_offsets[3])
	end
	if c.y < r then
		draw(c + sim.path_offsets[4])
	end
	if c.y > sim.h - r then
		draw(c + sim.path_offsets[2])
	end
end

function circle(c, r)
	local segs = 16
	for i=0,segs-1 do
		local a1 = i / segs * math.pi * 2
		local a2 = (i+1) / segs * math.pi * 2
		local p1 = vec2(math.cos(a1), math.sin(a1)) * r
		local p2 = vec2(math.cos(a2), math.sin(a2)) * r
		video.draw_seg(part_layer, c + p1, c + p2)
	end
end

function generate(n)
	for i = 1,n do
		sim.spawn_random()
	end
end

function enter()
end

function leave()
end

function update()
	sim.tick()
	effects.update()

	local push = mouse.pressed(mouse.primary)
	local pull = mouse.pressed(mouse.secondary)

	if push or pull then
		local mp = mouse.pos()
		effects.force_field(mp, pull, affect_radius)
		local parts = sim.query(mouse.pos(), affect_radius)
		for i,p in ipairs(parts) do
			local d, l = sim.path(mp, p.center)
			local r = affect_radius
			local f = 1 - clamp(0, 1, math.sqrt(l) / r)
			if push then
				f = -f
			end
			local dnorm = normalize(d)
			p.vel = p.vel + dnorm * f * affect_force
			p.in_t = math.min(2, p.in_t + time.dt() / 100)
		end
	end

	-- quit game on esc
	return not key.down(key.quit) 
end

function render(t)
	video.draw_rect(back_tex, 0, vec2(0, 0))
	effects.render(back_tex)
	
	for i,p in ipairs(sim.all) do
		render_particle(p)
	end

	for i,p in ipairs(sim.ghosts) do
		render_particle(p)
	end

	return true
end
