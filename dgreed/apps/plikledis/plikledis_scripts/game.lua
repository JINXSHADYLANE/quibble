module(..., package.seeall)

require 'levels'
require 'text'

tile_size = 16
tile_layer = 1
char_layer = 2
noise_layer = 3
text_layer = 5
level = nil
level_pos = nil

move_anim_len = 5/60 

moves = {}

function init()
	levels.init()

	tex_noise = tex.load(pre..'noise.png')
end

function close()
	tex.free(tex_noise)
end

function enter()
	level_reset()
	text.prologue()
end

function level_reset()
	tile_size = 32 
	level = levels.get_current()

	local width, height

	repeat
		tile_size = tile_size / 2
		width = level.size_w * tile_size
		height = level.size_h * tile_size
	until width <= scr_size.x and height <= scr_size.y

	level_pos = vec2(
		math.floor((scr_size.x - width) / 2),
		math.floor((scr_size.y - height) / 2)
	)

	moves = {}
	player_on_slot = false
end

function is_door_open()
	local res
	if level.slots_to_trigger > 1 then
		res = false
	elseif level.slots_to_trigger == 0 then
		res = true
	else
		-- player standing on a slot
		local x, y = level.char_pos.x, level.char_pos.y
		local t = level.tiles[idx2d(x, y, level.size_w)]

		if t ~= nil and bit.band(t, levels.b_slot) ~= 0 then
			return true
		end
	end

	return res
end

function leave()
end

