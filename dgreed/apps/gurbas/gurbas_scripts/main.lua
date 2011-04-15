-- sausra 

screen = rect(0, 0, 640, 960)
pre, src = 'sausra_assets/', 'sausra_scripts/'

--dofile(src..'game.lua')

function init()
        sound.init()
        video.init_exr(screen.r, screen.b, screen.r, screen.b, 'gurbas', false)

        --game.init()
end

function close()
        --game.close()

        video.close()
        sound.close()
end

function frame()
        --game.frame()
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

