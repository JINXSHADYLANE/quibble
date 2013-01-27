local game = {}

gravity = 0.4 

local character = require('character')

local update = true
local game_reset = false
local death_t = nil
local win_t = nil
local switch_t = nil

local tmap0 = nil
local tmap1 = nil
local last_tmap = nil
local objs = nil

local camera = {
	center = vec2()
}

local char0 = nil
local char1 = nil
local doors = {} 
local exits = {}

local obj_type = {
	start0 = 0,
	start1 = 1,
	finish = 2,
	door0 = 3,
	door1 = 4
}

local levels = {
	'level1',
	'level2',
	'ma',
	'level3',
	'mb'
}

local current_level = 1

local reached_exit = {}

function sweep_rect(r, offset, i, w)
	local off, d

	-- collide against level
	if char0.heart then
		off = tilemap.collide_swept(tmap0, r, offset)
		d = length_sq(off)
	else
		off = tilemap.collide_swept(tmap1, r, offset)
		d = length_sq(off)
	end

	-- collide against doors
	if d > 0 then
		for _,dr in ipairs(doors) do
			if dr.color ~= i and dr.world == w then
				local door_off = rect_rect_sweep(dr.rect, r, offset)
				local door_d = length_sq(door_off)
				if d - door_d > 0.001 then
					d = door_d
					off = door_off
				end
			end
		end
	end

	-- collide against exits
	for _,e in ipairs(exits) do
		if e.world == w then
			if rect_rect_collision(r, e.rect) then
				reached_exit[i] = true
			end
		end
	end

	return off
end

function game.init()
	game.reset()
end

function game.reset(level)
	tmap0 = nil
	tmap1 = nil
	objs = nil
	doors = {}

	if level == nil then
		level = 1
	end

	current_level = level
	local level_name = levels[level]

	tmap0 = tilemap.load(pre..level_name..'.0.btm')
	tmap1 = tilemap.load(pre..level_name..'.1.btm')
	objs = tilemap.objects(tmap0)

	local add_door = function(w, obj)
		local r = rect(
			obj.pos.x, obj.pos.y,
			obj.pos.x + 32, obj.pos.y + 64
		)
		local c = 1
		if obj.id == obj_type.door1 then
			c = 2
		end
		table.insert(doors, {
			rect = r,
			color = c,
			world = w
		})
	end

	local add_exit = function(w, obj)
		local r = rect(
			obj.pos.x, obj.pos.y,
			obj.pos.x + 32, obj.pos.y + 64
		)
		table.insert(exits, {
			rect = r,
			world = w
		})
	end

	-- set up objects
	for i,obj in ipairs(objs) do
		if obj.id == obj_type.start0 then
			char0 = character:new(obj, 1)
			camera.center = vec2(obj.pos)
		end
		if obj.id == obj_type.start1 then
			char1 = character:new(obj, 2)
		end
		if obj.id == obj_type.door0 or obj.id == obj_type.door1 then
			add_door(0, obj)	
		end
		if obj.id == obj_type.finish then
			add_exit(0, obj)
		end
	end

	objs = tilemap.objects(tmap1)
	for i,obj in ipairs(objs) do
		if obj.id == obj_type.door0 or obj.id == obj_type.door1 then
			add_door(1, obj)
		end
		if obj.id == obj_type.finish then
			add_exit(1, obj)
		end
	end

	game.update_camera()
	char0:update(sweep_rect, function() end, 0)
	char1:update(sweep_rect, function() end, 0)
end

function game.death()
	death_t = time.s()
	update = false
end

function game.close()
	if tmap0 and tmap1 then
		tilemap.free(tmap0)
		tilemap.free(tmap1)
	end
end

function game.update_camera()
	local pos, scale, rot = tilemap.camera(tmap0)
	local target = nil
	if char0.heart then
		target = char0.pos
	else
		target = char1.pos
	end
	local d = camera.center - target
	camera.center = camera.center - d * 0.02
	pos = vec2(camera.center)
	pos.x = math.floor(pos.x)
	pos.y = math.floor(pos.y)
	tilemap.set_camera(tmap0, pos, scale, rot)
	tilemap.set_camera(tmap1, pos, scale, rot)
end

