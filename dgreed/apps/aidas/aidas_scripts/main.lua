
pre = 'aidas_assets/'

scr_size = vec2(480, 320)
screen_rect = rect(0, 0, scr_size.x, scr_size.y)

local game = require('game')

function game_init()
	local scale = 2
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'aidas', false)
	sound.init()

	--mfx.init(pre..'effects.mml')
	sprsheet.init(pre..'sprsheet.mml')
	--mus = sound.load_stream(pre..'music.ogg')
	--sound.set_volume(mus, 0.7)
	--sound.play(mus, true)

	--fnt = font.load(pre..'lucida_grande_30px.bft', 0.5)

	--particles.init('aidas_assets/', 6)

	states.register('game', game)
	states.push('game')
end

function game_close()
	--particles.close()
	--font.free(fnt)
	--sound.free(mus)
	sprsheet.close()
	--mfx.close()
	sound.close()
	video.close()
end
