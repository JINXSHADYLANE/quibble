module(..., package.seeall)

screen = rect(0, 0, scr_size.x, scr_size.y)

world_rect = rect(0, 0, 1600, 1600)

gravity_dir = vec2(0, 1)
world_rot = 0 
world_scale = 1 

objt = {
	start = 0,
	finish = 1,
	star_left = 2,
	star_right = 3,
	star_grow = 4,
	star_shrink = 5
}

cat = {
	p = vec2(),
	v = vec2(),
	dir = false,
	ground = false,

	width = 32,
	height = 16,

	move_acc = 0.8,
	move_damp = 0.8,
	jump_acc = 6,
	gravity = 0.5
}

camera = {
	center = vec2()
}

function reset(level_name)
	if level ~= nil then
		tilemap.free(level)
	end

	level = tilemap.load(pre..level_name)
	objects = tilemap.objects(level)
	for i,obj in ipairs(objects) do
		-- find start obj, center camera on it
		if obj.id == objt.start then
			camera.center = vec2(obj.pos)
			cat.p = vec2(obj.pos)
		end
	end
end

function init()
	reset('testlevel.btm')
end

function close()
	if level ~= nil then
		tilemap.free(level)
	end
end

function enter()
end

function leave()
end

function update()
	if key.down(key.quit) then
		states.pop()
	end

	-- devmode world manipulation
	if char.down('j') then
		world_scale = world_scale / 2
	end
	if char.down('k') then
		world_scale = world_scale * 2
	end
	if char.down('h') then
		world_rot = world_rot + math.pi/2
		gravity_dir = vec2(gravity_dir.y, -gravity_dir.x)
	end
	if char.down('l') then
		world_rot = world_rot - math.pi/2
		gravity_dir = vec2(-gravity_dir.y, gravity_dir.x)
	end

	-- set camera
	local pos, scale, rot = tilemap.camera(level)
	local d = camera.center - cat.p
	camera.center = camera.center - d * 0.05
	pos = vec2(camera.center)
	pos.x = math.floor(pos.x * world_scale) / world_scale
	pos.y = math.floor(pos.y * world_scale) / world_scale
	scale = lerp(scale, world_scale, 0.1)
	rot = lerp(rot, world_rot, 0.1)
	tilemap.set_camera(level, pos, scale, rot)

	update_cat()

	return true
end

function update_cat()
	local up = -gravity_dir
	local down = gravity_dir
	local left = vec2(up.y, -up.x) 
	local right = -left 

	if key.pressed(key._left) then
		cat.v = cat.v + left * cat.move_acc
		cat.dir = true
	end
	if key.pressed(key._right) then
		cat.v = cat.v + right * cat.move_acc
		cat.dir = false
	end
	if (key.down(key._up) or key.down(key.a)) and cat.ground then
		cat.ground = false
		cat.v = (right * dot(cat.v, right)) + (up * cat.jump_acc)
	end

	cat.v = cat.v + down * cat.gravity
	cat.v = right * dot(right, cat.v) * cat.move_damp + down * dot(down, cat.v)
	
	local hw, hh = cat.width/2 / world_scale, cat.height/2 / world_scale

	local bbox_tl = cat.p + left * hw + down * hh
	local bbox_br = cat.p + right * hw + up * hh
	local bbox = rect(
		math.min(bbox_tl.x, bbox_br.x),
		math.min(bbox_tl.y, bbox_br.y), 
		math.max(bbox_tl.x, bbox_br.x),
		math.max(bbox_tl.y, bbox_br.y)
	)

	local dx = right * dot(right, cat.v) / world_scale
	local dy = down * dot(down, cat.v) / world_scale
	
	-- horizontal move
	local dx = tilemap.collide_swept(level, bbox, dx)
	cat.p = cat.p + dx
	bbox.l = bbox.l + dx.x
	bbox.t = bbox.t + dx.y
	bbox.r = bbox.r + dx.x
	bbox.b = bbox.b + dx.y
	
	-- vertical move
	local fall = dot(dy, up) < 0
	local dy = tilemap.collide_swept(level, bbox, dy)
	if fall then
		cat.ground = length_sq(dy) == 0
	end
	cat.p = cat.p + dy
	cat.v = dx * world_scale + dy * world_scale
	--bbox_tl = bbox_tl + dy
	--bbox_br = bbox_br + dy
end

function render_cat()
	local screen_pos = tilemap.world2screen(level, screen, cat.p)
	--screen_pos.x = math.floor(screen_pos.x)
	--screen_pos.y = math.floor(screen_pos.y)
	local hw, hh = cat.width / 2, cat.height / 2
	local screen_bbox = rect(
		screen_pos.x - hw,
		screen_pos.y - hh,
		screen_pos.x + hw,
		screen_pos.y + hh
	)

	local col = rgba(0.5, 0.5, 0.5, 1)
	sprsheet.draw('empty', 15, screen_bbox, col)
		
end

function render(t)
	if level then
		tilemap.render(level, screen)
	end

	render_cat()

	return true
end

