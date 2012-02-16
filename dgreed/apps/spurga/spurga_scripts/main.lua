pre = 'spurga_assets/'
scr_size = vec2(320, 480) 
grid_pos = vec2(scr_size.x / 2, 32 + 384/2)

local hud = require('hud')
local game = require('game')
local menu = require('menu')
local levels = require('levels')
local puzzles = require('puzzles')

function game_init()
	states.register('game', game)
	states.register('menu', menu)
	states.register('levels', levels)

	hud.init()
	puzzles.load(pre..'puzzles.mml', pre..'slices.mml')
	fnt = font.load(pre..'HelveticaNeueLTCom-Md_14px.bft', 1.0, pre)

	states.push('menu')
end

function game_close()
	font.free(fnt)
	puzzles.free()
	hud.close()
end
