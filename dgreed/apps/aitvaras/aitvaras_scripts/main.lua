
scr_size = vec2(1024, 768)
pre = 'aitvaras_assets/'
fnt = nil

local game = require('game')

function game_init()
	video.init(scr_size.x, scr_size.y, 'aitvaras')
	fnt = font.load(pre..'lucida_grande_60px.bft')

	sprsheet.init(pre..'sprsheet.mml')

	states.register('game', game)

	states.push('game')
end

function game_close()
	sprsheet.close()

	font.free(fnt)
	video.close()
end
