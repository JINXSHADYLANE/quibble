-- sausra 

screen = rect(0, 0, 480, 320)
pre, src = 'sausra_assets/', 'sausra_scripts/'

dofile(src..'game.lua')

function init()
	sound.init()
	video.init(screen.r, screen.b, 'sausra')

	music = sound.load_stream(pre..'Intractable.ogg')
	sound.play(music, true)

	game.init()
end

function close()
	game.close()

	video.close()
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

