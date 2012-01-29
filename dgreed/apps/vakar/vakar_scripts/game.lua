module(..., package.seeall)

screen = rect(0, 0, scr_size.x, scr_size.y)

world_rect = rect(0, 0, 1600, 1600)
shrunk_world_rect = rect(100, 100, 1500, 1500)

gravity_dir = vec2(0, 1)
world_rot = 0 
world_scale = 1 

tile_size = 16
star_size = 6
star_respawn_t = 5

levelend_fadeout_t = nil
levelend_fadeout_len = 3
levelend_did_reset = false

levels = {
	'first.btm',
	'curiouser.btm',
	'twister.btm',
	'drinkme.btm'
}

current_level = 1

objt = {
	start = 0,
	finish = 1,
	star_left = 2,
	star_right = 3,
	star_grow = 4,
	star_shrink = 5
}

star_imgs = {
	[objt.star_left] = 'star_left',
	[objt.star_right] = 'star_right',
	[objt.star_grow] = 'star_grow',
	[objt.star_shrink] = 'star_shrink'
}

cat = {
	p = vec2(),
	v = vec2(),
	dir = false,
	ground = false,

	width = 16,
	height = 16,

	move_acc = 0.8,
	move_damp = 0.8,
	jump_acc = 6,
	gravity = 0.5,

	fps = 10
}

cat.animations = {
	stand = {
		frames = {15, 14},
		on_finish="stop"
	},
	walk = {
		frames = {0, 1, 2, 3, 4, 5},
		on_finish="loop"
	},
	jump_up_vert = {
		frames = {16},
		on_finish="stop"
	},
	jump_down_vert = {
		frames = {17},
		on_finish="stop"
	},
	jump_up = {
		frames = {6, 7, 8},
		on_finish="stop"
	},
	jump_down = {
		frames = {11, 12, 13},
		on_finish="stop"
	},
	land = {
		frames = {6},
		on_finish="play walk"
	},
	land_vert = {
		frames = {18},
		on_finish="play stand"
	}
}

function cat.play(name)
	if cat.animations[name] ~= nil then
		cat.animation = cat.animations[name]
		cat.animation_t = time.s()
	end
end

function cat.frame()
	local i = 0
	local delta = time.s() - cat.animation_t
	local n_frames = #cat.animation.frames
	local frame = delta * cat.fps
	if cat.animation.on_finish == "stop" then
		i =  math.min(n_frames, math.floor(frame) + 1)
	elseif cat.animation.on_finish == "loop" then
		frame = frame - math.floor(frame / n_frames) * n_frames 
		i = math.floor(frame) + 1
	elseif string.find(cat.animation.on_finish, "play") ~= nil then
		if frame < n_frames then
			i = math.floor(frame) + 1
		else
			frame = frame - n_frames 
			local anim_name = string.sub(cat.animation.on_finish, 6)
			cat.play(anim_name)
			cat.animation_t = cat.animation_t - frame  / cat.fps
			return cat.frame()
		end
	end
	return cat.animation.frames[i] 
end

camera = {
	center = vec2()
}

function reset(level_name)
	if level ~= nil then
		tilemap.free(level)
	end

	level = tilemap.load(pre..level_name)
	objects = tilemap.objects(level)
	
	reset_level()
end

function reset_level()
	world_rot = 0
	world_scale = 1
	gravity_dir = vec2(0, 1)
	cat.v = vec2(0, 0)
	cat.play('stand')
	for i,obj in ipairs(objects) do
		-- find start obj, center camera on it
		if obj.id == objt.start then
			camera.center = vec2(obj.pos)
			cat.p = vec2(obj.pos)
		end
	end
end

function init()
	reset(levels[current_level])
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

	sound.update()
	mfx.update()

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
	scale = lerp(scale, world_scale, 0.2)
	rot = lerp(rot, world_rot, 0.2)
	tilemap.set_camera(level, pos, scale, rot)

	update_cat()

	return true
end

function raycast_move(pass_a, pass_b, left, right) 
	if not pass_a then
		cat.v = cat.v + left * cat.move_acc / 3
	end

	if not pass_b then
		cat.v = cat.v + right * cat.move_acc / 3
	end
end

