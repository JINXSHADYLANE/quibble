guy = {
	-- tweaks
	layer = 6,
	move_acc = 0.9,
	move_damp = 0.86,
	jump_acc = 22,
	gravity = 0.9,

	-- state
	size = vec2(60, 160),
	bbox = nil,
	dir = false,
	ground = false,
	did_win = false,
	fps = 15 
}

guy.animations = {
	idle = { 
		frames = { 3, 2, 1, 0, 1, 2 }, 
		on_finish="loop" 
	},
	jump_up = { 
		frames= { 4, 5, 6 },
		on_finish="stop"
	},
	jump_down = {
		frames = { 7, 8, 9, 10, 11, 11, 12 },
		on_finish="stop"
	},
	land = {
		frames = { 13, 14, 15, 13, 11, 3},
		on_finish="play idle"
	}
}	

function guy.init()
	guy.reset()
	guy.img = tex.load(pre..'spring.png')
	guy.snd_jump = sound.load_sample(pre..'jump.wav')
	guy.snd_win = sound.load_sample(pre..'victory.wav')
	guy.snd_death = sound.load_sample(pre..'death.wav')
end

function guy.close()
	sound.free(guy.snd_death)
	sound.free(guy.snd_win)
	sound.free(guy.snd_jump)
	tex.free(guy.img)
end

function guy.reset()
	guy.on_moving_platform = false 
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
			i = math.floor(frame) + 1
		else
			frame = frame - n_frames 
			local anim_name = string.sub(guy.animation.on_finish, 6)
			guy.play(anim_name)
			guy.animation_t = guy.animation_t - frame * (1000 / guy.fps)
			return guy.frame()
		end
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
	local on_moving_platform = false 

	for i,p in ipairs(pts) do
		local wall_ray = well.raycast(p, p + offset)
		local wall_sq_len = length_sq(wall_ray - p)

		local block_ray = block.raycast(p, p + offset)
		local block_sq_len = length_sq(block_ray - p)

		local ray = wall_ray
		local sq_len = wall_sq_len

		if block_sq_len < sq_len then
			ray = block_ray
			sq_len = block_sq_len
			on_moving_platform = true
		end

		if sq_len < min_sq_len then
			min_sq_len = sq_len
			min_ray = ray - p
		end
	end

	guy.on_moving_platform = on_moving_platform
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
		sound.play(guy.snd_jump)
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
	else	
		if guy.v.y > 0.5 and guy.animation ~= guy.animations.jump_down then
			guy.play("jump_down")
		end
	end

	-- check block collision with head
	local upper_hitbox = rect(guy.p.x+2, guy.p.y+1, 
		guy.p.x + guy.size.x - 2, guy.p.y + guy.size.y / 3)
	
	if block.collide_rect(upper_hitbox) then
		sound.play(guy.snd_death)
		return true
	end

	-- check head collision with well top (win condition)
	if upper_hitbox.t < 0 then
		guy.did_win = false
		sound.play(guy.snd_win)
		return true
	end

	return false
end

function guy.draw()
	local frame = guy.frame() 
	local src = rect(64 * frame, 0, 64 * (frame+1), 160) 
	video.draw_rect(guy.img, guy.layer, src, guy.p)
end
