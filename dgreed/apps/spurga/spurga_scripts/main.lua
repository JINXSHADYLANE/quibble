pre = 'spurga_assets/'
scr_size = vec2(320, 480) 
grid_pos = vec2(scr_size.x / 2, 32 + 384/2)

local hud = require('hud')
local game = require('game')
local menu = require('menu')
local levels = require('levels')
local puzzles = require('puzzles')

-- transition mask is list of numbers from 1 to n, shuffled randomly
transition_mask = nil

function new_transition_mask(n)
	-- generates new transition mask using Knuth's Algorithm P
	transition_mask = {}
	for i=1,n do
		transition_mask[i] = i
	end

	for i=n,1,-1 do
		local j = rand.int(1, i+1)
		transition_mask[i], transition_mask[j] = transition_mask[j], transition_mask[i]	
	end
end

function game_init()
	states.register('game', game)
	states.register('menu', menu)
	states.register('levels', levels)

	hud.init()
	puzzles.load(pre..'puzzles.mml', pre..'slices.mml')
	fnt = font.load(pre..'HelveticaNeueLTCom-Md_14px.bft', 1.0, pre)

	new_transition_mask(6*5)
	
	states.transition_len = 0.4 
	states.push('menu')
end

function game_close()
	font.free(fnt)
	puzzles.free()
	hud.close()
end
