module(..., package.seeall)

require 'sim'

part_tex = nil
part_layer = 2

affect_radius = 120 
affect_force = 0.02

function init()
	part_tex = {
		tex.load(pre..'particle1.png'),
		tex.load(pre..'particle2.png')
	}

	sim.init(scr_size.x, scr_size.y, 64, 64)

	generate(100)
end

function render_particle(self)
	video.draw_rect_centered(
				part_tex[self.color], part_layer, self.center,
				0, 0.5
	)
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

function close()
	for i,t in ipairs(part_tex) do
		tex.free(t)
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
	sim.draw_grid()
	for i,p in ipairs(sim.all) do
		p:render()
	end

	return true
end
