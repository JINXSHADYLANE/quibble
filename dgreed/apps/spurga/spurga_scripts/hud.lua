local hud = {}

local hud_layer = 14
local paper_layer = 15
local text_color = rgba(0.270, 0.270, 0.270, 1)
local hud_crossfade_len = 0.2

local pressed_button_color = rgba(1, 1, 1, 0.8)

local half_button_width = 32
local half_button_height = 32

hud.music = 0
hud.sound = 1
hud.help = 2
hud.back = 3
hud.replay = 4
hud.hint = 5
hud.play = 6

hud.music_state = true
hud.sound_state = true

function hud.init()
	hud.spr_paper = sprsheet.get_handle('paper')
	hud.spr_tile = sprsheet.get_handle('menu_block')
	hud.spr_tile_small = sprsheet.get_handle('menu_block')
	hud.spr_shadow = sprsheet.get_handle('m_shadow')
	hud.spr_music = sprsheet.get_handle('music')
	hud.spr_music_off = sprsheet.get_handle('music_off')
	hud.spr_sound = sprsheet.get_handle('sound')
	hud.spr_sound_off = sprsheet.get_handle('sound_off')
	hud.spr_help = sprsheet.get_handle('query')
	hud.spr_back = sprsheet.get_handle('quit')
	hud.spr_replay = sprsheet.get_handle('reset')
	hud.spr_hint = sprsheet.get_handle('hint')
	hud.spr_play = sprsheet.get_handle('play')

	video.set_blendmode(paper_layer, 'multiply')

	hud.title = ''
	hud.last_title = ''

	hud.buttons = {}
	hud.last_buttons = {}
end

function hud.close()
end

function hud.set_title(str)
	hud.last_title = hud.title
	hud.title = str
	hud.title_t = time.s()
end

function hud.set_buttons(buttons)
	hud.last_buttons = hud.buttons
	hud.buttons = buttons
	hud.buttons_t = time.s()
end

function hud.update()
	hud.touch_up = nil

	if touch.count() == 1 then
		local t = touch.get(0)
		if length_sq(t.pos - t.hit_pos) < 5*5 then
			hud.touch = t 
		else
			hud.touch = nil
		end
	else
		hud.touch_up = hud.touch
		hud.touch = nil
	end
end

local function render_stateless_button(spr, pos, r, col)
	if col then
		sprsheet.draw_centered(spr, hud_layer, pos, col)
	else
		if hud.touch and rect_point_collision(r, hud.touch.pos) then
			sprsheet.draw_centered(spr, hud_layer, pos, pressed_button_color)
		else
			sprsheet.draw_centered(spr, hud_layer, pos)
		end

		if hud.touch_up and rect_point_collision(r, hud.touch_up.pos) then
			return true
		end
	end

	return false
end

local function render_pressure_button(spr, pos, r, col)
	if col then
		sprsheet.draw_centered(spr, hud_layer, pos, col)
	else
		if hud.touch and rect_point_collision(r, hud.touch.pos) then
			sprsheet.draw_centered(spr, hud_layer, pos, pressed_button_color)
			return true
		else
			sprsheet.draw_centered(spr, hud_layer, pos)
		end
	end
	return false
end

local function render_stateful_button(spr_on, spr_off, pos, state, r, col)
	local spr
	if state then
		spr = spr_on	
	else
		spr = spr_off
	end

	local result = state

	if col then
		sprsheet.draw_centered(spr, hud_layer, pos, col)
	else
		if hud.touch and rect_point_collision(r, hud.touch.pos) then
			sprsheet.draw_centered(spr, hud_layer, pos, pressed_button_color)
		else
			sprsheet.draw_centered(spr, hud_layer, pos)
		end

		if hud.touch_up and rect_point_collision(r, hud.touch_up.pos) then
			result = not result
		end
	end

	return result
end


