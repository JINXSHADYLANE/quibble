local hud = {}

local hud_layer = 14
local paper_layer = 15
local text_color = rgba(0.270, 0.270, 0.270, 1)
local hud_crossfade_len = 0.2

local pressed_button_color = rgba(1, 1, 1, 0.8)

local half_button_width = 32
local half_button_height = 32

local sound_mute
local music_mute

hud.music = 0
hud.sound = 1
hud.help = 2
hud.back = 3
hud.replay = 4
hud.hint = 5
hud.play = 6
hud.play_next = 7

function hud.init()
	hud.spr_border = sprsheet.get_handle('border')
	hud.spr_empty = sprsheet.get_handle('empty')
	hud.spr_paper = sprsheet.get_handle('paper')
	hud.spr_tile = sprsheet.get_handle('menu_block')
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
	hud.spr_next = sprsheet.get_handle('next')
	hud.spr_shadow_grid = {
		sprsheet.get_handle('shadow_t'),
		sprsheet.get_handle('shadow_b'),
		sprsheet.get_handle('shadow_l'),
		sprsheet.get_handle('shadow_r'),
		sprsheet.get_handle('shadow_tl'),
		sprsheet.get_handle('shadow_tr'),
		sprsheet.get_handle('shadow_bl'),
		sprsheet.get_handle('shadow_br')
	}

	video.set_blendmode(paper_layer, 'multiply')

	hud.title = ''
	hud.last_title = ''

	hud.buttons = {}
	hud.last_buttons = {}

	hud.score = nil
	hud.last_score = nil

	if ipad then
		hud.render = hud.render_ipad
	else
		hud.render = hud.render_iphone
	end

	sound_mute = keyval.get('sound_mute', false)
	music_mute = keyval.get('music_mute', false)

	if sound_mute then
		mfx.snd_set_volume(0)
	end

	hud.sound_state = not sound_mute
	hud.music_state = not music_mute
end

function hud.close()
end

function hud.set_title(str)
	if str ~= hud.title then
		hud.last_title = hud.title
		hud.title = str
		hud.title_t = time.s()
	end
end

function hud.set_buttons(buttons)
	hud.last_buttons = hud.buttons
	hud.buttons = buttons
	hud.buttons_t = time.s()
end

function hud.set_score(score)
	if score ~= hud.score then
		hud.last_score = hud.score
		hud.score = score
		hud.score_t = time.s()
	end
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
			mfx.trigger('click')
		end
	end

	return result
end

local function render_button(btn, pos, r, col)
	if btn == hud.music then
		hud.music_state = render_stateful_button(
			hud.spr_music, hud.spr_music_off, pos, 
			hud.music_state, r, col
		)
		music_mute = not hud.music_state
		keyval.set('music_mute', music_mute)
	elseif btn == hud.sound then
		hud.sound_state = render_stateful_button(
			hud.spr_sound, hud.spr_sound_off, pos,
			hud.sound_state, r, col
		)
		sound_mute = not hud.sound_state
		keyval.set('sound_mute', sound_mute)
		if sound_mute then
			mfx.snd_set_volume(0)
		else
			mfx.snd_set_volume(1)
		end
	elseif btn == hud.help then
		if render_stateless_button(hud.spr_help, pos, r, col) then
			hud.touch_up = nil
			if hud.delegate and hud.delegate.help then
				hud.delegate.help()
			end
		end
	elseif btn == hud.back then
		if render_stateless_button(hud.spr_back, pos, r, col) then
			mfx.trigger('back')
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
	elseif btn == hud.play or btn == hud.play_next then
		local spr = hud.spr_play
		if btn == hud.play_next then
			spr = hud.spr_next
		end
		if render_stateless_button(spr, pos, r, col) then
			hud.touch_up = nil
			if hud.delegate and hud.delegate.play then
				hud.delegate.play()
			end
		end
	end
end

local vec2_zero = vec2(0, 0)
local color_white = rgba(1, 1, 1, 1)

local bottom_shadow_dest = rect(0, scr_size.y - 74, scr_size.x, scr_size.y - 64)
local top_background_dest = rect(0, 0, scr_size.x, 32)
local top_shadow_dest = rect(0, 32 + 10, scr_size.x, 32)

local title_pos
if ipad then
	title_pos = vec2(scr_size.x / 2, 42) 
else
	title_pos = vec2(scr_size.x / 2, 24)
end

local button_pos = {nil, nil, nil, nil, nil}
local button_rect = {nil, nil, nil, nil, nil}
if ipad then
	for i=1,5 do
		local pos = vec2(math.floor((i-1)*(768/5) + 64), scr_size.y - 128 + 64)
		button_pos[i] = pos
		button_rect[i] = rect(
			pos.x - half_button_width, pos.y - half_button_height,
			pos.x + half_button_width, pos.y + half_button_height
		)
	end
else
	for i=1,5 do
		local pos = vec2((i-1)*64+32, scr_size.y - 64 + 32)
		button_pos[i] = pos
		button_rect[i] = rect(
			pos.x - half_button_width, pos.y - half_button_height,
			pos.x + half_button_width, pos.y + half_button_height
		)
	end
