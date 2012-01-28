
pre = 'vakar_assets/'

scr_size = vec2(512, 384)

require 'game'

function game_init()
	local scale = 2
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'vakar', false)
	sound.init()

	--mfx.init(pre..'effects.mml')
	sprsheet.init(pre..'sprsheet.mml')
	--mus = sound.load_stream(pre..'Lightless Dawn.ogg')

	states.register('game', game)
	states.push('game')
end

function game_close()
	--sound.free(mus)
	sprsheet.close()
	--mfx.close()
	sound.close()
	video.close()
end
