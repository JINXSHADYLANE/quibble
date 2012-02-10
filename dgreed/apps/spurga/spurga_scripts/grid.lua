local grid = {}
grid.__index = grid

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
			video.draw_rect(p.tex, layer, p.tile_src[t], cursor)
			cursor.x = cursor.x + p.tile_w
		end
		cursor.x = pos.x - self.half_size.x
		cursor.y = cursor.y + p.tile_h
	end
end

return grid