end


local tile_pos = {nil, nil, nil, nil, nil}
if not ipad then
	for i=1,5 do
		tile_pos[i] = vec2((i-1)*64, scr_size.y - 64)
	end
end

local function render_buttons(t)
	if hud.buttons_t and t - hud.buttons_t < hud_crossfade_len then
		local tt = clamp(0, 1, (t - hud.buttons_t) / hud_crossfade_len)
		local c_last = rgba(1, 1, 1, 1 - tt)
		local c_current = rgba(1, 1, 1, tt)
		
		for i=1,5 do
			local p = button_pos[i] 
			local r = button_rect[i]
			if hud.buttons[i] == hud.last_buttons[i] then
				render_button(hud.buttons[i], p, r, color_white)
			else
				if hud.last_buttons[i] then
					render_button(hud.last_buttons[i], p, r, c_last)
				end
				if hud.buttons[i] then
					render_button(hud.buttons[i], p, r, c_current)
				end
			end
		end
	else
		for i=1,5 do
			render_button(hud.buttons[i], button_pos[i], button_rect[i])
		end
	end
end

local function render_title(t)
	if hud.title_t and t - hud.title_t < hud_crossfade_len then
		local tt = clamp(0, 1, (t - hud.title_t) / hud_crossfade_len)
		text_color.a = tt 
		video.draw_text_centered(fnt, hud_layer, hud.title, title_pos, 1.0, text_color)
		text_color.a = 1 - tt
		video.draw_text_centered(fnt, hud_layer, hud.last_title, title_pos, 1.0, text_color)
		text_color.a = 1
	else	
		video.draw_text_centered(fnt, hud_layer, hud.title, title_pos, 1.0, text_color)
	end
end

function hud.render_iphone()
	-- paper
	sprsheet.draw(hud.spr_paper, paper_layer, vec2_zero)

	-- bottom menu tiles
	for i=1,5 do
		sprsheet.draw(hud.spr_tile, hud_layer, tile_pos[i])
	end

	-- bottom shadow
	sprsheet.draw(hud.spr_shadow, hud_layer, bottom_shadow_dest)

	-- top background
	sprsheet.draw(hud.spr_empty, hud_layer, top_background_dest)

	-- top shadow
	sprsheet.draw(hud.spr_shadow, hud_layer, top_shadow_dest)

	local t = time.s()

	render_title(t)
	render_buttons(t)
end

local board = rect(
	grid_pos.x - 2.5 * 128, grid_pos.y - 3 * 128,
	grid_pos.x + 2.5 * 128, grid_pos.y + 3 * 128
)

local border_back_left_dest = rect(board.l - 128, board.t, board.l, board.b)
local border_back_right_dest = rect(board.r, board.t, board.r + 128, board.b) 
local border_back_top_dest = rect(board.l, board.t - 128, board.r, board.t)
local border_back_bottom_dest = rect(board.l, board.b, board.r, board.b + 128)

local border_left_dest = rect(board.l - 2, board.t, board.l, board.b)
local border_right_dest = rect(board.r, board.t, board.r + 2, board.b)
local border_top_dest = rect(board.l - 2, board.t - 2, board.r + 2, board.t)
local border_bottom_dest = rect(board.l - 2, board.b, board.r + 2, board.b + 2)

local shadow_dest = {
	rect(board.l + 62, board.t, board.r - 62, board.t + 62),
	rect(board.l + 62, board.b - 62, board.r - 62, board.b),
	rect(board.l, board.t + 62, board.l + 62, board.b - 62),
	rect(board.r - 62, board.t + 62, board.r, board.b - 62),
	vec2(board.l, board.t),
	vec2(board.r - 62, board.t),
	vec2(board.l, board.b - 62),
	vec2(board.r - 62, board.b - 62)
}

function hud.render_ipad()

	local pre_paper = paper_layer-1
	
	-- border background
	sprsheet.draw(hud.spr_empty, pre_paper, border_back_left_dest)
	sprsheet.draw(hud.spr_empty, pre_paper, border_back_right_dest)
	sprsheet.draw(hud.spr_empty, pre_paper, border_back_top_dest)
	sprsheet.draw(hud.spr_empty, pre_paper, border_back_bottom_dest)

	-- border
	sprsheet.draw(hud.spr_border, pre_paper, border_left_dest)
	sprsheet.draw(hud.spr_border, pre_paper, border_right_dest)
	sprsheet.draw(hud.spr_border, pre_paper, border_top_dest)
	sprsheet.draw(hud.spr_border, pre_paper, border_bottom_dest)

	-- shadow
	for i,s in ipairs(shadow_dest) do
		sprsheet.draw(hud.spr_shadow_grid[i], pre_paper, s)
	end

	-- paper
	sprsheet.draw(hud.spr_paper, paper_layer, vec2_zero)

	local t = time.s()

	render_title(t)
	render_buttons(t)
end

return hud

