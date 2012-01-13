module(..., package.seeall)

btype_dict = {
	['b'] = 'bubble_b',
	['w'] = 'bubble_w',
	['db'] = 'double_bubble_b',
	['dw'] = 'double_bubble_w',
	['gb'] = 'gravity_b',
	['gw'] = 'gravity_w',
	['tb'] = 'time_b',
	['tw'] = 'time_w'
}

grav_radius = 320
time_radius = 160

ui_color_dark = rgba(0.1, 0.1, 0.1)

balls = {
	{t='b', p=vec2(0,0), v=vec2(30,30), time=1, s=1},
	{t='gb', p=vec2(100,0), v=vec2(0,0), time=1, s=1}
}

-- input state
use_touch = false
idown = false
ipos = nil
ihit_pos = nil
ihit_time = 0

function init()
end

function close()
end

function enter()
	log.info('Entering editor')
end

function leave()
	log.info('Leaving editor')
end

function update()
	-- update input state
	if not use_touch then
		if mouse.down(mouse.primary) then
			ihit_pos = mouse.pos()
			ihit_time = time.s()
			idown = true
		end

		if mouse.up(mouse.primary) then
			idown = false
		end

		ipos = mouse.pos()
	end

	if touch.count() > 0 then
		use_touch = true
		local t = touch.get(0)
		ihit_pos = t.hit_pos
		ihit_time = t.hit_time
		idown = true
	else
		if use_touch then
			idown = false
		end
	end

	-- quit
	if key.up(key.quit) then
		states.pop()
	end
	return true
end

function draw_arrow(layer, s, e)
	local dir = normalize(e - s)
	local h1 = rotate(dir, math.pi/6)
	local h2 = rotate(dir, -math.pi/6)
	video.draw_seg(layer, s, e, ui_color_dark)
	video.draw_seg(layer, e, e - h1 * 8, ui_color_dark)
	video.draw_seg(layer, e, e - h2 * 8, ui_color_dark)
end

function draw_circle(layer, p, r)
	local segs = math.ceil(7 + r / 12)
	local d = vec2(r, 0)

	for i=1,segs do
		local a = rotate(d, i / segs * 2 * math.pi)
		local b = rotate(d, (i-1) / segs * 2 * math.pi)
		video.draw_seg(layer, p+a, p+b, ui_color_dark)
	end
end

function selected_ball(pos) 
	local best = nil
	local best_d = nil
	for i,b in ipairs(balls) do
		local d_sqr = length_sq(b.p - pos)
		if d_sqr < 32*32 then
			if best_d == nil or best_d > d_sqr then
				best_d = d_sqr
				best = b
			end
		end
	end
	return best
end

function render_balls()
	for i,b in ipairs(balls) do
		local p = b.p + vec2(scr_size.x/2, scr_size.y/2)
		local s = 1

		if b.t == 'gw' or b.t == 'gb' or b.t == 'tw' or b.t == 'tb' then
			-- radius
			local r = 16 + (b.s-1)*2 
			assert(r <= 24)

			-- scale
			s = r / 16
			if b.t == 'tw' or b.t == 'tb' then
				s = s * 0.5
				draw_circle(3, p, time_radius)
			end

			if b.t == 'gw' or b.t == 'gb' then
				-- quadratic gravity fallof makes forces
				-- very small farther away from ball, so
				--  show smaller influence circle
				draw_circle(3, p, grav_radius/2)
			end
		end

		sprsheet.draw_centered(btype_dict[b.t], 2, p, 0, s)
		if length_sq(b.v) > 1 then
			draw_arrow(3, p, p + b.v)
		end
	end
end

function new_ball_wheel()
	local t = time.s()

	if t - ihit_time > 0.5 then
		sprsheet.draw_centered('editor_circle', 2, ihit_pos, 0.0, 1.3)

		local n, i = 8, 1
		for k,v in pairs(btype_dict) do
			local p = ihit_pos + rotate(vec2(120, 0), (i / n) * math.pi * 2)
			sprsheet.draw_centered(v, 2, p)
			i = i + 1

			if length_sq(p - ipos) < 32*32 then
				draw_circle(3, p, 40)
			end
		end
	end
end

function render()
	sprsheet.draw('background', 0, rect(0, 0, scr_size.x, scr_size.y))

	local mp = mouse.pos()
	if touch.count() > 0 then
		local t = touch.get(0)
		mp = t.hit_pos
	end

	if idown then
		local hot = selected_ball(mp)
		if hot == nil then
			new_ball_wheel()
		end
	end

	render_balls()
	return true
end
