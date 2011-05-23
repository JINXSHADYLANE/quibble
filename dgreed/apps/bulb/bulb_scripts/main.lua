-- bulb 

screen = rect(0, 0, 960, 640)
pre, src = 'bulb_assets/', 'bulb_scripts/'

dofile(src..'game.lua')

function init()
        sound.init()
        video.init(screen.r, screen.b, 'bulb')

		music = sound.load_stream(pre..'bulb.ogg')
        game.init()

		sound.play(music, true)
end

function close()
		sound.free(music)
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

