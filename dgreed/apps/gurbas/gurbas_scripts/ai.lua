ai = {
	-- tweaks
	gap_cost = 100,
	fill_cost = 10000,

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
				cost = cost + ai.gap_cost
			else
				local dy = tiles_y - y
				cost = cost + dy*dy*dy
			end
			y = y + 1
		end
	end

	-- count full lines 
	for y = tiles_y-1,1,-1 do
		local full = true
		for x = 0,tiles_x-1 do
			if is_empty(x, y) then
				full = false
				break
			end
		end
		if full then
			cost = cost - ai.fill_cost
		end
	end

	return cost
end

function ai.best_move(b)
	local orig_off, orig_rot = vec2(b.offset), b.rotation
	local best_cost = 1/0
	local best_offset, best_rot = vec2(), 0

	for rot=0,3 do
		b.rotation = rot
		b.offset.x, b.offset.y = 0, -4 
		while b.offset.x <= tiles_x and well.collide_block(b) do
				b.offset.x = b.offset.x + 1	
		end
		while true do	
			if well.collide_block(b) then
				break	
			end

			-- find dropped block y
			while b.offset.y <= tiles_y and not well.collide_block(b) do	
				b.offset.y = b.offset.y + 1
			end
			b.offset.y = b.offset.y - 1

			-- check cost, remember if it is best
			local current_cost = ai.cost(b)
			if current_cost < best_cost then
				best_cost = current_cost
				best_offset = vec2(b.offset)
				best_rot = b.rotation
			end

			b.offset.x = b.offset.x + 1
		end
	end

	b.offset, b.rotation = orig_off, orig_rot
	return {
		offset = best_offset,
		rotation = best_rot
	}
end

function ai.move(b) 
	if ai.target == nil then
		ai.target = ai.best_move(b)
	end

	if rand.int(0, 2) == 0 then
		if ai.target.offset.x > b.offset.x then
			b.offset.x = b.offset.x + 1
		elseif ai.target.offset.x < b.offset.x then
			b.offset.x = b.offset.x - 1
		end
	end

	if rand.int(0, 2) == 0 then
		if ai.target.rotation > b.rotation then
			b.rotation = b.rotation + 1
		elseif ai.target.rotation < b.rotation then
			b.rotation = b.rotation - 1
		end
	end
end
