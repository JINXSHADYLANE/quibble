dofile(src..'well.lua')

game = {}

-- gets called once at the game start;
-- right place to load resources
function game.init()
	well.init()
end

-- called on game exit,
-- all resources should be freed here
function game.close()
	well.close()
end

-- called repeatedly from game loop
function game.frame()
	well.draw()
end

