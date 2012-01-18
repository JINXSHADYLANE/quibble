module(..., package.seeall)

btype_dict = {
	['b'] = 'bubble_b',
	['w'] = 'bubble_w',
	['db'] = 'double_bubble_b',
	['dw'] = 'double_bubble_w',
	['gb'] = 'gravity_b',
	['gw'] = 'gravity_w',
	['tb'] = 'time_b',
	['tw'] = 'time_w',
	['x'] = 'x_button'
}

iphone_scr = rect(1024/2 - 240, 768/2 - 160, 1024/2 + 240, 768/2 + 160)
snap_grid_size = 16

grav_radius = 320
time_radius = 160

ui_color_dark = rgba(0.1, 0.1, 0.1)
ui_color_light = rgba(0.4, 0.4, 0.5)
ui_color_barely_visible = rgba(0.6, 0.6, 0.6, 0.8)

-- level state
level_name = 'l1'
balls = {
}
spawn_at = 0
spawn_interval = 0.1

no_touch_zone = nil

-- input state
use_touch = false
idown = false
ipos = nil
ihit_pos = nil
ihit_time = 0

mdown = false
mpos = nil
mhit_pos = nil
mhit_time = 0

menu_visible = false
draw_grid = false
save_msg = nil
save_msg_t = 1.0

function init()
	scr_half = vec2(scr_size.x/2, scr_size.y/2)

	-- gui style
	gui_desc = {
		texture = tex.load(pre..'editor_atlas.png'),
		font = font.load(pre..'varela.bft'),
		text_color = ui_color_dark,

		first_layer = 5,
		second_layer = 6,
		text_layer = 7
	}

	local t
	t, gui_desc.src_button_down = sprsheet.get('button_pressed')
	t, gui_desc.src_button_up = sprsheet.get('button')
	t, gui_desc.src_switch_on_up = sprsheet.get('checkbox_on')
	t, gui_desc.src_switch_on_down = sprsheet.get('checkbox_on')
	t, gui_desc.src_switch_off_up = sprsheet.get('checkbox_off')
	t, gui_desc.src_switch_off_down = sprsheet.get('checkbox_off')
	t, gui_desc.src_slider = sprsheet.get('slider')
	t, gui_desc.src_slider_knob_up = sprsheet.get('slider_button')
	t, gui_desc.src_slider_knob_down = sprsheet.get('slider_button_pressed')

	gui.init(gui_desc)
end

function close()
	gui.close()
	tex.free(gui_desc.texture)
	font.free(gui_desc.font)
end

