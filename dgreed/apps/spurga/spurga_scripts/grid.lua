local grid = {}
grid.__index = grid

local puzzles = require('puzzles')

-- tweaks
-- do not move row/column if touch move distance is less than this
local touch_move_dist = 5 
-- how fast is sliding animation, when finger is lifted up (px/s)
local release_anim_speed = 120 
-- higher values correspond to slower shuffling speeds
local shuffle_speed = 0.000005
-- color of currently pressed tile
local active_color = rgba(1, 1, 1, 0.9)
-- length of single tile transition phase, in normalized time
local tile_transition_len = 0.2
-- length of immovable tile animation
local wallmark_len = 0.3

local vec2_zero = vec2(0, 0)


function grid:new(puzzle, relax) 
	local obj = {['puzzle'] = puzzle, ['relax'] = relax}
	setmetatable(obj, self) 
	obj:load_state()
	obj.color_map = {}
	obj.wallmarks = {}
	obj.show_wallmars = true
	return obj
end

function grid:reset_state(shuffle)
	local p = self.puzzle
	self.state = {}
	for i = 1, p.w * p.h do
		table.insert(self.state, i)
	end

	if shuffle then
		self:shuffle()
	end

	self.moves = 0
	self.last_move_x = nil
	self.last_move_y = nil
end

function grid:save_state()
	local name = self.puzzle.name
	
	if not self.shuffling then

		local state_key 
		if self.relax then
			state_key = 'rstate:'..name
		else
			state_key = 'pstate:'..name
		end

		local state = {}
		for i,s in ipairs(self.state) do
			state[i] = tostring(s)
		end

		local state_str = table.concat(state, ',')
		keyval.set(state_key, state_str)

		if not relax then
			keyval.set('pmoves:'..name, self.moves)
		end
	end
end

function grid:load_state()
	local name = self.puzzle.name

	local state_key 
	if self.relax then
		state_key = 'rstate:'..name
	else
		state_key = 'pstate:'..name
	end
	
	local state_str = keyval.get(state_key, '')
	if state_str == '' then
		if name == 'menu' or name == 'levels' or name == 'score' then
			self:reset_state()
		else	
			self:reset_state(true)
		end
	else
		local state = {}
		for s in state_str:gmatch('(%d+)') do
			table.insert(state, tonumber(s))
		end
		self.state = state
		self.moves = keyval.get('pmoves:'..self.puzzle.name, 0)
	end
end

function grid:score()
	local score = math.max(0, self.puzzle.par - self.moves)
	return score
end

function grid:is_solved()
	if not self.shuffling and self.can_shuffle and not self.must_shuffle then
		local p = self.puzzle
		for i,t in ipairs(self.state) do
			if p.solved[t] ~= p.solved[i] then
				return false
			end
		end

		if not self.relax then
			-- solved, remember score
			local keyname = 'pscore:'..p.name
			local old_score = keyval.get(keyname, -1)
			local new_score = self:score()
			if old_score < new_score then
				keyval.set(keyname, new_score)
			end

			-- unlock next two levels
			puzzles.unlock_next(p)
		end

		return true
	end
	return false
end

