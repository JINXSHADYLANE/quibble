
pre = 'kovas_assets/'

scr_size = vec2(1024, 768)
screen_rect = rect(0, 0, scr_size.x, scr_size.y)

local game = require('game')

function game_init()
	video.init(scr_size.x, scr_size.y, 'kovas')
	sound.init()

--	mfx.init(pre..'effects.mml')
	sprsheet.init(pre..'sprsheet.mml')
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