local function render_button(btn, pos, col)
	local r = rect(
		pos.x - half_button_width, pos.y - half_button_height,
		pos.x + half_button_width, pos.y + half_button_height
	)

	if btn == hud.music then
		hud.music_state = render_stateful_button(
			hud.spr_music, hud.spr_music_off, pos, 
			hud.music_state, r, col
		)
	elseif btn == hud.sound then
		hud.sound_state = render_stateful_button(
			hud.spr_sound, hud.spr_sound_off, pos,
			hud.sound_state, r, col
		)
	elseif btn == hud.help then
		if render_stateless_button(hud.spr_help, pos, r, col) then
			hud.touch_up = nil
			if hud.delegate and hud.delegate.help then
				hud.delegate.help()
			end
		end
	elseif btn == hud.back then
		if render_stateless_button(hud.spr_back, pos, r, col) then
			states.pop()
			hud.touch_up = nil

			if hud.delegate and hud.delegate.back then
				hud.delegate.back()
			end
		end
	elseif btn == hud.replay then
		if render_stateless_button(hud.spr_replay, pos, r, col) then
			hud.touch_up = nil
			if hud.delegate and hud.delegate.replay then
				hud.delegate.replay()
				
			end
		end
	elseif btn == hud.hint then
		if render_pressure_button(hud.spr_hint, pos, r, col) then
			if hud.delegate and hud.delegate.hint then
				hud.delegate.hint()
			end
		end
	elseif btn == hud.play then
		if render_stateless_button(hud.spr_play, pos, r, col) then
			hud.touch_up = nil
			if hud.delegate and hud.delegate.play then
				hud.delegate.play()
			end
		end
	end
end

function hud.render()
	-- paper
	sprsheet.draw(hud.spr_paper, paper_layer, vec2(0, 0))

	-- bottom menu tiles
	for i=1,5 do
		local p = vec2((i-1)*64, scr_size.y - 64)
		sprsheet.draw(hud.spr_tile, hud_layer, p)
	end

	-- bottom shadow
	sprsheet.draw(hud.spr_shadow, hud_layer, rect(0, scr_size.y - 74, scr_size.x, scr_size.y - 64))

	-- top background
	sprsheet.draw(hud.spr_tile_small, hud_layer, rect(0, 0, scr_size.x, 32))

	-- top shadow
	sprsheet.draw(hud.spr_shadow, hud_layer, rect(0, 32 + 10, scr_size.x, 32))

	local t = time.s()

	-- title
	if hud.title_t and t - hud.title_t < hud_crossfade_len then
		local tt = clamp(0, 1, (t - hud.title_t) / hud_crossfade_len)
		text_color.a = tt 
		video.draw_text_centered(fnt, hud_layer, hud.title, vec2(scr_size.x / 2, 24), 1.0, text_color)
		text_color.a = 1 - tt
		video.draw_text_centered(fnt, hud_layer, hud.last_title, vec2(scr_size.x / 2, 24), 1.0, text_color)
		text_color.a = 1
	else	
		video.draw_text_centered(fnt, hud_layer, hud.title, vec2(scr_size.x / 2, 24), 1.0, text_color)
	end

	-- buttons
	if hud.buttons_t and t - hud.buttons_t < hud_crossfade_len then
		local tt = clamp(0, 1, (t - hud.buttons_t) / hud_crossfade_len)
		local c_last = rgba(1, 1, 1, 1 - tt)
		local c_current = rgba(1, 1, 1, tt)
		
		for i=1,5 do
			local p = vec2((i-1)*64+32, scr_size.y - 64 + 32)
			if hud.buttons[i] == hud.last_buttons[i] then
				render_button(hud.buttons[i], p, rgba(1, 1, 1, 1))
			else
				if hud.last_buttons[i] then
					render_button(hud.last_buttons[i], p, c_last)
				end
				if hud.buttons[i] then
					render_button(hud.buttons[i], p, c_current)
				end
			end
		end
	else
		for i=1,5 do
			local p = vec2((i-1)*64+32, scr_size.y - 64 + 32)
			render_button(hud.buttons[i], p)
		end
	end
end

return hud
