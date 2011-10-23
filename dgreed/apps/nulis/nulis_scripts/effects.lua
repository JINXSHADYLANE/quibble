module(..., package.seeall)

ffield_spawn_t = 0.2
ffield_last_spawn = 0
ffield_circles = {}
ffield_circle_lifetime = 0.5
ffield_circle_rect = rect(2, 770, 254, 768 + 254)
ffield_circle_layer = 1
ffield_color_start = rgba(0/255, 129/255, 255/255)
ffield_color_end = rgba(255/255, 0/255, 236/255)


function init()
	particles.init(pre, 5)

	snd_crash = sound.load_sample(pre..'bounce.wav')
	sound.set_volume(snd_crash, 0.4)
	snd_merge = sound.load_sample(pre..'bad_string.wav')
	snd_triple = sound.load_sample(pre..'windup+click.wav')
	snd_bad = sound.load_sample(pre..'hit.wav')
	snd_vanish = sound.load_sample(pre..'vanish.wav')
	snd_appear = sound.load_sample(pre..'appear.wav')
	sound.set_volume(snd_appear, 0.3)
	snd_win = sound.load_sample(pre..'win.wav')
end

function close()
	particles.close()

	sound.free(snd_crash)
	sound.free(snd_merge)
	sound.free(snd_triple)
	sound.free(snd_bad)
	sound.free(snd_vanish)
	sound.free(snd_appear)
	sound.free(snd_win)
end

function force_field(p, dir, r)
	local t = time.s()
	if t - ffield_last_spawn > ffield_spawn_t then
		ffield_last_spawn = t
		table.insert(ffield_circles, {
			center = p,
			radius = r,
			t0 = t,
			shrink = dir,
			col = rand.float()
		})
	end
end

function update_ffield()
	local i = 1
	while i <= #ffield_circles do
		local c = ffield_circles[i]
		local t = (time.s() - c.t0) / ffield_circle_lifetime
		if t < 0 or t > 1 then
			-- remove
			ffield_circles[i] = ffield_circles[#ffield_circles]
			table.remove(ffield_circles)
		else
			i = i + 1
		end
	end
end

function render_ffield(tex)
	for i,c in ipairs(ffield_circles) do
		local t = (time.s() - c.t0) / ffield_circle_lifetime
		t = clamp(0, 1, t)
		
		local scale
		if c.shrink then
			scale = lerp(c.radius, c.radius * 0.5, t)
		else
			scale = lerp(c.radius * 0.5, c.radius, t)
		end
		scale = scale / 128

		local color = lerp(ffield_color_start, ffield_color_end, c.col)
		color.a = math.sin(t * math.pi)
		video.draw_rect_centered(
			tex, ffield_circle_layer, ffield_circle_rect,
			c.center, 0, scale, color
		)
	end
end

last_collide = 0
function collide(p, dir)
	if time.s() - last_collide < 0.1 then
		return
	end
	particles.spawn('collision0', p, dir+math.pi/2)
	particles.spawn('collision0', p, dir-math.pi/2)
	sound.play(snd_crash)
	last_collide = time.s()
end

function create(p)
	particles.spawn('collision2', p, 0)
	particles.spawn('collision2', p, math.pi/2)
	particles.spawn('collision2', p, math.pi)
	particles.spawn('collision2', p, math.pi*3/2)
	particles.spawn('diffusion', p)
	sound.play(snd_bad)
end

function spawn(p)
	particles.spawn('blast1', p)
	sound.play(snd_appear)
end

function merge_two(p)
	particles.spawn('blast0', p)
	sound.play(snd_merge)
end

function merge_three(p)
	particles.spawn('wicked_blast0', p)
	sound.play(snd_triple)
end

last_reset = 0
function reset(p)
	particles.spawn('gameover', p)
	if time.s() - last_reset < 0.1 then
		return
	end
	sound.play(snd_vanish)
	last_reset = time.s()
end

function win()
	sound.play(snd_win)
end

function render(back_tex)
	particles.draw()
	render_ffield(back_tex)
end

function update()
	particles.update()
	update_ffield()
end
