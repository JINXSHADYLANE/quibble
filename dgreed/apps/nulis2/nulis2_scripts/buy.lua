module(..., package.seeall)

require 'menu'

fadein_len = 3

background_layer = 6
text_layer = 7

color_white = rgba(1, 1, 1, 1)

sprs = {}

function is_unlocked()
	return keyval.get('unlocked', false)
end

function init()
	sprs.background = sprsheet.get_handle('background')
	sprs.text = sprsheet.get_handle('hint4')

	sprs.line_tex, sprs.line = sprsheet.get('stripe')
end

function close()
end

function enter()
	buy_enter_t = time.s()
end

function leave()
end

function update()
	if scr_type == 'ipad' and touch.count() == 5 then
		states.pop()
	end

	if scr_type == 'iphone' and touch.count() == 3 then
		states.pop()
	end

	menu.update_orientation()

	return true
end

local center = vec2(scr_size.x / 2, scr_size.y / 2)

local buy_text = 'buy the game for $0.99'
if scr_type == 'ipad' then
	buy_text = 'buy the rest of the game for $0.99'
end

local text_hack = 0
local line1_off = 12 
local line2_off = 42
if scr_type == 'ipad' then
	line1_off = 27
	line2_off = 77
	text_hack = 2
end
local off = (line1_off + line2_off) / 2 

local color_white = rgba(1, 1, 1, 1)

function render(t)
	local tt = clamp(0, 1, (time.s() - buy_enter_t) / fadein_len)
	local col = rgba(1, 1, 1, tt)

	sprsheet.draw(sprs.background, background_layer, rect(0, 0, scr_size.x, scr_size.y))

	sprsheet.draw_centered(sprs.text, text_layer, 
		center + rotate(vec2(0, -off), menu.angle),
		menu.angle, 1.0, col
	)

	local w, h = font.size(fnt, buy_text)

	video.draw_text_rotated(fnt, text_layer, buy_text, 
		center + rotate(vec2(0, off + h + text_hack), menu.angle),
		menu.angle, 1.0, col
	)

	local line_p = center + rotate(vec2(0, line1_off), menu.angle)

	video.draw_rect(sprs.line_tex, text_layer, sprs.line,
		rect(line_p.x - w/2, line_p.y-1, line_p.x + w/2, line_p.y+1),
		menu.angle, col
	)

	local line_p = center + rotate(vec2(0, line2_off), menu.angle)

	video.draw_rect(sprs.line_tex, text_layer, sprs.line,
		rect(line_p.x - w/2, line_p.y-1, line_p.x + w/2, line_p.y+1),
		menu.angle, col
	)

	return true	
end
