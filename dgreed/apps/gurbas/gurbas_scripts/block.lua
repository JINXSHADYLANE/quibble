
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
	-- animation offset
	anim_offset = vec2()
}

function block.init()
	block.tex = tex.load(pre..'by.png')	
	block.reset()

end

function block.close()
	tex.free(block.tex)
end

function block.update()
	-- playing with tiles
	if key.pressed(key._up) then
		block.shape = block_shapes[2]
	end
	if key.pressed(key._down) then
		block.shape = block_shapes[1]
	end
end

function block.draw()
	for id, tile in ipairs(block.shape) do
		-- count tile position in pixels
		local dest = (block.offset + tile) * tile_size
		video.draw_rect(block.tex, 1, dest)
	end
 
end

function block.reset()
	block.offset = vec2(0, 0)
	block.anim_offset = vec2(0, 0)
	-- pick random block
end

