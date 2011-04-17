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
	did_win = false
}

function guy.init()
	guy.reset()
	guy.snd_jump = sound.load_sample(pre..'jump.wav')
	guy.snd_death = sound.load_sample(pre..'death.wav')
end

function guy.close()
	sound.free(guy.snd_death)
	sound.free(guy.snd_jump)
end

function guy.reset()
	guy.p =	vec2((screen.r - guy.size.x) / 2, 500) 
	guy.v = vec2()
	guy.did_win = false
end

function guy.collide_swept(offset)
	local pts = {}
	if offset.y > 0 then
		table.insert(pts, guy.p + vec2(0, guy.size.y))
		table.insert(pts, guy.p + vec2(guy.size.x, guy.size.y))
	end
	if offset.y < 0 then
		table.insert(pts, vec2(guy.p))
		table.insert(pts, guy.p + vec2(guy.size.x, 0))
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
		return true
	end

	return false
end

function guy.draw()
	local pts = {
		guy.p,
		guy.p + vec2(guy.size.x, 0),
		guy.p + guy.size,
		guy.p + vec2(0, guy.size.y)
	}

	video.draw_seg(guy.layer, pts[1], pts[2], rgba(1, 1, 1, 1))	
	video.draw_seg(guy.layer, pts[2], pts[3], rgba(1, 1, 1, 1))	
	video.draw_seg(guy.layer, pts[3], pts[4], rgba(1, 1, 1, 1))	
	video.draw_seg(guy.layer, pts[4], pts[1], rgba(1, 1, 1, 1))	
end
