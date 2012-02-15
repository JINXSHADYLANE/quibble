local grid = {}
grid.__index = grid

-- tweaks
-- do not move row/column if touch move distance is less than this
local touch_move_dist = 5 
-- how fast is sliding animation, when finger is lifted up (px/s)
local release_anim_speed = 80

function grid:new(puzzle) 
	local obj = {['puzzle'] = puzzle}
	setmetatable(obj, self) 
	obj:reset_state()
	return obj
end

function grid:reset_state()
	local p = self.puzzle
	self.state = {}
	for i = 1, p.w * p.h do
		table.insert(self.state, i)
	end

	self:shuffle(300)
end

function grid:precalc_src()
	local p = self.puzzle

	local width, height = width(p.src), height(p.src)

	-- 0 width or height means 'up to the edge'
	if width == 0 or height == 0 then
		local tex_size = tex.size(p.tex)
		if width == 0 then
			width = tex_size.x - p.src.l
		end
		if height == 0 then
			height = tex_size.y - p.src.t
		end
	end

	p.tile_w = width / p.w
	p.tile_h = height / p.h

	-- precalc source rect for each tile
	p.tile_src = {}
	for y = 0, p.h-1 do
		for x = 0, p.w-1 do
			local n = y * p.w + x

			p.tile_src[n+1] = rect(
				p.src.l + x * p.tile_w,
				p.src.t + y * p.tile_h, 
				p.src.l + (x+1) * p.tile_w,
				p.src.t + (y+1) * p.tile_h
			)
		end
	end

	self.half_size = vec2(width / 2, height / 2)
end

-- when we're shifting/animating a single tile might be visible in two
-- places at once, this draws such tile
function grid:draw_shifted_tile(tex, layer, src, x, y, offset, p)
	local abs_offset = vec2(math.abs(offset.x), math.abs(offset.y))
	local steps = vec2(
		math.floor(abs_offset.x / p.tile_w),
		math.floor(abs_offset.y / p.tile_h)
	)
	local top_left = scr_size * 0.5 - self.half_size

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
		ax = math.fmod(p.w + ax + dx, p.w)
		while p.solved[self.state[ay * p.w + ax + 1]] == '@' do
			ax, ay = math.fmod(p.w + ax + dx, p.w), math.fmod(p.h + ay + dy, p.h)
		end
	end
	for i=1,steps.y do
		ay = math.fmod(p.h + ay + dy, p.h)
		while p.solved[self.state[ay * p.w + ax + 1]] == '@' do
			ax, ay = math.fmod(p.w + ax + dx, p.w), math.fmod(p.h + ay + dy, p.h)
		end
	end

	local bx = math.fmod(p.w + ax + dx, p.w)
	local by = math.fmod(p.h + ay + dy, p.h)
	while p.solved[self.state[by * p.w + bx + 1]] == '@' do
		bx, by = math.fmod(p.w + bx + dx, p.w), math.fmod(p.h + by + dy, p.h)
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
		video.draw_rect(tex, layer, src, pos_a - real_offset)
	else
		local alpha = clamp(0, 1, math.max(
			math.abs(real_offset.x) / p.tile_h, 
			math.abs(real_offset.y) / p.tile_w
		))

		local ghost_color = rgba(1, 1, 1, alpha)
		local color = rgba(1, 1, 1, 1 - alpha)

		-- draw with ghost
		video.draw_rect(tex, layer, src, pos_a - real_offset, color)
		video.draw_rect(tex, layer, src, pos_b + neg_offset, ghost_color)
	end
end

