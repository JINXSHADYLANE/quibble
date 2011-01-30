
eyes = {
	img = nil,
	anim = {},
	n_eyes = 24,
	half_size = vec2(16, 16),

	-- tweaks
	blink_time = 0.5 
}

level_rect = rect(0, 0, 3840, 2560) 

function eyes.init()
	eyes.img = tex.load(pre..'eyes.png')	
	for i=1,4 do
		eyes.anim[i] = rect(0, (i-1)*16, 32, i*16)
	end
	for i=5,7 do
		eyes.anim[i] = rect(eyes.anim[4-(i-5)])
	end
	eyes.anim[0] = rect(eyes.anim[1])
end

function eyes.reset()
	for i=1,eyes.n_eyes do
		eyes[i] = {}
		eyes[i].p = vec2(
			rand.float(level_rect.l, level_rect.r),
			rand.float(level_rect.t, level_rect.b)
		)	
		eyes[i].v = vec2()
		eyes[i].f = 1
		eyes[i].blink = -100
	end
end

function eyes.close()
	tex.free(eyes.img)
end

function eyes.update(lights)
	local min_d = 10000
	for i,e in ipairs(eyes) do
		local run_vect = vec2()
		for j,l in ipairs(lights) do
			local lp = tilemap.screen2world(level, screen, l.pos)
			if length(lp - e.p) < 1.1 * l.inten then
				if rand.int(0, 3) == 2 then
					run_vect = run_vect + normalize(e.p - lp) * 120	
				end
			end
		end

		for j,ee in ipairs(eyes) do
			if j ~= i and length(e.p - ee.p) < 50 then
				if rand.int(0, 4) == 2 then
					run_vect = run_vect + normalize(e.p - ee.p) * 60
				end
			end
		end

		if length(e.p - robo.pos) > 500 then
			if rand.int(0, 43) == 42 then
				run_vect = run_vect + normalize(robo.pos - e.p) * 800
			end
		end

		if run_vect.x == 0 and run_vect.y == 0 then
			if rand.int(0, 100) == 42 then
				run_vect = vec2(rand.float(), rand.float()) * 120 
			end
			if rand.int(0, 100) == 42 then
				run_vect = normalize(robo.pos - e.p) * 160 
			end
		end

		if time.s() - e.blink > eyes.blink_time then
			if rand.int(0, 250) == 42 then
				e.blink = time.s()
			end
		end

		e.v = e.v + run_vect
		e.p = e.p + e.v * time.dt()/1000
		e.v = e.v * 0.9

		-- distance to player
		local d = length(e.p - robo.pos)
		min_d = math.min(d, min_d)
	end
	if robo.energy < 0.2 and min_d < 20 then
		sound.play(sfx.death)
		robo.dead = true
		robo.death_t = time.s()
	end
	sfx.vol_creatures = 1 - clamp(0, 1, (min_d - 40) / 200)
end

function eyes.draw()
	local world_screen = tilemap.screen2world(level, screen, screen)
	for i,e in ipairs(eyes) do
		local r = rect(
			e.p.x - eyes.half_size.x, e.p.y - eyes.half_size.y,
			e.p.x + eyes.half_size.x, e.p.y + eyes.half_size.y
		)	

		local f = e.f
		if time.s() - e.blink < eyes.blink_time then
			local t = (time.s() - e.blink) / eyes.blink_time
			f = math.floor(t * 7)
		end

		if rect_rect_collision(world_screen, r) then
			local p = tilemap.world2screen(level, screen, e.p)
			video.draw_rect_centered(eyes.img, 3, eyes.anim[f], p)	
		end
	end
end

