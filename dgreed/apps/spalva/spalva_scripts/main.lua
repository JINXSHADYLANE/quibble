
asset_dir = 'spalva_assets/'

tile_size = 32
scr_size = vec2(480, 320)
scr_rect = rect(0, 0, 480, 320)
screen_rect = rect(0, 0, scr_size.x, scr_size.y)

tile_layer = 2
character_layer = 3

bg_color = rgba(148/255, 74/255, 38/255)

local game = require('game')

function game_init()
	local scale = 2
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'spalva', false)
	sound.init()

	--mfx.init(asset_dir..'effects.mml')
	sprsheet.init(asset_dir..'spritesheet.mml')
	anim.init(asset_dir..'animations.mml')
	--mus = sound.load_stream(asset_dir..'aftermath.ogg')
	--fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)
	--sound.play(mus, true)

	states.register('game', game)
	states.push('game')
end

function game_close()
--	font.free(fnt)
--	sound.free(mus)
	anim.close()
	sprsheet.close()
--	mfx.close()
	sound.close()
	video.close()
end