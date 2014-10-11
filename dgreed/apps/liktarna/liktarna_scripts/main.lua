
asset_dir = 'liktarna_assets/'

scr_size = vec2(800, 600)
scr_rect = rect(0, 0, scr_size.x, scr_size.y)
fnt = asset_dir..'AlegreyaSans.ttf'
text_color = rgba(1, 1, 1)

local math_floor = math.floor
function feql(a, b)
	local d = a - b
	return d*d < 0.0000001
end

function eql_pos(a, b)
	return feql(a.x, b.x) and feql(a.y, b.y)
end

local game = require('game')
local room = require('room')
local title = require('title')
local epilogue = require('epilogue')

function game_init()
	local scale = 1
	local real_size = scr_size * scale
	video.init_ex(real_size.x, real_size.y, scr_size.x, scr_size.y, 'Liktarna', false)
	sound.init()

--	particles.init(asset_dir, 1)
	mfx.init(asset_dir..'effects.mml')
	sprsheet.init(asset_dir..'sprsheet.mml')
	vfont.select(fnt, 40)
--	mus = sound.load_stream(asset_dir..'agoodday.ogg')
--	sound.play(mus, true)

	states.prerender_callback(function()
		sound.update()
		mfx.update()
--		particles.update()
--		particles.draw()
	end)

	states.transition_len = 0.6
	states.register('game', game)
	states.register('room', room)
	states.register('title', title)
	states.register('epilogue', epilogue)
	
	--states.push('title')
	states.push('game')
end

function game_close()
	--sound.free(mus)
	vfont.close()
	sprsheet.close()
	mfx.close()
--	particles.close()
	sound.close()
	video.close()
end
