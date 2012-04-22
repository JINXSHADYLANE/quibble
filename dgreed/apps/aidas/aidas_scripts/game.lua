local objs = require('objs')

local game = {}

local levels = {
	'first.btm',
	'goal.btm',
	'obstacles.btm',
	'path.btm',
	'fall.btm',
	'control.btm',
	'push.btm',
	'chasm.btm'
}

local texts = {
	'a tiny world of my own',
	'I move it wherever I want to',
	'I overcome obstacles along the way',
	'and I upen up new paths',
	'fall',
	'control',
	'push',
	'chasm',
	'end'
}

local text_len = 4
local fadeout_len = 5

local level_index = 1
local text_t = -100 
local fadeout_t = -100

local function load_level(name)
	if level then
		tilemap.free(level)
	end

	level = tilemap.load(pre..name)
	objs.reset(level)

	tilemap.set_camera(level, objs.player.pos)
end

-- game state --

function game.init()
	load_level(levels[level_index])
	text_t = time.s() + 4
end

function game.close()
	if level then
		tilemap.free(level)
	end
end

function game.enter()
end

function game.leave()
end

function game.update()
	if key.up(key.quit) then
		states.pop()
	end

	if char.up('r') then
		load_level(levels[level_index])
	end

	sound.update()
	--mfx.update()

	if not game.is_fading() then
		if objs.update() then
			mfx.trigger('win')
			fadeout_t = time.s() + fadeout_len / 2
			loaded = false
		end
	end

	return true
end

function game.is_fading()
	local t = time.s()
	local d = (t - (fadeout_t - fadeout_len/2)) / fadeout_len
	if not loaded and d > 0.5 and d < 1 then
		level_index = level_index + 1
		text_t = time.s() + text_len / 2
		loaded = true
		if level_index <= #levels then
			load_level(levels[level_index])
		else
			gameover = true
		end
	end
	return gameover or (d >= 0 and d <= 1)
end

function game.render_fade()
	local t = time.s()
	local d = (t - (fadeout_t - fadeout_len/2)) / fadeout_len
	if gameover or (d >= 0 and d <= 1) then
		local a = math.sin(d * math.pi)
		if gameover then
			a = 1
		end
		local c = rgba(0, 0, 0, a)
		sprsheet.draw('empty', 4, screen_rect, c)
	end
end

function game.render_text()
	local t = time.s()
	if gameover then
		t = math.min(t, text_t)
	end
	local d = math.abs(t - text_t)
	if gameover or (d < text_len / 2) then
		local a = 1 - d / (text_len / 2)
		local col = rgba(1, 1, 1, math.min(1, a*2))
		video.draw_text_centered(fnt, 5, texts[level_index], scr_size/2 - vec2(0, 120), col)
	end
end

function game.render(t)

	sprsheet.draw('back', 0, screen_rect)
	
	objs.render()

	if level then
		tilemap.render(level, screen_rect)
	end

	game.render_text()
	game.render_fade()

	return true
end

return game

