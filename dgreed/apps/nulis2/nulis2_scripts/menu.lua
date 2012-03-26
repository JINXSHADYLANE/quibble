module(..., package.seeall)

require 'tutorials'
require 'buy'

paid_level = 12 

n_levels = 50

background_layer = 6
icons_layer = 7

color_white = rgba(1, 1, 1, 1)
color_selected = rgba(0.5, 0.5, 0.5, 0.8)

sprs = {}

-- touch
touch_up = nil
touch_current = nil
touch_sliding = false

-- ui state
state_sound = true
state_music = true

-- icon angle
angle = nil

function init()
	sprs.background = sprsheet.get_handle('vignette')
	sprs.resume = sprsheet.get_handle('resume')
	sprs.replay = sprsheet.get_handle('replay')
	sprs.sound = sprsheet.get_handle('sound')
	sprs.sound_off = sprsheet.get_handle('sound_mute')
	sprs.music = sprsheet.get_handle('music')
	sprs.music_off = sprsheet.get_handle('music_mute')
	sprs.score = sprsheet.get_handle('score')
	sprs.back = sprsheet.get_handle('back')
	sprs.locked = sprsheet.get_handle('locked')
	sprs.levels = sprsheet.get_handle('levels')
	sprs.leaderboards = sprsheet.get_handle('high_score')
	sprs.achievements = sprsheet.get_handle('achievements')
	sprs.twitter = sprsheet.get_handle('twitter')
	sprs.plus = sprsheet.get_handle('plus')

	local t, icon_rect = sprsheet.get(sprs.locked)
	half_icon_w, half_icon_h = (icon_rect.r - icon_rect.l)/2, (icon_rect.b - icon_rect.t)/2

	-- positions
	if scr_type == 'ipad' then
		pos_replay = vec2(94, 174)
		pos_sound = vec2(94, 314)
		pos_music = vec2(94, 454)
		pos_score = vec2(94, 594)
		pos_levels = vec2(244, 174)
		off_levels = vec2(140, 140)
	elseif scr_type == 'iphone' then
		pos_replay = vec2(36, 37)
		pos_sound = vec2(36, 119)
		pos_music = vec2(36, 201)
		pos_score = vec2(36, 283)
		pos_levels = vec2(116, 37)
		off_levels = vec2(80, 82)
	end

	angle = orientation.angle(orientation.current())

	state_sound = keyval.get('state_sound', true)
	if not state_sound then
		mfx.snd_set_volume(0)
	end
	state_music = keyval.get('state_music', true)
end

function close()
end

function enter()
	local l = string.match(csim.level(), 'l(%d+)')
	current_level = tonumber(l) - 1
	touch_current = nil
	touch_up = nil

	touch_released = false

	unlocked = {}
	score = {}
	for i=1,n_levels do
		unlocked[i] = clevels.is_unlocked_n(i)	
		score[i] = clevels.score_n(i)
	end
end

function leave()
end

function split_rect(r, ratio, w)
	local a = rect(r.l, r.t, r.l + ratio * w, r.b)
	local b = rect(a.r, r.t, a.r + (1-ratio) * w, r.b)
	return a, b
end

local filled_anim_scale = 1
if retina then
	filled_anim_scale = 0.5
end

function draw_filled_anim(spr, frame, layer, pos, rot, col, fill)
	local tex, src = sprsheet.get_anim(spr, frame)
	local half_width, half_height = half_icon_w, half_icon_h
	local src_filled, src_empty = split_rect(src, fill, half_width * 2)
	local pf = pos + rotate(vec2(lerp(-half_width*filled_anim_scale, 0, fill), 0), rot)
	local pe = pos + rotate(vec2(lerp(0, half_width*filled_anim_scale,  fill), 0), rot) 
	local empty_col = col * 0.5 

	if fill > 0.01 then
		video.draw_rect_centered(tex, layer, src_filled, pf, rot, filled_anim_scale, col)
	end

	if fill < 0.99 then
		video.draw_rect_centered(tex, layer, src_empty, pe, rot, filled_anim_scale, empty_col)
	end
end

