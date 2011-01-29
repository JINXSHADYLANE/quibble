
eyes = {
	img = nil,
	n_eyes = 20,
	half_size = vec2(16, 16)
}

level_rect = rect(0, 0, 1920, 1280) 

function eyes.init()
	eyes.img = tex.load(pre..'eyes.png')	

	for i=1,eyes.n_eyes do
		eyes[i] = {}
		eyes[i].p = vec2(
			rand.float(level_rect.l, level_rect.r),
			rand.float(level_rect.t, level_rect.b)
		)	
		eyes[i].v = vec2()
	end
end

function eyes.close()
	tex.free(eyes.img)
end

function eyes.update(lights)
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

		if length(e.p - robo.pos) > 700 then
			if rand.int(0, 50) == 42 then
				run_vect = run_vect + normalize(robo.pos - e.p) * 400
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

		e.v = e.v + run_vect
		e.p = e.p + e.v * time.dt()/1000
		e.v = e.v * 0.9
	end
end

function eyes.draw()
	local world_screen = tilemap.screen2world(level, screen, screen)
	for i,e in ipairs(eyes) do
		local r = rect(
			e.p.x - eyes.half_size.x, e.p.y - eyes.half_size.y,
			e.p.x + eyes.half_size.x, e.p.y + eyes.half_size.y
		)	

		if rect_rect_collision(world_screen, r) then
			local p = tilemap.world2screen(level, screen, e.p)
			video.draw_rect_centered(eyes.img, 3, p)	
		end
	end
end

