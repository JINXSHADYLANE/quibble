dofile(src..'well.lua')
dofile(src..'block.lua')
dofile(src..'ai.lua')
dofile(src..'guy.lua')
dofile(src..'bullet.lua')

game = {}

-- gets called once at the game start;
-- right place to load resources
function game.init()
	well.init()
	block.init()
	guy.init()
	bullet.init()
end

-- called on game exit,
-- all resources should be freed here
function game.close()
	block.close()
	well.close()
	guy.close()
	bullet.close()
end

-- called repeatedly from game loop
function game.frame()
	well.draw()

	if not well.animates() then
		block.update()
		block.draw()

		guy.update()

		if bullet.update() then
			block.reset()	
		end

		bullet.draw()
	end
	guy.draw()
end