function grid:draw(pos, layer)
	local p = self.puzzle
	
	-- cache texture handle and source rectangle
	if not p.tex then
		p.tex, p.src = sprsheet.get(p.spr)
	end

	-- cache tile source rectangles
	if not p.tile_src then
		self:precalc_src()
	end

	-- precalc animation parameter
	local t = nil
	local anim_offset = nil
	if self.start_anim_t then
		local t = (time.s() - self.start_anim_t) / self.anim_len
		if t >= 1 then
			-- end anim
			self.start_anim_t = nil
			self.anim_mask_x = nil
			self.anim_mask_y = nil
			if self.anim_end_callback then
				self.anim_end_callback()
			end
		else
			anim_offset = smoothstep(
				self.start_anim_offset, 
				self.end_anim_offset, t
			)
		end
	end

	-- top left corner coordinates
	local cursor = pos - self.half_size

	-- draw current grid state
	local t
	for y = 0, p.h-1 do
		for x = 0, p.w-1 do
			t = self.state[y * p.w + x + 1] 
			if p.solved[t] == '@' then
				-- portal tile
				video.draw_rect(p.tex, layer-1, p.tile_src[t], cursor)
			elseif x ~= self.move_mask_x and y ~= self.move_mask_y then
				if x ~= self.anim_mask_x and y ~= self.anim_mask_y then
					-- plain tile
					video.draw_rect(p.tex, layer, p.tile_src[t], cursor)
				else
					if anim_offset == nil then
						anim_offset = vec2(0, 0)
					end
					-- animated tile
					self:draw_shifted_tile(
						p.tex, layer, p.tile_src[t], 
						x, y, anim_offset, p
					)
				end
			else
				-- moved tile
				self:draw_shifted_tile(
					p.tex, layer, p.tile_src[t], 
					x, y, self.move_offset, p
				)
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
		if p.solved[t] ~= '@' then
			table.insert(prev, t)
		end
		cx, cy = cx + dx, cy + dy
	end

	-- make a new, rotated row/column table
	local new = {}
	local off = math.fmod(offset, #prev)
	for i = 1, #prev do
		new[i] = prev[math.fmod(#prev - off + (i-1), #prev) + 1]
	end
	
	-- update current state with rotated row/column
	local i = 1
	cx, cy = x, y
	while cx < p.w and cy < p.h do
		local idx = cy * p.w + cx + 1
		local t = self.state[idx]
		if p.solved[t] ~= '@' then
			self.state[idx] = new[i]
			i = i + 1
		end
		cx, cy = cx + dx, cy + dy
	end
end

function grid:shuffle(n)
	local p = self.puzzle
	self.shuffling = true

	self.start_anim_t = time.s()
	self.start_anim_offset = vec2(0, 0)
	self.end_anim_offset = vec2(0, 0)
	self.anim_len = 1

	local mask_x, mask_y, shift_offset

	local cb = function()
		if shift_offset then
			self:shift(mask_x, mask_y, -shift_offset)
		end

		if n > 0 then
			n = n - 1

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

			self.anim_mask_x, self.anim_mask_y = mask_x, mask_y

			self.start_anim_offset = vec2(0, 0)
			self.start_anim_t = time.s()
			self.anim_len = 0.000001 * n*n
		else
			self.shuffling = false
		end
	end
	
	self.anim_end_callback = cb 
end

-- rows/columns with immovable tiles cannot be moved
function grid:can_move(move_x, move_y, tx, ty, p)
	local x, y, dx, dy = 0, 0, 0, 0

	if move_x then
		y, dx = ty, 1
	else
		x, dy = tx, 1
	end
	while x < p.w and y < p.h do
		local t = self.state[y * p.w + x + 1] 
		if p.solved[t] == '#' then
			return false
		end
		x, y = x + dx, y + dy
	end

	return true
end

function grid:touch(t)
	local p = self.puzzle

	if self.shuffling then
		return
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
		end

		return
	end

	local off_x = t.hit_pos.x - t.pos.x
	local off_y = t.hit_pos.y - t.pos.y
	local move_x = self.move_mask_x == nil
	local move_y = self.move_mask_y == nil
	if move_x and move_y then
		move_x = math.abs(off_x) > touch_move_dist
		move_y = math.abs(off_y) > touch_move_dist
	end

	if not (self.move_mask_x or self.move_mask_y) then
		if (move_x or move_y) and move_x ~= move_y then
			-- touch hit pos in tile space
			local tile_pos = t.hit_pos - (scr_size / 2 - self.half_size)
			tile_pos.x = math.floor(tile_pos.x / p.tile_w)
			tile_pos.y = math.floor(tile_pos.y / p.tile_h)

			-- touch is outside puzzle grid, back out 
			if  tile_pos.x < 0 or tile_pos.x >= p.w or
				tile_pos.y < 0 or tile_pos.y >= p.h then
				return
			end

			-- also back out if row/column has immovable tiles
			if not self:can_move(move_x, move_y, tile_pos.x, tile_pos.y, p) then
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
	end	

	-- calculate move offset
	if not self.move_offset then
		self.move_offset = vec2(0, 0)
	end

	if move_x then
		self.move_offset = vec2(off_x, 0) 
	elseif move_y then
		self.move_offset = vec2(0, off_y)
	end
end

return grid
