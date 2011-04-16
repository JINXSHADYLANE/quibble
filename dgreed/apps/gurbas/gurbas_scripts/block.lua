
block = {
	-- local block coordinates
	shape = 0,
	-- coordinates in map tiles
	offset = vec2(),
	-- time when offset position was last changed
	off_t = 0,
	-- how much time tile animation takes
	fall_time = 440,
	-- 1=left, 2=up, 3=right, 4=down, 0=none
	animate = 0,
	next_anim = 0,
	-- 0=none, 1=90, 2=180, 3=270 degrees
	rotation = 0,

	layer = 1
}

-- block types in local coordinates
block.shapes = {
	{ vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 2) },
	{ vec2(1, 0), vec2(0, 1), vec2(1, 1), vec2(2, 1) }
}

-- shows directions of block movements
block.moves = {
	vec2(-1, 0), vec2(0, -1), vec2(1, 0), vec2(0, 1) 
}

function block.reset()
	block.offset = vec2(0, 0)
	block.off_t = time.ms()
	-- TODO: pick random block
	block.shape = block.shapes[1]
	block.animate = 4
	block.next_anim = 4
end

-- TODO: remove init() and close()
function block.init()
	block.tex = tex.load(pre..'by.png')	
	block.reset()
end

function block.close()
	tex.free(block.tex)
end

-- returns block tiles in well coordinates
function block.parts()
	list = {}
	for id, tile in ipairs(block.shape) do
		list[id] = tile + block.offset
	end
	return list
end

function block.update()
	-- Update keyboard arrows
	if key.down(key._left) then
		block.next_anim = 1
	elseif key.down(key._right) then
		block.next_anim = 3
	elseif key.down(key._up) then
		block.rotation = block.rotation + 1
		if block.rotation == 4 then 
			block.rotation = 0 
		end
	end

	-- check if we can change block position
	if (time.ms() - block.off_t) / block.fall_time >= 1.0 then
		if block.animate ~= 0 then
			block.offset = block.offset + block.moves[block.animate]
		end
	
		-- check block collisions with well
		block.offset.y = block.offset.y + 1
		local collide_down = well.collide_block(block)
		block.offset.y = block.offset.y - 1
		
		block.offset.x = block.offset.x - 1
		local collide_left = well.collide_block(block)
		block.offset.x = block.offset.x + 2
		local collide_right = well.collide_block(block)
		block.offset.x = block.offset.x - 1

		-- respond in case of collisions
		if collide_down then			
			well.put_block(block)
			block.reset()
			return
		end
		if (collide_left and block.next_anim == 1) or 
			(collide_right and block.next_anim == 3) then
			block.next_anim = 4
		end

		block.animate = block.next_anim
		block.next_anim = 4
		block.off_t = time.ms()
	end
end

function block.draw()
	for id, tile in ipairs(block.parts()) do
		-- count tile position in pixels
		if block.animate ~= 0 then
			tile = lerp(tile, tile + block.moves[block.animate],
				 (time.ms() - block.off_t) / block.fall_time)
		end

		video.draw_rect(block.tex, block.layer, tile * tile_size)
	end
end

