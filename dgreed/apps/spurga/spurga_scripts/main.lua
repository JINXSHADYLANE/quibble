pre = 'spurga_assets/'
scr_size = vec2(320, 480) 

local game = require('game')
local puzzles = require('puzzles')

function game_init()
	states.register('game', game)

	states.push('game')

	puzzles.load(pre..'puzzles.mml', pre..'slices.mml')
end

function game_close()
	puzzles.free()
end