-- when we're shifting/animating a single tile might be visible in two
-- places at once, this draws such tile
function grid:draw_shifted_tile(tex, layer, src, x, y, offset, p, top_left, c)
	local fmod = math.fmod
	local abs = math.abs

	local abs_offset = vec2(abs(offset.x), abs(offset.y))
	local steps = vec2(
		math.floor(abs_offset.x / p.tile_w),
		math.floor(abs_offset.y / p.tile_h)
	)

	local dx, dy = 0, 0
	if offset.x > 0 then
		dx = -1
	elseif offset.x < 0 then
		dx = 1
	elseif offset.y > 0 then
		dy = -1
	elseif offset.y < 0 then
		dy = 1
	end

	-- at any given point in time, shifted tile is intersecting with two
	-- grid cells; let's calculate them, taking portals into account

	local ax, ay = x, y
	for i=1,steps.x do
		ax = fmod(p.w + ax + dx, p.w)
		while is_portal(p.solved[self.state[ay * p.w + ax + 1]]) do
			ax, ay = fmod(p.w + ax + dx, p.w), fmod(p.h + ay + dy, p.h)
		end
	end
	for i=1,steps.y do
		ay = fmod(p.h + ay + dy, p.h)
		while is_portal(p.solved[self.state[ay * p.w + ax + 1]]) do
			ax, ay = fmod(p.w + ax + dx, p.w), fmod(p.h + ay + dy, p.h)
		end
	end

	local bx = fmod(p.w + ax + dx, p.w)
	local by = fmod(p.h + ay + dy, p.h)
	while is_portal(p.solved[self.state[by * p.w + bx + 1]]) do
		bx, by = fmod(p.w + bx + dx, p.w), fmod(p.h + by + dy, p.h)
	end

	-- shift offset from grid cells
	local real_offset = abs_offset - vec2(steps.x * p.tile_w, steps.y * p.tile_h)
	local neg_offset = vec2(
		(p.tile_w - real_offset.x) * dx * dx, 
		(p.tile_h - real_offset.y) * dy * dy
	)

	-- positions of two grid cells
	local pos_a = vec2(ax * p.tile_w + top_left.x, ay * p.tile_h + top_left.y)
	local pos_b = vec2(bx * p.tile_w + top_left.x, by * p.tile_h + top_left.y)

	if dy > 0 or dx > 0 then
		real_offset = -real_offset
		neg_offset = -neg_offset
	end
	if bx-ax == dx and by-ay == dy then
		-- draw
		if c then
			video.draw_rect(tex, layer, src, pos_a - real_offset, c)
		else
			video.draw_rect(tex, layer, src, pos_a - real_offset)
		end
	elseif abs(bx-ax) == p.w-1 or abs(by-ay) == p.h-1 then
		-- draw with wrap around
		if c then
			video.draw_rect(tex, layer, src, pos_a - real_offset, c)
			video.draw_rect(tex, layer, src, pos_b + neg_offset, c)
		else
			video.draw_rect(tex, layer, src, pos_a - real_offset)
			video.draw_rect(tex, layer, src, pos_b + neg_offset)
		end
	else
		local alpha = clamp(0, 1, math.max(
			abs(real_offset.x) / p.tile_h, 
			abs(real_offset.y) / p.tile_w
		))

		local ghost_color = rgba(1, 1, 1, alpha)
		local color = rgba(1, 1, 1, 1 - alpha)

		-- draw with ghost
		video.draw_rect(tex, layer, src, pos_a - real_offset, color)
		video.draw_rect(tex, layer, src, pos_b + neg_offset, ghost_color)
	end
end

