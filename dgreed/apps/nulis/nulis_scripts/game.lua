module(..., package.seeall)

require 'sim'

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

function init()
	part_tex = tex.load(pre..'atlas.png')
	back_tex = tex.load(pre..'background.png')

	sim.init(scr_size.x, scr_size.y, 64, 64)

	generate(60)
end

function close()
	tex.free(part_tex)
	tex.free(back_tex)
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
		local p = vec2(
			rand.float(0, scr_size.x),
			rand.float(0, scr_size.y)
		)

		local v = vec2(0, rand.float()/100)
		v = rotate(v, rand.float(0, math.pi * 2))

		local col = rand.int(1, 3)

		sim.add(sim.particle:new({
			center = p,
			vel = v,
			color = col,
			render = render_particle
		}))
	end
end

function enter()
end

function leave()
end

function update()
	sim.tick()

	local push = mouse.pressed(mouse.primary)
	local pull = mouse.pressed(mouse.secondary)

	if push or pull then
		local mp = mouse.pos()
		circle(mp, affect_radius)
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
		end
	end

	-- quit game on esc
	return not key.down(key.quit) 
end

function render(t)
	video.draw_rect(back_tex, 0, vec2(0, 0))
	
	for i,p in ipairs(sim.all) do
		render_particle(p)
	end

	for i,p in ipairs(sim.ghosts) do
		render_particle(p)
	end

	return true
end