function game.switch_hearts()
	char0.heart = not char0.heart
	char1.heart = not char1.heart

	-- if one of the players is stuck - reset level
	local col0, col1
	if char0.heart then
		col0 = tilemap.collide(tmap0, char0.bbox) 
		col1 = tilemap.collide(tmap0, char1.bbox)
	else
		col0 = tilemap.collide(tmap1, char0.bbox)
		col1 = tilemap.collide(tmap1, char1.bbox)
	end

	if col0 or col1 then
		game.death()
	end

	switch_t = time.s()
	update = false

	if char0.heart then
		last_tmap = tmap0
	else
		last_tmap = tmap1
	end

	char0.heart_store, char0.heart = char0.heart, false
	char1.heart_store, char1.heart = char1.heart, false

	char0:switch_heart()
	char1:switch_heart()
end

function game.update()
	sound.update()

	if not update then
		return true
	end

	game.update_camera()

	local w = 0
	if char1.heart then
		w = 1
	end

	assert(char0 and char1)
	char0:update(sweep_rect, game.switch_hearts, w)
	char1:update(sweep_rect, game.switch_hearts, w)

	if reached_exit[1] and reached_exit[2] then
		win_t = time.s()
		update = false
	end

	reached_exit = {}

	if char0.bbox and char1.bbox then
		if char0.bbox.b >= 575 * 3 or char1.bbox.b >= 575 * 3 then
			game.death()
		end
	end

	return not key.down(key.quit)
end

function game.render_doors(map, w)
	for _,dr in pairs(doors) do
		if dr.world == w then
			local pos = tilemap.world2screen(map, scr_rect, dr.rect)
			local spr = 'door_red'
			if dr.color == 2 then
				spr = 'door_blue'
			end
			if rect_rect_collision(pos, scr_rect) then
				sprsheet.draw(spr, 3, pos)
			end
		end
	end
end

function game.render_exits(map, w)
	for _,e in pairs(exits) do
		if e.world == w then
			local pos = tilemap.world2screen(map, scr_rect, e.rect)
			local spr = 'exit_blue'
			if w == 1 then
				spr = 'exit_red'
			end
			if rect_rect_collision(pos, scr_rect) then
				sprsheet.draw(spr, 3, pos)
			end
		end
	end
end

function game.render(t)
	sprsheet.draw('empty', 0, scr_rect)

	if char0.heart then
		--tilemap.render(tmap1, scr_rect)
		--sprsheet.draw('empty', 0, scr_rect, rgba(1, 1, 1, 0.2))
		last_tmap = tmap0
	end
	
	if char1.heart then
		--tilemap.render(tmap0, scr_rect)
		--sprsheet.draw('empty', 0, scr_rect, rgba(1, 1, 1, 0.2))
		last_tmap = tmap1
	end

	local i = 0
	if last_tmap == tmap1 then
		i = 1
	end
	game.render_doors(last_tmap, i)
	game.render_exits(last_tmap, i)
	tilemap.render(last_tmap, scr_rect)

	if char0 then
		char0:render(tmap0)
	end

	if char1 then
		char1:render(tmap1)
	end

	local t, ts = 0, time.s()

	-- win
	if win_t then
		t = (ts - win_t) / 2
		if t < 1 then
			local alpha = math.sin(t * math.pi)
			local c = rgba(1, 1, 1, alpha)
			sprsheet.draw('empty', 15, scr_rect, c)
			if not game_reset and t > 0.5 then
				game_reset = true
				current_level = current_level + 1
				game.reset(current_level)
			end
		else
			update = true
			win_t = nil
			game_reset = false
		end
	end

	-- death
	if death_t then
		t = (ts - death_t) / 1.5
		if t < 1 then
			local alpha = math.sin(t * math.pi)
			local c = rgba(0, 0, 0, alpha)
			sprsheet.draw('empty', 15, scr_rect, c)
			if not game_reset and t > 0.5 then
				game_reset = true
				game.reset(current_level)
			end
		else
			update = true
			death_t = nil
			game_reset = false
		end
	end

	-- switch hearts
	if switch_t then
		t = (ts - switch_t) / 0.33
		if t < 1 then
			local s_pos, e_pos
			local c = nil
			if char0.heart_store then
				s_pos = char1.pos
				e_pos = char0.pos
				c = rgba(223/255, 161/255, 169/255)
			else
				s_pos = char0.pos
				e_pos = char1.pos
				c = rgba(132/255, 150/255, 183/255)
			end

			local p = lerp(s_pos, e_pos, t)

			p = tilemap.world2screen(tmap0, scr_rect, p)
			sprsheet.draw_centered('heart', 4, p, t, 3, c)
		else
			char0.heart = char0.heart_store
			char1.heart = char1.heart_store
			char0:switch_heart()
			char1:switch_heart()
			update = true
			switch_t = nil
		end
	end

	return true
end

return game

