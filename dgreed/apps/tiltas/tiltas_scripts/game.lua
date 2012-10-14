local game = {}

local levels = require('levels')
local ghost = require('ghost')

local player = nil
local player_pos = nil

local level_end_t = nil
local last_input_t = 0
local current_level = 1

function game.init()
	video.set_blendmode(glow_layer, 'add')
	video.set_blendmode(ghost_layer, 'add')
	video.set_blendmode(overlay_layer, 'multiply')
	game.reset()
end

function game.reset()
	player_pos = levels.reset(current_level)
	player = ghost.make(player_pos, rgba(0.5, 0.5, 0.5))
end

function game.close()
end

function game.enter()
end

function game.close()
end

function game.update()
	local ts = time.s()

	local take_input = ts - last_input_t > 0.13

	if level_end_t == nil then
		if take_input then
			local delta = vec2(0, 0) 
			if key.pressed(key._up) or char.pressed('w') then
				delta = vec2(0, -1)
			end
			if key.pressed(key._right) or char.pressed('d') then
				delta = vec2(1, 0)
			end
			if key.pressed(key._down) or char.pressed('s') then
				delta = vec2(0, 1)
			end
			if key.pressed(key._left) or char.pressed('a') then
				delta = vec2(-1, 0)
			end

			if delta then
				local new_pos = player_pos + delta
				if not levels.is_solid(new_pos) then
					ghost.move(player, delta)
					player_pos = new_pos
					last_input_t = ts
				end
			end
		end

		-- update level, perform level finish logic
		if levels.update(player_pos) then
			player_pos = vec2(-100, -100)
			level_end_t = ts
			current_level = current_level + 1
			mfx.trigger('noise')
		end
	end

	sound.update()
	mfx.update()

	return not key.down(key.quit)
end

function game.render_noise()
	local tex, src = sprsheet.get('noise')
	local rand_rect = rect(
		rand.int(0, 64),
		rand.int(0, 64),
		rand.int(64, 128),
		rand.int(64, 128)
	)
	video.draw_rect(tex, 15, rand_rect, scr_rect)
end

function game.render(t)
	local ts = time.s()

	sprsheet.draw('empty', 0, scr_rect, bg_color)
	sprsheet.draw('overlay', overlay_layer, scr_rect)
	--local frame = math.fmod(math.floor(ts*15), 9)
	--sprsheet.draw_anim('grain', frame, overlay_layer, scr_rect, rgba(1, 1, 1, 0.5))

	levels.draw()
	ghost.draw(player, ts)

	if text then
		for i,t in ipairs(text) do
			video.draw_text(fnt, 1, t.text, t.pos, rgba(0.8, 0.8, 0.8, 0.8))
		end
	end

	if level_end_t then
		local tt = (ts - level_end_t) / 0.5
		if tt > 1 then
			level_end_t = nil
			game.reset()
		else
			local tf = math.sin(tt * math.pi)
			sprsheet.draw('empty', 15, scr_rect, rgba(0, 0, 0, tf))
		end

		if tt > 0.8 then
			game.render_noise()
		end
	end

	if noise > 0 then
		if noise == 1 then
			mfx.trigger('noise')
		end
		noise = noise - 1
		game.render_noise()
	end

	return true
end

return game