function grid:draw(pos, layer, transition, hint)
	local p = self.puzzle
	local ts = time.s()

	-- do delayed shuffling
	if self.must_shuffle then
		if self.can_shuffle then
			self:shuffle(self.must_shuffle)
			self.must_shuffle = nil
		end
	end

	-- do delayed unscrambling
	if self.must_unscramble then
		if self.can_shuffle then
			self:unscramble()
			self.must_unscramble = nil
		end
	end

	-- cache texture handle and tile source rectangles
	if not p.tex then
		puzzles.preload(p)
	end

	-- move puzzle to lru list front
	puzzles.lru_list_front(p)

	if not self.half_size then
		self.half_size = vec2(
			p.tile_w * p.w / 2, 
			p.tile_h * p.h / 2
		)
	end

	-- precalc animation parameter
	local t = nil
	local anim_offset = nil
	if self.start_anim_t then
		local t = (ts - self.start_anim_t) / self.anim_len
		if t >= 1 then
			-- end anim
			self.start_anim_t = nil
			self.anim_mask_x = nil
			self.anim_mask_y = nil
			if self.anim_end_callback then
				mfx.snd_ambient('slinkt.wav', 0.25)
				self.anim_end_callback()
			end
		else
			anim_offset = smoothstep(
				self.start_anim_offset, 
				self.end_anim_offset, t
			)
			if length_sq(self.start_anim_offset - self.end_anim_offset) > 4 then
				mfx.snd_ambient('slinkt.wav', 0.2)
			end
		end
	end

	-- top left corner coordinates
	local top_left = pos - self.half_size
	local cursor = vec2(top_left)

	-- draw current grid state
	local idx, tile, solved_tile, r, c
	for y = 0, p.h-1 do
		for x = 0, p.w-1 do
			idx = y * p.w + x + 1
			tile = self.state[idx] 
			solved_tile = p.solved[tile]
			r = p.tile_src[solved_tile]


			-- pressed color
			c = nil
			if self.touched_tile then
				if x == self.touched_tile.x and y == self.touched_tile.y then
					c = active_color
				else
					c = nil
				end
			end

			-- transition color
			local skip = false
			if transition and transition ~= 0 then
				t = 1 - math.abs(transition)

				local mask = (transition_mask[idx] - 1) / (p.w * p.h-1)
				local start_t = mask * (1 - tile_transition_len) 
				if transition > 0 then
					start_t = tile_transition_len - start_t
				end
				local alpha = smoothstep(0, 1, (t - start_t) / tile_transition_len)
				c = rgba(1, 1, 1, alpha)

				-- draw hint
				if hint then
					if p.solved[idx] ~= solved_tile then
						local hint_r = p.tile_src[p.solved[idx]]
						if alpha < 1 then
						local hint_color = rgba(1, 1, 1, 1 - alpha)
							video.draw_rect(p.tex, layer, hint_r, cursor, hint_color)
							if alpha == 0 then
								skip = true	
							end
						end
					else
						c = nil
					end
				end
			end

			-- colorize
			local colorized = self.color_map[solved_tile]
			if colorized then
				if c then
					c = c * colorized
				else
					c = colorized
				end
			end

			if not skip then
				if is_portal(solved_tile) then
					-- portal tile
					video.draw_rect(p.tex, layer-1, r, cursor)
				elseif x ~= self.move_mask_x and y ~= self.move_mask_y then
					if x ~= self.anim_mask_x and y ~= self.anim_mask_y then
						-- plain tile
						if c then
							video.draw_rect(p.tex, layer, r, cursor, c)
						else
							video.draw_rect(p.tex, layer, r, cursor)
						end
					else
						if anim_offset == nil then
							anim_offset = vec2_zero
						end
						-- animated tile
						self:draw_shifted_tile(
							p.tex, layer, r, 
							x, y, anim_offset, p, top_left, c
						)
					end
				else
					-- moved tile
					self:draw_shifted_tile(
						p.tex, layer, r, 
						x, y, self.move_offset, p, top_left, c
					)
				end
			end

			-- draw wallmark
			local wm = self.wallmarks[tile]
			if wm ~= nil then
				local tt = (ts - wm) / wallmark_len
				if tt >= 1 then
					self.wallmarks[tile] = nil
					self.show_wallmarks = false
				else
					local col = rgba(1, 1, 1, math.sin(tt * math.pi))
					sprsheet.draw_centered(
						'frontier', layer+1, 
						cursor + vec2(p.tile_w/2, p.tile_h/2), col
					)
				end
			end

			cursor.x = cursor.x + p.tile_w
		end
		cursor.x = pos.x - self.half_size.x
		cursor.y = cursor.y + p.tile_h
	end
end