function menu_icon(spr, alt_spr, pos, state, color, rot, frame, fill)
	local c = color
	local hit = false, false

	-- handle touch input
	if c == nil then
		c = color_white

		-- check for hover
		if touch_current and not touch_sliding then
			if math.abs(pos.x - touch_current.pos.x) < half_icon_w then
				if math.abs(pos.y - touch_current.pos.y) < half_icon_h then
					c = color_selected	
				end
			end
		end

		-- check for hit
		if touch_up and not touch_sliding then
			if math.abs(pos.x - touch_up.pos.x) < half_icon_w then
				if math.abs(pos.y - touch_up.pos.y) < half_icon_h then
					hit = true
				end
			end
		end
	end

	local sprite = spr
	if alt_spr ~= nil and state == false then
		sprite = alt_spr
	end

	if not fill then
		if frame == nil then
			sprsheet.draw_centered(sprite, icons_layer, pos, rot, 1.0, c) 
		else
			sprsheet.draw_anim_centered(sprite, frame, icons_layer, pos, rot, 1.0, c)
		end
	else
		draw_filled_anim(sprite, frame, icons_layer, pos, rot, c, fill)	
	end
	

	if state == nil then
		return hit
	else
		-- emulate xor
		return not (state == hit)
	end
end

function animate_menu(t)
	local off, c
	if t then
		t = smoothstep(0, 1, t)
		off = vec2(64 * t, 0)
		c = rgba(1, 1, 1, clamp(0, 1, (1 - math.abs(t))))
	else
		off = vec2(0, 0)
		c = nil 
	end

	return off, c
end

function draw_competitive(t)
	local off, c = animate_menu(t)
	if gamecenter and gamecenter.is_active() then
		if menu_icon(sprs.leaderboards, nil, pos_replay + off, nil, c, angle) then
			-- show game center leaderboard
			gamecenter.show_leaderboard('default', 'all')
		end

		if menu_icon(sprs.achievements, nil, pos_sound + off, nil, c, angle) then
			-- show game center achievements
			gamecenter.show_achievements()
		end
	end
	

	-- twitter
	if os.has_twitter() then
		if menu_icon(sprs.twitter, nil, pos_music + off, nil, c, angle) then
			local s = clevels.total_score()
			local msg = "I'm chilling out playing #nulis, got "..tostring(s)..' points so far!'
			os.tweet(msg, 'http://www.bit.ly/nulis')
		end
	end

	if menu_icon(sprs.back, nil, pos_score + off, nil, c, angle) then
		show_scores = false
		touch_released = false
		score_transition_t = states.time()
	end
end

function draw_options(t)
	local off, c = animate_menu(t)
	if menu_icon(sprs.replay, nil, pos_replay - off, nil, c, angle) then
		if not popped then
			if buy.is_unlocked() or current_level+1 < paid_level or current_level+1 == 50 then
				csim.reset('l'..tostring(current_level+1))
				tutorials.render('...')
				tutorials.start_t = states.time('game')
				states.pop()
			else
				states.push('buy')
			end
			popped = true
		end
	end

	local new_state_sound = menu_icon(sprs.sound, sprs.sound_off, pos_sound - off, state_sound, c, angle) 
	if state_sound ~= new_state_sound then
		if new_state_sound then
			mfx.snd_set_volume(1.0)
		else
			mfx.snd_set_volume(0.0)
		end
		keyval.set('state_sound', new_state_sound)
	end
	state_sound = new_state_sound

	local new_state_music = menu_icon(sprs.music, sprs.music_off, pos_music - off, state_music, c, angle) 
	if state_music ~= new_state_music then
		if new_state_music then
			sound.resume(music_source)
		else
			sound.pause(music_source)
		end
		keyval.set('state_music', new_state_music)
	end
	state_music = new_state_music

	if menu_icon(sprs.score, nil, pos_score - off, nil, c, angle) then
		-- score
		show_scores = true
		touch_released = false
		score_transition_t = states.time()
	end
end

levels_off = 0
levels_off_target = 0

function draw_level_icon(p, col, angle, n, score)
	if n ~= current_level then
		local is_unlocked = unlocked[n+1] 
		if is_unlocked then
			if menu_icon(sprs.levels, nil, p, nil, col, angle, n, score) then
				if not popped then
					if buy.is_unlocked() or n+1 < paid_level or n+1 == 50 then
						csim.reset('l'..tostring(n+1))
						states.pop()
					else
						states.push('buy')
					end
					popped = true
				end
			end
		else
			if not buy.is_unlocked() and unlocked[n] then
				if menu_icon(sprs.plus, nil, p, nil, col, angle) then
					states.push('buy')
				end
			else
				menu_icon(sprs.locked, nil, p, nil, col, angle)
			end
		end
	else
		menu_icon(sprs.levels, nil, p, nil, col, angle, n, score)
		if menu_icon(sprs.resume, nil, p, nil, col, angle) then
			if not popped then
				if buy.is_unlocked() or n+1 < paid_level or n+1 == 50 then
					states.pop()
				else
					states.push('buy')
				end
				popped = true
			end
		end
	end
