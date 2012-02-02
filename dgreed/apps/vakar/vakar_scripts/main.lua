
pre = 'vakar_assets/'

scr_size = vec2(512, 384)

require 'game'

-- is rect a completely inside rect b ?
function is_rect_inside(a, b)
	if a.l >= b.l and a.r <= b.r then
		if a.t >= b.t and a.b <= b.b then
			return true
		end
	end
	return false
end

function game_init()
	local scale = 2
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'vakar', false)
	sound.init()

	mfx.init(pre..'effects.mml')
	sprsheet.init(pre..'sprsheet.mml')
	mus = sound.load_stream(pre..'Cool Vibes.ogg')
	sound.set_volume(mus, 0.7)
	sound.play(mus, true)

	fnt = font.load(pre..'lucida_grande_30px.bft', 0.5)

	particles.init('vakar_assets/', 6)

	states.register('game', game)
	states.push('game')
end

function game_close()
	particles.close()
	font.free(fnt)
	sound.free(mus)
	sprsheet.close()
	mfx.close()
	sound.close()
	video.close()
end