function grid:shift(column_x, row_y, offset)
	assert(column_x ~= nil or row_y ~= nil)
	assert(column_x == nil or row_y == nil)

	local fmod = math.fmod

	-- count a move
	if not self.shuffling then
		if not (self.last_move_x == column_x and self.last_move_y == row_y) then
			self.moves = self.moves + 1
			self.last_move_x, self.last_move_y = column_x, row_y
		end
	end

	local p = self.puzzle

	-- calc step offset and start pos
	local dx, dy, x, y = 1, 1, 0, 0
	if column_x then
		dx, x = 0, column_x
	else
		dy, y = 0, row_y
	end

	-- fill a table with previuos row/column values
	local prev, cx, cy = {}, x, y
	while cx < p.w and cy < p.h do
		local t = self.state[cy * p.w + cx + 1]
		if not is_portal(p.solved[t]) then
			table.insert(prev, t)
		end
		cx, cy = cx + dx, cy + dy
	end

	-- make a new, rotated row/column table
	local new = {}
	local off = fmod(offset, #prev)
	for i = 1, #prev do
		new[i] = prev[fmod(#prev - off + (i-1), #prev) + 1]
	end
	
	-- update current state with rotated row/column
	local i = 1
	cx, cy = x, y
	while cx < p.w and cy < p.h do
		local idx = cy * p.w + cx + 1
		local t = self.state[idx]
		if not is_portal(p.solved[t]) then
			self.state[idx] = new[i]
			i = i + 1
		end
		cx, cy = cx + dx, cy + dy
	end
end

function grid:random_move()
	local p = self.puzzle
	local shift_offset, mask_x, mask_y

	if rand.int(0, 2) == 0 then
		shift_offset = -1
	else
		shift_offset = 1
	end

	repeat	
		if rand.int(0, 2) == 0 then
			mask_x, mask_y = rand.int(0, p.w), nil
			self.end_anim_offset = vec2(0, shift_offset * p.tile_w)
		else	
			mask_x, mask_y = nil, rand.int(0, p.h)
			self.end_anim_offset = vec2(shift_offset * p.tile_h, 0)
		end
	until self:can_move(mask_y ~= nil, mask_x ~= nil, mask_x or 0, mask_y or 0, p)

	return shift_offset, mask_x, mask_y
end

function grid:shuffle()
	local n = 140
	if not self.can_shuffle then
		self.must_shuffle = n
		return
	end

	if self.relax then
		rand.seed(time.ms())
	else
		rand.seed(hash(self.puzzle.name))
	end

	self.shuffling = true

	self.start_anim_t = time.s()
	self.start_anim_offset = vec2_zero
	self.end_anim_offset = vec2_zero
	self.anim_len = 1

	local mask_x, mask_y, shift_offset

	local cb = function()
		if shift_offset then
			self:shift(mask_x, mask_y, -shift_offset)
		end

		if n > 0 then
			n = n - 1

			shift_offset, mask_x, mask_y = self:random_move()
			self.anim_mask_x, self.anim_mask_y = mask_x, mask_y

			self.start_anim_t = time.s() 
			self.anim_len = shuffle_speed * n*n
		else
			self.shuffling = false
		end
	end
	
	self.anim_end_callback = cb 
end

function grid:scramble()
	local n = 18 
	local moves_x = {}
	local moves_y = {}
	local moves_offset = {}

	local p = self.puzzle

	if not p.tile_w then
		puzzles.preload(p)
	end
	
	local offset, move_x, move_y
	for i=1,n do
		offset, move_x, move_y = self:random_move()	

		self:shift(move_x, move_y, -offset)

		moves_x[i] = move_x
		moves_y[i] = move_y
		moves_offset[i] = offset
	end

	self.scramble_x = moves_x
	self.scramble_y = moves_y
	self.scramble_offset = moves_offset
end

function grid:unscramble()
	self.start_anim_t = nil

	if not self.can_shuffle then
		self.must_unscramble = true
		return
	end

	local p = self.puzzle
	self.shuffling = true

	self.start_anim_t = time.s()
	self.start_anim_offset = vec2_zero
	self.end_anim_offset = vec2_zero
	self.anim_len = 1

	local n = #self.scramble_offset
	local mask_x, mask_y, offset

	local cb = function()
		if offset then
			self:shift(mask_x, mask_y, offset)
		end

		if n > 0 then
			offset, mask_x, mask_y = self.scramble_offset[n], self.scramble_x[n], self.scramble_y[n]
			n = n - 1

			self.anim_mask_x, self.anim_mask_y = mask_x, mask_y
			if mask_x then
				self.end_anim_offset = vec2(0, -offset * p.tile_h)
			else
				self.end_anim_offset = vec2(-offset * p.tile_w, 0)
			end
			self.start_anim_t = time.s()
			self.anim_len = 0.05 
		else
			self.shuffling = false
		end
	end

	self.anim_end_callback = cb
end


-- rows/columns with immovable tiles cannot be moved
function grid:can_move(move_x, move_y, tx, ty, p, mark)
	local x, y, dx, dy = 0, 0, 0, 0

	if move_x then
		y, dx = ty, 1
	else
		x, dy = tx, 1
	end
	local result = true
	while x < p.w and y < p.h do
		local t = self.state[y * p.w + x + 1] 
		if is_wall(p.solved[t]) then
			if not mark then
				return false
			else
				if self.show_wallmarks and self.wallmarks[t] == nil then
					self.wallmarks[t] = time.s()
				end
				result = false
			end
		end
		x, y = x + dx, y + dy
	end

	return result
end

function grid:touch(t, pos, touch_cb)
	local p = self.puzzle

	-- do not do anything if we're shuffling or in animation
	if self.shuffling or self.start_anim_t then
		self.blocked_touch = t
		return
	end

	-- prevent touches which started during shuffle or animation
	-- from doing anything
	if t and self.blocked_touch then
		if t.hit_time == self.blocked_touch.hit_time then
			return
		end
	end

	if not t then
		-- finger might have been lifted up, 
		-- process a move if one was made
		if self.move_offset ~= nil and length_sq(self.move_offset) > 1 then
			-- animate offset
			self.start_anim_t = time.s() 
			self.start_anim_offset = self.move_offset
			self.end_anim_offset = vec2(
				math.floor(self.move_offset.x / p.tile_w + 0.5) * p.tile_w,
				math.floor(self.move_offset.y / p.tile_h + 0.5) * p.tile_h
			)
			self.anim_len = length(self.start_anim_offset - self.end_anim_offset) 
			self.anim_len = self.anim_len / release_anim_speed

			-- perform a shift on current state
			local shift_offset
			local mask_x, mask_y = self.move_mask_x, self.move_mask_y
			if self.move_mask_x then
				shift_offset = self.end_anim_offset.y / p.tile_h
			else
				shift_offset = self.end_anim_offset.x / p.tile_w
			end
			if math.abs(shift_offset) > 0 then
				self.anim_end_callback = function()
					self:shift(mask_x, mask_y, -shift_offset)
				end
			else
				self.anim_end_callback = nil
			end
	
			-- just like we were masking the row/column user is 
			-- interacting with, mask it as 'animating' row/column
			-- now that user has made a move
			self.anim_mask_x = self.move_mask_x
			self.anim_mask_y = self.move_mask_y
			self.move_mask_x, self.move_mask_y = nil, nil

			self.move_offset = nil
		else
			local tile = self.touched_tile
			if tile and self.anim_mask_x == nil 
				and self.anim_mask_y == nil then

				-- do something with touched tile here
				if touch_cb then
					local t = p.solved[self.state[tile.y * p.w + tile.x + 1]]
					touch_cb(t)
				end
			end
		end

		self.touched_tile = nil
		self.show_wallmarks = true
		return
	end

	local abs = math.abs

	local off_x = t.hit_pos.x - t.pos.x
	local off_y = t.hit_pos.y - t.pos.y
	local move_x = self.move_mask_x == nil
	local move_y = self.move_mask_y == nil
	if move_x and move_y then
		move_x = abs(off_x) > touch_move_dist
		move_y = abs(off_y) > touch_move_dist

		if move_x and move_y then
			if abs(off_x) > abs(off_y) then
				move_y = nil
			else
				move_x = nil
			end
		end
	end

	if not (self.move_mask_x or self.move_mask_y) then
		-- touch hit pos in tile space
		local tile_pos = t.hit_pos - (pos - self.half_size)
		tile_pos.x = math.floor(tile_pos.x / p.tile_w)
		tile_pos.y = math.floor(tile_pos.y / p.tile_h)
		-- touch is outside puzzle grid, back out 
		if  tile_pos.x < 0 or tile_pos.x >= p.w or
			tile_pos.y < 0 or tile_pos.y >= p.h then
			return
		end

		if (move_x or move_y) and move_x ~= move_y then
			self.touched_tile = nil	
		
			-- also back out if row/column has immovable tiles
			if not self:can_move(move_x, move_y, tile_pos.x, tile_pos.y, p, true) then
				return
			end
		
			-- mask out moving row/column from regular rendering
			if move_x then
				self.move_mask_x = nil
				self.move_mask_y = tile_pos.y
			else
				self.move_mask_x = tile_pos.x
				self.move_mask_y = nil
			end
		end
		self.touched_tile = tile_pos
	end	

	-- calculate move offset
	if not self.move_offset then
		self.move_offset = vec2_zero 
	end

	local new_move_offset
	if move_x then
		new_move_offset = vec2(off_x, 0) 
	elseif move_y then
		new_move_offset = vec2(0, off_y)
	end

	if new_move_offset then
		local d = length_sq(new_move_offset - self.move_offset)
		if d > 0 then
			local vol = math.sqrt(d)/10
			mfx.snd_ambient('slinkt.wav', math.min(1, vol))
		end
		self.move_offset = new_move_offset
	end
end

return grid
