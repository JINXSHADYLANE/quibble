
block = {
	-- local block coordinates
	shape = 0,
	-- coordinates in map tiles
	offset = vec2(),
	-- time when offset position was last changed
	off_t = 0,
	-- how much time tile animation takes
	fall_time = 200,
	-- 0=none, 1=90, 2=180, 3=270 degrees
	rotation = 0,
	layer = 1
}

-- block types in local coordinates
block.shapes = {
	{ vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 2) },
	{ vec2(1, 0), vec2(0, 1), vec2(1, 1), vec2(2, 1) },
	{ vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(2, 1) },
	{ vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1) },
	{ vec2(0, 0), vec2(0, 1), vec2(0, 2), vec2(0, 3) }
}

function block.reset()
	block.shape = block.shapes[rand.int(1, #block.shapes + 1)]
	block.rotation = 0
	
	block.offset = vec2(3, -2)
	block.off_t = time.ms()
end

function block.init()
	block.tex = tex.load(pre..'tile.png')	
	block.reset()
end

function block.close()
	tex.free(block.tex)
end

-- returns block tiles in well coordinates
function block.parts()
	local shapes = {}
	for key, value in ipairs(block.shape) do
		shapes[key] = vec2(value)
	end

	block.rotate(shapes, block.rotation)
	
	list = {}
	for id, tile in ipairs(shapes) do
		list[id] = tile + block.offset
	end
	return list
end

function block.vis_parts()
	list = {}
	for id, tile in ipairs(block.parts()) do
		-- count tile position in pixels
		list[id] = lerp(tile, tile + vec2(0, 1), 
			(time.ms() - block.off_t) / block.fall_time)
	end
	return list
end

function block.rotate(blk, rotation)
	if rotation % 4 == 0 then
		return 
	end

	for id = 1, #blk do
		blk[id].x, blk[id].y = 1 - blk[id].y, blk[id].x
	end
	return block.rotate(blk, rotation - 1)
end

function block.update()
	-- check if we can change block position
	if (time.ms() - block.off_t) / block.fall_time >= 1.0 then
		block.offset = block.offset + vec2(0, 1)
	
		-- check block collisions with well
		block.offset.y = block.offset.y + 1
		local collide_down = well.collide_block(block)
		block.offset.y = block.offset.y - 1
		
		-- respond in case of collisions
		if collide_down then			
			well.put_block(block)
			block.reset()
			ai.target = nil
			return
		end

		block.off_t = time.ms()

		ai.move(block)
	end
end

function block.draw()
	for id, tile in ipairs(block.parts()) do
		-- count tile position in pixels
		tile = lerp(tile, tile + vec2(0, 1),
			(time.ms() - block.off_t) / block.fall_time)

		local dest = tile * tile_size
		dest = rect(dest.x, dest.y, dest.x + tile_size, dest.y + tile_size)
		video.draw_rect(block.tex, block.layer, dest)
	end
end

function block.raycast(s, e)
	local min_sq_dist = length_sq(s - e)
	local min_hitp = e

	for i,r in ipairs(block.vis_parts()) do
		r = r * tile_size
		local rec = rect(r.x, r.y, r.x + tile_size, r.y + tile_size)
		local hitp = rect_raycast(rec, s, e)
		local sq_dist = length_sq(hitp - s)
		if sq_dist < min_sq_dist then
			min_sq_dist = sq_dist
			min_hitp = hitp
		end
	end

	return min_hitp
end

function block.collide_rect(r)
	local parts = block.vis_parts()
	for i,p in ipairs(parts) do
		local pos = p * tile_size
		local block_rect = rect(pos.x, pos.y, pos.x + tile_size, pos.y + tile_size)
		if rect_rect_collision(block_rect, r) then
			return true
		end
	end
	return false
end

