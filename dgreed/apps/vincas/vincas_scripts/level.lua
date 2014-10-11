local level = {}

function level.generate(width, height, texture)
	local tm = tilemap.new(8, 8, width, height, 1)
	tilemap.set_tileset(tm, 0, texture)

	for i=0,height-1 do
		local left = rand.int(1,5)
		local right = rand.int(1,5)

		for j=0,left-1 do
			tilemap.set_tile(tm, j, i, 0, 0, rand.int(4,8))
			tilemap.set_collision(tm, j, i, true)
		end
		for j=1,right do
			tilemap.set_tile(tm, width-j, i, 0, 0, rand.int(4,8))
			tilemap.set_collision(tm, width-j, i, true)
		end
	end

	return tm
end

return level
