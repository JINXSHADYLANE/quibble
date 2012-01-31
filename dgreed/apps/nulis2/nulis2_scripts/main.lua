require 'game'
require 'editor'
require 'menu'

pre = 'nulis2_assets/'
scr_size = { x = 1024, y = 768 }

music = nil
music_source = nil

function game_init()
	sim_size = scr_size
	levels_file = 'levels.mml' 
	if argv then
		for i,arg in ipairs(argv) do
			if arg == '-s' then
				sim_size = { x = 480, y = 320 }
				levels_file = 'levels_small.mml'
			end
		end
	end
	
	states.register('game', game)
	states.register('editor', editor)
	states.register('menu', menu)

	states.push('game')

	music = sound.load_stream(pre..'theme.ogg')
	music_source = sound.play(music, true)
end

function game_close()
	sound.free(music)
end
