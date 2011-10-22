module(..., package.seeall)

require 'sim'
require 'effects'

part_tex = nil
back_tex = nil
vignette_tex = nil

vignette_rect = rect(0, 0, 512, 512)
vignette_t = nil
vignette_len = 7
vignette_layer = 7
vignette_did_reset = false

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

levels = {
	{name = 'origin', w=4, b=1, start_spawning=0, spawn_interval=1000},
	{name = 'nulis', r=20, start_spawning=20, spawn_interval=2}
}

current_level = 1
to_spawn = {w=0, b=0, r=0, c=0}

function init()
	part_tex = tex.load(pre..'atlas.png')
	back_tex = tex.load(pre..'background.png')
	vignette_tex = tex.load(pre..'vignette.png')

	effects.init()

	-- preprocess inside colours
	for i,pair in ipairs(inside_cols) do
		for j,col in ipairs(pair) do
			col.r = col.r / 255
			col.g = col.g / 255
			col.b = col.b / 255
		end
	end

	reset(levels[1])
	current_level = 1
end

reset_t = 0
function reset(level) 
	reset_t = time.ms()
	sim.init(scr_size.x, scr_size.y, 64, 64, 
		level.spawn_interval, level.start_spawning)

	if level.w then
		to_spawn.w = to_spawn.w + level.w
	end

	if level.b then
		to_spawn.b = to_spawn.b + level.b
	end

	if level.r then
		to_spawn.r = to_spawn.r + level.r
	end

	to_spawn.c = to_spawn.w + to_spawn.b
end

function close()
	effects.close()
	tex.free(part_tex)
	tex.free(back_tex)
	tex.free(vignette_tex)
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

function enter()
end

function leave()
end

last_spawn_t = 0
spawn_interval = 0.1
function update()
	if time.s() - last_spawn_t > spawn_interval then
		last_spawn_t = time.s()
		-- spawn particles if we need to
		local cc = to_spawn.w + to_spawn.b
		if cc > 0 then
			local a = (cc / to_spawn.c) * math.pi * 2
			local p = vec2(sim.w/2, sim.h/2)
			p = p + rotate(vec2(0, 160), a)

			-- first spawn white, then black
			local col
			if to_spawn.w > 0 then
				col = 2
				to_spawn.w = to_spawn.w - 1
			else
				col = 1
				to_spawn.b = to_spawn.b - 1
			end

			effects.spawn(p)
			sim.add(sim.particle:new({
				center = p,
				vel = vec2(),
				color = col
			}))
		else
			if to_spawn.r > 0 then
				local p = sim.spawn_random()
				effects.spawn(p)
				to_spawn.r = to_spawn.r - 1
			end
		end
	end

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

	if time.s() - reset_t > 3 then
		-- start new level if no more particles
		if #sim.all + #sim.ghosts == 0 then
			current_level = current_level+1
			reset(levels[current_level])
		end

		-- check if game is not winnable
		if vignette_t == nil and 
			levels[current_level].start_spawning == 0 then
			local winnable = #sim.all >= 3 

			if not winnable then
				vignette_t = time.s() + 4 
				vignette_did_reset = false
			end
		end
	end

	-- quit game on esc
	return not key.down(key.quit) 
end

function render(t)
	-- handle vignette
	if vignette_t ~= nil then
		local t = (time.s() - vignette_t) / vignette_len
		if t >= 0 and t < 1 then
			local color = rgba(1, 1, 1, math.sin(t * math.pi))

			video.draw_rect(
				vignette_tex, vignette_layer, vignette_rect,
				rect(0, 0, sim.w, sim.h), 0, color
			)
		end
		if t >= 0.5 and not vignette_did_reset then
			vignette_did_reset = true
			for i,p in ipairs(sim.all) do
				effects.spawn(p.center)
			end
			reset(levels[current_level])
		end
		if t >= 1 then
			vignette_t = nil
		end
	end

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