end

function draw_levels(scores, t)
	if touch_up then
		local up_d = touch_up.pos.y - touch_up.hit_pos.y
		levels_off = levels_off + up_d
	end
	local off = levels_off 

	if not touch_current then
		levels_off = lerp(levels_off, levels_off_target, 0.1)
	end

	local fadeoff = off_levels.y / 2

	local d = 0
	if touch_current then
		d = touch_current.pos.y - touch_current.hit_pos.y
	end
	if touch_sliding and math.abs(d) > 4 then
		off = off + d
		levels_off_target = math.floor(0.5 + off / off_levels.y) * off_levels.y
		levels_off_target = clamp(-off_levels.y*6, 0, levels_off_target)
	end

	for y=1,10 do
		local y_pos = (y-1)*off_levels.y + off
		local alpha = 1

		-- fade out
		if y_pos < -0.1 then
			alpha = 1 + (y_pos / fadeoff)
		elseif y_pos > off_levels.y * 3 then
			alpha = 1.01 - ((y_pos - off_levels.y*3) / fadeoff)
			if alpha > 1 then
				alpha = 1
			end
		end

		if alpha > 0 then
			local col = nil
			if alpha ~= 1 then
				col = rgba(1, 1, 1, alpha)
			end
			for x=1,5 do
				local p = vec2(
					pos_levels.x + (x-1)*off_levels.x,
					pos_levels.y + y_pos
				)

				local level_n = ((y-1)*5) + (x-1)
				
				local s = nil
				if scores then
					s = score[level_n+1]
					if t then
						s = smoothstep(s, 1, t)
					end
				else
					if t then
						s = score[level_n+1]
						s = smoothstep(1, s, t)
					end
				end
				draw_level_icon(p, col, angle, level_n, s)
			end
		end
	end
end

function update_orientation()
	-- update orientation angle
	prev_angle = angle
	local next_orientation, transition_t, transition_len = orientation.did_change() 	
	if next_orientation then
		orientation_anim = true

		orientation_transition_t = transition_t
		orientation_transition_len = transition_len
		orientation_next = next_orientation
	end

	if orientation_anim then
		local ct = time.ms_current() / 1000
		local t = (ct - orientation_transition_t) / orientation_transition_len
		if t > 1.2 then
			orientation_anim = nil
			touch_released = true
		else	
			local curr_orientation = orientation.current()
			local curr_angle = orientation.angle(curr_orientation)
			local next_angle = orientation.angle(orientation_next)
			
			-- make sure to rotate along a shortest path between two angles
			if math.abs(curr_angle - next_angle) > math.pi then
				if math.abs(curr_angle - (next_angle+math.pi*2)) < math.pi then
					next_angle = next_angle+math.pi*2
				end
				if math.abs(curr_angle - (next_angle-math.pi*2)) < math.pi then
					next_angle = next_angle-math.pi*2
				end
			end
			angle = smoothstep(curr_angle, next_angle, t)
		end
		return true
	else
		angle = orientation.angle(orientation.current())
		return false
	end
end

function update()
	if key.up(key.quit) then
		states.pop()
	end

	if not update_orientation() then
		-- update touch state
		if touch_released and touch.count() == 1 then
			local t = touch.get(0)
			if touch_current == nil then
				touch_sliding = false
			end
			touch_current = t
			touch_up = nil

			if not touch_sliding and length_sq(t.pos - t.hit_pos) > 8*8 then
				touch_sliding = true
			end
		end

		if touch.count() == 0 then
			touch_released = true
			if touch_current ~= nil then
				touch_up = touch_current
			else
				touch_up = nil
			end
			touch_current = nil
		end
	end

	return true
end

function render(t)
	popped = false
	local a = 1 - math.abs(t)
	local c = rgba(0, 0, 0, a)
	
	tutorials.render(csim.level())
	csim.render()

	sprsheet.draw(sprs.background, background_layer, rect(0, 0, scr_size.x, scr_size.y), c)

	local t = states.time()
	local score_t = nil
	if score_transition_t and t - score_transition_t < 0.3 then
		score_t = 1 - (t - score_transition_t) / 0.3
	end

	draw_levels(show_scores, score_t)

	if not show_scores then
		draw_options(score_t)
		if score_t then
			draw_competitive(1 - score_t)
		end
	else
		draw_competitive(score_t)
		if score_t then
			draw_options(1 - score_t)
		end
	end

	return true
end

