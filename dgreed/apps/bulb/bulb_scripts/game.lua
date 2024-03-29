
dofile(src..'objects.lua')
dofile(src..'eyes.lua')

game = {
	font = nil,
	disp_text = nil,
	endscreen_show = false,
	level_loaded = false,
	endscreen_t = 0,

	endscreen_duration = 3
}	

draw_hitbox = false 

robo = {
	levels = {
		'tutorial level.btm',
		'pre entry.btm',
		'test level.btm',
		'level2.btm',
		'massive out.btm',
		'malcolm x.btm',
	},

	level_endscreens = {
		"They seem unfriendly...",
		"... and somewhat aggressive ...",
		"What if I won't find an exit?",
		"I hope there are more batteries.",
		"I can't run through this darkness forever..."
	},	

	atlas = nil,
	shadow = nil,
	img_empty = nil,
	dir = 0,
	frame = 1,
	img = nil,
	pos = vec2(),
	size = vec2(52, 52),
	bbox = nil,
	energy = 1,
	dead = false,
	death_t = nil,
	level = 1,

	-- tweakables
	speed = 0.2,
	anim_speed = 60,
	cam_speed = 0.05,
	max_light_radius = 300,
	energy_decr_speed = 30,
	battery_juice = 0.4,
	title_duration = 5,
}

function robo.anim_frames()
	robo.anim = {}
	local n_frames = 41
	-- up
	for i=0,n_frames-1 do
		local x, y = i*64, 0
		while x+64 > 1024 do
			x = x - 1024
			y = y + 96
		end
		robo.anim[i] = rect(x, y, x+64, y+96)
	end

	-- down
	for i=0,n_frames-1 do
		local x, y = i*64, 96*3 
		while x+64 > 1024 do
			x = x - 1024
			y = y + 96
		end
		robo.anim[100+i] = rect(x, y, x+64, y+96)
	end

	-- left/right
	for i=0,n_frames-2 do
		local x, y = i*96, 96*6
		while x+96 > 960 do
			x = x - 960 
			y = y + 96
		end
		robo.anim[200+i] = rect(x, y, x+96, y+96)
	end
	--robo.anim[200+40] = rect(96*7, 96*6, 96*8, 96*7)
end

function game.init()
	level = tilemap.load(pre..robo.levels[1])
	robo.anim_frames()
	game.reset()
	clighting.init(screen)
	objects.init()
	eyes.init()
	game.font = font.load(pre..'monaco.bft', 0.5)
	robo.atlas = tex.load(pre..'atlas.png')
	robo.img_empty = tex.load(pre..'obj_start.png')
	robo.shadow = tex.load(pre..'shadow.png')
	robo.img = tex.load(pre..'robo_anim_atlas.png')
	robo.title = tex.load(pre..'title.png')

	sfx = {}
	sfx.pickup = sound.load_sample(pre..'pickup.wav')
	sfx.footsteps = sound.load_sample(pre..'footsteps.wav')
	sfx.push = sound.load_sample(pre..'push.wav')
	sfx.creatures = sound.load_sample(pre..'creatures.wav')
	sfx.death = sound.load_sample(pre..'end1.wav')
	sfx.switch = sound.load_sample(pre..'switch.wav')
	sfx.win = sound.load_sample(pre..'end2.wav')

	sfx.src_footsteps = sound.play(sfx.footsteps, true)
	sfx.vol_footsteps = 0

	sfx.src_push = sound.play(sfx.push, true)
	sfx.vol_push = 0

	sfx.src_creatures = sound.play(sfx.creatures, true)
	sfx.vol_creatures = 0
end

function game.draw_title()
	if time.s() - 1 > robo.title_duration then
		return
	end

	local off_src = rect(0, 0, 512, 256)
	local on_src = rect(0, 256, 512, 512)


	local nt = clamp(0, 1, (time.s() - 1) / robo.title_duration)
	t = math.sin(nt * math.pi)

	local src = off_src
	if rand.int(0, math.floor(lerp(10, 1, t))) == 0 then
		src = on_src
	end
	if nt > 0.6 then
		src = off_src
	end

	local col = lerp(rgba(1, 1, 1, 0), rgba(1, 1, 1, 1), t)	
	video.draw_rect(robo.title, 5, src, vec2(0, 0), col)
end

