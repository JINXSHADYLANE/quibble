well = {
	layer = 5,
	state = nil,
	shapes = nil,

	-- images
	img_block = nil
}

function well.init()
	well.img_block = tex.load(pre..'by.png')	
	well.reset()
end

function well.reset()
	well.state = {} 
	for x = 0,tiles_x-1 do
		well.state[widx(x, tiles_y-1)] = true	
	end
end

function well.draw()
	for y = 0,tiles_y-1 do
		for x = 0,tiles_x-1 do
			if well.state[widx(x, y)] ~= nil then
				local dest = vec2(x * tile_size, y * tile_size) 
				video.draw_rect(well.img_block, well.layer, dest)	
			end
		end
	end
end

-- returns true if block collides with well
function well.collide_block(b)
	for i,part in ipairs(b.shape) do
		if well.state[widx(part[0], part[1])] ~= nil then
			return true 
		end
	end
	return false 
end

-- 'bakes' block into well
function well.put_block(b)
	for i,part in ipairs(b.shape) do
		local pos = part + b.offset		
		if pos.x >= 0 and pos.x < tiles_x then
			if pos.y >= 0 and pos.y < tiles_y then
				well.state[widx(pos.x, pos.y)] = true
			end
		end
	end
end

-- raycast against all blocks in well
function well.raycast(s, e)
	local min_sq_dist = 1/0
	local min_hitp = e
	for pos,state in pairs(well.state) do
		if state ~= nil then
			local tl = pos * tile_size
			local br = tl + tile
			local r = rect(tl.x, tl.y, br.x, br.y)
			local hitp = rect_raycast(r, s, e)
			local sq_dist = length_sq(hitp - s)
			if sq_dist < min_sq_dist then
				sq_dist = min_sq_dist
				min_hitp = hitp
			end
		end
	end
	return e
end

function well.close()
	tex.free(well.img_block)
end