function update_cat()
	local up = -gravity_dir
	local down = gravity_dir
	local left = vec2(up.y, -up.x) 
	local right = -left 

	if key.pressed(key._left) then
		if length_sq(cat.v) > 0 
			and cat.animation ~= cat.animations.walk
			and cat.animation ~= cat.animations.jump_down
			and cat.animation ~= cat.animations.jump_up then
			cat.play('walk')
		end
		cat.v = cat.v + left * cat.move_acc
		cat.dir = true
		if cat.ground then
			mfx.trigger('walk')
		end
	end
	if key.pressed(key._right) then
		if length_sq(cat.v) > 0
			and cat.animation ~= cat.animations.walk 
			and cat.animation ~= cat.animations.jump_down
			and cat.animation ~= cat.animations.jump_up then 
			cat.play('walk')
		end
		cat.v = cat.v + right * cat.move_acc
		cat.dir = false
		if cat.ground then
			mfx.trigger('walk')
		end
	end
	if (key.down(key._up) or key.down(key.a)) and cat.ground then
		cat.ground = false
		cat.v = (right * dot(cat.v, right)) + (up * cat.jump_acc)
		if math.abs(dot(right, cat.v)) > 1 then
			cat.play('jump_up')
		else
			cat.play('jump_up_vert')
		end
		mfx.trigger('jump')
	end

	cat.v = cat.v + down * cat.gravity
	cat.v = right * dot(right, cat.v) * cat.move_damp + down * dot(down, cat.v)

	local hw, hh = cat.width / (2 * world_scale), cat.height / (2 * world_scale)

	if cat.ground then
		-- fall from edge check
		local lr = cat.p + left * hw/2 + down * hh	
		local ll = cat.p + right * hw/2 + down * hh 
		local lrd = lr + down
		local lld = ll + down
		local ray_a = tilemap.raycast(level, lr, lrd)
		local ray_b = tilemap.raycast(level, ll, lld)
		local pass_a = length_sq(ray_a - lrd) < 0.1
		local pass_b = length_sq(ray_b - lld) < 0.1

		if pass_a or pass_b then
			if not (pass_a and pass_b) then
				hw = hw / 2
				raycast_move(pass_b, pass_a, left, right)
			end
		end

		if pass_a and pass_b then
			-- stand on very edge check
			local lr = cat.p + left * hw + down * hh	
			local ll = cat.p + right * hw + down * hh 
			local lrd = lr + down
			local lld = ll + down
			local ray_a = tilemap.raycast(level, lr, lrd)
			local ray_b = tilemap.raycast(level, ll, lld)
			local pass_a = length_sq(ray_a - lrd) < 0.1
			local pass_b = length_sq(ray_b - lld) < 0.1
			if not (pass_a and pass_b) then
				raycast_move(pass_b, pass_a, left, right)
			end
		end
	elseif dot(down, cat.v) > 0.1 then
		-- push from wall when falling
		local lr = cat.p + left * hw + down * hh	
		local ll = cat.p + right * hw + down * hh 
		local lrr = lr + right
		local lll = ll + left
		local ray_a = tilemap.raycast(level, lr, lrr)
		local ray_b = tilemap.raycast(level, ll, lll)
		local pass_a = length_sq(ray_a - lrr) < 0.1
		local pass_b = length_sq(ray_b - lll) < 0.1
		raycast_move(pass_a, pass_b, left, right)
	end


	--if not cat.ground or cat.animation == cat.animations.stand then
	--	hw = hw / 2
	--end

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
		local l = length_sq(dy)
		if not cat.ground and l == 0 then
			if math.abs(dot(right, cat.v)) > 1 then
				cat.play('land')
			else
				cat.play('land_vert')
			end
			mfx.trigger('land')
		end
		cat.ground = length_sq(dy) == 0
	end
	cat.p = cat.p + dy
	cat.v = dx * world_scale + dy * world_scale

	if cat.animation ~= cat.animations.jump_down 
		and cat.animation ~= cat.animations.jump_down_vert
		and dot(down, cat.v) > 0.1 then
		if math.abs(dot(right, cat.v)) > 3 then
			cat.play('jump_down')
		else
			cat.play('jump_down_vert')
		end
	elseif cat.ground and cat.animation ~= cat.animations.stand 
		and cat.animation ~= cat.animations.jump_down
		and math.abs(dot(right, cat.v)) < 0.1 then
		cat.play('stand')
	end

	bbox.l = bbox.l + dx.x
	bbox.t = bbox.t + dx.y
	bbox.r = bbox.r + dx.x
	bbox.b = bbox.b + dx.y

	if not rect_rect_collision(shrunk_world_rect, bbox) then
		-- death, reset pos to start
		reset_level()
	end

	collide_objs(bbox)
end

