
dofile(src..'lighting.lua')
dofile(src..'objects.lua')

game = {}

robo = {
	img = nil,
	pos = vec2(),
	size = vec2(96, 96),
	bbox = nil,
	energy = 1,

	-- tweakables
	speed = 0.5,
	cam_speed = 0.05,
	max_light_radius = 300,
	energy_decr_speed = 30,
	battery_juice = 0.4,
	beacon_radius = 180,
}


function game.init()
	level = tilemap.load(pre..'test level.btm')
	robo.img = tex.load(pre..'robo.png')
	game.reset()
	lighting.init()
	objects.init()
end

function game.reset()
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

	objects.seal()

	if not start_found then
		log.warning('no start obj found!')
	end
end

function game.close()
	tilemap.free(level)
	tex.free(robo.img)
	lighting.destroy()
	objects.close()
end

function game.update()
	robo.bbox = rect(robo.pos.x, robo.pos.y)
	robo.bbox.r = robo.bbox.l + robo.size.x
	robo.bbox.b = robo.bbox.t + robo.size.y

	local new_pos = vec2(robo.pos)	
	if key.pressed(key._left) then
		new_pos.x = new_pos.x - robo.speed * time.dt()
	end
	if key.pressed(key._right) then
		new_pos.x = new_pos.x + robo.speed * time.dt()
	end
	if key.pressed(key._up) then
		new_pos.y = new_pos.y - robo.speed * time.dt()
	end
	if key.pressed(key._down) then
		new_pos.y = new_pos.y + robo.speed * time.dt()
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
end

function robo.draw()
	local dest = tilemap.world2screen(level, screen, robo.bbox)
	video.draw_rect(robo.img, 0, dest)	

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
			table.insert(lights, {pos=p, inten=robo.beacon_radius, base=0.8})
		end
	end
	
	lighting.render(2, lights)
end

function game.frame()
	game.update()

	tilemap.set_camera(level, camera_pos)
	tilemap.render(level, screen)
	robo.draw()
	objects.draw()
end

