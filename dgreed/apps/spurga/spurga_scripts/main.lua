pre = 'spurga_assets/'
scr_size = { x = 320, y = 480 }

local game = require('game')

function game_init()
	states.register('game', game)

	states.push('game')
end

function game_close()
end
