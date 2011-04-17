dofile(src..'well.lua')
dofile(src..'block.lua')
dofile(src..'ai.lua')
dofile(src..'guy.lua')
dofile(src..'bullet.lua')

game = {
	lose_screen_t = nil,
	lose_screen_len = 2000,

	-- imgs
	img_empty = nil
}

-- gets called once at the game start;
-- right place to load resources
function game.init()
	well.init()
	block.init()
	guy.init()
	bullet.init()

	game.img_empty = tex.load(pre..'dark.png')
end

-- called on game exit,
-- all resources should be freed here
function game.close()
	block.close()
	well.close()
	guy.close()
	bullet.close()

	tex.free(game.img_empty)
end

function game.lose_frame()
	local t = (time.ms() - game.lose_screen_t) / game.lose_screen_len
	if t >= 1 then
		game.lose_screen_t = nil
		well.reset()
		guy.reset()
		block.reset()
	end
	
	well.draw()
	block.draw_static()
	guy.draw()

	local col = rgba(1, 1, 1, t)
	video.draw_rect(game.img_empty, 10, screen, col)
end

-- called repeatedly from game loop
function game.frame()
	if game.lose_screen_t ~= nil then
		game.lose_frame()
		return
	end

	well.draw()

	if not well.animates() then
		block.update()

		if guy.update() then
			game.lose_screen_t = time.ms()
		end

		if bullet.update() then
			block.reset()	
		end

		bullet.draw()
	end

	block.draw()
	guy.draw()
end

