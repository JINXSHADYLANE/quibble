local game = {}

gravity = 0.4 

local character = require('character')

local tmap0 = nil
local tmap1 = nil
local objs = nil

local camera = {
	center = vec2()
}

local char0 = nil
local char1 = nil
local doors = {} 

local obj_type = {
	start0 = 0,
	start1 = 1,
	finish = 2,
	door0 = 3,
	door1 = 4
}

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

	return off
end

function game.init()
	game.reset()
end

function game.reset(level_name)
	tmap0 = nil
	tmap1 = nil
	objs = nil
	doors = {}

	if level_name == nil then
		level_name = 'level2'
	end

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
	end

	objs = tilemap.objects(tmap1)
	for i,obj in ipairs(objs) do
		if obj.id == obj_type.door0 or obj.id == obj_type.door1 then
			add_door(1, obj)
		end
	end
end

function game.death()
	game.reset()
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

	char0:switch_heart()
	char1:switch_heart()

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
end

function game.update()
	local ts = time.s()

	sound.update()

	game.update_camera()

	local w = 0
	if char1.heart then
		w = 1
	end

	assert(char0 and char1)
	char0:update(sweep_rect, game.switch_hearts, w)
	char1:update(sweep_rect, game.switch_hearts, w)

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
			local spr = 'char1'
			if dr.color == 2 then
				spr = 'char2'
			end
			if rect_rect_collision(pos, scr_rect) then
				sprsheet.draw(spr, 3, pos)
			end
		end
	end
end

function game.render(t)
	local ts = time.s()

	sprsheet.draw('empty', 0, scr_rect)

	if char0.heart and tmap0 then
		tilemap.render(tmap0, scr_rect)
		game.render_doors(tmap0, 0)
	end
	
	if char1.heart and tmap1 then
		tilemap.render(tmap1, scr_rect)
		game.render_doors(tmap1, 1)
	end

	if char0 then
		char0:render(tmap0)
	end

	if char1 then
		char1:render(tmap1)
	end

	return true
end

return game

