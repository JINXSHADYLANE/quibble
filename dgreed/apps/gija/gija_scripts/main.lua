
asset_dir = 'gija_assets/'

scr_size = vec2(1024, 576)
scr_rect = rect(0, 0, scr_size.x, scr_size.y)

local game = require('game')

function rand_vec2(maxlen)
	return vec2(
		rand.float(-maxlen, maxlen),
		rand.float(-maxlen, maxlen)
	)
end

function game_init()
	local scale = 1
	local real_size = scr_size * scale
	video.init_ex(real_size.x, real_size.y, scr_size.x, scr_size.y, 'tiltas', false)
	sound.init()

	--mfx.init(asset_dir..'effects.mml')
	--sprsheet.init(asset_dir..'sprsheet.mml')
	--mus = sound.load_stream(asset_dir..'aftermath.ogg')
	--fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)
	--sound.play(mus, true)

	states.register('game', game)
	states.push('game')
end

function game_close()
	--font.free(fnt)
	--sound.free(mus)
	--sprsheet.close()
	--mfx.close()
	sound.close()
	video.close()
end
