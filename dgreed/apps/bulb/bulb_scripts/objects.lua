
objects = {
	layer = 0,
	crate_img = nil,
	battery_img = nil,
	beacon_img = nil,
	button_img = nil,
	exit_img = nil,

	-- srcs
	normal_battery = rect(0, 0, 64, 64),
	glow_battery = rect(64, 0, 128, 64),
	src_button_on = rect(0, 0, 64, 64),
	src_button_off = rect(64, 0, 128, 64),
	src_beacon_on = rect(0, 0, 64, 64),
	src_beacon_off = rect(64, 0, 128, 64),

	src_exit = rect(0, 0, 64, 64),
	src_exit_glow = rect(64, 0, 128, 64),
	src_exit2 = rect(0, 64, 64, 128),
	src_exit2_glow = rect(64, 64, 128, 128),

	list = {}
}

function center(r)
	return vec2((r.l + r.r) / 2, (r.t + r.b) / 2)
end

function shrinked_bbox(bbox, r)
	return rect(bbox.l + r, bbox.t + r, bbox.r - r, bbox.b - r)
end

function objects.reset()
	cobjects.reset(level);
	objects.list = {}
end

function objects.init()
	objects.button_img = tex.load(pre..'button.png')
	objects.battery_img = tex.load(pre..'battery.png')
	objects.beacon_img = tex.load(pre..'beacon.png')
	objects.crate_img = tex.load(pre..'crate.png')
	objects.exit_img = tex.load(pre..'obj_end.png')
end

function objects.close()
	tex.free(objects.crate_img)
	tex.free(objects.battery_img)
	tex.free(objects.beacon_img)
	tex.free(objects.button_img)
	tex.free(objects.exit_img)
	cobjects.close()
end

function objects.add(id, pos)
	local r = rect(pos.x, pos.y, pos.x+64, pos.y+64)
	local p = center(r)

	-- start
	if id == 0  then
		r.r = r.l + robo.size.x
		r.b = r.t + robo.size.y
		objects.start = rect(r)
	end

	-- end
	if id == 1 then
		table.insert(objects.list, {rect=r, id=1})
	end

	-- crate
	if id == 2 then	
		cobjects.add(cobjects.obj_crate, pos)
	end

	-- battery
	if id == 3 then
		cobjects.add(cobjects.obj_battery, pos)
	end

	-- beacon
	if id == 4 then
		cobjects.add(cobjects.obj_beacon, pos)
	end

	-- button
	if id == 5 then
		cobjects.add(cobjects.obj_button, pos)
	end

	-- exit decor 1 & 2
	if id == 6 or id == 7 then
		local i = id
		table.insert(objects.list, {rect=r, id=i})
	end
end

function objects.seal()
	cobjects.seal(objects.start)
end

function objects.draw()
	local camera_rect = tilemap.screen2world(level, screen, screen)	

	-- crates
	local crates = cobjects.get_crates(camera_rect)
	for i, obj in ipairs(crates) do
		local p = tilemap.world2screen(level, screen, obj)
		video.draw_rect(objects.crate_img, objects.layer, p)
	end

	-- batteries
	local batteries = cobjects.get_batteries(camera_rect)
	for i, obj in ipairs(batteries) do
		local p = tilemap.world2screen(level, screen, obj)
		video.draw_rect(objects.battery_img, objects.layer, objects.normal_battery, p)
		video.draw_rect(objects.battery_img, objects.layer+2, objects.glow_battery, p)
	end

	-- beacons
	local beacons = cobjects.get_beacons(camera_rect)
	for i, obj in ipairs(beacons) do
		local p = tilemap.world2screen(level, screen, obj.pos) + vec2(32, 32)
		local src = objects.src_beacon_off
		if obj.intensity > 5 then
			src = objects.src_beacon_on
		end
		video.draw_rect_centered(objects.beacon_img, objects.layer, src, p, 0.0, 1.5)
	end

	-- buttons
	local buttons = cobjects.get_buttons(camera_rect)
	for i, obj in ipairs(buttons) do
		local p = tilemap.world2screen(level, screen, obj.pos)
		local src = objects.src_button_off
		if obj.state then
			src = objects.src_button_on
		end
		video.draw_rect(objects.button_img, objects.layer, src, p)
	end

	-- other objs
	for i, obj in ipairs(objects.list) do
		local r = tilemap.world2screen(level, screen, obj.rect)	
		if obj.id == 6 then
			video.draw_rect(objects.exit_img, objects.layer,
				objects.src_exit, r)
			video.draw_rect(objects.exit_img, 3,
				objects.src_exit_glow, r)
		end
		if obj.id == 7 then
			video.draw_rect(objects.exit_img, objects.layer,
				objects.src_exit2, r)
			video.draw_rect(objects.exit_img, 3,
				objects.src_exit2_glow, r)
		end
	end
end

function objects.interact(player_bbox)
	local res = true

	for i, obj in ipairs(objects.list) do
		if rect_rect_collision(obj.rect, player_bbox) then
			-- level end
			if obj.id == 1 then
				sound.play(sfx.win)

				robo.level = robo.level+1
				if robo.level <= #robo.levels then
					tilemap.free(level)
					level = tilemap.load(pre..robo.levels[robo.level])
					game.reset()
				else
					robo.finished = true
					robo.finished_t = time.s()
				end

				break
			end
		end
	end
end

