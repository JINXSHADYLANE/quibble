pre = 'spurga_assets/'
scr_size = { x = 320, y = 480 }

local game = require('game')
local puzzles = require('puzzles')

function game_init()
	states.register('game', game)

	states.push('game')

	puzzles.load(pre..'puzzles.mml')
end

function game_close()
end