function collide_objs(bbox)
	local t = time.s()
	local hs = tile_size/2
	for i,obj in ipairs(objects) do
		local center = vec2(obj.pos.x + hs, obj.pos.y + hs)
		if not obj.taken then
			local r = rect(
				center.x - star_size, center.y + hs - star_size,
				center.x + star_size, center.y + star_size
			)

			if rect_rect_collision(bbox, r) then
				if levelend_fadeout_t == nil and obj.id == objt.finish then
					levelend_fadeout_t = time.s()	
					levelend_did_reset = false
				end

				if obj.id == objt.star_right then
					world_rot = world_rot + math.pi/2
					gravity_dir = vec2(gravity_dir.y, -gravity_dir.x)
					mfx.trigger('star_take', center)
					obj.taken = true
					obj.t = t
				end

				if obj.id == objt.star_left then
					world_rot = world_rot - math.pi/2
					gravity_dir = vec2(-gravity_dir.y, gravity_dir.x)
					mfx.trigger('star_take', center)
					obj.taken = true
					obj.t = t
				end

				if obj.id == objt.star_shrink then
					world_scale = world_scale / 2
					mfx.trigger('star_take', center)
					obj.taken = true
					obj.t = t
				end

				if obj.id == objt.star_grow then
					world_scale = world_scale * 2
					mfx.trigger('star_take', center)
					obj.taken = true
					obj.t = t
				end
			end
		else
			if t - obj.t > star_respawn_t then
				mfx.trigger('star_respawn', center)
				obj.taken = false
			end
		end
	end
end

function render_background()
	-- lower
	local sw = scr_size.x / 2
	local sh = scr_size.y / 2
	local w = 1024 
	local h = 768 
	local x = w + math.fmod(-camera.center.x / 3, w) 
	local y = 192 + (camera.center.y / world_rect.b) * 384

	sprsheet.draw_centered('background1', 0, vec2(sw + x, y), 0, 2)
	sprsheet.draw_centered('background1', 0, vec2(sw + x-w,  y), 0, 2)

	-- higher
	w = 512 
	h = 384 
	local x = w + math.fmod(-camera.center.x / 1.5, w)
	local y = h + math.fmod(-camera.center.y / 1.5, h)

	sprsheet.draw_centered('background2', 0, vec2(sw + x, sh + y), 0, 1)
	sprsheet.draw_centered('background2', 0, vec2(sw + x-w, sh + y), 0, 1)
	sprsheet.draw_centered('background2', 0, vec2(sw + x, sh + y-h), 0, 1)
	sprsheet.draw_centered('background2', 0, vec2(sw + x-w, sh + y-h), 0, 1)
end

function render_cat()
	local screen_pos = tilemap.world2screen(level, screen, cat.p)
	--screen_pos.x = math.floor(screen_pos.x)
	--screen_pos.y = math.floor(screen_pos.y)
	local hw, hh = cat.width / 2, cat.height / 2
	local screen_bbox = rect(
		screen_pos.x - hw*2,
		screen_pos.y - hh*3,
		screen_pos.x + hw*2,
		screen_pos.y + hh
	)

	if cat.dir then
		screen_bbox.l, screen_bbox.r = screen_bbox.r, screen_bbox.l
	end

	local frame = cat.frame()
	if frame == 16 then
		screen_bbox.t = screen_bbox.t + hh
		screen_bbox.b = screen_bbox.b + hh
	end

	local col = rgba(0.5, 0.5, 0.5, 1)
	sprsheet.draw_anim('cat_walk', frame, 1, screen_bbox, col)
end

function render_objs()
	local hs = tile_size/2
	local ext_screen = rect(
		screen.l - 64, screen.t - 64,
		screen.r + 64, screen.b + 64
	)
	local world_screen = tilemap.screen2world(level, screen, ext_screen)
	for i,obj in ipairs(objects) do
		if not obj.taken and obj.id >= objt.star_left and obj.id <= objt.star_shrink then
			local p = vec2(obj.pos.x + hs, obj.pos.y + hs)
			local r = rect(obj.pos.x, obj.pos.y, p.x + hs, p.y + hs)
			if rect_rect_collision(world_screen, r) then
				local sp = tilemap.world2screen(level, screen, p)
				local img = star_imgs[obj.id]
				local alpha = (math.sin(time.s() + p.x*2 + p.y) + 1) / 2
				alpha = 0.5 + alpha / 2
				local col = rgba(1, 1, 1, alpha)
				sprsheet.draw_centered(img, 1, sp, world_rot, world_scale, col)
			end
		end
	end
end

function render_fadeout()
	local t = (time.s() - levelend_fadeout_t) / levelend_fadeout_len
	local col = rgba(0, 0, 0, math.sin(t * math.pi))

	if t >= 1 then
		levelend_fadeout_t = nil
		levelend_did_reset = false
		return
	end

	sprsheet.draw('empty', 4, screen, col)

	if not levelend_did_reset and t > 0.5 then
		current_level = current_level + 1
		if current_level > #levels then
			current_level = 1
		end
		levelend_did_reset = true
		reset(levels[current_level])
	end

end

function render(t)

	render_background()
	render_cat()
	render_objs()

	if levelend_fadeout_t then
		render_fadeout()
	end

	if level then
		tilemap.render(level, screen)
	end

	return true
end