function enter()
	level_name = csim.level()
	balls = {}

	log.info('Entering editor, level: '..level_name)

	if file.exists(pre..'edlevel.mml') then
		local level_mml = mml.read(pre..'edlevel.mml')
		assert(#level_mml.childs == 1)
		local level = level_mml.childs[1]
		if level.value == level_name then
			load_level(level)
			return
		end
	end

	local levels_mml = mml.read(pre..'levels.mml')
	local level = nil
	for i,l in ipairs(levels_mml.childs) do
		if l.value == level_name then
			level = l
			break
		end
	end

	assert(level)

	load_level(level)
end

function leave()
	log.info('Leaving editor')
end

function update()
	if char.up('m') or touch.count() == 4 then
		menu_visible = true
		selected = nil
	end

	if not menu_visible then	
		-- update input state
		if touch.count() == 1 and not char.pressed('a') then
			local t = touch.get(0)
			ipos = t.pos
			ihit_pos = t.hit_pos
			ihit_time = t.hit_time
			idown = true
		else
			idown = false
		end

		if touch.count() == 2 or (char.pressed('a') and touch.count() == 1) then
			local ta, tb
			ta = touch.get(0)
			if not char.pressed('a') then
				tb = touch.get(1)
			else
				tb = ta
			end
			mpos = (ta.pos + tb.pos) * 0.5
			mhit_pos = (ta.hit_pos + tb.hit_pos) * 0.5
			mhit_time = math.max(ta.hit_time, tb.hit_time)
			mdown = true
		else
			mdown = false
		end

		if no_touch_zone then
			if idown then
				local ip = rect_point_collision(no_touch_zone, ipos)
				local iph = rect_point_collision(no_touch_zone, ihit_pos)
				if ip or iph then
					idown = false
				end
			end

			if mdown and (mp or mph) then
				local mp = rect_point_collision(no_touch_zone, mpos)
				local mph = rect_point_collision(no_touch_zone, mhit_pos)
				mdown = false
				if mp or mph then
					mdown = false
				end
			end
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

function draw_circle(layer, p, r, c)
	local segs = math.ceil(7 + r / 10)
	local d = vec2(r, 0)

	if c == nil then
		c = ui_color_dark
	end

	for i=1,segs do
		local a = rotate(d, i / segs * 2 * math.pi)
		local b = rotate(d, (i-1) / segs * 2 * math.pi)
		video.draw_seg(layer, p+a, p+b, c)
	end
end

function selected_ball(pos) 
	local best = nil
	local best_d = nil
	local p = pos - scr_half
	for i,b in ipairs(balls) do
		local d_sqr = length_sq(b.p - p)
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
		local p = b.p + scr_half
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
				s = s * 32 / 48
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

ball_wheel_angle = 0
function ball_wheel(pos, show_x)
	local t = time.s()
	local res = nil


	if t - ihit_time > 0.7 then
		sprsheet.draw_centered('editor_circle', 4, pos, 0.0, 1.3)

		local n, i = 8, 1
		if show_x then 
			n = 9 
		end

		for k,v in pairs(btype_dict) do
			if k ~= 'x' or show_x then
				local angle = ball_wheel_angle + (i / n) * math.pi * 2
				local p = pos + rotate(vec2(110, 0), angle)
				sprsheet.draw_centered(v, 5, p)
				i = i + 1

				if length_sq(p - ipos) < 32*32 then
					draw_circle(5, p, 40)
					res = k
				end

				local move = ipos - ihit_pos
				if length_sq(move) > 170*170 then
					if exit_angle == nil then
						exit_angle = move 
					else
						local cos_a = dot(normalize(exit_angle), normalize(move))
						local a = math.atan2(exit_angle.y, exit_angle.x) 
						a = a + math.atan2(move.y, move.x)
						ball_wheel_angle = a * 3
					end
				end
			end
		end
	else	
		exit_angle = nil
	end

	return res
end

function snap(v)
	v.x = math.floor(v.x/snap_grid_size) * snap_grid_size
	v.y = math.floor(v.y/snap_grid_size) * snap_grid_size
	return v
end

function round(x)
	return math.floor(x*100)/100
end

function render_grid(layer, col)
	vline = function(x)
		video.draw_seg(layer, vec2(x, 0), vec2(x, 768), col)
	end

	hline = function(y)
		video.draw_seg(layer, vec2(0, y), vec2(1024, y), col)
	end

	local w, h = 1024, 768

	local x = w / 2
	while x > 0 do
		vline(x)
		x = x - snap_grid_size
	end
	x = w / 2 
	while x < w do
		vline(x)
		x = x + snap_grid_size
	end

	local y = h / 2
	while y > 0 do
		hline(y)
		y = y - snap_grid_size
	end
	y = h / 2
	while y < h do
		hline(y)
		y = y + snap_grid_size
	end
end

function render_menu()
	sprsheet.draw('empty', 4, rect(0, 0, scr_size.x, scr_size.y), rgba(0, 0, 0, 0.5))

	gui.label(vec2(10, 10), '['..csim.level()..']')

	if save_msg then
		gui.label(vec2(100, 10), save_msg)
		save_msg_t = save_msg_t - 1/60
		if save_msg_t < 0 then 
			save_msg = nil
		end
	end

	draw_grid = gui.switch(vec2(10, 60), "grid")
	draw_iphone = gui.switch(vec2(10, 140), "iphone")

	gui.slider_set_state(vec2(256, 60), spawn_at / 30)
	local r = gui.slider(vec2(256, 60))
	spawn_at = math.floor(r * 30)
	gui.label(vec2(296, 75), 'Spawn when n < '..tostring(spawn_at))

	gui.slider_set_state(vec2(256, 170), spawn_interval / 10)
	local i = gui.slider(vec2(256, 170))
	spawn_interval = i * 10
	gui.label(vec2(296, 185), 'Spawn interval - '..tostring(round(spawn_interval))..'s')

	if gui.button(vec2(314, 300), "Play") then
		play()
	end

	if not in_fsdev() then
		if gui.button(vec2(314, 400), "Soft save") then
			soft_save()
		end
	end

	if gui.button(vec2(314, 500), "Hard save") then
		hard_save()
	end

	if gui.button(vec2(314, 650), 'Back') then
		menu_visible = false
	end
end

function render()
	if menu_visible then
		render_menu()
	end

	if draw_grid then
		render_grid(1, ui_color_barely_visible)
	end

	-- draw iphone screen size
	if draw_iphone then
		local col = ui_color_barely_visible
		if draw_grid then
			col = ui_color_dark
		end

		local x1, y1 = iphone_scr.l, iphone_scr.t
		local x2, y2 = iphone_scr.r, iphone_scr.b

		video.draw_seg(1, vec2(x1, y1), vec2(x2, y1), col)	
		video.draw_seg(1, vec2(x2, y1), vec2(x2, y2), col)	
		video.draw_seg(1, vec2(x2, y2), vec2(x1, y2), col)	
		video.draw_seg(1, vec2(x1, y2), vec2(x1, y1), col)	
	end

	sprsheet.draw('background', 0, rect(0, 0, scr_size.x, scr_size.y))

	local mp = mouse.pos()
	if touch.count() > 0 then
		local t = touch.get(0)
		mp = t.hit_pos
	end

	-- new ball wheel
	if idown then
		hot = selected_ball(ihit_pos)
		if hot == nil then
			new_ball_type = ball_wheel(ihit_pos, false)
		else	
			local p = hot.p + scr_half
			switch_ball_type = ball_wheel(p, true)
			draw_circle(3, p, 45)
			selected = hot
		end
	elseif new_ball_type ~= nil then
		-- add new ball

		local new_ball = {
			t=new_ball_type,
			p=snap(ihit_pos - scr_half),
			v=vec2(0,0),
			s=0,
			time=1
		}

		new_ball_type = nil

		table.insert(balls, new_ball) 
	elseif switch_ball_type ~= nil then
		-- change ball type/remove
		if switch_ball_type == 'x' then
			-- find position of ball to remove
			local pos = nil
			for i=1,#balls do
				if balls[i] == hot then
					pos = i
					break
				end
			end
			assert(pos)

			-- copy last ball to its place, shrink size
			balls[pos] = balls[#balls]
			table.remove(balls)
			selected = nil
		else
			hot.t = switch_ball_type
		end

		switch_ball_type = nil
	else
		local t = time.s()
		if ihit_time ~= nil and t - ihit_time > 0.3 and t - ihit_time < 0.7 then
			if selected then
				local v = ihit_pos - (selected.p + scr_half)
				if length_sq(v) < 120*120 then
					selected.v = v
					if length_sq(v) < 20*20 then
						selected.v = vec2(0, 0)
					end
				end
			end
		end
		ihit_time = nil
	end

	if selected and mdown then
		if length_sq(selected.p + scr_half - mpos) < 100*100 then
			selected.p = snap(mpos - scr_half)
		end
	end

	if selected then
		local upper_ui = selected.p.y > 0
		draw_circle(3, selected.p + scr_half, 40, ui_color_light)

		local ts_pos = vec2(10, 10)
		local ss_pos = vec2(10, 110)

		local h = 100
		local edit_scale = selected.t == 'gw' or selected.t == 'gb'
		edit_scale = edit_scale or selected.t == 'tw' or selected.t == 'tb'

		if edit_scale then
			h = 200
		end

		if upper_ui then
			no_touch_zone = rect(0, 0, 540, h)
		else
			no_touch_zone = rect(0, 768 - h, 540, 768)
			ts_pos.y = ts_pos.y + 768 - h
			ss_pos.y = ss_pos.y + 768 - h
		end


		gui.slider_set_state(ts_pos, selected.time / 10)
		gui.label(ts_pos + vec2(40, 12), 't = '..tostring(round(selected.time))) 
		selected.time = gui.slider(ts_pos) * 10

		if edit_scale then
			gui.slider_set_state(ss_pos, selected.s / 4)
			gui.label(ss_pos + vec2(40, 12), 's = '..tostring(selected.s)) 
			selected.s = math.floor(gui.slider(ss_pos) * 4 + 0.5)
		end

		sprsheet.draw('empty', 4, no_touch_zone, rgba(0, 0, 0, 0.5))
	else
		no_touch_zone = nil
	end

	render_balls()
	return true
end

function vec2str(v)
	return tostring(v.x)..','..tostring(v.y)
end

function str2vec(str)
	local x, y = string.match(str, '(.+),(.+)')
	return vec2(tonumber(x), tonumber(y))
end

function load_level(mml_node)
	spawn_at = 0
	spawn_interval = 0.1
	for i,c in ipairs(mml_node.childs) do
		if c.name == 'random_at' then
			spawn_at = tonumber(c.value)
		end

		if c.name == 'random_interval' then
			spawn_interval = tonumber(c.value)
		end

		if c.name == 'spawns' and c.childs ~= nil then
			for j,s in ipairs(c.childs) do
				local ball = {
					t = s.name,
					p = str2vec(s.value),
					v = vec2(0, 0),
					time = 0,
					s = 0
				}

				if s.childs ~= nil then
					for k,b in ipairs(s.childs) do
						if b.name == 't' then
							ball.time = tonumber(b.value)
						end

						if b.name == 's' then
							ball.s = tonumber(b.value)
						end

						if b.name == 'vel' then
							ball.v = str2vec(b.value)
						end
					end
				end

				table.insert(balls, ball)
			end
		end
	end
end

function make_mml_tree()
	local level = {
		name = 'level',
		value = level_name,
		childs = {}
	}

	if spawn_at > 0 and spawn_interval > 0 then
		table.insert(level.childs, {
			name = 'random_at',
			value = tostring(spawn_at)
		})

		table.insert(level.childs, {
			name = 'random_interval',
			value = tostring(spawn_interval)
		})
	end

	local spawns = {
		name = 'spawns',
		value = '_',
		childs = {}
	}

	for i,b in ipairs(balls) do
		local ball = {
			name = b.t,
			value = vec2str(b.p),
			childs = {}
		}

		if length_sq(b.v) > 1 then
			table.insert(ball.childs, {
				name = 'vel',
				value = vec2str(b.v)
			})
		end

		if b.time ~= 0 then
			table.insert(ball.childs, {
				name = 't',
				value = tostring(b.time)
			})
		end

		if b.s ~= 0 then
			table.insert(ball.childs, {
				name = 's',
				value = tostring(b.s)
			})
		end

		table.insert(spawns.childs, ball)
	end

	table.insert(level.childs, spawns)

	return level
end

function play()
	local level = make_mml_tree()
	local edlevel = {
		name = 'edlevel',
		value = '_',
		childs = {level}
	}

	mml.write(edlevel, pre..'edlevel.mml')

	clevels.parse_ed(pre..'edlevel.mml')
	csim.reset(level.value)

	states.pop()
end

fsdev = nil
function in_fsdev()
	if fsdev == nil then
		fsdev = false
		for i,p in ipairs(argv) do
			if p == '-fsdev' then
				fsdev = true
				break
			end
		end
	end

	return fsdev
end

function soft_save()
	if not in_fsdev() then
		local path = pre..'levels.mml'
		local levels_mml = mml.read(path)
		save(levels_mml)
		mml.write(levels_mml, path)
		save_msg = 'saved to '..path
		save_msg_t = 1.0 
	else
		log.warning('Soft save does not work in fsdev!')
	end
end

function hard_save()
	local path = '../apps/nulis2/nulis2_assets/levels.mml'
	if in_fsdev() then
		path = pre..'levels.mml'
	end

	local levels_mml = mml.read(path)
	save(levels_mml)
	mml.write(levels_mml, path)
	save_msg = 'saved to '..path
	save_msg_t = 1.0 
end

function save(levels_mml)
	local level = nil
	for i,l in ipairs(levels_mml.childs) do
		if l.value == level_name then
			local new = make_mml_tree()
			l.childs = new.childs
			break
		end
	end
end

