
asset_dir = 'gruodis_assets/'

scr_size = vec2(200, 150)
scr_rect = rect(0, 0, scr_size.x, scr_size.y)

local game = require('game')

function game_init()
	local scale = 4
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'gruodis', false)
	sound.init()

--	mfx.init(asset_dir..'effects.mml')
--	sprsheet.init(asset_dir..'sprsheet.mml')
--	mus = sound.load_stream(asset_dir..'aftermath.ogg')
--	fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)
--	sound.play(mus, true)

	states.register('game', game)
	states.push('game')
	states.transition_len = 0.5
end

function game_close()
--	font.free(fnt)
--	sound.free(mus)
--	sprsheet.close()
--	mfx.close()
	sound.close()
	video.close()
end
