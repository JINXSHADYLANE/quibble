dofile(src..'well.lua')
dofile(src..'block.lua')
dofile(src..'ai.lua')
dofile(src..'guy.lua')
dofile(src..'bullet.lua')

game = {
	lose_screen_t = nil,
	lose_screen_len = 2000,
	win_screen_t = nil,
	win_screen_len = 2500,
	ai_lose_t = nil,
	start_t = nil,
	time_str = '',
	drop_blocks = true,

	-- imgs
	img_empty = nil,
	img_back = nil,
	font = nil
}

-- gets called once at the game start;
-- right place to load resources
function game.init()
	well.init()
	block.init()
	guy.init()
	bullet.init()

	game.img_empty = tex.load(pre..'dark.png')
	game.img_back = tex.load(pre..'back.png')
	game.font = font.load(pre..'nova_333px.bft')
	game.snd_win = sound.load_sample(pre..'victory.wav')

	game.start_t = time.ms()
end

-- called on game exit,
-- all resources should be freed here
function game.close()
	block.close()
	well.close()
	guy.close()
	bullet.close()

	tex.free(game.img_empty)
	tex.free(game.img_back)
	font.free(game.font)
	sound.free(game.snd_win)
end

function game.lose_frame()
	local t = (time.ms() - game.lose_screen_t) / game.lose_screen_len
	if t >= 1 then
		game.lose_screen_t = nil
		game.ai_lose_t = nil
		well.reset()
		guy.reset()
		block.reset()
		game.start_t = time.ms()
		game.drop_blocks = true
	end
	
	well.draw()
	block.draw_static()
	guy.draw()

	local col = rgba(0, 0.04, 0.01, t)
	video.draw_rect(game.img_empty, 10, screen, col)
end

function game.win_frame()
	local t = (time.ms() - game.win_screen_t) / game.win_screen_len
	if t >= 1 then
		if t >= 2 or key.pressed(key._left) or key.pressed(key._right) then
			game.win_screen_t = nil
			game.ai_lose_t = nil
			well.reset()
			guy.reset()
			block.reset()
			bullet.reset()
			game.start_t = time.ms()
			game.drop_blocks = true
			game.snd_win_played = false
		end
		t = 1	
	end
	
	well.draw()
	block.draw_static()
	guy.draw()

	local col = rgba(0.75, 0.8, 0.91, t)
	video.draw_rect(game.img_empty, 5, screen, col)
end

function game.time()
	local t = time.ms() - game.start_t
	local s = math.floor(t / 1000)
	local ds = math.floor((t - s * 1000) / 100) 
	local str = tostring(s)..':'..tostring(ds)
	if s < 100 then
		str = '0'..str
		if s < 10 then
			str = '0'..str
		end
	end
	return str 
end

function game.draw_title()
	local t = (time.ms() - 1500) / 8000
	if t >= 0 and t <= 1 then
		local p = lerp(vec2(30, 590), vec2(-390, 590), t)
		local col =	rgba(0.3, 0.3, 0.3)
		col.a = math.sin(math.pi*t)
		video.draw_text(game.font, 1, "gurbas", p, col)
	end
end

function game.music_fadein()
	if game.snd_win_played then
		return false
	end

	local volume = sound.volume(music)
	if volume >= 0.65 then
		return false
	end

	sound.set_volume(music, volume * 1.05)
	return true
end

function game.music_fadeout()
	local volume = sound.volume(music)
	if volume < 0.15 then
		return false
	end

	sound.set_volume(music, volume * 0.95)
	return true
end

function game.win_sound()
	if game.snd_win_played or game.music_fadeout() then
		return
	end

	sound.play(game.snd_win)
	game.snd_win_played = true

end

-- called repeatedly from game loop
function game.frame()
	local text_size = 1
	if not game.drop_blocks then
		text_size = 1 + math.fmod(time.s(), 2) / 8
	end
	local text_w, text_h = font.size(game.font, game.time_str)
	local text_pos = vec2(-50 + text_w/2, 100 + text_h/2)
	video.draw_text_centered(game.font, 1, game.time_str, 
		text_pos, text_size, rgba(0, 0, 0, 0.3))
	video.draw_rect(game.img_back, 0, vec2(0, 0))

	game.draw_title()

	if game.lose_screen_t ~= nil then
		game.lose_frame()
		return
	end

	if game.win_screen_t ~= nil then
		game.win_sound()
		game.win_frame()
		return
	end

	if game.ai_lose_t ~= nil then
		if time.ms() - game.ai_lose_t > 6000 then
			game.lose_screen_t = time.ms()
			game.ai_lose_t = nil
			sound.play(guy.snd_death)
		end
	end

	game.music_fadein()

	well.draw()

	if not well.animates() then
		block.update()
		if well.did_lose() then
			game.drop_blocks = false
			if game.ai_lose_t == nil then
				game.ai_lose_t = time.ms()
			end
		end

		if guy.update() then
			if guy.did_win then
				game.win_screen_t = time.ms()
			else
				game.lose_screen_t = time.ms()
			end
		end

		if bullet.update() then
			block.reset(true)	
		end

		bullet.draw()
		block.draw()
	else
		block.draw_static()
	end
	
	game.time_str = game.time()
	guy.draw()
end
