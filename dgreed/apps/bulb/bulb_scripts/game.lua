
dofile(src..'lighting.lua')

game = {}

robo = {
	img = nil,
	pos = vec2(),
	size = vec2(96, 96),
	bbox = nil,

	-- tweakables
	speed = 0.5,
	cam_speed = 0.05
}


function game.init()
	level = tilemap.load(pre..'test level.btm')
	robo.img = tex.load(pre..'robo.png')
	game.reset()
	lighting.init()
end

function game.reset()
	objects = tilemap.objects(level)
	local start_found = false
	for i = 1,#objects do
		-- start obj
		if objects[i].id == 0 then
			camera_pos = objects[i].pos	
			camera_pos = camera_pos + robo.size/2
			robo.pos = objects[i].pos
			start_found = true
		end
	end

	if not start_found then
		log.warning('no start obj found!')
	end
end

function game.close()
	tilemap.free(level)
	tex.free(robo.img)
	lighting.destroy()
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
	robo.pos = robo.pos + dx
	robo.bbox.l = robo.bbox.l + dx.x
	robo.bbox.r = robo.bbox.r + dx.x

	-- y
	local dy = tilemap.collide_swept(level, robo.bbox, vec2(0, -offset.y))
	robo.pos = robo.pos + dy
	robo.bbox.t = robo.bbox.t + dy.y
	robo.bbox.b = robo.bbox.b + dy.y

	-- move camera
	camera_pos.x = lerp(camera_pos.x, robo.pos.x + robo.size.x/2, robo.cam_speed)
	camera_pos.y = lerp(camera_pos.y, robo.pos.y + robo.size.y/2, robo.cam_speed)
end

function robo.draw()
	local dest = tilemap.world2screen(level, screen, robo.bbox)
	video.draw_rect(robo.img, 0, dest)	

	local light = {}
	light.pos = vec2((dest.l + dest.r) / 2, (dest.t + dest.b) / 2)
	light.inten = 300 
	light.base = 1 
	
	local lights = {light}
	lighting.render(2, lights)
end

function game.frame()
	game.update()

	tilemap.set_camera(level, camera_pos)
	tilemap.render(level, screen)
	robo.draw()
end