function game.reset()
	objects.reset()
	objs = tilemap.objects(level)
	local start_found = false
	for i = 1,#objs do
		-- start obj
		if objs[i].id == 0 then
			camera_pos = objs[i].pos	
			camera_pos = camera_pos + robo.size/2
			robo.pos = objs[i].pos
			robo.bbox = rect(robo.pos.x, robo.pos.y)
			robo.bbox.r = robo.bbox.l + robo.size.x
			robo.bbox.b = robo.bbox.t + robo.size.y
			start_found = true
		end

		-- pass all objs elsewhere
		objects.add(objs[i].id, objs[i].pos)
	end

	eyes.reset()
	objects.seal()

	if not start_found then
		log.warning('no start obj found!')
	end

	robo.dead = false
	robo.energy = 1
	sound.set_volume(music, 0.3)
end

function game.close()
	sound.free(sfx.pickup)
	sound.free(sfx.footsteps)
	sound.free(sfx.push)
	sound.free(sfx.creatures)
	sound.free(sfx.death)
	sound.free(sfx.switch)
	sound.free(sfx.win)

	font.free(game.font)
	tilemap.free(level)
	tex.free(robo.atlas)
	tex.free(robo.img_empty)
	tex.free(robo.img)
	tex.free(robo.shadow)
	tex.free(robo.title)
	clighting.close()
	objects.close()
	eyes.close()
end

function game.update()
	if robo.dead or game.show_endscreen then
		return
	end

	robo.bbox = rect(robo.pos.x, robo.pos.y)
	robo.bbox.r = robo.bbox.l + robo.size.x
	robo.bbox.b = robo.bbox.t + robo.size.y

	local move = false
	local new_pos = vec2(robo.pos)	
	if key.pressed(key._left) then
		new_pos.x = new_pos.x - robo.speed * time.dt()
		robo.dir = 2
		move = true
	end
	if key.pressed(key._right) then
		new_pos.x = new_pos.x + robo.speed * time.dt()
		robo.dir = 3
		move = true
	end
	if key.pressed(key._up) then
		new_pos.y = new_pos.y - robo.speed * time.dt()
		robo.dir = 0
		move = true
	end
	if key.pressed(key._down) then
		new_pos.y = new_pos.y + robo.speed * time.dt()
		robo.dir = 1
		move = true
	end

	local offset = new_pos - robo.pos	

	if offset.x ~= 0 and offset.y ~= 0 then
		offset = offset / 2^0.5
	end
	
	local battery, snd_push, snd_button
	robo.bbox, battery, snd_push, snd_button = cobjects.move_player(offset)
	if char.pressed('w') and char.pressed('d') and char.pressed('v')
		and robo.energy < 1 then
		battery = true
	end
	if battery then
		sound.play(sfx.pickup)
		robo.energy = robo.energy + robo.battery_juice
		robo.energy = math.min(1, robo.energy)
	end
	if snd_button then
		sound.play(sfx.switch)
	end
	if snd_push then
		sfx.vol_push = lerp(sfx.vol_push, 0.3, 0.2)
	else
		sfx.vol_push = lerp(sfx.vol_push, 0, 0.2)
	end

	robo.pos.x = robo.bbox.l
	robo.pos.y = robo.bbox.t
	objects.interact(robo.bbox)

	-- move camera
	camera_pos.x = lerp(camera_pos.x, robo.pos.x + robo.size.x/2, robo.cam_speed)
	camera_pos.y = lerp(camera_pos.y, robo.pos.y + robo.size.y/2, robo.cam_speed)

	-- update energy
	robo.energy = robo.energy - time.dt()/1000 * 1/robo.energy_decr_speed
	robo.energy = math.max(0, robo.energy)

	-- update animation
	local sp = 0
	local ft = 0
	if move then
		sp = robo.anim_speed 
		ft = 0.3
	end
	sfx.vol_footsteps = lerp(sfx.vol_footsteps, ft, 0.3)
	robo.frame = robo.frame + time.dt()/1000 * sp 
	while math.floor(robo.frame) > 40 do 
		robo.frame = robo.frame - 40
	end

	-- sounds
	sound.set_src_volume(sfx.src_footsteps, sfx.vol_footsteps)
	sound.set_src_volume(sfx.src_push, sfx.vol_push)
	sound.set_src_volume(sfx.src_creatures, sfx.vol_creatures)
end

