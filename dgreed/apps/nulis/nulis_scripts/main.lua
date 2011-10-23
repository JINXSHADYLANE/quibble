require 'game'

pre = 'nulis_assets/'
scr_size = {
	x = 1024,
	y = 768
}	

function game_init()
	-- reduce screen size if we don't have enough space
	local w, h = video.native_resolution()
	local reduction = 1
	if h <= 768 then
		reduction = 2
	end

	-- init video, sound
	sound.init()
	video.init_ex(
		scr_size.x / reduction, scr_size.y / reduction, 
		scr_size.x, scr_size.y, 'nulis', false 
	)

	-- register and enter game state
	states.register('game', game)
	states.push('game')
end

function game_close()
	video.close()
	sound.close()
end
