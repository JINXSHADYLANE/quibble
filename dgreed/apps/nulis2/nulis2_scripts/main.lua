
pre = 'nulis2_assets/'
scr_size = { x = 1024, y = 768 }
sim_size = scr_size
scr_type = 'ipad'
retina = nil

music = nil
music_source = nil

function setup_screen()
	levels_file = 'levels.mml' 
	if argv then
		for i,arg in ipairs(argv) do
			if arg == '-s' or arg == '-iphone' or arg == '-retina' then
				sim_size = { x = 480, y = 320 }
				levels_file = 'levels_small.mml'
			end

			if arg == '-iphone' or arg == '-retina' then
				scr_size = { x = 480, y = 320 }
				scr_type = 'iphone'
			end
			if arg == '-retina' then
				retina = true
			end
		end
	else
		local w, h = video.native_resolution()
		if w ~= 1024 and h ~= 768 then
			sim_size = { x = 480, y = 320 }
			levels_file = 'levels_small.mml'
			scr_size = sim_size
			scr_type = 'iphone'
		end
		if w == 960 and h == 640 then
			retina = true
		end
	end
end

setup_screen()

require 'game'
require 'editor'
require 'menu'

function game_init()
	
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
