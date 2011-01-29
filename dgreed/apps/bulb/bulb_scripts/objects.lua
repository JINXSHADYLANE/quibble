
dofile(src..'quadtree.lua')

objects = {
	layer = 0,
	crate_img = nil,
	battery_img = nil,
	beacon_img = nil,
	button_img = nil,

	-- srcs
	normal_battery = rect(0, 0, 64, 64),
	glow_battery = rect(64, 0, 128, 64),
	src_button_on = rect(0, 0, 64, 64),
	src_button_off = rect(64, 0, 128, 64),


	crates = {},
	beacons = {},
	buttons = {},

	-- all other static objs are here
	list = {}
}

function center(r)
	return vec2((r.l + r.r) / 2, (r.t + r.b) / 2)
end

function shrinked_bbox(bbox, r)
	return rect(bbox.l + r, bbox.t + r, bbox.r - r, bbox.b - r)
end

function objects.init()
	objects.button_img = tex.load(pre..'button.png')
	objects.battery_img = tex.load(pre..'battery.png')
	objects.crate_img = tex.load(pre..'crate.png')
	objects.beacon_img = tex.load(pre..'beacon.png')
end

function objects.close()
	tex.free(objects.crate_img)
	tex.free(objects.battery_img)
	tex.free(objects.beacon_img)
	tex.free(objects.button_img)
end

function objects.add(id, pos)
	local r = rect(pos.x, pos.y, pos.x+64, pos.y+64)
	local p = center(r)
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
		table.insert(objects.beacons, {pos=p, rect=r, id=4, has_btn=false})
		table.insert(objects.list, {pos=p, rect=r, id=4})
	end

	-- button
	if id == 5 then
		table.insert(objects.buttons, {pos=p, rect=r, id=5, state=false})
		table.insert(objects.list, objects.buttons[#objects.buttons])
	end
end

function objects.seal()
	objects.qtree = Quadtree:new(objects.list)

	-- find closest unused beacon for each button 
	for i,b in ipairs(objects.buttons) do
		local min_d, min_l = 100000, nil
		for j,l in ipairs(objects.beacons) do
			local len = length(b.pos - l.pos)
			if min_d > len and not l.has_btn then
				min_d = len
				min_l = l
			end
		end

		min_l.has_btn = true
		b.lights = {min_l}
		min_l.inten = 0
	end
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
			video.draw_rect(objects.battery_img, objects.layer, objects.normal_battery, r)
			-- glow-in-the-dark batteries
			video.draw_rect(objects.battery_img, 3, objects.glow_battery, r)
		end
		if obj.id == 4 then
			local p = center(r)		
			video.draw_rect_centered(objects.beacon_img, objects.layer, p, 0.0, 1.5)
		end
		if obj.id == 5 then
			local src = objects.src_button_off
			if obj.state == true then
				src = objects.src_button_on
			end
			video.draw_rect(objects.button_img, objects.layer, src, r)
		end
	end
end

function objects.interact(player_bbox, player_offset)
	local res = true

	-- buttons
	for i,b in ipairs(objects.buttons) do
		local new_state = false

		-- collide with player
		if rect_rect_collision(b.rect, player_bbox) then
			new_state = true	
		end

		-- collide with crates
		-- potential bottleneck here - #buttons * #crates collision checks
		for j,c in ipairs(objects.crates) do
			if rect_rect_collision(b.rect, c.rect) then
				new_state = true
			end
		end	

		-- change beacon intens accordingly
		for j,l in ipairs(b.lights) do
			local t = 0
			if new_state then
				t = robo.beacon_radius
			end
			l.inten = lerp(l.inten, t, 0.1)
		end

		b.state = new_state
	end

	-- pick up batteries
	local hit_objs = objects.qtree:query(player_bbox)
	for i, obj in ipairs(hit_objs) do
		if obj.id == 3 and obj.taken == false then
			--local r = tilemap.world2screen(level, screen, obj.rect)
			local r = shrinked_bbox(obj.rect, 20)
			r.l = r.l + 10
			r.r = r.r - 10
			if rect_rect_collision(player_bbox, r) then
				-- pick up battery!
				obj.taken = true
				robo.energy = robo.energy + robo.battery_juice
				robo.energy = math.min(1, robo.energy)
				sound.play(sfx.pickup)
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

