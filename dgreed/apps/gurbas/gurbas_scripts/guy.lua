guy = {
	-- tweaks
	layer = 4,
	move_acc = 0.7,
	move_damp = 0.84,
	jump_acc = 16,
	gravity = 0.5,

	-- state
	size = vec2(62, 126),
	bbox = nil,
	dir = false,
	ground = false,
	did_win = false,
	fps = 12
}

guy.animations = {
	idle = { 
		frames = { 0, 0, 1, 1, 2, 2, 1, 1 }, 
		on_finish="loop" 
	},
	jump_up = { 
		frames= { 7, 9, 10, 11, 12 },
		on_finish="stop"
	},
	jump_down = {
		frames = { 13, 14, 15, 16 },
		on_finish="stop"
	},
	land = {
		frames = { 17, 19, 21, 23, 24, 25},
		on_finish="play idle"
	}
}	

function guy.init()
	guy.reset()
	guy.img = tex.load(pre..'spring.png')
end

function guy.close()
	tex.free(guy.img)
end

function guy.reset()
	guy.p =	vec2((screen.r - guy.size.x) / 2, 500) 
	guy.v = vec2()
	guy.did_win = false
	guy.play("idle")
end

function guy.play(name)
	if guy.animations[name] ~= nil then
		guy.animation = guy.animations[name]
		guy.animation_t = time.ms()
	end
end

function guy.frame()
	local i = 0
	local delta = time.ms() - guy.animation_t
	local n_frames = #guy.animation.frames
	local frame = delta / (1000 / guy.fps)
	if guy.animation.on_finish == "stop" then
		i =  math.min(#guy.animation.frames, math.floor(frame) + 1)
	elseif guy.animation.on_finish == "loop" then
		frame = frame - math.floor(frame / n_frames) * n_frames 
		i = math.floor(frame) + 1
	elseif string.find(guy.animation.on_finish, "play") ~= nil then
		if frame < #guy.animation.frames then
			return math.floor(frame) + 1
		end
		frame = frame - n_frames 
		local anim_name = string.sub(guy.animation.on_finish, 6)
		guy.play(anim_name)
		guy.animation_t = guy.animation_t - frame * (1000 / guy.fps)
		return guy.frame()
	end
	return guy.animation.frames[i]
end

function guy.collide_swept(offset)
	local pts = {}
	if offset.y > 0 then
		table.insert(pts, guy.p + vec2(2, guy.size.y))
		table.insert(pts, guy.p + vec2(guy.size.x - 2, guy.size.y))
	end
	if offset.y < 0 then
		table.insert(pts, guy.p + vec2(2, 0))
		table.insert(pts, guy.p + vec2(guy.size.x - 2, 0))
	end
	if offset.x > 0 then
		table.insert(pts, guy.p + vec2(guy.size.x, 0))
		table.insert(pts, guy.p + vec2(guy.size.x, guy.size.y/3))
		table.insert(pts, guy.p + vec2(guy.size.x, 2*(guy.size.y/3)))
		table.insert(pts, guy.p + vec2(guy.size.x, guy.size.y))
	end
	if offset.x < 0 then
		table.insert(pts, vec2(guy.p))
		table.insert(pts, guy.p + vec2(0, guy.size.y/3))
		table.insert(pts, guy.p + vec2(0, 2*(guy.size.y/3)))
		table.insert(pts, guy.p + vec2(0, guy.size.y))
	end

	local min_sq_len = length_sq(offset) 
	local min_ray = offset

	for i,p in ipairs(pts) do
		local wall_ray = well.raycast(p, p + offset)
		local wall_sq_len = length_sq(wall_ray - p)

		local block_ray = block.raycast(p, p + offset)
		local block_sq_len = length_sq(block_ray - p)

		local ray = block_ray
		local sq_len = block_sq_len

		if wall_sq_len < block_sq_len then
			ray = wall_ray
			sq_len = wall_sq_len
		end

		if sq_len < min_sq_len then
			min_sq_len = sq_len
			min_ray = ray - p
		end
	end

	return min_ray 
end

function guy.update()
	if key.pressed(key._left) and not guy.ground then
		guy.v.x = guy.v.x - guy.move_acc
		guy.dir = true
	end
	if key.pressed(key._right) and not guy.ground then
		guy.v.x = guy.v.x + guy.move_acc
		guy.dir = false
	end
	if key.down(key._up) and guy.ground then
		guy.ground = false
		guy.v.y = -guy.jump_acc
		guy.play("jump_up")
	end

	guy.v.y = guy.v.y + guy.gravity
	guy.v.x = guy.v.x * guy.move_damp

	dx = guy.collide_swept(vec2(guy.v.x, 0))
	guy.p = guy.p + dx
	guy.v.x = dx.x

	dy = guy.collide_swept(vec2(0, guy.v.y))
	if guy.v.y > 0 then
		guy.ground = dy.y == 0	
	end
	guy.p = guy.p + dy
	guy.v.y = dy.y

	if guy.ground then
		guy.v.x = 0
		if guy.animation ~= guy.animations.idle then
			if guy.animation ~= guy.animations.land then
				guy.play("land")
			end
		end
	end

	if not guy.ground and guy.v.y > 1.5 then
		if not guy.animation == guy.animations.jump_down then
			guy.play("jump_down")
		end
	end

	-- check block collision with head
	local upper_hitbox = rect(guy.p.x+2, guy.p.y+1, 
		guy.p.x + guy.size.x - 2, guy.p.y + guy.size.y / 3)
	
	if block.collide_rect(upper_hitbox) then
		return true
	end

	-- check head collision with well top (win condition)
	if upper_hitbox.t < 0 then
		guy.did_win = false
		return true
	end

	return false
end

function guy.draw()
	local frame = guy.frame() 
	local src = rect(64 * frame, 0, 64 * (frame+1), 128) 
	video.draw_rect(guy.img, guy.layer, src, guy.p)
end
