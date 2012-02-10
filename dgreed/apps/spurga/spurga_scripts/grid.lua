local grid = {}
grid.__index = grid

-- tweaks
-- do not move row/column if touch move distance is less than this
local touch_move_dist = 5 

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

	-- top left corner coordinates
	local cursor = pos - self.half_size

	-- draw current grid state
	local n, t
	for y = 0, p.h-1 do
		for x = 0, p.w-1 do
			n = y * p.w + x + 1
			t = self.state[n]
			if x ~= self.draw_mask_x and y ~= self.draw_mask_y then
				video.draw_rect(p.tex, layer, p.tile_src[t], cursor)
			else
				video.draw_rect(p.tex, layer, p.tile_src[t], cursor - self.move_offset)
			end
			cursor.x = cursor.x + p.tile_w
		end
		cursor.x = pos.x - self.half_size.x
		cursor.y = cursor.y + p.tile_h
	end
end

function grid:touch(t)
	self.draw_mask_x = nil
	self.draw_mask_y = nil

	if not t then
		return
	end

	local p = self.puzzle

	local off_x = t.hit_pos.x - t.pos.x
	local off_y = t.hit_pos.y - t.pos.y
	local move_x = math.abs(off_x) > touch_move_dist
	local move_y = math.abs(off_y) > touch_move_dist

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

		-- calculate move offset
		if not self.move_offset then
			self.move_offset = vec2(0, 0)
		end

		-- mask out moving row/column from regular rendering
		if move_x then
			self.draw_mask_x = nil
			self.draw_mask_y = tile_pos.y
			self.move_offset = lerp(self.move_offset, vec2(off_x, 0), 0.5) 
		else
			self.draw_mask_x = tile_pos.x
			self.draw_mask_y = nil
			self.move_offset = lerp(self.move_offset, vec2(0, off_y), 0.5)
		end
	else
		self.move_offset = nil
	end
end

return grid
