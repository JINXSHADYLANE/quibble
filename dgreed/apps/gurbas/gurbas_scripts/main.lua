-- gurbas 

screen = rect(0, 0, 640, 960)
tile_size = 64
tiles_x, tiles_y = screen.r / tile_size, screen.b / tile_size

pre, src = 'gurbas_assets/', 'gurbas_scripts/'

-- utils

-- 2d -> 1d index mapping
function idx_2d(x, y, w)
	return y * w + x
end

-- well index
function widx(x, y)
	return y * tiles_x + x
end

function inv_widx(i)
	local y = math.floor(i / tiles_x)
	local x = i - (y * tiles_x)
	return vec2(x, y)
end

dofile(src..'game.lua')

function init()
	sound.init()

	local w, h = video.native_resolution()
	local reduction = 1
	if h < 1000 then
		reduction = 2
	end
    video.init_exr(screen.r / reduction, screen.b / reduction, screen.r, screen.b, 'gurbas', false)
	
	music = sound.load_stream(pre..'gurbas.ogg')
	sound.set_volume(music, 0.65)
	sound.play(music, true)

    game.init()
end

function close()
    game.close()

    video.close()
	sound.free(music)
    sound.close()
end

function frame()
    game.frame()
    return not key.pressed(key.quit) 
end

function main()
	init()
	repeat
		cont = frame()
		sound.update()
	until not video.present() or not cont
	close()
end

main()

