
-- block types in local coordinates
block_shapes = {
	{ vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 2) },
	{ vec2(1, 0), vec2(0, 1), vec2(1, 1), vec2(2, 1) }
}

block = {
	-- local block coordinates
	shape = block_shapes[1],
	-- coordinates in map tiles
	offset = vec2(),
	-- time when offset position was last changed
	off_t = 0,
	-- how much time tile animation takes
	fall_time = 240,
	-- 1=left, 2=up, 3=right, 4=down, 0=none
	animate = 0
}

function block.init()
	block.tex = tex.load(pre..'by.png')	
	block.reset()

end

function block.close()
	tex.free(block.tex)
end

function block.update()
	-- Update keyboard arrows
	if key.down(key._left) then
		block.animate = 1
		block.off_t = time.ms()
	elseif key.down(key._up) then
		block.animate = 2
		block.off_t = time.ms()
	elseif key.down(key._right) then
		block.animate = 3
		block.off_t = time.ms()
	elseif key.down(key._down) then
		block.animate = 4
		block.off_t = time.ms()
	end

	-- check if we can change block position
	if (time.ms() - block.off_t) / block.fall_time >= 1.0 then
		if block.animate == 1 then
			block.offset.x = block.offset.x - 1
		elseif block.animate == 2 then
			block.offset.y = block.offset.y - 1
		elseif block.animate == 3 then
			block.offset.x = block.offset.x + 1
		elseif block.animate == 4 then
			block.offset.y = block.offset.y + 1
		end

		block.animate = 0
		block.off_t = time.ms()
	end

end

function block.draw()
	for id, tile in ipairs(block.shape) do
		-- count animation offset between two fixed positions
		local t = (time.ms() - block.off_t) / block.fall_time
		
		-- count tile position in pixels
		local dest = (block.offset + tile)

		if block.animate == 1 then
			dest.x = lerp(dest.x, dest.x - 1, t)
		elseif block.animate == 2 then
			dest.y = lerp(dest.y, dest.y - 1, t)
		elseif block.animate == 3 then
			dest.x = lerp(dest.x, dest.x + 1, t)
		elseif block.animate == 4 then
			dest.y = lerp(dest.y, dest.y + 1, t)
		end

		video.draw_rect(block.tex, 1, dest * tile_size)
	end
 
end

function block.reset()
	block.offset = vec2(0, 0)
	block.off_t = time.ms()
	-- pick random block
end

