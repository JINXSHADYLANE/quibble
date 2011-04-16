ai = {
	-- tweaks
	gap_cost = 100,

	-- state
	target = nil
}	

function ai.cost(b)
	local cost = 0
	local parts = b.parts()

	local is_empty = function(x, y)
		if well.state[widx(x, y)] ~= nil then
			return false
		end
		for i,part in ipairs(parts) do
			if part.x == x and part.y == y then
				return false
			end
		end
		return true
	end

	for x = 0,tiles_x-1 do
		-- skip straight to first block
		local y = 0
		while y < tiles_y and is_empty(x, y) do
			y = y + 1
		end

		-- count weight for column
		while y < tiles_y do
			if is_empty(x, y) then
				cost = cost + gap_cost
			else
				local dy = tiles_y - y - 1
				cost = cost + dy*dy*dy
			end
			y = y + 1
		end
	end

	return cost
end

function ai.best_move(b)
	local orig_off, orig_rot = b.offset, b.rotation
	local best_cost = 1/0
	local best_offset, best_rot = vec2(), 0

	for rot=0,3 do
		b.rotation = rot
		local x, y = 0, tiles_y
		while true do	
			-- check if block can even be at this x
			b.offset.x = x
			b.offset.y = -4
			x = x + 1
			if well.collide_block(b) then
				break
			end

			-- find dropped block y
			while y >= 0 and well.collide_block(b) do	
				b.offset.y = y
				y = y - 1
			end

			-- check cost, remember if it is best
			local current_cost = ai.cost(b)
			if current_cost < best_cost then
				best_cost = current_cost
				best_offset = vec2(b.offset)
				best_rot = b.rot
			end
		end
	end

	b.offset, b.rot = orig_off, orig_rot
	return {
		offset = best_offset,
		rotation = best_rotation
	}
end

function ai.move(b) 
	if ai.target == nil then
		ai.target = ai.best_move(b)
	end

	-- stochasticly mutate offset to target state
	if rand.int(0, 3) == 2 then
		if ai.target.offset.x > b.offset.x then
			b.offset.x = b.offset.x + 1
		elseif ai.target.offset.x < b.offset.x then
			b.offset.x = b.offset.x - 1
		end
	else
		if rand.int(0, 3) == 2 then
			if ai.target.rot > b.rot then
				b.rot = b.rot + 1
			elseif ai.target.rot < b.rot then
				b.rot = b.rot - 1
			end
		end
	end
end
