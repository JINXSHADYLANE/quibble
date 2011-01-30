
dofile(src..'lighting.lua')
dofile(src..'objects.lua')
dofile(src..'eyes.lua')

game = {}

draw_hitbox = false 

robo = {
	shadow = nil,
	img_empty = nil,
	dir = 0,
	frame = 1,
	img = nil,
	pos = vec2(),
	size = vec2(62, 62),
	bbox = nil,
	energy = 1,
	dead = false,
	death_t = nil,

	-- tweakables
	speed = 0.2,
	anim_speed = 60,
	cam_speed = 0.05,
	max_light_radius = 300,
	energy_decr_speed = 30,
	battery_juice = 0.4,
	beacon_radius = 180,
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
	level = tilemap.load(pre..'test level.btm')
	robo.anim_frames()
	game.reset()
	lighting.init()
	objects.init()
	eyes.init()
	robo.img_empty = tex.load(pre..'obj_start.png')
	robo.shadow = tex.load(pre..'shadow.png')
	robo.img = tex.load(pre..'robo_anim_atlas.png')


	sfx = {}
	sfx.pickup = sound.load_sample(pre..'pickup.wav')
	sfx.footsteps = sound.load_sample(pre..'footsteps.wav')
	sfx.push = sound.load_sample(pre..'push.wav')
	sfx.creatures = sound.load_sample(pre..'creatures.wav')
	sfx.death = sound.load_sample(pre..'end2.wav')

	sfx.src_footsteps = sound.play(sfx.footsteps, true)
	sfx.vol_footsteps = 0

	sfx.src_push = sound.play(sfx.push, true)
	sfx.vol_push = 0

	sfx.src_creatures = sound.play(sfx.creatures, true)
	sfx.vol_creatures = 0
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
			start_found = true
		end

		-- pass other objs elsewhere
		if objs[i].id ~= 0 then
			objects.add(objs[i].id, objs[i].pos)
		end
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

	tilemap.free(level)
	tex.free(robo.img_empty)
	tex.free(robo.img)
	tex.free(robo.shadow)
	lighting.destroy()
	objects.close()
	eyes.close()
end

function game.update()
	if robo.dead then
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

	local offset = robo.pos - new_pos	

	-- x
	local dx = tilemap.collide_swept(level, robo.bbox, vec2(-offset.x, 0))
	local old_rect, old_pos = rect(robo.bbox), vec2(robo.pos)
	robo.pos = robo.pos + dx
	robo.bbox.l = robo.bbox.l + dx.x
	robo.bbox.r = robo.bbox.r + dx.x
	if not objects.interact(robo.bbox, dx) then
		robo.pos = vec2(old_pos)
		robo.rect = rect(old_rect)
	end

	-- y
	local dy = tilemap.collide_swept(level, robo.bbox, vec2(0, -offset.y))
	old_rect, old_pos = rect(robo.bbox), vec2(robo.pos)
	robo.pos = robo.pos + dy
	robo.bbox.t = robo.bbox.t + dy.y
	robo.bbox.b = robo.bbox.b + dy.y
	objects.interact(robo.bbox, dy)
	if not objects.interact(robo.bbox, dy) then
		robo.pos = vec2(old_pos)
		robo.rect = rect(old_rect)
	end

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
	light.pos = vec2((dest.l + dest.r) / 2, (dest.t + dest.b) / 2)
	light.inten = robo.max_light_radius * robo.energy 
	light.base = lerp(1, 0.3, 1 - robo.energy) 
	local lights = {light}

	-- find visible beacons
	local window = tilemap.screen2world(level, screen, screen)
	for i, obj in ipairs(objects.beacons) do
		local p = vec2((obj.rect.l + obj.rect.r) / 2, (obj.rect.t + obj.rect.b) / 2)
		if rect_circle_collision(window, p, robo.beacon_radius) or
			rect_point_collision(window, p) then
			p = tilemap.world2screen(level, screen, p)
			local int = robo.beacon_radius
			if obj.inten ~= nil then
				int = obj.inten
			end
			table.insert(lights, {pos=p, inten=int, base=0.8})
		end
	end
	
	lighting.render(2, lights)
	if not robo.dead then
		eyes.update(lights)
	end
	lights_cache = lights
end

function game.frame()
	game.update()

	tilemap.set_camera(level, camera_pos)
	tilemap.render(level, screen)
	robo.draw()
	objects.draw(lights_cache)
	eyes.draw()

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