function undo_move()
	if #moves == 0 then
		return
	end

	local last_move = moves[#moves]
	table.remove(moves)

	level.char_pos.x = last_move.x
	level.char_pos.y = last_move.y
	
	if last_move.slots then
		level.slots_to_trigger = last_move.slots
	end

	if last_move.box_from and last_move.box_to then
		local from = last_move.box_from
		local to = last_move.box_to

		if level.tiles[from] then
			level.tiles[from] = bit.band(level.tiles[from], bit.bnot(levels.b_box))
			if level.tiles[from] == 0 then
				level.tiles[from] = nil
			end
		end

		level.tiles[to] = level.tiles[to] or 0
		level.tiles[to] = bit.bor(level.tiles[to], levels.b_box)
	end

	if last_move.switch_walls then
		levels.switch_walls(level, last_move.switch_walls)
	end

	if last_move.put_fiend then
		local to = last_move.put_fiend
		level.tiles[to] = level.tiles[to] or 0
		level.tiles[to] = bit.bor(level.tiles[to], levels.b_fiend)
	end
end

function move_box(x, y, dx, dy, move)
	local nx, ny = x + dx, y + dy
	local old_idx = idx2d(x, y, level.size_w)
	local idx = idx2d(nx, ny, level.size_w)
	local t = level.tiles[idx]
	local success = nil

	if t == nil then
		success = true
	elseif t <= levels.b_wall or 
		bit.band(t, levels.b_exit) ~= 0 or 
		bit.band(t, levels.b_box) ~= 0 then
		success = false
	elseif bit.band(t, levels.b_slot) ~= 0 then
		success = true
	end

	if success then
		move.slots = level.slots_to_trigger

		level.tiles[old_idx] = bit.band(level.tiles[old_idx], bit.bnot(levels.b_box))
		if level.tiles[old_idx] == 0 then
			level.tiles[old_idx] = nil
		elseif bit.band(level.tiles[old_idx], levels.b_slot) ~= 0 then
			if level.slots_to_trigger == 0 then
				mfx.trigger('door_close')
			end
			level.slots_to_trigger = level.slots_to_trigger + 1
		end

		level.tiles[idx] = level.tiles[idx] or 0
		level.tiles[idx] = bit.bor(level.tiles[idx], levels.b_box)

		if bit.band(level.tiles[idx], levels.b_slot) ~= 0 then
			level.slots_to_trigger = level.slots_to_trigger - 1
			if level.slots_to_trigger == 0 then
				mfx.trigger('door_open')
			end
		end

		move.box_from = idx
		move.box_to = old_idx

		return true
	end

	return false
end

function move_char(dx, dy) 
	local x, y = level.char_pos.x + dx, level.char_pos.y + dy
	local idx = idx2d(x, y, level.size_w)
	local t = level.tiles[idx]
	local success = nil

	local closed = not is_door_open()

	local move = {}

	if t == nil then
		success = true
		mfx.trigger('move')
	elseif t <= levels.b_wall or (bit.band(t, levels.b_exit) ~= 0 and closed) then
		success = false 
	elseif bit.band(t, levels.b_box) ~= 0 then
		success = move_box(x, y, dx, dy, move)
		if success then
			mfx.trigger('push')
		end
	elseif bit.band(t, levels.b_slot) ~= 0 then
		success = true 
		mfx.trigger('move')
	elseif bit.band(t, levels.b_fiend) ~= 0 then
		level.fiend = true
		success = true
		level.tiles[idx] = nil
		move.put_fiend = idx
		mfx.trigger('fiend')
	end

	if success then
		move.x = level.char_pos.x
		move.y = level.char_pos.y
		level.char_pos.x = x
		level.char_pos.y = y

		if level.fiend then
			show_end = levels.fiend(level, move, not closed)
		end

		table.insert(moves, move)
		level.last_move_t = time.s()

		if t ~= nil and bit.band(t, levels.b_slot) ~= 0 then
			if level.slots_to_trigger == 1 then
				mfx.trigger('door_open')
				player_on_slot = true
			end
		else
			if player_on_slot then
				mfx.trigger('door_close')
				player_on_slot = false
			end
		end
	else
		mfx.trigger('wall')
	end

	if t ~= nil and bit.band(t, levels.b_exit) ~= 0 and not closed then
		text_t = time.s()
		text.push(levels.get_text())
		levels.current_level = levels.current_level + 1
		level_reset()
		mfx.trigger('win')
	end
end

function update()
	if key.up(key.quit) then
		states.pop()
	end

	if not text.update() then
		if not mus_playing then
			sound.play(mus, true)
			mus_playing = true
		end

		if show_end and level.noise < 2 then
			text.epilogue()
		end
	
		if char.up('r') then
			level_reset()
		end

		if char.up('z') then
			undo_move()
		end

		if key.down(key._left) then
			move_char(-1, 0)
		end

		if key.down(key._right) then
			move_char(1, 0)
		end

		if key.down(key._up) then
			move_char(0, -1)
		end

		if key.down(key._down) then
			move_char(0, 1)
		end	

		if level.last_move_t then
			local t = time.s()
			if level.last_move_t + move_anim_len <= t then
				level.last_move_t = nil
			end
		end
	end

	mfx.update()
	sound.update()

	return true
end

function render_level()
	for idx,t in pairs(level.tiles) do
		if t ~= nil then
			local x, y = rev_idx2d(idx, level.size_w)
			local off = vec2(x*tile_size, y*tile_size)
			local p = level_pos + off
			local d = rect(p.x, p.y, p.x + tile_size, p.y + tile_size)

			if t <= levels.b_wall then
				sprsheet.draw_anim('walls', t, tile_layer, d)	
			end

			if bit.band(t, levels.b_box) ~= 0 then
				local skip = false
				if level.last_move_t ~= nil then
					local last_move = moves[#moves]
					if last_move and last_move.box_from == idx then
						skip = true
					end
				end

				if not skip then
					sprsheet.draw('crate', char_layer, d) 
				end
			end

			if bit.band(t, levels.b_slot) ~= 0 then
				sprsheet.draw('switch', tile_layer, d)
			end

			if bit.band(t, levels.b_fiend) ~= 0 then
				sprsheet.draw('fiend', tile_layer, d)
			end

			if bit.band(t, levels.b_exit) ~= 0 then
				if is_door_open() then
					sprsheet.draw('door_open', tile_layer, d)
				else	
					sprsheet.draw('door', tile_layer, d)
				end	
			end
		end
	end

	local px = level.char_pos.x
	local py = level.char_pos.y

	if level.last_move_t then
		local last_move = moves[#moves]
		local t = time.s()
		local at = (t - level.last_move_t) / move_anim_len 
		px = smoothstep(last_move.x, px, at)
		py = smoothstep(last_move.y, py, at)

		-- also, draw pushed box
		if last_move.box_from then
			local from_x, from_y = rev_idx2d(last_move.box_from, level.size_w)
			local to_x, to_y = rev_idx2d(last_move.box_to, level.size_w)
			local x = smoothstep(to_x, from_x, at)
			local y = smoothstep(to_y, from_y, at)
			local p = level_pos + vec2(
				x * tile_size,
				y * tile_size
			)
			local d = rect(p.x, p.y, p.x + tile_size, p.y + tile_size)
			sprsheet.draw('crate', char_layer, d)
		end
	end

	local p = level_pos + vec2(
		px * tile_size,
		py * tile_size
	)

	local d = rect(p.x, p.y, p.x + tile_size, p.y + tile_size)

	sprsheet.draw('char', char_layer, d)
end

function render_noise()
	if level.noise ~= nil and level.noise > 1 then
		level.noise = level.noise - 1
		local x = rand.float(0, 64)
		local y = rand.float(0, 64)
		local w = rand.float(16, 64)
		local h = rand.float(16, 64)
		local src = rect(x, y, x+w, y+h)
		local dest = rect(0, 0, scr_size.x, scr_size.y)
		video.draw_rect(tex_noise, noise_layer, src, dest)	
	end
end

function render(t)
	text.render(text_layer)
	render_level()
	render_noise()
	return true
end

