
asset_dir = 'urvas_assets/'

scr_size = vec2(640, 400)
scr_rect = rect(0, 0, 640, 400)
screen_rect = rect(0, 0, scr_size.x, scr_size.y)

local textmode = require('textmode')
local intro = require('intro')
local game = require('game')

function obj_sqr_distance(a, b)
	return (a.pos.x-b.pos.x)^2 + (a.pos.y-b.pos.y)^2	
end

function pt_to_seg_distance(s, e, pt)
	local l2 = length_sq(s - e)
	if l2 == 0 then
		return length(s - pt)
	end
	local t = dot(pt - s, e - s) / l2
	if t < 0 then
		return length(s - pt)
	elseif t > 1 then
		return length(e - pt)
	else
		local proj = s + (e - s) * t
		return length(pt - proj)
	end
end

function round(x)
	if x > 0 then
		return math.floor(x + 0.5)
	else
		return math.ceil(x - 0.5)
	end
end

function game_init()
	local scale = 1
	local real_size = scr_size * scale
	video.init_ex(real_size.x, real_size.y, scr_size.x, scr_size.y, 'urvas', false)
	sound.init()

--	mfx.init(asset_dir..'effects.mml')
--	sprsheet.init(asset_dir..'sprsheet.mml')
--	mus = sound.load_stream(asset_dir..'aftermath.ogg')
--	fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)
--	sound.play(mus, true)

	textmode.init()

	states.register('intro', intro)
	states.register('game', game)
	states.push('intro')

	states.transition_len = 0.5
end

function game_close()
--	font.free(fnt)
--	sound.free(mus)
--	sprsheet.close()
--	mfx.close()
	textmode.close()
	sound.close()
	video.close()
end
