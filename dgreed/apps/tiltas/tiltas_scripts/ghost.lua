local ghost = {}

function ghost.make(p, color)
	local g = {}
	for i=1,16 do
		g[i] = {
			pos = p + rand_vec2(0.2),
			target_pos = p + rand_vec2(0.2),
			weight = rand.float(0.05, 0.2),
			size = rand.float(0.4, 0.7),
			rand = rand.float(0, 1) 
		}
	end
	g.color = color
	return g
end

function ghost.move(g, delta)
	for i, speck in ipairs(g) do
		speck.target_pos = speck.target_pos + delta
	end
end

function ghost.draw(g, t)
	for i, speck in ipairs(g) do
		-- update
		speck.pos = lerp(speck.pos, speck.target_pos, speck.weight)
		local ts = 2 + speck.rand / 4
		local p = speck.pos + vec2(math.sin(t*ts + speck.rand), math.cos(t - speck.rand))/8
		p = world2screen(p)
		sprsheet.draw_centered('blob_big', ghost_layer, p, 0, speck.size, g.color)
	end
end


return ghost
