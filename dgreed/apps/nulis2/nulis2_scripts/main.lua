
pre = 'nulis2_assets/'
scr_size = { x = 1024, y = 768 }
sim_size = scr_size
scr_type = 'ipad'
retina = nil

music = nil
music_source = nil

ed = false

function setup_screen()
	levels_file = 'levels.mml' 
	if argv then
		for i,arg in ipairs(argv) do
			if arg == '-iphone5' then
				iphone5 = true
				sim_size = { x = 568, y = 320 }
				scr_size = sim_size
				levels_file = 'levels_small.mml'
				scr_type = 'iphone'
				retina = true
			end
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
			if arg == '-retinaipad' then
				retina = true
			end
			if arg == '-e' then
				ed = true
			end
		end
	else
		local w, h = video.native_resolution()
		if w == 1136 and h == 640 then
			iphone5 = true
			retina = true
			levels_file = 'levels_small.mml'
			sim_size = { x = 568, y = 320 }
			scr_size = sim_size
			scr_type = 'iphone'
		elseif w == 2048 and h == 1536 then
			retina = true
		elseif w ~= 1024 and h ~= 768 then
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
if ed then
	require 'editor'
end
require 'menu'

function game_init()
	states.register('game', game)
	if ed then
		states.register('editor', editor)
	end
	states.register('menu', menu)

	states.push('game')

	music = sound.load_stream(pre..'theme.ogg')
	music_source = sound.play(music, true)
	if not keyval.get('state_music', true) then
		sound.pause(music_source)
	end

	if scr_type == 'ipad' then
		if retina then
			fnt = font.load(pre .. 'VarelaRound-Regular_60px.bft', 0.5, pre)
		else
			fnt = font.load(pre .. 'VarelaRound-Regular_30px.bft', 1.0, pre)
		end
	else
		if retina then
			fnt = font.load(pre .. 'VarelaRound-Regular_46px.bft', 0.5, pre)
		else
			fnt = font.load(pre .. 'VarelaRound-Regular_23px.bft', 1.0, pre)
		end
	end
end

function game_close()
	font.free(fnt)
	sound.free(music)
end
