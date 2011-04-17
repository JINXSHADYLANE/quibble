well = {
	layer = 5,
	state = nil,
	shapes = nil,
	drop_anim_t = nil,
	drop_anim_len = 1000,
	full_lines = {},
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
	well.img_block = tex.load(pre..'tile.png')	
	well.reset()
end

function well.reset()
	well.state = {} 
end

function well.animates()
	if well.drop_anim_t ~= nil then
		return true
	end
	return false
end

function well.clear_filled()
	for i = 1,#well.full_lines do
		local fy = well.full_lines[#well.full_lines - i + 1]
		for y = fy,0,-1 do
			for x = 0,tiles_x-1 do
				well.state[widx(x, y)] = well.state[widx(x, y-1)]
			end
		end
	end
	well.full_lines = {}
end

function well.draw()
	if well.animates() then
		local t = (time.ms() - well.drop_anim_t) / well.drop_anim_len 
		if t > 1 then
			well.clear_filled()
			well.drop_anim_t = nil
		else
			well.draw_fadeout(t)
			return
		end
	end

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

function well.draw_fadeout(t)
	local col = rgba(1, 1, 1, 1 - t)
	for y = 0,tiles_y-1 do
		local fade = false
		for i,f in ipairs(well.full_lines) do
			if f == y then
				fade = true
			end
		end

		for x = 0,tiles_x-1 do
			if well.state[widx(x, y)] ~= nil then
				local dest = vec2(x * tile_size, y * tile_size) 
				dest = rect(dest.x, dest.y, 
					dest.x + tile_size, dest.y + tile_size)
				if fade then
					video.draw_rect(well.img_block, well.layer, dest, col)
				else
					video.draw_rect(well.img_block, well.layer, dest)	
				end
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

function well.check_lines()
	for y=tiles_y-1,0,-1 do
		local full = true
		for x=0,tiles_x-1 do
			if well.state[widx(x, y)] == nil then
				full = false
			end
		end
		if full then
			table.insert(well.full_lines, y)
		end
	end

	return #well.full_lines > 0
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
	
	if well.check_lines() then
		well.drop_anim_t = time.ms()
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

function well.collide_rect(r)
	for pos,state in pairs(well.state) do
		if state ~= nil then
			local p = inv_widx(pos)
			local tl = p * tile_size
			local br = tl + vec2(tile_size, tile_size)
			local well_rect = rect(tl.x, tl.y, br.x, br.y)
			if rect_rect_collision(well_rect, r) then
				return true
			end	
		end
	end
	return false
end

function well.did_lose()
	for x = 0,tiles_x-1 do
		if well.state[widx(x, 2)] ~= nil then
			return true
		end
	end
	return false
end

function well.close()
	tex.free(well.img_block)
end

