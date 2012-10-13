
asset_dir = 'tiltas_assets/'

scr_size = vec2(480, 320)
screen_rect = rect(0, 0, scr_size.x, scr_size.y)

glow_layer = 1
tile_layer = 2
ghost_layer = 3

local game = require('game')

function rand_vec2(maxlen)
	return vec2(
		rand.float(-maxlen, maxlen),
		rand.float(-maxlen, maxlen)
	)
end

function world2screen(p)
	return vec2(
		scr_size.x / 2 + p.x * 16,
		scr_size.y / 2 + p.y * 16
	)
end

function game_init()
	local scale = 2
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'tiltas', false)
	sound.init()

--	mfx.init(pre..'effects.mml')
	sprsheet.init(asset_dir..'sprsheet.mml')
--	mus = sound.load_stream(pre..'aidas.ogg')
--	sound.play(mus, true)

--	fnt = font.load(pre..'gentium_60px.bft', 0.5, pre)

	states.register('game', game)
	states.push('game')
end

function game_close()
--	font.free(fnt)
--	sound.free(mus)
	sprsheet.close()
--	mfx.close()
	sound.close()
	video.close()
end
