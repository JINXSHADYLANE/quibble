local hud = {}

local hud_layer = 14
local paper_layer = 15
local text_color = rgba(0.270, 0.270, 0.270, 1)
local title_crossfade_len = 0.5

function hud.init()
	hud.paper_spr = sprsheet.get_handle('paper')
	hud.tile = sprsheet.get_handle('menu_block')
	hud.tile_small = sprsheet.get_handle('menu_block')
	hud.shadow = sprsheet.get_handle('m_shadow')

	video.set_blendmode(paper_layer, 'multiply')

	hud.title = ''
	hud.last_title = ''
end

function hud.close()
end

function hud.set_title(str)
	hud.last_title = hud.title
	hud.title = str
	hud.title_t = time.s()
end

function hud.render()
	-- paper
	sprsheet.draw(hud.paper_spr, paper_layer, vec2(0, 0))

	-- bottom menu tiles
	for i=1,5 do
		local p = vec2((i-1)*64, scr_size.y - 64)
		sprsheet.draw(hud.tile, hud_layer, p)
	end

	-- bottom shadow
	sprsheet.draw(hud.shadow, hud_layer, rect(0, scr_size.y - 74, scr_size.x, scr_size.y - 64))

	-- top background
	sprsheet.draw(hud.tile_small, hud_layer, rect(0, 0, scr_size.x, 32))

	-- top shadow
	sprsheet.draw(hud.shadow, hud_layer, rect(0, 32 + 10, scr_size.x, 32))

	local t = time.s()

	-- title
	if hud.title_t and t - hud.title_t < title_crossfade_len then
		local tt = clamp(0, 1, (t - hud.title_t) / title_crossfade_len)
		text_color.a = tt 
		video.draw_text_centered(fnt, hud_layer, hud.title, vec2(scr_size.x / 2, 24), 1.0, text_color)
		text_color.a = 1 - tt
		video.draw_text_centered(fnt, hud_layer, hud.last_title, vec2(scr_size.x / 2, 24), 1.0, text_color)
		text_color.a = 1
	else	
		video.draw_text_centered(fnt, hud_layer, hud.title, vec2(scr_size.x / 2, 24), 1.0, text_color)
	end
end

return hud
