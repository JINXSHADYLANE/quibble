module(..., package.seeall)

mfont = nil
text_layer = 1
text_len = 4
back_tex = nil
vignette_tex = nil

vignette_rect = rect(0, 0, 512, 512)
vignette_t = nil
vignette_len = 7
vignette_layer = 7
vignette_did_reset = false

title_rect = rect(256, 768, 750, 1024)

affect_radius = 160 
affect_force = 0.02

levels = {
	{name = 'nulis', b=4, gb=1},
	{name = 'cascajal', b=3},
	{name = 'guariviara', w=2, b=2},
	{name = 'chiriqui', w=2, b=1},
	{name = 'tabasara', w=1, b=3},
	{name = 'cangandi', r=9},
	{name = 'pacora', w=1, b=1, start_spawning=3, spawn_interval=10},
	{name = 'sixoala', r=15, start_spawning=10, spawn_interval=5},
	{name = 'grande', r=100, start_spawning=20, spawn_interval=0.8}
}

current_level = 1
to_spawn = {w=0, b=0, gw=0, gb=0, tw=0, tb=0, r=0, c=0}

function init()
	mfont = font.load(pre..'varela.bft')
	back_tex = tex.load(pre..'background.png')
	vignette_tex = tex.load(pre..'vignette.png')

	csim.init()
	ceffects.init()

	reset(levels[1])
	current_level = 1
end

reset_t = 0
played_win_sound = false
function reset(level) 
	reset_t = time.s()
	played_win_sound = false
	local spawn_interval = level.spawn_interval or 1000
	local start_spawning = level.start_spawning or 0
	csim.reset(spawn_interval, start_spawning)

	if level.w then
		to_spawn.w = to_spawn.w + level.w
	end

	if level.b then
		to_spawn.b = to_spawn.b + level.b
	end

	if level.gw then
		to_spawn.gw = to_spawn.gw + level.gw
	end

	if level.gb then
		to_spawn.gb = to_spawn.gb + level.gb
	end

	if level.tw then
		to_spawn.tw = to_spawn.tw + level.tw
	end

	if level.tb then
		to_spawn.tb = to_spawn.tb + level.tb
	end

	if level.r then
		to_spawn.r = to_spawn.r + level.r
	end


	to_spawn.c = to_spawn.w + to_spawn.b
	to_spawn.c = to_spawn.c + to_spawn.gw + to_spawn.gb
	to_spawn.c = to_spawn.c + to_spawn.tw + to_spawn.tb
end

function close()
	ceffects.close()
	csim.close()
	font.free(mfont)
	tex.free(back_tex)
	tex.free(vignette_tex)
end

function enter()
end

function leave()
end

last_spawn_t = 0
spawn_interval = 0.1
function update()
	sound.update()

	if time.s() - last_spawn_t > spawn_interval then
		last_spawn_t = time.s()
		-- spawn particles if we need to
		local cc = to_spawn.w + to_spawn.b + to_spawn.gw + to_spawn.gb + to_spawn.tw + to_spawn.tb
		if cc > 0 then
			local a = (cc / to_spawn.c) * math.pi * 2
			local p = vec2(scr_size.x/2, scr_size.y/2)
			p = p + rotate(vec2(0, 170), a)

			-- first spawn white, then black ...
			local col_order = {'w', 'b', 'gw', 'gb', 'tw', 'tb'}
			local col_type = {1, 0, 3, 2, 5, 4}
			local col
			for i,c in ipairs(col_order) do
				if to_spawn[c] ~= nil and to_spawn[c] > 0 then
					col = col_type[i]
					to_spawn[c] = to_spawn[c] - 1
					break
				end
			end

			ceffects.spawn(p)
			csim.spawn(p, vec2(), col)
		else
			if to_spawn.r > 0 then
				csim.spawn_random()
				ceffects.spawn(p)
				to_spawn.r = to_spawn.r - 1
			end
		end
	end

	csim.update()
	ceffects.update()

	local push = mouse.pressed(mouse.primary)
	local pull = mouse.pressed(mouse.secondary)

	if push or pull then
		local mp = mouse.pos()
		local force = affect_force
		if pull then
			force = -force
		end
		csim.force_field(mp, affect_radius, force)
	end

	if time.s() - reset_t > 3 then
		-- play win sound
		if not played_win_sound and csim.count_alive() == 0 then
			ceffects.win()
			played_win_sound = true
		end

		-- start new level if no more particles
		if csim.count_alive() + csim.count_ghosts() == 0 then
			current_level = current_level+1
			if levels[current_level] == nil then
				current_level = 1
			end
			reset(levels[current_level])
		else
			-- check if game is not winnable
			if vignette_t == nil and 
				levels[current_level].start_spawning == nil then
				local mass_sum = csim.total_mass() 
				local winnable = mass_sum >= 3 or mass_sum == 0 

				if not winnable then
					vignette_t = time.s() + 4 
					vignette_did_reset = false
				end
			end
		end
	end

	-- quit game on esc
	if key.down(key.quit) then
		states.pop()
	end

	return true
end

function render(t)
	-- handle vignette
	if vignette_t ~= nil then
		local t = (time.s() - vignette_t) / vignette_len
		if t >= 0 and t < 1 then
			local color = rgba(1, 1, 1, math.sin(t * math.pi))

			video.draw_rect(
				vignette_tex, vignette_layer, vignette_rect,
				rect(0, 0, scr_size.x, scr_size.y), 0, color
			)
		end

		if t >= 0.5 and not vignette_did_reset then
			vignette_did_reset = true
			csim.destroy()
			reset(levels[current_level])
		end
		
		if t >= 1 then
			vignette_t = nil
		end
	end

	-- show title
	local t = (time.s() - 2) / text_len
	if t >= 0 and t < 1 then
		local color = rgba(1, 1, 1, math.sin(t * math.pi))
		video.draw_rect_centered(
			back_tex, text_layer, title_rect, 
			vec2(scr_size.x/2, scr_size.y/2), color
		)
	end

	-- show level name
	t = (time.s() - reset_t) / text_len
	if t >= 0 and t < 1 then
		local color = rgba(1, 1, 1, math.sin(t * math.pi))
		if current_level > 1 then
			video.draw_text(
				mfont, text_layer, 
				levels[current_level].name,
				vec2(32, 768 - 32 - 48), color
			)
		end
	end

	video.draw_rect(back_tex, 0, vec2(0, 0))
	ceffects.render()
	csim.render()

	return true
end