function robo.draw()
	local dest = tilemap.world2screen(level, screen, robo.bbox)
	if draw_hitbox then
		video.draw_rect(robo.img_empty, 3, dest, rgba(1, 1, 1, 0.5))
	end

	local f = math.floor(robo.frame)
	if robo.dir == 0 then
		f = f + 100
	end
	if robo.dir == 2 or robo.dir == 3 then
		f = f + 200 - 1
	end
	local d = vec2((dest.l + dest.r) / 2, (dest.t + dest.b) / 2 - 24)
	if robo.dir ~= 3 then
		video.draw_rect_centered(robo.img, 0, robo.anim[f], d)	
	else
		local dest = rect(d.x - 48, d.y - 48, d.x + 48, d.y + 48)
		dest.l, dest.r = dest.r, dest.l
		video.draw_rect(robo.img, 0, robo.anim[f], dest)
	end

	-- draw shadow
	local shadow_pos = vec2(d)
	shadow_pos.y = shadow_pos.y + 32
	local col = rgba(1, 1, 1, 0.7)
	video.draw_rect_centered(robo.shadow, 0, shadow_pos, 0.0, 0.7, col)

	local light = {}
	light.pos = vec2((dest.l + dest.r) / 2, (dest.t + dest.b) / 2 - 50)
	light.radius = robo.max_light_radius * robo.energy 
	light.alpha = lerp(1, 0.3, 1 - robo.energy) 
	local lights = {light}

	-- find visible beacons
	local window = tilemap.screen2world(level, screen, screen)
	local beacons = cobjects.get_beacons(window)
	for i, obj in ipairs(beacons) do
		local p = obj.pos + vec2(32, 7)
		p = tilemap.world2screen(level, screen, p)
		table.insert(lights, {pos=p, radius=obj.intensity, alpha=0.8})
	end
	
	clighting.render(2, lights)
	if not robo.dead then
		eyes.update(lights)
	end
	lights_cache = lights
end

function game.frame()
	game.update()

	local c_pos = vec2(
		math.floor(camera_pos.x+0.5),
		math.floor(camera_pos.y+0.5)
	)
	tilemap.set_camera(level, c_pos)
	tilemap.render(level, screen)
	robo.draw()
	objects.draw(lights_cache)
	eyes.draw()

	game.draw_title()

	if game.show_endscreen then
		local nt = (time.s() - game.endscreen_t) / game.endscreen_duration 
		local nt_fade = clamp(0, 1, nt)
		local nt_text = clamp(0, 1, (nt - 0.3)*1/(0.7))
		if robo.level > #robo.levels and nt_fade > 0.5 then
			nt_fade = 0.5
			nt_text = 0.5
		end
		local t_fade = math.sin(clamp(0, 1, nt_fade) * math.pi)
		local t_text = math.sin(clamp(0, 1, nt_text) * math.pi)
		local col = lerp(rgba(0, 0, 0, 0), rgba(0, 0, 0, 1), t_fade)
		video.draw_rect(robo.img_empty, 4, screen, col)
		local col = lerp(rgba(1, 1, 1, 0), rgba(1, 1, 1, 1), t_text)
		local pos = center(screen) + vec2(0, 160)
		if game.disp_text then
			video.draw_text_centered(game.font, 5, game.disp_text, pos, col)	
		else
			local p = center(screen) + vec2(0, 180)		
			video.draw_text_centered(game.font, 5, "I'm safe now.", p, col) 
			p.y = p.y + 60 
			video.draw_text_centered(game.font, 5, "... but why it's still dark?", p, col) 
		end
		
		if nt_fade > 0.5 and not game.level_loaded and robo.level <= #robo.levels then
			game.level_loaded = true
			tilemap.free(level)
			level = tilemap.load(pre..robo.levels[robo.level])
			game.reset()
		end
		if nt >= 1 and robo.level <= #robo.levels then
			game.show_endscreen = false
			game.level_loaded = false
		end
	end

	if robo.dead then
		-- fadeout

		sfx.vol_footsteps = lerp(sfx.vol_footsteps, 0, 0.2)
		sfx.vol_push = lerp(sfx.vol_footsteps, 0, 0.2)
		sfx.vol_creatures = lerp(sfx.vol_footsteps, 0, 0.2)
		local vol_music = lerp(sound.volume(music), 0, 0.2)
		sound.set_volume(music, vol_music)
		sound.set_src_volume(sfx.src_footsteps, sfx.vol_footsteps)
		sound.set_src_volume(sfx.src_push, sfx.vol_push)
		sound.set_src_volume(sfx.src_creatures, sfx.vol_creatures)


		local t = (time.s() - robo.death_t) / 5
		t = clamp(0, 1, t)
		local col = lerp(rgba(0, 0, 0, 0), rgba(0, 0, 0, 1), t)
		video.draw_rect(robo.img_empty, 4, screen, col)
		
		if t == 1 then
			game.reset()
		end
	end
end

