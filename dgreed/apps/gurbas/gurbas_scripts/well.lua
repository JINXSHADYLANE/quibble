well = {
	layer = 5,
	state = nil,
	shapes = nil,
	vertices = {
		vec2(0, 0),
		vec2(0, screen.b),
		vec2(screen.r, screen.b),
		vec2(screen.r, 0)
	},

	-- images
	img_block = nil
}

function well.init()
	well.img_block = tex.load(pre..'by.png')	
	well.reset()
end

function well.reset()
	well.state = {} 
end

function well.draw()
	for y = 0,tiles_y-1 do
		for x = 0,tiles_x-1 do
			if well.state[widx(x, y)] ~= nil then
				local dest = vec2(x * tile_size, y * tile_size) 
				dest = rect(dest.x, dest.y, 
					dest.x + tile_size, dest.y + tile_size)
				video.draw_rect(well.img_block, well.layer, dest)	
			end
		end
	end
end

-- returns true if block collides with well
function well.collide_block(b)
	local parts = b.parts()
	for i,part in ipairs(parts) do
		if part.x < 0 or part.x >= tiles_x or part.y >= tiles_y then
			return true
		end
		if well.state[widx(part.x, part.y)] ~= nil then
			return true 
		end
	end
	return false 
end

-- 'bakes' block into well
function well.put_block(b)
	for i,part in ipairs(b.parts()) do		
		if part.x >= 0 and part.x < tiles_x then
			if part.y >= 0 and part.y < tiles_y then
				well.state[widx(part.x, part.y)] = true
			end
		end
	end
end

-- raycast against all blocks in well
function well.raycast(s, e)
	local min_sq_dist = length_sq(s - e) 
	local min_hitp = e

	if e.x > screen.r then
		e.x = screen.r
	end
	if e.x < 0 then
		e.x = 0
	end
	if e.y > screen.b then
		e.y = screen.b
	end

	for pos,state in pairs(well.state) do
		if state ~= nil then
			local p = inv_widx(pos)
			local tl = p * tile_size
			local br = tl + vec2(tile_size, tile_size)
			local r = rect(tl.x, tl.y, br.x, br.y)
			local hitp = rect_raycast(r, s, e)
			local sq_dist = length_sq(hitp - s)
			if sq_dist < min_sq_dist then
				min_sq_dist = sq_dist
				min_hitp = hitp
			end
		end
	end
	return min_hitp 
end

function well.close()
	tex.free(well.img_block)
end

