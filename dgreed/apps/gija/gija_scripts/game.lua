local game = {}

gravity = 1

local character = require('character')

local tmap0 = nil
local tmap1 = nil
local objs = nil

local camera = {
	center = vec2()
}

local char0 = nil
local char1 = nil

local obj_type = {
	start0 = 0,
	start1 = 1,
	finish = 2
}

function sweep_rect(r, offset, no_push)
	local off, d

	-- collide against level
	if tmap0 then
		off = tilemap.collide_swept(tmap0, r, offset)
		d = length_sq(off)
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

	if level_name == nil then
		level_name = 'level1'
	end

	tmap0 = tilemap.load(pre..level_name..'.0.btm')
	tmap1 = tilemap.load(pre..level_name..'.1.btm')
	objs = tilemap.objects(tmap0)

	-- set up objects
	for i,obj in ipairs(objs) do
		print(obj.id, obj.pos)
		if obj.id == obj_type.start0 then
			char0 = character:new(obj)
			camera.center = vec2(obj.pos)
		end
		if obj.id == obj_type.start1 then
			char1 = character:new(obj)
		end
	end
end

function game.close()
	if tmap0 and tmap1 then
		tilemap.free(tmap0)
		tilemap.free(tmap1)
	end
end

function game.update_camera()
	local pos, scale, rot = tilemap.camera(tmap0)
	--local d = camera.center - cat.p
	local d = vec2(0, 0)
	camera.center = camera.center - d * 0.05
	pos = vec2(camera.center)
	tilemap.set_camera(tmap0, pos, scale, rot)
	tilemap.set_camera(tmap1, pos, scale, rot)
end

function game.update()
	local ts = time.s()

	sound.update()

	game.update_camera()

	if char0 then
		char0:update(sweep_rect)
	end
	if char1 then
		char1:update(sweep_rect)
	end

	return not key.down(key.quit)
end

function game.render(t)
	local ts = time.s()

	--sprsheet.draw('empty', 0, scr_rect)

	if tmap1 then
		tilemap.render(tmap1, scr_rect)
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

