local game = {}

local levels = require('levels')
local ghost = require('ghost')

local player = nil
local player_pos = nil

local level_end_t = nil
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

	local take_input = length_sq(player[1].pos - player[1].target_pos) < 0.06

	if level_end_t == nil then
		if take_input then
			local delta = vec2(0, 0) 
			if key.pressed(key._up) or char.pressed('w') then
				delta = delta + vec2(0, -1)
			end
			if key.pressed(key._right) or char.pressed('d') then
				delta = delta + vec2(1, 0)
			end
			if key.pressed(key._down) or char.pressed('s') then
				delta = delta + vec2(0, 1)
			end
			if key.pressed(key._left) or char.pressed('a') then
				delta = delta + vec2(-1, 0)
			end

			if delta then
				local new_pos = player_pos + delta
				if not levels.is_solid(new_pos) then
					ghost.move(player, delta)
					player_pos = new_pos
				end
			end
		end

		-- update level, perform level finish logic
		if levels.update(player_pos) then
			player_pos = vec2(-100, -100)
			level_end_t = time.s()
			current_level = current_level + 1
		end
	end

	sound.update()

	return not key.down(key.quit)
end

function game.render(t)
	local ts = time.s()

	sprsheet.draw('empty', 0, scr_rect, bg_color)
	sprsheet.draw('overlay', overlay_layer, scr_rect)

	levels.draw()
	ghost.draw(player, ts)

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
			local tex, src = sprsheet.get('noise')
			local rand_rect = rect(
				rand.int(0, 64),
				rand.int(0, 64),
				rand.int(64, 128),
				rand.int(64, 128)
			)
			video.draw_rect(tex, 15, rand_rect, scr_rect)
		end
	end

	return true
end

return game

