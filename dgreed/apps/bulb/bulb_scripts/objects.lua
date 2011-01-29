
dofile(src..'quadtree.lua')

objects = {
	layer = 0,
	crate_img = nil,
	battery_img = nil,
	beacon_img = nil,


	crates = {},
	beacons = {},

	-- all other static objs are here
	list = {}
}

function shrinked_bbox(bbox, r)
	return rect(bbox.l + r, bbox.t + r, bbox.r - r, bbox.b - r)
end

function objects.init()
	objects.crate_img = tex.load(pre..'crate.png')
	objects.battery_img = tex.load(pre..'battery.png')
	objects.beacon_img = tex.load(pre..'beacon.png')
end

function objects.close()
	tex.free(objects.crate_img)
	tex.free(objects.battery_img)
	tex.free(objects.beacon_img)
end

function objects.add(id, pos)
	local r = rect(pos.x, pos.y, pos.x+64, pos.y+64)

	-- crate
	if id == 2 then	
		table.insert(objects.crates, {rect=r, id=2})
	end

	-- battery
	if id == 3 then
		table.insert(objects.list, {rect=r, id=3, taken=false})
	end

	-- beacon
	if id == 4 then
		table.insert(objects.beacons, {rect=r, id=4})
		table.insert(objects.list, {rect=r, id=4})
	end
end

function objects.seal()
	objects.qtree = Quadtree:new(objects.list)
end

function objects.draw()
	local camera_rect = tilemap.screen2world(level, screen, screen)	

	-- crates
	for i, obj in ipairs(objects.crates) do
		if rect_rect_collision(camera_rect, obj.rect) then
			local r = tilemap.world2screen(level, screen, obj.rect)
			video.draw_rect(objects.crate_img, objects.layer, r)
		end
	end

	-- static objs
	local vis_objs = objects.qtree:query(camera_rect)
	for i, obj in ipairs(vis_objs) do
		local r = tilemap.world2screen(level, screen, obj.rect)	
		if obj.id == 3 and obj.taken == false then
			video.draw_rect(objects.battery_img, objects.layer, r)
		end
		if obj.id == 4 then
			video.draw_rect(objects.beacon_img, objects.layer, r)
		end
	end
end

function objects.interact(player_bbox, player_offset)
	local res = true

	-- pick up batteries
	local hit_objs = objects.qtree:query(player_bbox)
	for i, obj in ipairs(hit_objs) do
		if obj.id == 3 and obj.taken == false then
			local r = tilemap.world2screen(level, screen, obj.rect)
			r = shrinked_bbox(r, 10)
			if rect_rect_collision(player_bbox, obj.rect) then
				-- pick up battery!
				obj.taken = true
				robo.energy = robo.energy + robo.battery_juice
				robo.energy = math.min(1, robo.energy)
			end
		end
	end

	-- abort if hitting multiple crates
	local hits = 0
	for i, crate in ipairs(objects.crates) do
		if rect_rect_collision(player_bbox, crate.rect) then
			hits = hits + 1
		end
	end

	if hits == 0 then
		return true
	end
	if hits > 1 then
		return false
	end

	-- check for crate hits
	for i, crate in ipairs(objects.crates) do
		if rect_rect_collision(player_bbox, crate.rect) then
			local new_rect, offset = rect(crate.rect), nil 
			if player_offset.x < 0 then
				new_rect.r = player_bbox.l-1
				new_rect.l = new_rect.r - 64
			end
			if player_offset.x > 0 then
				new_rect.l = player_bbox.r+1
				new_rect.r = new_rect.l + 64
			end
			if player_offset.y < 0 then
				new_rect.b = player_bbox.t-1
				new_rect.t = new_rect.b - 64
			end
			if player_offset.y > 0 then
				new_rect.t = player_bbox.b+1
				new_rect.b = new_rect.t + 64
			end
			offset = vec2(
				new_rect.l - crate.rect.l,
				new_rect.t - crate.rect.t
			)	

			-- prevent sudden jumps
			if math.abs(offset.x) > 20 then
				offset.x = 0
			end
			if math.abs(offset.y) > 20 then
				offset.y = 0
			end

			if offset.x ~= 0 or offset.y ~= 0 then
				local small_bbox = shrinked_bbox(new_rect, 2)
				-- check for collision with other crates
				for j, ncrate in ipairs(objects.crates) do
					if i ~= j and rect_rect_collision(small_bbox, ncrate.rect) then
						offset = vec2()
						res = false
						break
					end
				end
			end

			if rect_rect_collision(player_bbox, new_rect) then
				offset = vec2()
				res = false
			end

			if offset.x ~= 0 or offset.y ~= 0 then
				-- check for collision with walls
				local dx = tilemap.collide_swept(level, crate.rect, vec2(offset.x, 0))
				crate.rect.l = crate.rect.l + dx.x
				crate.rect.r = crate.rect.r + dx.x

				local dy = tilemap.collide_swept(level, crate.rect, vec2(0, offset.y))
				crate.rect.t = crate.rect.t + dy.y
				crate.rect.b = crate.rect.b + dy.y

				if dx.x ~= player_offset.x or dy.y ~= player_offset.y then		
					res = false
				end
			end
		end
	end
	return res
end
