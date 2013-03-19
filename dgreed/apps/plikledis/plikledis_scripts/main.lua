require 'game'

pre = 'plikledis_assets/'

scr_size = vec2(240, 160)
fnt = nil

function idx2d(x,y,w)
	return y*w + x
end

function rev_idx2d(idx, w)
	local y = math.floor(idx / w)
	local x = idx - y*w
	return x, y
end

function game_init()
	local scale = 2
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'plikledis', false)
	sound.init()
	mfx.init(pre..'effects.mml')

	sprsheet.init(pre..'sprsheet.mml')

	fnt = font.load(pre..'openbaskerville_32px.bft', 0.5, pre) 

	mus = sound.load_stream(pre..'Lightless Dawn.ogg')

	states.register('game', game)
	states.push('game')
end

function game_close()
	font.free(fnt)
	sound.free(mus)
	sprsheet.close()
	mfx.close()
	sound.close()
	video.close()
end
